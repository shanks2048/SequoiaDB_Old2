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

   Source File Name = coordAggrOperator.cpp

   Descriptive Name = Coord Aggregation

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   aggregation logic on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordAggrOperator.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"
#include "pmd.hpp"
#include "aggrBuilder.hpp"
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAggrOperator implement
   */
   _coordAggrOperator::_coordAggrOperator()
   {
   }

   _coordAggrOperator::~_coordAggrOperator()
   {
   }

   const CHAR* _coordAggrOperator::getName() const
   {
      return "Arrg" ;
   }

   BOOLEAN _coordAggrOperator::isReadOnly() const
   {
      return TRUE ;
   }

   INT32 _coordAggrOperator::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      aggrBuilder *pAggrBuilder = pmdGetKRCB()->getAggrCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      const CHAR *pCollectionName = NULL ;
      const CHAR *pObjs = NULL ;
      INT32 count = 0 ;
      BSONObj objs ;
      INT32 flags = 0 ;
      INT32 cataVer = 0;
      BSONObj obj ;

      contextID = -1 ;

      rc = msgExtractAggrRequest( (const CHAR*)pMsg, &pCollectionName,
                                  &pObjs, count, &flags ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse aggregate request "
                   "message, rc: %d", rc ) ;

      cb->setCurProcessName( pCollectionName ) ;
      MONQUERY_SET_NAME( cb, pCollectionName ) ;

      try
      {
         objs = BSONObj( pObjs ) ;

         /// Prepare last info
         CHAR szTmp[ MON_APP_LASTOP_DESC_LEN + 1 ] = { 0 } ;
         UINT32 len = 0 ;
         const CHAR *pObjData = pObjs ;
         for ( INT32 i = 0 ; i < count ; ++i )
         {
            BSONObj tmpObj( pObjData ) ;
            len += ossSnprintf( szTmp + len, MON_APP_LASTOP_DESC_LEN - len,
                                "%s", tmpObj.toString().c_str() ) ;
            pObjData += ossAlignX( (UINT32)tmpObj.objsize(), 4 ) ;
            if ( len >= MON_APP_LASTOP_DESC_LEN )
            {
               break ;
            }
         }
         // add last op info
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, ObjNum:%u, Objs:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName, count, szTmp,
                             flags, flags ) ;

         rc = pAggrBuilder->build( objs, count, pCollectionName,
                                   BSONObj(), cb, contextID,
                                   ((MsgOpAggregate*)pMsg)->version,
                                   &cataVer ) ;

         if ( buf && CATALOG_INVALID_VERSION != cataVer )
         {
            // DDL operation return SDB_OK, no need to set error bson
            if( ( SDB_CLIENT_CATA_VER_OLD == rc ) && ( 0 == buf->size() ) )
            {
                obj = utilGetErrorBson( rc, NULL, NULL ) ;
               *buf = rtnContextBuf( obj ) ;
            }

            buf->setStartFrom( cataVer );
         }
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DQL, pMsg->opCode, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "ContextID:%lld, ObjNum:%u, Objs:%s, Flag:0x%08x(%u)",
                      contextID, count, szTmp, flags, flags ) ;
         /// CHECK RESULT
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute aggregation operation, rc: %d",
                      rc );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception in aggregation: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

}

