/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dpsOp2Record.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains implementation for log record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/05/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsOp2Record.hpp"
#include "dpsLogRecordDef.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsUtil.hpp"
#include "ossUtil.hpp"
#include "sdbInterface.hpp"

namespace engine
{

   static BOOLEAN _dpsIsTimeOn()
   {
      return NULL != sdbGetThreadExecutor() ?
                         sdbGetThreadExecutor()->isLogTimeOn() :
                         dpsGetGlobalLogConfig().isLogTimeOn() ;
   }

   static INT32 checkAndAddTimeInfo( dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      UINT64 *timeMicroSeconds = NULL ;
      if ( !_dpsIsTimeOn() )
      {
         goto done ;
      }

      record.sampleTime() ;
      timeMicroSeconds = record.getTime() ;

      rc = record.push( DPS_LOG_PUBLIC_TIME, sizeof(*timeMicroSeconds),
                        (CHAR *)timeMicroSeconds ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed push time(%lld), rc = %d",
                   *timeMicroSeconds, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /// warning: any value can not be value-passed.
   static INT32 dpsPushTran( const DPS_TRANS_ID &transID,
                             const DPS_LSN_OFFSET &preTransLsn,
                             const DPS_LSN_OFFSET &relatedLSN,
                             dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      if ( DPS_INVALID_TRANS_ID != transID )
      {
         rc = record.push( DPS_LOG_PUBLIC_TRANSID,
                           sizeof( transID ), (CHAR *)(&transID)) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      if ( DPS_INVALID_LSN_OFFSET != preTransLsn )
      {
         rc = record.push( DPS_LOG_PUBLIC_PRETRANS,
                           sizeof( preTransLsn ),
                           (CHAR *)(&preTransLsn) ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      if ( DPS_INVALID_LSN_OFFSET != relatedLSN )
      {
         rc = record.push( DPS_LOG_PUBLIC_RELATED_TRANS,
                           sizeof( relatedLSN ),
                           (CHAR *)( &relatedLSN ) ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _dpsUnqIdxHashArray implement
    */
   INT32 _dpsUnqIdxHashArray::prepare( UINT32 unqIdxNum,
                                       BOOLEAN isNew )
   {
      INT32 rc = SDB_OK ;

      if ( unqIdxNum > 0 )
      {
         rc = resize( unqIdxNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed resize array for hash values "
                      "with %u, rc: %d", unqIdxNum, rc ) ;
         // reserved 0 for all values
         ossMemset( _dynamicBuf, DPS_UNQIDX_INVALID_HASH,
                    unqIdxNum * sizeof( UINT16 ) ) ;
         _eleSize = unqIdxNum ;
         _curSize = 0 ;
      }
      else
      {
         // clear elements
         _eleSize = 0 ;
         _curSize = 0 ;
      }

      _isNew = isNew ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _dpsUnqIdxHashArray::saveKey( UINT32 hashValue )
   {
      if ( _curSize < _eleSize )
      {
         // value from 1 - 65535
         _dynamicBuf[ _curSize ++ ] =
               (UINT16)( hashValue % DPS_UNQIDX_HASH_MODULE ) + 1 ;
      }
   }

   INT32 _dpsUnqIdxHashArray::pushToRecord( dpsLogRecord &record ) const
   {
      INT32 rc = SDB_OK ;

      if ( _eleSize > 0 )
      {
         // we need push eleSize size of elements which have been
         // reserved in prepare()
         rc = record.push( _isNew ?
                                 DPS_LOG_PUBLIC_NEW_UNQIDX_HASH :
                                 DPS_LOG_PUBLIC_OLD_UNQIDX_HASH,
                           _eleSize * sizeof( UINT16 ),
                           (CHAR *)_dynamicBuf ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push unique index hash values, "
                      "rc: %d", rc ) ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _dpsUnqIdxHashArray::parseFromRecord( const dpsLogRecord &record,
                                               BOOLEAN isNew )
   {
      INT32 rc = SDB_OK ;

      dpsLogRecord::iterator itr ;

      // clear before parse
      _eleSize = 0 ;
      _curSize = 0 ;

      itr = record.find( isNew ? DPS_LOG_PUBLIC_NEW_UNQIDX_HASH :
                                 DPS_LOG_PUBLIC_OLD_UNQIDX_HASH ) ;
      if ( !itr.valid() )
      {
         goto done ;
      }

      if ( itr.len() > 0 )
      {
         UINT32 eleSize = itr.len() / sizeof( UINT16 ) ;
         SDB_ASSERT( eleSize * sizeof( UINT16 ) == itr.len(),
                     "size of hash list is invalid" ) ;

         // make sure the space
         rc = resize( eleSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to resize hash list to [%u] "
                      "elements, rc: %d", rc ) ;
         // copy from record
         ossMemcpy( _dynamicBuf, itr.value(), itr.len() ) ;
         _eleSize = eleSize ;
         _curSize = _eleSize ;

         _isNew = isNew ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   ossPoolString _dpsUnqIdxHashArray::toString() const
   {
      StringBuilder ss ;
      if ( !empty() )
      {
         UINT16 hashValue = 0 ;
         iterator iter( *this ) ;
         while ( iter.next( hashValue ) )
         {
            if ( ss.len() > 0 )
            {
               ss << "," ;
            }
            ss << hashValue ;
         }
      }
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_INSERT2RECORD, "dpsInsert2Record" )
   INT32 dpsInsert2Record( const CHAR *fullName,
                           const BSONObj &obj,
                           const dpsUnqIdxHashArray *pUnqIdxHashArray,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_INSERT2RECORD ) ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_DATA_INSERT ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_INSERT_OBJ, obj.objsize(), obj.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push obj to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      // push unique index hash values if needed
      if ( NULL != pUnqIdxHashArray )
      {
         rc = pUnqIdxHashArray->pushToRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add unique index hash values, "
                      "rc: %d", rc ) ;
      }

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_INSERT2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_DPS_INSERT2RECORD, "dpsRecord2Insert")
   INT32 dpsRecord2Insert( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &obj,
                           UINT64 *microSeconds,
                           dpsUnqIdxHashArray *pUnqIdxHashArray )
   {
      PD_TRACE_ENTRY( SDB_DPS_INSERT2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load insert record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName, itrObj ;
      itrFullName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrObj = record.find( DPS_LOG_INSERT_OBJ ) ;
      if ( !itrObj.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag obj in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL != microSeconds )
      {
         dpsLogRecord::iterator itrTime ;
         itrTime = record.find( DPS_LOG_PUBLIC_TIME ) ;
         if ( itrTime.valid() )
         {
            *microSeconds = *( UINT64 *) itrTime.value() ;
         }
      }

      // parse unique index hash values if needed
      if ( NULL != pUnqIdxHashArray )
      {
         rc = pUnqIdxHashArray->parseFromRecord( record, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse unique index hash values, "
                      "rc: %d", rc ) ;
      }

      *fullName = itrFullName.value() ;
      obj = BSONObj( itrObj.value() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB_DPS_INSERT2RECORD, rc) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_UPDATE2RECORD, "dpsUpdate2Record" )
   INT32 dpsUpdate2Record( const CHAR *fullName,
                           const BSONObj &oldMatch,
                           const BSONObj &oldObj,
                           const BSONObj &newMatch,
                           const BSONObj &newObj,
                           const BSONObj &oldShardingKey,
                           const BSONObj &newShardingKey,
                           const dpsUnqIdxHashArray *pNewUnqIdxHashArray,
                           const dpsUnqIdxHashArray *pOldUnqIdxHashArray,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           const UINT32 *writeMod,
                           dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_UPDATE2RECORD ) ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_DATA_UPDATE ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_UPDATE_OLDMATCH,
                        oldMatch.objsize(),
                        oldMatch.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push oldmatch to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_UPDATE_OLDOBJ,
                        oldObj.objsize(),
                        oldObj.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push oldobj to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_UPDATE_NEWMATCH,
                        newMatch.objsize(),
                        newMatch.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push newmatch to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_UPDATE_NEWOBJ,
                        newObj.objsize(),
                        newObj.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push newobj to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      if ( !oldShardingKey.isEmpty() && !newShardingKey.isEmpty() )
      {
         rc = record.push( DPS_LOG_UPDATE_OLDSHARDINGKEY,
                           oldShardingKey.objsize(),
                           oldShardingKey.objdata() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to push oldShardingKey to record, rc: %d", rc ) ;
            goto error ;
         }

         // To save space,
         // do not log new sharding key when it equals old sharding key.
         if ( newShardingKey.woCompare( oldShardingKey ) != 0 )
         {
            SDB_ASSERT( !oldShardingKey.isEmpty(), "must have old sharding key" ) ;

            rc = record.push( DPS_LOG_UPDATE_NEWSHARDINGKEY,
                              newShardingKey.objsize(),
                              newShardingKey.objdata() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to push newShardingKey to record, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

      if ( NULL != writeMod )
      {
         rc = record.push( DPS_LOG_UPDATE_WRITEMOD, sizeof(*writeMod),
                           (CHAR *)writeMod ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push writeMod(%d) to record, "
                      "rc = %d", *writeMod, rc ) ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      // push unique index hash values if needed
      if ( NULL != pNewUnqIdxHashArray )
      {
         rc = pNewUnqIdxHashArray->pushToRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add new unique index "
                      "hash values, rc: %d", rc ) ;
      }
      if ( NULL != pOldUnqIdxHashArray )
      {
         rc = pOldUnqIdxHashArray->pushToRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add old unique index "
                      "hash values, rc: %d", rc ) ;
      }

      header._length = record.alignedLen() ;

   done:
      PD_TRACE_EXITRC( SDB__DPS_UPDATE2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2UPDATE, "dpsRecord2Update" )
   INT32 dpsRecord2Update( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &oldMatch,
                           BSONObj &oldObj,
                           BSONObj &newMatch,
                           BSONObj &newObj,
                           BSONObj *oldShardingKey,
                           BSONObj *newShardingKey,
                           UINT64 *microSeconds,
                           UINT32 *writeMod,
                           dpsUnqIdxHashArray *pNewUnqIdxHashArray,
                           dpsUnqIdxHashArray *pOldUnqIdxHashArray )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2UPDATE ) ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      INT32 rc = SDB_OK ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load update record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName, itrOldM,
                             itrOldObj, itrNewM, itrNewObj ;
      itrFullName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrOldM = record.find( DPS_LOG_UPDATE_OLDMATCH ) ;
      if ( !itrOldM.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag oldmatch in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrOldObj = record.find( DPS_LOG_UPDATE_OLDOBJ ) ;
      if ( !itrOldObj.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag oldobj in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrNewM = record.find( DPS_LOG_UPDATE_NEWMATCH ) ;
      if ( !itrNewM.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag newmatch in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrNewObj = record.find( DPS_LOG_UPDATE_NEWOBJ ) ;
      if ( !itrNewObj.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag newobj in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *fullName = itrFullName.value() ;
      oldMatch = BSONObj( itrOldM.value() ) ;
      oldObj = BSONObj( itrOldObj.value() ) ;
      newMatch = BSONObj( itrNewM.value() ) ;
      newObj = BSONObj( itrNewObj.value() ) ;
      }

      if ( NULL != oldShardingKey )
      {
         dpsLogRecord::iterator itrOldSK ;
         itrOldSK = record.find( DPS_LOG_UPDATE_OLDSHARDINGKEY ) ;
         if ( itrOldSK.valid() )
         {
            *oldShardingKey = BSONObj( itrOldSK.value() ) ;
         }
      }

      if ( NULL != newShardingKey )
      {
         dpsLogRecord::iterator itrNewSK ;
         itrNewSK = record.find( DPS_LOG_UPDATE_NEWSHARDINGKEY ) ;
         if ( itrNewSK.valid() )
         {
            *newShardingKey = BSONObj( itrNewSK.value() ) ;
         }
         else if ( NULL != oldShardingKey )
         {
            // new sharding key equals old sharding key
            *newShardingKey = *oldShardingKey ;
         }
         else
         {
            // new sharding key equals old sharding key
            dpsLogRecord::iterator itrOldSK ;
            itrOldSK = record.find( DPS_LOG_UPDATE_OLDSHARDINGKEY ) ;
            if ( itrOldSK.valid() )
            {
               *newShardingKey = BSONObj( itrOldSK.value() ) ;
            }
         }
      }

      if ( NULL != microSeconds )
      {
         dpsLogRecord::iterator itrTime ;
         itrTime = record.find( DPS_LOG_PUBLIC_TIME ) ;
         if ( itrTime.valid() )
         {
            *microSeconds = *( UINT64 *) itrTime.value() ;
         }
      }

      if ( NULL != writeMod )
      {
         dpsLogRecord::iterator itrTime ;
         itrTime = record.find( DPS_LOG_UPDATE_WRITEMOD ) ;
         if ( itrTime.valid() )
         {
            *writeMod = *( UINT32 *) itrTime.value() ;
         }
      }

      // parse unique index hash values if needed
      if ( NULL != pNewUnqIdxHashArray )
      {
         rc = pNewUnqIdxHashArray->parseFromRecord( record, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse new unique index "
                      "hash values, rc: %d", rc ) ;
      }
      if ( NULL != pOldUnqIdxHashArray )
      {
         rc = pOldUnqIdxHashArray->parseFromRecord( record, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse old unique index "
                      "hash values, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2UPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_DELETE2RECORD, "dpsDelete2Record" )
   INT32 dpsDelete2Record( const CHAR *fullName,
                           const BSONObj &oldObj,
                           const dpsUnqIdxHashArray *pUnqIdxHashArray,
                           const INT64 *position,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_DELETE2RECORD ) ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_DATA_DELETE ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_DELETE_OLDOBJ,
                        oldObj.objsize(),
                        oldObj.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push oldobj to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      // push unique index hash values if needed
      if ( NULL != pUnqIdxHashArray )
      {
         rc = pUnqIdxHashArray->pushToRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add unique index hash values, "
                      "rc: %d", rc ) ;
      }

      if ( NULL != position && -1 != *position )
      {
         rc = record.push( DPS_LOG_DELETE_POSITION,
                           sizeof( INT64 ),
                           (CHAR *)( position ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add position for mark delete"
                      "record, rc: %d", rc ) ;
      }

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_DELETE2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2DELETE, "dpsRecord2Delete" )
   INT32 dpsRecord2Delete( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &oldObj,
                           UINT64 *microSeconds,
                           dpsUnqIdxHashArray *pUnqIdxHashArray,
                           INT64 *position )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2DELETE ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load delete record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName, itrObj ;
      itrFullName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrObj = record.find( DPS_LOG_DELETE_OLDOBJ ) ;
      if ( !itrObj.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag oldobj in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL != microSeconds )
      {
         dpsLogRecord::iterator itrTime ;
         itrTime = record.find( DPS_LOG_PUBLIC_TIME ) ;
         if ( itrTime.valid() )
         {
            *microSeconds = *( UINT64 *) itrTime.value() ;
         }
      }

      // parse unique index hash values if needed
      if ( NULL != pUnqIdxHashArray )
      {
         rc = pUnqIdxHashArray->parseFromRecord( record, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse unique index hash values, "
                      "rc: %d", rc ) ;
      }

      // parse position if needed
      if ( NULL != position )
      {
         _dpsLogRecord::iterator iterPos =
                                    record.find( DPS_LOG_DELETE_POSITION ) ;
         if ( iterPos.valid() )
         {
            *position = *( (const INT64 *)( iterPos.value() ) ) ;
         }
         else
         {
            // not found, set to default
            *position = -1 ;
         }
      }

      *fullName = itrFullName.value() ;
      oldObj = BSONObj( itrObj.value() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2DELETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_POP2RECORD, "dpsPop2Record" )
   INT32 dpsPop2Record( const CHAR *fullName, const INT64 &logicalID,
                        const INT8 &direction, dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_POP2RECORD ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_DATA_POP ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( fullName ) + 1,
                        fullName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push fullname to record, rc: %d", rc ) ;

      rc = record.push( DPS_LOG_POP_LID, sizeof( logicalID ),
                        (CHAR *)( &logicalID ) ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push LogicalID object to record, rc: %d", rc ) ;

      rc = record.push( DPS_LOG_POP_DIRECTION, sizeof( direction ),
                        (CHAR *)( &direction ) ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push Direction object to record, rc: %d", rc ) ;

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;

   done:
      PD_TRACE_EXITRC( SDB__DPS_POP2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2POP, "dpsRecord2Pop" )
   INT32 dpsRecord2Pop( const CHAR *logRecord, const CHAR **fullName,
                        INT64 &logicalID, INT8 &direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2POP ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load pop record, rc: %d", rc ) ;

      {
         dpsLogRecord::iterator itrName, itrLID, itrDirection ;
         itrName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         if ( !itrName.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         itrLID = record.find( DPS_LOG_POP_LID ) ;
         if ( !itrLID.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag LogicalID in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         itrDirection = record.find( DPS_LOG_POP_DIRECTION ) ;
         if ( !itrDirection.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag Direction in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         *fullName = itrName.value() ;
         logicalID = *(INT64 *)( itrLID.value() ) ;
         direction = *(INT8 *)( itrDirection.value() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2POP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_CSCRT2RECORD, "dpsCSCrt2Record" )
   INT32 dpsCSCrt2Record( const CHAR *csName,
                          const utilCSUniqueID &csUniqueID,
                          const INT32 &pageSize,
                          const INT32 &lobPageSize,
                          const INT32 &type,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CSCRT2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != csName, "Collectionspace name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CS_CRT ;

      rc = record.push( DPS_LOG_CSCRT_CSNAME,
                        ossStrlen( csName) + 1,
                        csName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push csname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CSCRT_CSUNIQUEID,
                        sizeof( csUniqueID ),
                        (CHAR *)( &csUniqueID ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push cs unique id to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CSCRT_PAGESIZE,
                        sizeof( pageSize ),
                        (CHAR *)( &pageSize ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push pagesize to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CSCRT_LOBPAGESZ,
                        sizeof( lobPageSize ),
                        (CHAR *)( &lobPageSize ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push lob pagesize to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CSCRT_CSTYPE,
                        sizeof( type ),
                        (CHAR *)( &type ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push cs type to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CSCRT2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2CSCRT, "dpsRecord2CSCrt" )
   INT32 dpsRecord2CSCrt( const CHAR *logRecord,
                          const CHAR **csName,
                          utilCSUniqueID &csUniqueID,
                          INT32 &pageSize,
                          INT32 &lobPageSize,
                          INT32 &type )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CSCRT ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cs create record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrCsName, itrUniqueID, itrPageSize,
                             itrLobPageSz, itrType ;
      itrCsName = record.find( DPS_LOG_CSCRT_CSNAME ) ;
      if ( !itrCsName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag csname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrUniqueID = record.find( DPS_LOG_CSCRT_CSUNIQUEID ) ;
      if ( itrUniqueID.valid() )
      {
         csUniqueID = *((utilCSUniqueID *)itrUniqueID.value()) ;
      }

      itrPageSize = record.find( DPS_LOG_CSCRT_PAGESIZE ) ;
      if ( !itrPageSize.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag pagesize in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *csName = itrCsName.value() ;
      pageSize = *((INT32 *)itrPageSize.value()) ;

      itrLobPageSz = record.find( DPS_LOG_CSCRT_LOBPAGESZ ) ;
      if ( !itrLobPageSz.valid() )
      {
         PD_LOG( PDWARNING, "Failed to find tag lob pagesize in record"
                 ", use default value(256KB)" ) ;
         lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }
      else
      {
         lobPageSize = *(( INT32 *)itrLobPageSz.value()) ;
      }

      itrType = record.find( DPS_LOG_CSCRT_CSTYPE ) ;
      if ( !itrType.valid() )
      {
         // For compatible with elder versions.
         PD_LOG( PDWARNING, "Failed to find tag CS type in record, use normal "
                 "type(0)" ) ;
         type = 0 ;
      }
      else
      {
         type = *( ( INT32 *)itrType.value() ) ;
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CSCRT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_CSDEL2RECORD, "dpsCSDel2Record" )
   INT32 dpsCSDel2Record( const CHAR *csName,
                          bson::BSONObj *boOptions,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CSDEL2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != csName, "Collectionspace name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CS_DELETE ;

      rc = record.push( DPS_LOG_CSDEL_CSNAME,
                        ossStrlen( csName) + 1,
                        csName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push csname to record, rc: %d", rc ) ;
         goto error ;
      }

      if ( NULL != boOptions && !( boOptions->isEmpty() ) )
      {
         rc = record.push( DPS_LOG_CSDEL_OPTIONS,
                           boOptions->objsize(),
                           boOptions->objdata() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push drop collection space "
                      "options, rc: %d", rc ) ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CSDEL2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2CSDEL, "dpsRecord2CSDel" )
   INT32 dpsRecord2CSDel( const CHAR *logRecord,
                          const CHAR **csName,
                          bson::BSONObj *boOptions )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CSDEL ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cs del record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrCsName =
                           record.find( DPS_LOG_CSDEL_CSNAME ) ;
      if ( !itrCsName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag csname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *csName = itrCsName.value() ;
      }

      if ( NULL != boOptions )
      {
         dpsLogRecord::iterator itrOptions =
               record.find( DPS_LOG_CSDEL_OPTIONS ) ;
         try
         {
            if ( itrOptions.valid() )
            {
               *boOptions = BSONObj( itrOptions.value() ) ;
            }
            else
            {
               *boOptions = BSONObj() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CSDEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_CSRENAME2RECORD, "dpsCSRename2Record" )
   INT32 dpsCSRename2Record( const CHAR * csName,
                             const CHAR * newCSName,
                             dpsLogRecord & record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CSRENAME2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != csName, "Collectionspace name can't be NULL" ) ;
      SDB_ASSERT( NULL != newCSName, "New collectionspace name can't be NULL" ) ;

      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CS_RENAME ;

      rc = record.push( DPS_LOG_CSRENAME_CSNAME,
                        ossStrlen( csName) + 1,
                        csName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push csname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CSRENAME_NEWNAME,
                        ossStrlen( newCSName ) + 1,
                        newCSName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to push new csname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CSRENAME2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2CSRENAME, "dpsRecord2CSRename" )
   INT32 dpsRecord2CSRename( const CHAR * logRecord,
                             const CHAR **csName,
                             const CHAR **newCSName )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CSRENAME ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cs rename record, rc: %d", rc ) ;
         goto error ;
      }

      {
         dpsLogRecord::iterator itrCsName =
                              record.find( DPS_LOG_CSRENAME_CSNAME ) ;
         if ( !itrCsName.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag csname in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *csName = itrCsName.value() ;

         itrCsName = record.find( DPS_LOG_CSRENAME_NEWNAME ) ;
         if ( !itrCsName.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag new csname in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *newCSName = itrCsName.value() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CSRENAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_CLCRT2RECORD, "dpsCLCrt2Record" )
   INT32 dpsCLCrt2Record( const CHAR *fullName,
                          const utilCLUniqueID &clUniqueID,
                          const UINT32 &attribute,
                          const UINT8 &compressorType,
                          const BSONObj *extOptions,
                          const BSONObj *idIdxDef,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CLCRT2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CL_CRT ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CLCRT_CLUNIQUEID,
                        sizeof( clUniqueID ),
                        (const CHAR *)(&clUniqueID)) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push cl unique id to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CLCRT_ATTRIBUTE,
                        sizeof( attribute ),
                        (const CHAR *)(&attribute)) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push attribute to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CLCRT_COMPRESS_TYPE, sizeof( compressorType ),
                        (const CHAR *)(&compressorType)) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to push compression type to record, rc: %d", rc ) ;
         goto error ;
      }

      if ( extOptions )
      {
         if ( !extOptions->valid() )
         {
            PD_LOG( PDERROR, "Extend option object is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = record.push( DPS_LOG_CLCRT_EXT_OPTIONS, extOptions->objsize(),
                           extOptions->objdata() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push extend options to record, "
                      "rc: %d", rc ) ;
      }

      if ( idIdxDef )
      {
         if ( !idIdxDef->valid() )
         {
            PD_LOG( PDERROR, "$id index definition is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = record.push( DPS_LOG_CLCRT_IDIDX_DEF, idIdxDef->objsize(),
                           idIdxDef->objdata() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to push $id index definition to record, "
                      "rc: %d", rc ) ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CLCRT2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2CLCRT, "dpsRecord2CLCrt" )
   INT32 dpsRecord2CLCrt( const CHAR *logRecord,
                          const CHAR **fullName,
                          utilCLUniqueID &clUniqueID,
                          UINT32 &attribute,
                          UINT8 &compressorType,
                          BSONObj &extOptions,
                          BSONObj &idIdxDef )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CLCRT ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      attribute = 0 ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cl create record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator recordItr ;
      recordItr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !recordItr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *fullName = recordItr.value() ;

      recordItr = record.find( DPS_LOG_CLCRT_CLUNIQUEID ) ;
      if ( recordItr.valid() )
      {
         clUniqueID = *((utilCLUniqueID *)recordItr.value() ) ;
      }

      recordItr = record.find( DPS_LOG_CLCRT_ATTRIBUTE ) ;
      if ( recordItr.valid() )
      {
         attribute = *((UINT32 *)recordItr.value() ) ;
      }

      recordItr = record.find( DPS_LOG_CLCRT_COMPRESS_TYPE ) ;
      if ( recordItr.valid() )
      {
         compressorType = *((UINT8 *)recordItr.value());
      }

      recordItr = record.find( DPS_LOG_CLCRT_EXT_OPTIONS ) ;
      if ( recordItr.valid() )
      {
         try
         {
            extOptions = BSONObj( recordItr.value() ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Exception occurred when fetching option "
                    "object: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      recordItr = record.find( DPS_LOG_CLCRT_IDIDX_DEF ) ;
      if ( recordItr.valid() )
      {
         try
         {
            idIdxDef = BSONObj( recordItr.value() ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CLCRT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_CLDEL2RECORD, "dpsCLDel2Record" )
   INT32 dpsCLDel2Record( const CHAR *fullName,
                          bson::BSONObj *boOptions,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CLDEL2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CL_DELETE;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d", rc ) ;
         goto error ;
      }

      if ( NULL != boOptions && !( boOptions->isEmpty() ) )
      {
         rc = record.push( DPS_LOG_CLDEL_OPTIONS,
                           boOptions->objsize(),
                           boOptions->objdata() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push drop collection options, "
                      "rc: %d", rc ) ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CLDEL2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2CLDEL, "dpsRecord2CLDel" )
   INT32 dpsRecord2CLDel( const CHAR *logRecord,
                          const CHAR **fullName,
                          BSONObj *boOptions )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CLDEL ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cl del record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName =
                           record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *fullName = itrFullName.value() ;
      }

      if ( NULL != boOptions )
      {
         dpsLogRecord::iterator itrOptions =
               record.find( DPS_LOG_CLDEL_OPTIONS ) ;
         try
         {
            if ( itrOptions.valid() )
            {
               *boOptions = BSONObj( itrOptions.value() ) ;
            }
            else
            {
               *boOptions = BSONObj() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CLDEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_IXCRT2RECORD, "dpsIXCrt2Record" )
   INT32 dpsIXCrt2Record( const CHAR *fullName,
                          const BSONObj &index,
                          const BSONObj &option,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_IXCRT2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_IX_CRT ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_IXCRT_IX,
                        index.objsize(),
                        index.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push ix to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_IXCRT_OPTION,
                        option.objsize(),
                        option.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push option to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_IXCRT2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2IXCRT, "dpsRecord2IXCrt" )
   INT32 dpsRecord2IXCrt( const CHAR *logRecord,
                          const CHAR **fullName,
                          BSONObj &index,
                          BSONObj &option )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2IXCRT ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load ix create record, rc: %d",rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName, itrIndex, itrOpt ;
      itrFullName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrIndex = record.find( DPS_LOG_IXCRT_IX ) ;
      if ( !itrIndex.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag ix in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *fullName = itrFullName.value() ;
      index = BSONObj( itrIndex.value() ) ;

      itrOpt = record.find( DPS_LOG_IXCRT_OPTION ) ;
      if ( itrOpt.valid() )
      {
         option = BSONObj( itrOpt.value() ) ;
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2IXCRT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_IXDEL2RECORD, "dpsIXDel2Record" )
   INT32 dpsIXDel2Record( const CHAR *fullName,
                          const BSONObj &index,
                          const BSONObj &option,
                          dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_IXDEL2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_IX_DELETE ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_IXDEL_IX,
                        index.objsize(),
                        index.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push ix to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_IXDEL_OPTION,
                        option.objsize(),
                        option.objdata() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push option to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_IXDEL2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2IXDEL, "dpsRecord2IXDel" )
   INT32 dpsRecord2IXDel( const CHAR *logRecord,
                          const CHAR **fullName,
                          BSONObj &index,
                          BSONObj &option )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2IXDEL ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load ix delete record, rc: %d",rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrFullName, itrIndex, itrOpt ;
      itrFullName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itrFullName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrIndex = record.find( DPS_LOG_IXDEL_IX ) ;
      if ( !itrIndex.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag ix in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *fullName = itrFullName.value() ;
      index = BSONObj( itrIndex.value() ) ;

      itrOpt = record.find( DPS_LOG_IXDEL_OPTION ) ;
      if ( itrOpt.valid() )
      {
         option = BSONObj( itrOpt.value() ) ;
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2IXDEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_CLRENAME2RECORD, "dpsCLRename2Record" )
   INT32 dpsCLRename2Record( const CHAR *csName,
                             const CHAR *clOldName,
                             const CHAR *clNewName,
                             dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CLRENAME2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != csName &&
                  NULL != clOldName &&
                  NULL != clNewName, "csName and clOldName and clNewName "
                  "cat't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CL_RENAME ;

      rc = record.push( DPS_LOG_CLRENAME_CSNAME,
                        ossStrlen( csName) + 1,
                        csName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push csname to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CLRENAME_CLOLDNAME,
                        ossStrlen( clOldName) + 1,
                        clOldName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push oldname to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_CLRENAME_CLNEWNAME,
                        ossStrlen(clNewName)+1,
                        clNewName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push newname to record, rc: %d",rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CLRENAME2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2CLRENAME, "dpsRecord2CLRename" )
   INT32 dpsRecord2CLRename( const CHAR *logRecord,
                             const CHAR **csName,
                             const CHAR **clOldName,
                             const CHAR **clNewName )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CLRENAME ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cl rename record, rc: %d",rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itrCsName, itrOldName, itrNewName ;
      itrCsName = record.find( DPS_LOG_CLRENAME_CSNAME ) ;
      if ( !itrCsName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag cs name in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrOldName = record.find( DPS_LOG_CLRENAME_CLOLDNAME ) ;
      if ( !itrOldName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag oldname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      itrNewName = record.find( DPS_LOG_CLRENAME_CLNEWNAME ) ;
      if ( !itrNewName.valid() )
      {
         PD_LOG( PDERROR, "Failed to find tag newname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *csName = itrCsName.value() ;
      *clOldName = itrOldName.value() ;
      *clNewName = itrNewName.value() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CLRENAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_CLTRUNC2RECORD, "dpsCLTrunc2Record" )
   INT32 dpsCLTrunc2Record( const CHAR *fullName,
                            BSONObj *boOptions,
                            dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_CLTRUNC2RECORD ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullName, "Collection name can't be NULL" ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_CL_TRUNC ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen(fullName) + 1, // '1 for '\0'
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d",rc ) ;
         goto error ;
      }

      if ( NULL != boOptions && !( boOptions->isEmpty() ) )
      {
         rc = record.push( DPS_LOG_CLTRUNC_OPTIONS,
                           boOptions->objsize(),
                           boOptions->objdata() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push drop collection options, "
                      "rc: %d", rc ) ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_CLTRUNC2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2CLTRUNC, "dpsRecord2CLTrunc" )
   INT32 dpsRecord2CLTrunc( const CHAR * logRecord,
                            const CHAR ** fullName,
                            BSONObj *boOptions )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2CLTRUNC ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load cl truncate record, rc: %d", rc ) ;
         goto error ;
      }

      {
         dpsLogRecord::iterator itrCLName ;
         itrCLName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         if ( !itrCLName.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *fullName = itrCLName.value() ;
      }

      if ( NULL != boOptions )
      {
         dpsLogRecord::iterator itrOptions =
               record.find( DPS_LOG_CLTRUNC_OPTIONS ) ;
         try
         {
            if ( itrOptions.valid() )
            {
               *boOptions = BSONObj( itrOptions.value() ) ;
            }
            else
            {
               *boOptions = BSONObj() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2CLTRUNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2TRANSCOMMIT, "dpsRecord2TransCommit" )
   INT32 dpsRecord2TransCommit( const CHAR *logRecord,
                                DPS_TRANS_ID &transID,
                                DPS_LSN_OFFSET &preTransLsn,
                                DPS_LSN_OFFSET &firstTransLsn,
                                UINT8  &attr,
                                UINT32 &nodeNum,
                                const UINT64 **ppNodes )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2TRANSCOMMIT ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;
      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load trans commit record, rc: %d",rc ) ;
         goto error ;
      }

      {
         dpsLogRecord::iterator itr ;

         rc = dpsGetTransIDFromRecord( record, TRUE, transID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get transaction ID, rc: %d", rc ) ;

         itr = record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
         if ( itr.valid() )
         {
            preTransLsn = *(DPS_LSN_OFFSET*)itr.value() ;
         }
         else
         {
            preTransLsn = DPS_INVALID_LSN_OFFSET ;
         }

         itr = record.find( DPS_LOG_PUBLIC_FIRSTTRANS ) ;
         if ( itr.valid() )
         {
            firstTransLsn = *(DPS_LSN_OFFSET*)itr.value() ;
         }
         else
         {
            firstTransLsn = DPS_INVALID_LSN_OFFSET ;
         }

         itr = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
         if ( itr.valid() )
         {
            attr = *(UINT8*)itr.value() ;
         }
         else
         {
            attr = 0 ;
         }

         if ( DPS_TS_COMMIT_ATTR_PRE == attr )
         {
            itr = record.find( DPS_LOG_TSCOMMIT_NODE_NUM ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "Failed to find tag node num in record" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            nodeNum = *(UINT32*)itr.value() ;

            if ( nodeNum > 0 )
            {
               itr = record.find( DPS_LOG_TSCOMMIT_NODES ) ;
               if ( !itr.valid() )
               {
                  PD_LOG( PDERROR, "Failed to find tag nodes in record" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               *ppNodes = (const UINT64*)itr.value() ;
            }
            else
            {
               *ppNodes = NULL ;
            }
         }
         else
         {
            nodeNum = 0 ;
            *ppNodes = NULL ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2TRANSCOMMIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* dpsTSCommitAttr2String( UINT8 attr )
   {
      const CHAR *pAttr = "Unknown" ;

      switch ( attr )
      {
         case DPS_TS_COMMIT_ATTR_PRE :
            pAttr = DPS_TS_COMMIT_ATTR_PRE_STR ;
            break ;
         case DPS_TS_COMMIT_ATTR_SND :
            pAttr = DPS_TS_COMMIT_ATTR_SND_STR ;
            break ;
         default :
            break ;
      }

      return pAttr ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_TRANSCOMMIT2RECORD, "dpsTransCommit2Record" )
   INT32 dpsTransCommit2Record( const DPS_TRANS_ID &transID,
                                const DPS_LSN_OFFSET &preTransLsn,
                                const DPS_LSN_OFFSET &firstTransLsn,
                                const UINT8  &attr,
                                const UINT32 *pNodeNum,
                                const UINT64 *pNodes,
                                dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB__DPS_TRANSCOMMIT2RECORD ) ;
      INT32 rc = SDB_OK ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_TS_COMMIT ;

      rc = record.push( DPS_LOG_PUBLIC_TRANSID,
                        sizeof( transID),
                        (CHAR *)(&transID)) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push transid to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_PUBLIC_PRETRANS,
                        sizeof( preTransLsn ),
                        ( CHAR* )&preTransLsn ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = record.push( DPS_LOG_PUBLIC_FIRSTTRANS,
                        sizeof( firstTransLsn ),
                        ( CHAR* )&firstTransLsn ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( 0 != attr )
      {
         rc = record.push( DPS_LOG_TSCOMMIT_ATTR,
                           sizeof( UINT8 ),
                           (const CHAR*)&attr ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( DPS_TS_COMMIT_ATTR_PRE == attr )
         {
            if ( !pNodeNum || !pNodes )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            rc = record.push( DPS_LOG_TSCOMMIT_NODE_NUM,
                              sizeof( UINT32 ),
                              ( const CHAR* )pNodeNum ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( *pNodeNum > 0 )
            {
               rc = record.push( DPS_LOG_TSCOMMIT_NODES,
                                 sizeof( UINT64 ) * ( *pNodeNum ),
                                 ( const CHAR* )pNodes ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_TRANSCOMMIT2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_TRANSROLLBACK2RECORD, "dpsTransRollback2Record" )
   INT32 dpsTransRollback2Record( const DPS_TRANS_ID &transID,
                                  const DPS_LSN_OFFSET &preTransLSN,
                                  const DPS_LSN_OFFSET &relatedLSN,
                                  dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_TRANSROLLBACK2RECORD ) ;

      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_TS_ROLLBACK ;

      rc = dpsPushTran( transID, preTransLSN, relatedLSN, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push transaction information, "
                   "rc: %d", rc ) ;

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time information, "
                   "rc: %d", rc ) ;

      header._length = record.alignedLen() ;

   done :
      PD_TRACE_EXITRC( SDB__DPS_TRANSROLLBACK2RECORD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_INVALIDCATA2RECORD, "dpsInvalidCata2Record" )
   INT32 dpsInvalidCata2Record( const UINT8 &type,
                                const CHAR * clFullName,
                                const CHAR * ixName,
                                dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_INVALIDCATA2RECORD ) ;

      // For invalidate catalog, must have clFullName
      SDB_ASSERT( NULL != clFullName, "Collection name can't be NULL" ) ;

      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_INVALIDATE_CATA ;

      rc = record.push( DPS_LOG_INVALIDCATA_TYPE,
                        sizeof( type ),
                        (CHAR *)( &type ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push type to record, rc: %d",rc ) ;
         goto error ;
      }

      if ( clFullName )
      {
         rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                           ossStrlen( clFullName ) + 1,
                           clFullName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to push fullname to record, rc: %d",rc ) ;
            goto error ;
         }
      }

      if ( ixName )
      {
         rc = record.push( DPS_LOG_INVALIDCATA_IXNAME,
                           ossStrlen( ixName ) + 1,
                           ixName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to push ixname to record, rc: %d",rc ) ;
            goto error ;
         }
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;

   done:
      PD_TRACE_EXITRC( SDB__DPS_INVALIDCATA2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2INVALIDCATA, "dpsRecord2InvalidCata")
   INT32 dpsRecord2InvalidCata( const CHAR *logRecord,
                                UINT8 &type,
                                const CHAR **clFullName,
                                const CHAR **ixName )
   {
      PD_TRACE_ENTRY( SDB__DPS_RECORD2INVALIDCATA ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;
      dpsLogRecord record ;

      rc = record.load( logRecord ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load invalid cata record, rc: %d",rc ) ;
         goto error ;
      }

      {
         dpsLogRecord::iterator itrType =
                     record.find( DPS_LOG_INVALIDCATA_TYPE ) ;
         if ( itrType.valid() )
         {
            type = *( ( UINT8 * )( itrType.value() ) ) ;
         }
         else
         {
            type = DPS_LOG_INVALIDCATA_TYPE_CATA ;
         }
      }

      {
         dpsLogRecord::iterator itrFullName =
                     record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         if ( !itrFullName.valid() )
         {
            PD_LOG( PDERROR, "Failed to find tag fullname in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *clFullName = itrFullName.value() ;
      }

      {
         dpsLogRecord::iterator itrIXName =
                     record.find( DPS_LOG_INVALIDCATA_IXNAME ) ;
         if ( itrIXName.valid() )
         {
            *ixName = itrIXName.value() ;
         }
         else
         {
            *ixName = NULL ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2INVALIDCATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_LOBW2RECORD, "dpsLobW2Record" )
   INT32 dpsLobW2Record( const CHAR *fullName,
                         const bson::OID *oid,
                         const UINT32 &sequence,
                         const UINT32 &offset,
                         const UINT32 &hash,
                         const UINT32 &len,
                         const CHAR *data,
                         const UINT32 &pageSize,
                         const DMS_LOB_PAGEID &pageID,
                         const DPS_TRANS_ID &transID,
                         const DPS_LSN_OFFSET &preTransLsn,
                         const DPS_LSN_OFFSET &relatedLSN,
                         dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_LOBW2RECORD ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_LOB_WRITE ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( fullName ) + 1,
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push fullname to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OID,
                        sizeof( bson::OID ),
                        ( const CHAR * )oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push oid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_SEQUENCE,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &sequence ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push sequence to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OFFSET,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &offset ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push offset to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_HASH,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &hash ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push hash to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_LEN,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &len ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push len to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_DATA,
                        len,
                        data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push data to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE,
                        sizeof( DMS_LOB_PAGEID ),
                        ( const CHAR * )( &pageID ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push pageid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE_SIZE,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &pageSize ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push page size to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_LOBW2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2LOBW, "dpsRecord2LobW" )
   INT32 dpsRecord2LobW( const CHAR *raw,
                         const CHAR **fullName,
                         const bson::OID **oid,
                         UINT32 &sequence,
                         UINT32 &offset,
                         UINT32 &len,
                         UINT32 &hash,
                         const CHAR **data,
                         DMS_LOB_PAGEID &pageID,
                         UINT32* pageSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2LOBW ) ;
      SDB_ASSERT( NULL != raw, "can not be null" ) ;
      dpsLogRecord record ;

      rc = record.load( raw ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load lobw record:%d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *fullName = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OID ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag oid in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *oid = ( bson::OID * )( itr.value() ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_SEQUENCE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag sequence in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      sequence = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OFFSET ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag offset in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      offset = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_HASH ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag hash in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      hash = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_LEN ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag len in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      len = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_DATA ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag data in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *data = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag page in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pageID = *( ( DMS_LOB_PAGEID * )( itr.value() ) ) ;
      }

      if ( NULL != pageSize )
      {
         dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE_SIZE ) ;
         if ( !itr.valid() )
         {
            PD_LOG( PDERROR, "failed to find page size tag in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *pageSize = *( ( UINT32 * )( itr.value() ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2LOBW, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_LOBU2RECORD, "dpsLobU2Record" )
   INT32 dpsLobU2Record(  const CHAR *fullName,
                          const bson::OID *oid,
                          const UINT32 &sequence,
                          const UINT32 &offset,
                          const UINT32 &hash,
                          const UINT32 &len,
                          const CHAR *data,
                          const UINT32 &oldLen,
                          const CHAR *oldData,
                          const UINT32 &pageSize,
                          const DMS_LOB_PAGEID &pageID,
                          const DPS_TRANS_ID &transID,
                          const DPS_LSN_OFFSET &preTransLsn,
                          const DPS_LSN_OFFSET &relatedLSN,
                          dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_LOBU2RECORD ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_LOB_UPDATE ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( fullName ) + 1,
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push fullname to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OID,
                        sizeof( bson::OID ),
                        ( const CHAR * )oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push oid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_SEQUENCE,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &sequence ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push sequence to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OFFSET,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &offset ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push offset to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_HASH,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &hash ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push hash to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_LEN,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &len ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push len to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_DATA,
                        len,
                        data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push data to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE,
                        sizeof( DMS_LOB_PAGEID ),
                        ( const CHAR * )( &pageID ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push pageid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OLD_LEN,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &oldLen ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push old len to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OLD_DATA,
                        oldLen,
                        oldData ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push old data to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE_SIZE,
                        sizeof ( UINT32 ),
                        (const CHAR*)( &pageSize ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push page size to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_LOBU2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2LOBU, "dpsRecord2LobU" )
   INT32 dpsRecord2LobU( const CHAR *raw,
                         const CHAR **fullName,
                         const bson::OID **oid,
                         UINT32 &sequence,
                         UINT32 &offset,
                         UINT32 &len,
                         UINT32 &hash,
                         const CHAR **data,
                         UINT32 &oldLen,
                         const CHAR **oldData,
                         DMS_LOB_PAGEID &pageID,
                         UINT32* pageSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2LOBU ) ;
      dpsLogRecord record ;
      rc = record.load( raw ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load lobu record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *fullName = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OID ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag oid in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *oid = ( bson::OID * )( itr.value() ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_SEQUENCE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag sequence in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      sequence = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OFFSET ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag offset in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      offset = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_HASH ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag hash in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      hash = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_LEN ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag len in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      len = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_DATA ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag data in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *data = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OLD_DATA ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag old data in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *oldData = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OLD_LEN ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag old len in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      oldLen = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag page in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pageID = *( ( DMS_LOB_PAGEID * )( itr.value() ) ) ;
      }

      if ( NULL != pageSize )
      {
         dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE_SIZE ) ;
         if ( !itr.valid() )
         {
            PD_LOG( PDERROR, "failed to find page size tag in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *pageSize = *( ( UINT32 * )( itr.value() ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2LOBU, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_LOBRM2RECORD, "dpsLobRm2Record" )
   INT32 dpsLobRm2Record( const CHAR *fullName,
                          const bson::OID *oid,
                          const UINT32 &sequence,
                          const UINT32 &offset,
                          const UINT32 &hash,
                          const UINT32 &len,
                          const CHAR *data,
                          const UINT32 &pageSize,
                          const DMS_LOB_PAGEID &page,
                          const DPS_TRANS_ID &transID,
                          const DPS_LSN_OFFSET &preTransLsn,
                          const DPS_LSN_OFFSET &relatedLSN,
                          dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_LOBRM2RECORD ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_LOB_REMOVE ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( fullName ) + 1,
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push fullname to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OID,
                        sizeof( bson::OID ),
                        ( const CHAR * )oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push oid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_SEQUENCE,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &sequence ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push sequence to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_OFFSET,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &offset ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push offset to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_HASH,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &hash ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push hash to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_LEN,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &len ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push len to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_DATA,
                        len,
                        data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push data to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE,
                        sizeof( DMS_LOB_PAGEID ),
                        ( const CHAR * )( &page ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push pageid to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = record.push( DPS_LOG_LOB_PAGE_SIZE,
                        sizeof( UINT32 ),
                        ( const CHAR * )( &pageSize ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push page size to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = dpsPushTran( transID, preTransLsn, relatedLSN, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push trans to record, rc: %d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_LOBRM2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2LOBRM, "dpsRecord2LobRm" )
   INT32 dpsRecord2LobRm( const CHAR *raw,
                          const CHAR **fullName,
                          const bson::OID **oid,
                          UINT32 &sequence,
                          UINT32 &offset,
                          UINT32 &len,
                          UINT32 &hash,
                          const CHAR **data,
                          DMS_LOB_PAGEID &pageID,
                          UINT32* pageSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2LOBRM ) ;
      dpsLogRecord record ;
      rc = record.load( raw ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load lobrm record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *fullName = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OID ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag oid in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *oid = ( bson::OID * )( itr.value() ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_SEQUENCE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag sequence in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      sequence = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OFFSET ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag offset in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      offset = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_HASH ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag hash in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      hash = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_LEN ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag len in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      len = *( ( UINT32 * )( itr.value() ) ) ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_DATA ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag data in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *data = itr.value() ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag page in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pageID = *( ( DMS_LOB_PAGEID * )( itr.value() ) ) ;
      }

      if ( NULL != pageSize )
      {
         dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_PAGE_SIZE ) ;
         if ( !itr.valid() )
         {
            PD_LOG( PDERROR, "failed to find page size tag in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *pageSize = *( ( UINT32 * )( itr.value() ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2LOBRM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_LOBTRUNCATE2RECORD, "dpsLobTruncate2Record" )
   INT32 dpsLobTruncate2Record( const CHAR *fullName,
                                dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_LOBTRUNCATE2RECORD ) ;
      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_LOB_TRUNCATE ;
      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( fullName ) + 1,
                        fullName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push fullname to record, rc:%d", rc ) ;
         goto error ;
      }

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;
   done:
      PD_TRACE_EXITRC( SDB__DPS_LOBTRUNCATE2RECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_RECORD2LOBTRUNCATE, "dpsRecord2LobTruncate" )
   INT32 dpsRecord2LobTruncate( const CHAR *raw,
                                const CHAR **fullName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2LOBTRUNCATE ) ;
      dpsLogRecord record ;
      rc = record.load( raw ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load lobu record, rc: %d", rc ) ;
         goto error ;
      }

      {
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      if ( !itr.valid() )
      {
         PD_LOG( PDERROR, "failed to find tag fullname in record" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *fullName = itr.value() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPS_RECORD2LOBTRUNCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_ALTER2RECORD, "dpsAlter2Record" )
   INT32 dpsAlter2Record ( const CHAR * name,
                           const INT32 & objectType,
                           const bson::BSONObj & alterObject,
                           dpsLogRecord & record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_ALTER2RECORD ) ;

      dpsLogRecordHeader & header = record.head() ;

      header._type = LOG_TYPE_ALTER ;

      rc = record.push( DPS_LOG_PUBLIC_FULLNAME,
                        ossStrlen( name ) + 1,
                        name ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push name to record, rc: %d", rc ) ;

      rc = record.push( DPS_LOG_ALTER_OBJECT_TYPE, sizeof( INT32 ),
                        ( CHAR * )( &objectType ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push alter objec type to record, "
                   "rc: %d", rc ) ;

      rc = record.push( DPS_LOG_ALTER_OBJECT, alterObject.objsize(),
                        alterObject.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push alter object to record, "
                   "rc: %d", rc ) ;

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;

   done :
      PD_TRACE_EXITRC( SDB__DPS_ALTER2RECORD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2ALTER, "dpsRecord2Alter" )
   INT32 dpsRecord2Alter ( const CHAR * logRecord,
                           const CHAR ** name,
                           INT32 & objectType,
                           bson::BSONObj & alterObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_RECORD2ALTER ) ;

      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;

      dpsLogRecord record ;
      dpsLogRecord::iterator iterRecord ;

      rc = record.load( logRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load alter record, rc: %d",rc ) ;

      iterRecord = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
      PD_CHECK( iterRecord.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag name in record" ) ;

      ( *name ) = iterRecord.value() ;

      iterRecord = record.find( DPS_LOG_ALTER_OBJECT_TYPE ) ;
      PD_CHECK( iterRecord.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag alter object type in record" ) ;

      objectType = *( (INT32 *)( iterRecord.value() ) ) ;

      iterRecord = record.find( DPS_LOG_ALTER_OBJECT ) ;
      PD_CHECK( iterRecord.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag alter object in record" ) ;

      alterObject = BSONObj( iterRecord.value() ) ;

   done :
      PD_TRACE_EXITRC( SDB__DPS_RECORD2ALTER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPS_UID2RECORD, "dpsAddUniqueID2Record" )
   INT32 dpsAddUniqueID2Record ( const CHAR* csname,
                                 const utilCSUniqueID& csUniqueID,
                                 const bson::BSONObj& clInfoObj,
                                 dpsLogRecord& record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_UID2RECORD ) ;

      dpsLogRecordHeader & header = record.head() ;

      header._type = LOG_TYPE_ADDUNIQUEID ;

      rc = record.push( DPS_LOG_ADDUNIQUEID_CSNAME,
                        ossStrlen( csname ) + 1,
                        csname ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push csname to record, rc: %d",
                   rc ) ;

      rc = record.push( DPS_LOG_ADDUNIQUEID_CSUNIQUEID,
                        sizeof( csUniqueID ),
                        ( const CHAR * )( &csUniqueID ) ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push cs unique id, rc: %d",
                   rc ) ;

      rc = record.push( DPS_LOG_ADDUNIQUEID_CLINFO,
                        clInfoObj.objsize(),
                        clInfoObj.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push cl info, rc: %d",
                   rc ) ;

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc = %d", rc ) ;

      header._length = record.alignedLen() ;

   done :
      PD_TRACE_EXITRC( SDB__DPS_UID2RECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RECORD2UID, "dpsRecord2AddUniqueID" )
   INT32 dpsRecord2AddUniqueID ( const CHAR* logRecord,
                                 const CHAR** csname,
                                 utilCSUniqueID& csUniqueID,
                                 bson::BSONObj & clInfoObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPS_RECORD2UID ) ;

      SDB_ASSERT( NULL != logRecord, "Record can't be NULL" ) ;

      dpsLogRecord record ;
      dpsLogRecord::iterator it ;

      rc = record.load( logRecord ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to load add unique id record, rc: %d",
                   rc ) ;

      it = record.find( DPS_LOG_ADDUNIQUEID_CSNAME ) ;
      PD_CHECK( it.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag cs name in record" ) ;

      ( *csname ) = it.value() ;

      it = record.find( DPS_LOG_ADDUNIQUEID_CSUNIQUEID ) ;
      PD_CHECK( it.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag cs unique id in record" ) ;

      csUniqueID = *( (utilCSUniqueID *)( it.value() ) ) ;

      it = record.find( DPS_LOG_ADDUNIQUEID_CLINFO ) ;
      PD_CHECK( it.value(), SDB_SYS, error, PDERROR,
                "Failed to find tag cl info in record" ) ;

      clInfoObj = BSONObj( it.value() ) ;

   done :
      PD_TRACE_EXITRC( SDB__DPS_RECORD2UID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_RTRN2REC, "dpsReturn2Record" )
   INT32 dpsReturn2Record( bson::BSONObj *boOptions,
                           dpsLogRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_RTRN2REC ) ;

      dpsLogRecordHeader &header = record.head() ;
      header._type = LOG_TYPE_RETURN ;

      SDB_ASSERT( NULL != boOptions && !boOptions->isEmpty(),
                  "options should be valid" ) ;
      PD_CHECK( NULL != boOptions && !boOptions->isEmpty(),
                SDB_SYS, error, PDERROR,
                "Failed to build return record, options is invalid" ) ;

      rc = record.push( DPS_LOG_RETURN_OPTIONS,
                        boOptions->objsize(),
                        boOptions->objdata() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push return options, "
                   "rc: %d", rc ) ;

      rc = checkAndAddTimeInfo( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add time info, rc: %d", rc ) ;

      header._length = record.alignedLen() ;

   done:
      PD_TRACE_EXITRC( SDB__DPS_RTRN2REC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_REC2RTRN, "dpsRecord2Return" )
   INT32 dpsRecord2Return( const CHAR *logRecord,
                           BSONObj *boOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_REC2RTRN ) ;

      dpsLogRecord record ;

      rc = record.load( logRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load recycle record, rc: %d", rc ) ;

      if ( NULL != boOptions )
      {
         dpsLogRecord::iterator itrOptions =
                                 record.find( DPS_LOG_RETURN_OPTIONS ) ;
         try
         {
            if ( itrOptions.valid() )
            {
               *boOptions = BSONObj( itrOptions.value() ) ;
            }
            else
            {
               *boOptions = BSONObj() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get options, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_REC2RTRN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_GETTRANSIDFROMEREC, "dpsGetTransIDFromRecord" )
   INT32 dpsGetTransIDFromRecord( const CHAR* logRecord,
                                  BOOLEAN isRequired,
                                  DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_GETTRANSIDFROMEREC ) ;

      SDB_ASSERT( NULL != logRecord, "record should not be NULL" ) ;

      dpsLogRecord record ;
      dpsLogRecord::iterator it ;

      rc = record.load( logRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load record, rc: %d", rc ) ;

      rc = dpsGetTransIDFromRecord( record, isRequired, transID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get transaction ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DPS_GETTRANSIDFROMEREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__DPS_GETTRANSIDFROMEREC_REC, "dpsGetTransIDFromRecord" )
   INT32 dpsGetTransIDFromRecord( const dpsLogRecord &record,
                                  BOOLEAN isRequired,
                                  DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPS_GETTRANSIDFROMEREC_REC ) ;

      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_TRANSID ) ;
      if ( itr.valid() )
      {
         DPS_TRANS_ID tmpTransID = *( (DPS_TRANS_ID *)( itr.value() ) ) ;
         itr = record.find( DPS_LOG_PUBLIC_TRANSID_NODEID ) ;
         if ( itr.valid() )
         {
            DPS_TRANSID_NODEID nodeID = *( (DPS_TRANSID_NODEID *)( itr.value() ) ) ;
            dpsTransID_v1 transIDV1( tmpTransID, nodeID ) ;
            transID = dpsTransIDDowngrade( transIDV1 ) ;
         }
         else
         {
            transID = tmpTransID ;
         }
      }
      else
      {
         transID = DPS_INVALID_TRANS_ID ;
         if ( isRequired )
         {
            PD_LOG( PDERROR, "Failed to find tag transid in record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPS_GETTRANSIDFROMEREC_REC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
