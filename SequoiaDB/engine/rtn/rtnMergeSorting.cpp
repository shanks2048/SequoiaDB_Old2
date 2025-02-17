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

   Source File Name = rtnMergeSorting.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnMergeSorting.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEBLOCK_NEXT, "_rtnMergeBlock::next" )
   INT32 _rtnMergeBlock::next( _dmsTmpBlkUnit *unit,
                               _rtnSortTuple **tuple )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGEBLOCK_NEXT ) ;
      SDB_ASSERT( _read <= _loadSize, "impossible" ) ;

      if ( 0 == _limit )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      do
      {
         if ( _loadSize - _read < sizeof( INT32 ) )
         {
            rc = _loadData( unit ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else if ( _loadSize - _read <
                   *((UINT32 *)( _buf + _read )))
         {
            rc = _loadData( unit ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            *tuple = ( _rtnSortTuple *)(_buf + _read) ;
            _read += (*tuple)->len() ;
            break ;
         }
      } while ( TRUE ) ;

      SDB_ASSERT( 0 != _limit, "impossible" ) ;
      if ( 0 < _limit )
      {
         --_limit;
      }
      //0 < _limit ? --_limit:_limit;
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGEBLOCK_NEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEBLOCK_INIT, "_rtnMergeBlock::init" )
   void _rtnMergeBlock::init( _dmsTmpBlk &blk, CHAR *begin,
                              UINT64 size, SINT64 limit )
   {
      PD_TRACE_ENTRY( SDB__RTNMERGEBLOCK_INIT ) ;
      SDB_ASSERT( NULL != begin, "begin is NULL" ) ;
      _blk = blk ;
      _buf = begin ;
      _size = size ;
      _read = 0 ;
      _loadSize = 0 ;
      _limit = limit ;
      PD_TRACE_EXIT( SDB__RTNMERGEBLOCK_INIT ) ;
      return  ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEBLOCK__LOADDATA, "_rtnMergeBlock::_loadData" )
   INT32 _rtnMergeBlock::_loadData( _dmsTmpBlkUnit *unit )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGEBLOCK__LOADDATA ) ;
      SDB_ASSERT( _read <= _loadSize, "impossible" ) ;
      UINT64 lastLen = _loadSize - _read ;

      if ( 0 != lastLen )
      {
         /// keep the last obj in buf.
         ossMemcpy( _buf, _buf + _read, lastLen ) ;
      }

      rc = unit->read( _blk, _size - lastLen,
                       _buf + lastLen, _loadSize ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _loadSize += lastLen ;
      _read = 0 ;
      SDB_ASSERT( _loadSize <= _size, "impossible" ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGEBLOCK__LOADDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

///////////_rtnMergeSorting
   _rtnMergeSorting::_rtnMergeSorting( _dmsTmpBlkUnit *unit,
                                       const BSONObj &orderby )
   :_order( Ordering::make(orderby) ),
    _buf( NULL ),
    _size( 0 ),
    _mergeBufSize( 0 ),
    _mergePos( 0 ),
    _limit(-1),
    _heap(_order),
    _mergeBlkSize( 0 ),
    _mergeMax(0),
    _src( NULL ),
    _mergeDone( FALSE),
    _unit( unit )
   {
      SDB_ASSERT( NULL != _unit, "impossible" ) ;
      _unitHelper._originalSize = _unit->totalSize() ;
      _unitHelper._outputStart = _unitHelper._originalSize ;
   }

   _rtnMergeSorting::~_rtnMergeSorting()
   {
   }

   UINT64 _rtnMergeSorting::calcMinBufSize( UINT32 maxRecordSize )
   {
      return ( maxRecordSize * 2 * RTN_SORT_MIN_MERGESIZE ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING_INIT, "_rtnMergeSorting::init" )
   INT32 _rtnMergeSorting::init( CHAR *buf, UINT64 size,
                                 RTN_SORT_BLKS &src,
                                 UINT32 maxRecordSize,
                                 SINT64 limit )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING_INIT ) ;
      SDB_ASSERT( NULL != buf && size != 0, "impossible" ) ;

      UINT32 mergeSize = size / ( maxRecordSize * 2 );
      if ( mergeSize < RTN_SORT_MIN_MERGESIZE )
      {
         PD_LOG( PDERROR, "buf is too small to merge records." ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( RTN_SORT_MAX_MERGESIZE < mergeSize )
      {
         _mergeMax = RTN_SORT_MAX_MERGESIZE ;
      }
      else
      {
         _mergeMax = mergeSize ;
      }

      _src = &src ;
      _buf = buf ;
      _size = size ;
      _limit = limit ;

      PD_LOG( PDDEBUG, "number of blks[%d], size of buf[%lld]"
              " max merge size[%d], limit[%lld]",
              _src->size(), _size, _mergeMax, _limit ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING_FETCH, "_rtnMergeSorting::fetch" )
   INT32 _rtnMergeSorting::fetch( BSONObj &key, const CHAR** obj, INT32* objLen,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING_FETCH ) ;
      SDB_ASSERT( NULL != _buf && 0 < _size && NULL != _src,
                  "impossible" ) ;

      _rtnMergeTuple node ;
      if ( !_mergeDone )
      {
         rc = _merge( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to merge records:%d", rc ) ;
            goto error ;
         }

         rc = _makeHeap( *_src, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         _mergeDone = TRUE ;
      }

      rc = _heap.pop( node ) ;
      if ( SDB_OK == rc )
      {
         SDB_ASSERT( NULL != node.tuple() && NULL != node.tuple()->obj(),
                     "can not be NULL" ) ;
         BSONObj poppedKey( node.key() ) ;
         INT32 keySize = poppedKey.objsize() ;
         INT32 objSize = node.objLen() ;
         ossMemcpy( _buf, poppedKey.objdata(), keySize + objSize ) ;
         try
         {
             key = BSONObj( _buf ) ;
             *obj = _buf + keySize ;
             *objLen = objSize ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         /// TODO: if there is only one blk left, we do not need to push record to
         /// heap.
         rc = _pushObjFromSink( node.getI() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to push obj to heap:%d", rc ) ;
            goto error ;
         }
         else
         {
            /// do noting.
         }
      }
      else if ( SDB_CLS_EMPTY_HEAP != rc )
      {
         PD_LOG( PDERROR, "failed to fetch next from heap:%d", rc ) ;
         goto error ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING__MERGE, "_rtnMergeSorting::_merge" )
   INT32 _rtnMergeSorting::_merge( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING__MERGE ) ;
      RTN_SORT_BLKS *dst = &_merged ;
      RTN_SORT_BLKS *tmp = NULL ;
      UINT32 loop = 0;
      
      while ( _mergeMax - 1 < _src->size() )
      {
         rc = _mergeBlks( *_src, *dst, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to merge blks:%d", rc ) ;
            goto error ;
         }

         SDB_ASSERT( _src->empty() && !dst->empty(), "impssible" ) ;
         tmp = _src ;
         _src = dst ;
         dst = tmp ;
         ++loop;

         SDB_ASSERT( _unitHelper._outputStart <= _unitHelper._originalSize * 2,
                     "impossible" ) ;
         _unitHelper._outputStart = (0 == loop % 2) ?
                                      _unitHelper._originalSize : 0;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING__MERGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING__MERGEBLKS, "_rtnMergeSorting::_mergeBlks" )
   INT32 _rtnMergeSorting::_mergeBlks( RTN_SORT_BLKS &src,
                                       RTN_SORT_BLKS &dst,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING__MERGEBLKS ) ;
      _rtnMergeTuple node ;

      while ( !src.empty() )
      {
         if ( cb->isInterrupted() )
         {
            PD_LOG( PDERROR, "cb is interrupted" ) ;
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = _makeHeap( src, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to make heap:%d", rc ) ;
            goto error ;
         }

         while ( TRUE )
         {
            if ( _heap.more() )
            {
               rc = _heap.pop( node ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to pop from heap:%d", rc ) ;
                  goto error ;
               }

               SDB_ASSERT( 0 < node.getI() && NULL != node.tuple(),
                           "impossible" ) ;
               SDB_ASSERT( _mergePos <= _mergeBufSize, "impossible" ) ;

               if ( node.tuple()->len() <=
                    _mergeBufSize - _mergePos )
               {
                  ossMemcpy( _buf + _mergePos,
                             node.tuple(),
                             node.tuple()->len());
                  _mergePos += node.tuple()->len() ;
               }
               /// if the buf is full. out put to unit.
               else
               {
                  rc = _unit->seek( _unitHelper._outputStart ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to seek file:%d", rc ) ;
                     goto error ;
                  }

                  rc = _unit->write( _buf, _mergePos,
                                     _unitHelper._outputStart ==
                                     _unit->totalSize() ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to write to unit:%d", rc ) ;
                     goto error ;
                  }

                  PD_LOG( PDDEBUG, "output start[%lld], [%lld] bytes "
                          "were mv to file at this time",
                          _unitHelper._outputStart, _mergePos ) ;


                  _unitHelper._outputStart += _mergePos ;
                  _unitHelper._newBlkSize += _mergePos ;

                  /// reset mem info.
                  ossMemcpy( _buf, node.tuple(), node.tuple()->len() ) ;
                  _mergePos = node.tuple()->len() ;
               }

               rc = _pushObjFromSink( node.getI() ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to push obj to heap:%d", rc ) ;
                  goto error ;
               }
               else
               {
                  /// do nothing.
               }
            }
            else
            {
               if ( 0 != _mergePos )
               {
                  rc = _unit->seek( _unitHelper._outputStart ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to seek file:%d", rc ) ;
                     goto error ;
                  }

                  rc = _unit->write( _buf, _mergePos,
                                     _unitHelper._outputStart ==
                                     _unit->totalSize() ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to write to unit:%d", rc ) ;
                     goto error ;
                  }

                  _unitHelper._outputStart += _mergePos ;
                  _unitHelper._newBlkSize += _mergePos ;

                  _mergePos = 0 ;
               }
               break ;
            }
         } // while (TRUE)

         /// one merge done.
         dmsTmpBlk blk ;
         rc = _unit->buildBlk( _unitHelper._outputStart - _unitHelper._newBlkSize,
                               _unitHelper._newBlkSize,
                               blk ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build block:%d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "build new blk[%s]", blk.toString().c_str() ) ;

         dst.push_back( blk ) ;
         _unitHelper._newBlkSize = 0 ;

      }// whiel (!src.empty())
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING__MERGEBLKS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING__MAKEHEAP, "_rtnMergeSorting::_makeHeap" )
   INT32 _rtnMergeSorting::_makeHeap( RTN_SORT_BLKS &src,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING__MAKEHEAP ) ;
      UINT32 blkSize = src.size() + 1 ;
      _mergeBlkSize = blkSize < _mergeMax ?
                      blkSize : _mergeMax ;
      _mergeBufSize = _size / _mergeBlkSize ;
      PD_LOG( PDDEBUG, "build [%d] blks to heap at tihs time.", _mergeBlkSize - 1 ) ;
      /// start from 1. 0 is out put buf.
      for ( UINT32 i = 1; i < _mergeBlkSize; i++ )
      {
         PD_LOG( PDDEBUG, "put the [%d] blk [%s] to the heap",
                 i, src.front().toString().c_str() ) ;

         /// init data sink
         _dataSink[i].init( src.front(),
                            _buf + i * _mergeBufSize,
                            _mergeBufSize, _limit );
         rc = _pushObjFromSink( i ) ;
         if ( SDB_OK == rc || SDB_DMS_EOC == rc )
         {
            /// do noting. if first push returned eoc, means limit is 0.
         }
         else
         {
            goto error ;
         }

         src.pop_front() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING__MAKEHEAP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGESORTING__PUSHOBJFROMSINK, "_rtnMergeSorting::_pushObjFromSink" )
   INT32 _rtnMergeSorting::_pushObjFromSink( UINT32 i )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMERGESORTING__PUSHOBJFROMSINK ) ;
      SDB_ASSERT( 0 < i && i < _mergeBlkSize, "impossible" ) ;
      _rtnSortTuple *tuple = NULL ;
      rc = _dataSink[i].next( _unit, &tuple ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _heap.push( _rtnMergeTuple( i, tuple)) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNMERGESORTING__PUSHOBJFROMSINK, rc ) ;
      return  rc ;
   error:
      goto done ;
   }
}

