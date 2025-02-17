/******************************************************************************
@Description : seqDB-13758:查询指定字段排序，字段值包含时间戳、日期（包含不带索引、带索引）
@Modify list :
               2020-08-13 wuyan  modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_cl_13758";

main( test );
function test ( testPara )
{
   var recs = [
      { "a": { "$timestamp": "2001-04-03-10.11.12.123456" } },
      { "a": { "$timestamp": "2001-05-03-10.12.12.123456" } },
      { "a": { "$timestamp": "2002-04-03-10.12.12.123456" } },
      { "a": { "$timestamp": "2001-04-04-10.12.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-09.12.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.13.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.11.13.123456" } },
      { "a": { "$timestamp": "2001-04-04-00.00.00.000000" } },
      { "a": { "$date": "2001-04-04" } },
      { "a": { "$date": "2001-04-03" } },
      { "a": { "$date": "2001-05-03" } }];
   testPara.testCL.insert( recs );

   // 不走索引
   var cursor = testPara.testCL.find( {}, { "a": 1 } ).sort( { "a": 1 } );
   var expRecs = [
      { "a": { "$date": "2001-04-03" } },
      { "a": { "$timestamp": "2001-04-03-09.12.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.11.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.11.13.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.13.12.123456" } },
      { "a": { "$timestamp": "2001-04-04-00.00.00.000000" } },
      { "a": { "$date": "2001-04-04" } },
      { "a": { "$timestamp": "2001-04-04-10.12.12.123456" } },
      { "a": { "$date": "2001-05-03" } },
      { "a": { "$timestamp": "2001-05-03-10.12.12.123456" } },
      { "a": { "$timestamp": "2002-04-03-10.12.12.123456" } }];
   commCompareResults( cursor, expRecs );

   // 走索引
   var idxName = "idx";
   testPara.testCL.createIndex( idxName, { a: -1 } );
   var cursor = testPara.testCL.find( {}, { "a": 1 } ).hint( { "": idxName } ).sort( { "a": 1 } );
   var expRecs = [
      { "a": { "$date": "2001-04-03" } },
      { "a": { "$timestamp": "2001-04-03-09.12.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.11.12.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.11.13.123456" } },
      { "a": { "$timestamp": "2001-04-03-10.13.12.123456" } },
      { "a": { "$date": "2001-04-04" } },
      { "a": { "$timestamp": "2001-04-04-00.00.00.000000" } },
      { "a": { "$timestamp": "2001-04-04-10.12.12.123456" } },
      { "a": { "$date": "2001-05-03" } },
      { "a": { "$timestamp": "2001-05-03-10.12.12.123456" } },
      { "a": { "$timestamp": "2002-04-03-10.12.12.123456" } }];
   commCompareResults( cursor, expRecs );
}
