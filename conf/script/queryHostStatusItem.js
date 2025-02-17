/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: the following items are need to check by queryHostStatus.js
@modify list:
   2014-9-30 Youbin Lin  Init
*/

function CPUSnapInfo()
{
   this.Sys        = 0 ;
   this.Idle       = 0 ;
   this.Other      = 0 ;
   this.User       = 0 ;
}

function MemorySnapInfo()
{
   this.Size       = 0 ;
   this.Free       = 0 ;
   this.Used       = 0 ;
}

function NetSnapInfo()
{
   this.CalendarTime = 0 ;
   this.Netcards     = [] ;
}

function DiskSnapInfo()
{
   this.Name        = "" ;
   this.Mount       = "" ;
   this.Size        = 0 ;
   this.Free        = 0 ;
   this.ReadSec     = 0 ;
   this.WriteSec    = 0 ;
}


