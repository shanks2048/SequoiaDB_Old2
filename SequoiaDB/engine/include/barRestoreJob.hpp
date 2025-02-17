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

   Source File Name = rtnPrefetchJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef BAR_RESTORE_JOB_HPP__
#define BAR_RESTORE_JOB_HPP__

#include "rtnBackgroundJob.hpp"
#include "barBkupLogger.hpp"

namespace engine
{

   /*
      _barRestoreJob define
   */
   class _barRestoreJob : public _rtnBaseJob
   {
      public:
         _barRestoreJob ( barRSBaseLogger *pRSLogger ) ;
         virtual ~_barRestoreJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      private:
         barRSBaseLogger         *_rsLogger ;

   } ;
   typedef _barRestoreJob  barRestoreJob ;

   INT32 startRestoreJob ( EDUID *pEDUID, barRSBaseLogger *pRSLogger ) ;

}

#endif //BAR_RESTORE_JOB_HPP__

