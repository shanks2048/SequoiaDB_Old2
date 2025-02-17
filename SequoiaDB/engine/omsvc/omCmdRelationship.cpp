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

   Source File Name = omBusinessCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/06//2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "omCommand.hpp"
#include "omDef.hpp"
#include <set>
#include <stdlib.h>
#include <sstream>

using namespace bson;
using namespace boost::property_tree;


namespace engine
{

   // ***************** omCreateRelationshipCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omCreateRelationshipCommand ) ;

   omCreateRelationshipCommand::~omCreateRelationshipCommand()
   {
   }

   INT32 omCreateRelationshipCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj fromBuzInfo ;
      BSONObj toBuzInfo ;
      BSONObj options ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "sssj",
                             OM_REST_FIELD_NAME,    &_name,
                             OM_REST_FIELD_FROM,    &_fromBuzName,
                             OM_REST_FIELD_TO,      &_toBuzName,
                             OM_REST_FIELD_OPTIONS, &options ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( fromBuzInfo, toBuzInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _createRelationship( options, fromBuzInfo, toBuzInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to create relationship: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      try
      {
         _recordHistory( rc, OM_CREATE_RELATIONSHIP_REQ,
                         BSON( OM_BSON_NAME << _name <<
                               OM_BSON_FROM << _fromBuzName <<
                               OM_BSON_TO << _toBuzName ) ) ;
      }
      catch( std::exception &e )
      {
      }
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omCreateRelationshipCommand::_check( BSONObj &fromBuzInfo,
                                              BSONObj &toBuzInfo )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;
      string formBuzType ;

      //get from business info
      rc = dbTool.getOneBusinessInfo( _fromBuzName, fromBuzInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: %s",
                             _fromBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      formBuzType = fromBuzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;

      //get to business info
      rc = dbTool.getOneBusinessInfo( _toBuzName, toBuzInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: %s",
                             _toBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == dbTool.isRelationshipExist( _name ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "relationship already exist: from=%s, to=%s",
                             _fromBuzName.c_str(), _toBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( ( OM_BUSINESS_SEQUOIASQL_MYSQL == formBuzType ||
             OM_BUSINESS_SEQUOIASQL_MARIADB == formBuzType ) &&
           dbTool.isRelationshipExistByBusiness( _fromBuzName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Can't be relationship multiple times: "
                                   "from=%s, type=%s",
                             _fromBuzName.c_str(), formBuzType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //check business does not running
      taskID = dbTool.getTaskIdOfRunningBuz( _fromBuzName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task[" OSS_LL_PRINT_FORMAT "]",
                             _fromBuzName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _toBuzName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task[" OSS_LL_PRINT_FORMAT "]",
                             _toBuzName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCreateRelationshipCommand::_createRelationship(
                                                   const BSONObj &options,
                                                   const BSONObj &fromBuzInfo,
                                                   const BSONObj &toBuzInfo )

   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      BSONObj request ;
      BSONObj result ;
      omDatabaseTool dbTool( _cb ) ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = _generateRequest( options, fromBuzInfo, toBuzInfo, request ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate request: rc=%d", rc ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentMsg( CMD_ADMIN_PREFIX OM_CREATE_RELATIONSHIP_REQ,
                                    request, errDetail, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "%s", errDetail.c_str() ) ;
         PD_LOG( PDERROR, "failed to notify agent: detail=%s, rc=%d",
                 errDetail.c_str(), rc ) ;
         goto error ;
      }

      rc = dbTool.createRelationship( _name, _fromBuzName, _toBuzName,
                                      options ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to create relationship: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCreateRelationshipCommand::_generateRequest(
                                                   const BSONObj &options,
                                                   const BSONObj &fromBuzInfo,
                                                   const BSONObj &toBuzInfo,
                                                   BSONObj &request )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder requestBuilder ;
      list<BSONObj> fromBuzConfig ;
      list<BSONObj> toBuzConfig ;
      omDatabaseTool dbTool( _cb ) ;

      requestBuilder.append( OM_BSON_NAME, _name ) ;

      //from business config
      rc = dbTool.getConfigByBusiness( _fromBuzName, fromBuzConfig ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business address: name=%s",
                             _fromBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //build from business info
      {
         string businessType ;
         BSONObj authInfo ;
         BSONObj filter ;
         BSONObjBuilder fromBuilder ;
         BSONArrayBuilder configBuilder ;
         list<BSONObj>::iterator iter ;

         //from business auth
         rc = dbTool.getAuth( _fromBuzName, authInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: name=%s",
                                _fromBuzName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         fromBuilder.append( OM_BSON_INFO, fromBuzInfo ) ;

         businessType = fromBuzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == businessType )
         {
            filter = BSON( OM_CONFIGURE_FIELD_PORT2 << "" <<
                           OM_CONFIGURE_FIELD_INSTALLPATH << "" ) ;
         }

         for ( iter = fromBuzConfig.begin(); iter != fromBuzConfig.end();
                                                                        ++iter )
         {
            string hostName ;
            string installPath ;
            BSONObj configInfo ;

            hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
            configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

            if( FALSE == dbTool.getHostPackagePath( hostName, businessType,
                                                    installPath ) )
            {
               rc = SDB_SYS ;
               _errorMsg.setError( TRUE, "Install path not found: "
                                         "name=%s, host=%s, type=%s",
                                   _fromBuzName.c_str(), hostName.c_str(),
                                   businessType.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               BSONObjIterator configIter( configInfo ) ;

               while ( configIter.more() )
               {
                  BSONObjBuilder nodeInfoBuilder ;
                  BSONElement ele = configIter.next() ;
                  BSONObj tmpNodeInfo = ele.embeddedObject() ;

                  nodeInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;

                  if ( filter.isEmpty() )
                  {
                     nodeInfoBuilder.appendElements( tmpNodeInfo ) ;
                  }
                  else
                  {
                     BSONObj nodeInfo = tmpNodeInfo.filterFieldsUndotted(
                                                               filter, TRUE ) ;

                     nodeInfoBuilder.appendElements( nodeInfo ) ;
                  }

                  nodeInfoBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;

                  configBuilder.append( nodeInfoBuilder.obj() ) ;
               }
            }
         }
         fromBuilder.append( OM_BSON_CONFIG, configBuilder.arr() ) ;

         fromBuilder.appendElements( authInfo ) ;

         requestBuilder.append( OM_BSON_FROM, fromBuilder.obj() ) ;
      }

      //to business config
      rc = dbTool.getConfigByBusiness( _toBuzName, toBuzConfig ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business address: name=%s",
                             _toBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //builder to business info
      {
         string businessType ;
         BSONObj authInfo ;
         BSONObj filter ;
         BSONObjBuilder toBuilder ;
         BSONArrayBuilder configBuilder ;
         list<BSONObj>::iterator iter ;

         //to business auth
         rc = dbTool.getAuth( _toBuzName, authInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: name=%s",
                                _toBuzName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         toBuilder.append( OM_BSON_INFO, toBuzInfo ) ;

         businessType = toBuzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         if ( OM_BUSINESS_SEQUOIADB == businessType )
         {
            filter = BSON( OM_CONFIGURE_FIELD_ROLE    << "" <<
                           OM_CONFIGURE_FIELD_SVCNAME << "" ) ;
         }

         for ( iter = toBuzConfig.begin(); iter != toBuzConfig.end(); ++iter )
         {
            string hostName ;
            BSONObj configInfo ;

            hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
            configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

            {
               BSONObjIterator configIter( configInfo ) ;

               while ( configIter.more() )
               {
                  BSONObjBuilder nodeInfoBuilder ;
                  BSONElement ele = configIter.next() ;
                  BSONObj tmpNodeInfo = ele.embeddedObject() ;
                  BSONObj nodeInfo = tmpNodeInfo.filterFieldsUndotted( filter,
                                                                       TRUE ) ;

                  nodeInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
                  nodeInfoBuilder.appendElements( nodeInfo ) ;

                  configBuilder.append( nodeInfoBuilder.obj() ) ;
               }
            }
         }
         toBuilder.append( OM_BSON_CONFIG, configBuilder.arr() ) ;

         toBuilder.appendElements( authInfo ) ;

         requestBuilder.append( OM_BSON_TO, toBuilder.obj() ) ;
      }

      requestBuilder.append( OM_BSON_OPTIONS, options ) ;

      request = requestBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // ***************** omRemoveRelationshipCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omRemoveRelationshipCommand ) ;

   omRemoveRelationshipCommand::~omRemoveRelationshipCommand()
   {
   }

   INT32 omRemoveRelationshipCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj fromBuzInfo ;
      BSONObj toBuzInfo ;
      BSONObj options ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "s", OM_REST_FIELD_NAME, &_name) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( options, fromBuzInfo, toBuzInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _removeRelationship( options, fromBuzInfo, toBuzInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate request: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      try
      {
         _recordHistory( rc, OM_REMOVE_RELATIONSHIP_REQ,
                         BSON( OM_BSON_NAME << _name ) ) ;
      }
      catch( std::exception &e )
      {
      }
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omRemoveRelationshipCommand::_check( BSONObj &options,
                                              BSONObj &fromBuzInfo,
                                              BSONObj &toBuzInfo )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      string businessType ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getRelationshipInfo( _name, _fromBuzName, _toBuzName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "relationship does not exist: %s",
                             _name.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //get from business info
      rc = dbTool.getOneBusinessInfo( _fromBuzName, fromBuzInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //get to business info
      rc = dbTool.getOneBusinessInfo( _toBuzName, toBuzInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //get relationship options
      rc = dbTool.getRelationshipOptions( _name, options ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         _errorMsg.setError( TRUE, "relationship does not exist: from=%s, to=%s",
                             _fromBuzName.c_str(), _toBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get relationship info,rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //check business does not running
      taskID = dbTool.getTaskIdOfRunningBuz( _fromBuzName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task[" OSS_LL_PRINT_FORMAT "]",
                             _fromBuzName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _toBuzName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task[" OSS_LL_PRINT_FORMAT "]",
                             _toBuzName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveRelationshipCommand::_removeRelationship(
                                                   const BSONObj &options,
                                                   const BSONObj &fromBuzInfo,
                                                   const BSONObj &toBuzInfo )
   {
       INT32 rc = SDB_OK ;
       string errDetail ;
       BSONObj request ;
       BSONObj result ;
       omDatabaseTool dbTool( _cb ) ;
       omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

       rc = _generateRequest( options, fromBuzInfo, toBuzInfo, request ) ;
       if ( rc )
       {
          PD_LOG( PDERROR, "failed to generate request: rc=%d", rc ) ;
          goto error ;
       }

       rc = taskTool.notifyAgentMsg( CMD_ADMIN_PREFIX OM_REMOVE_RELATIONSHIP_REQ,
                                     request, errDetail, result ) ;
       if ( rc )
       {
          _errorMsg.setError( TRUE, "%s", errDetail.c_str() ) ;
          PD_LOG( PDERROR, "failed to notify agent: detail=%s, rc=%d",
                  errDetail.c_str(), rc ) ;
          goto error ;
       }

       rc = dbTool.removeRelationship( _name ) ;
       if ( rc )
       {
          _errorMsg.setError( TRUE, "failed to create relationship: rc=%d", rc ) ;
          PD_LOG( PDERROR, _errorMsg.getError() ) ;
          goto error ;
       }

    done:
       return rc ;
    error:
       goto done ;
    }

   INT32 omRemoveRelationshipCommand::_generateRequest(
                                                   const BSONObj &options,
                                                   const BSONObj &fromBuzInfo,
                                                   const BSONObj &toBuzInfo,
                                                   BSONObj &request )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder requestBuilder ;
      list<BSONObj> fromBuzConfig ;
      list<BSONObj> toBuzConfig ;
      omDatabaseTool dbTool( _cb ) ;

      requestBuilder.append( OM_BSON_NAME, _name ) ;

      //from business config
      rc = dbTool.getConfigByBusiness( _fromBuzName, fromBuzConfig ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business address: name=%s",
                             _fromBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //build from business info
      {
         string businessType ;
         BSONObj authInfo ;
         BSONObj filter ;
         BSONObjBuilder fromBuilder ;
         BSONArrayBuilder configBuilder ;
         list<BSONObj>::iterator iter ;

         //from business auth
         rc = dbTool.getAuth( _fromBuzName, authInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: name=%s",
                                _fromBuzName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         fromBuilder.append( OM_BSON_INFO, fromBuzInfo ) ;

         businessType = fromBuzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == businessType )
         {
            filter = BSON( OM_CONFIGURE_FIELD_PORT2 << "" <<
                           OM_CONFIGURE_FIELD_INSTALLPATH << "" ) ;
         }

         for ( iter = fromBuzConfig.begin(); iter != fromBuzConfig.end();
                                                                        ++iter )
         {
            string installPath ;
            string hostName ;
            BSONObj configInfo ;

            hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
            configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

            if( FALSE == dbTool.getHostPackagePath( hostName, businessType,
                                                    installPath ) )
            {
               rc = SDB_SYS ;
               _errorMsg.setError( TRUE, "Install path not found: "
                                         "name=%s, host=%s, type=%s",
                                   _fromBuzName.c_str(), hostName.c_str(),
                                   businessType.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               BSONObjIterator configIter( configInfo ) ;

               while ( configIter.more() )
               {
                  BSONObjBuilder nodeInfoBuilder ;
                  BSONElement ele = configIter.next() ;
                  BSONObj tmpNodeInfo = ele.embeddedObject() ;

                  nodeInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;

                  if ( filter.isEmpty() )
                  {
                     nodeInfoBuilder.appendElements( tmpNodeInfo ) ;
                  }
                  else
                  {
                     BSONObj nodeInfo = tmpNodeInfo.filterFieldsUndotted(
                                                               filter, TRUE ) ;

                     nodeInfoBuilder.appendElements( nodeInfo ) ;
                  }

                  nodeInfoBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;

                  configBuilder.append( nodeInfoBuilder.obj() ) ;
               }
            }
         }
         fromBuilder.append( OM_BSON_CONFIG, configBuilder.arr() ) ;

         fromBuilder.appendElements( authInfo ) ;

         requestBuilder.append( OM_BSON_FROM, fromBuilder.obj() ) ;
      }

      //to business config
      rc = dbTool.getConfigByBusiness( _toBuzName, toBuzConfig ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business address: name=%s",
                             _toBuzName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //builder to business info
      {
         string businessType ;
         BSONObj authInfo ;
         BSONObj filter ;
         BSONObjBuilder toBuilder ;
         BSONArrayBuilder configBuilder ;
         list<BSONObj>::iterator iter ;

         //to business auth
         rc = dbTool.getAuth( _toBuzName, authInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: name=%s",
                                _toBuzName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         toBuilder.append( OM_BSON_INFO, toBuzInfo ) ;

         businessType = toBuzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         if ( OM_BUSINESS_SEQUOIADB == businessType )
         {
            filter = BSON( OM_CONFIGURE_FIELD_ROLE    << "" <<
                           OM_CONFIGURE_FIELD_SVCNAME << "" ) ;
         }

         for ( iter = toBuzConfig.begin(); iter != toBuzConfig.end(); ++iter )
         {
            string hostName ;
            BSONObj configInfo ;

            hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
            configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

            {
               BSONObjIterator configIter( configInfo ) ;

               while ( configIter.more() )
               {
                  BSONObjBuilder nodeInfoBuilder ;
                  BSONElement ele = configIter.next() ;
                  BSONObj tmpNodeInfo = ele.embeddedObject() ;
                  BSONObj nodeInfo = tmpNodeInfo.filterFieldsUndotted( filter,
                                                                       TRUE ) ;

                  nodeInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
                  nodeInfoBuilder.appendElements( nodeInfo ) ;

                  configBuilder.append( nodeInfoBuilder.obj() ) ;
               }
            }
         }
         toBuilder.append( OM_BSON_CONFIG, configBuilder.arr() ) ;

         toBuilder.appendElements( authInfo ) ;

         requestBuilder.append( OM_BSON_TO, toBuilder.obj() ) ;
      }

      requestBuilder.append( OM_BSON_OPTIONS, options ) ;

      request = requestBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // ***************** omListRelationshipCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omListRelationshipCommand ) ;

   omListRelationshipCommand::~omListRelationshipCommand()
   {
   }

   INT32 omListRelationshipCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> relationshipList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = dbTool.getRelationshipList( relationshipList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get relationship info: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = relationshipList.begin(); iter != relationshipList.end();
                                                                        ++iter )
      {
         BSONObj relationshipInfo = *iter ;

         rc = restTool.appendResponeContent( relationshipInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to build respone: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

}

