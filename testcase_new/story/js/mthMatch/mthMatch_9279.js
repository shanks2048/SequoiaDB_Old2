/************************************
*@Description: add head tree,
               one field use one time
               use arr element
               match tree and match condition have different order.
               head tree and string comparison of the field name have different order.
*@author:      zhaoyu
*@createdate:  2016.8.8
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, a: [1000, 1001, 1002] },
   { No: 2, a: [1001, 1002, 1003] },
   { No: 3, a: [1002, 1003, 1004] },
   { No: 4, a: [1003, 1004, 1005] }];
   dbcl.insert( doc );

   //and
   //the head tree and the match tree have the same order
   var findCondition1 = { $and: [{ "a.1": { $in: [1001, 1002, 1008] } }, { No: { $nin: [2, 3, 4] } }] };
   var expRecs1 = [{ No: 1, a: [1000, 1001, 1002] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //the head tree has different order
   var findCondition2 = { $and: [{ No: { $gt: 2 } }, { "a.1": { $in: [1001, 1002, 1008] } }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //the match tree has different order
   var findCondition3 = { $and: [{ "a.1": { $nin: [1001] } }, { No: { $in: [2, 3] } }] };
   var expRecs3 = [{ No: 2, a: [1001, 1002, 1003] },
   { No: 3, a: [1002, 1003, 1004] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition4 = { $and: [{ No: { $nin: [2, 5, 1] } }, { "a.1": { $in: [1005, 1002, 1008] } }] };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );
}
