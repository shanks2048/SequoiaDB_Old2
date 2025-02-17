/***************************************************************************************************
 * @Description: getCollectionStat()获取切分表的集合统计信息
 * @ATCaseID: collectionStat_3
 * @Author: Cheng Jingjing
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== ============== =========================================================
 * 11/21/2022 Cheng Jingjing Get collection's statistics when the wqcollection is sharding cl.
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    当集合为分区表时，验证其未插入数据/插入数据/部分过期/全部过期/truncate时，返回的集合统计信息的值是否正确
 * 测试步骤：
 *    1.建分区表
 *    2.是否做analyze，返回相应的统计信息
 *    3.向集合中插入记录，使集合统计信息部分过期
 *    4.analyze，更新缓存中的集合统计信息
 *    5.truncate，清空缓存中的集合统计信息
 *    6.向集合中插入记录，使集合统计信息全部过期
 * 期望结果：git
 *    1.未插入数据时，返回集合统计信息默认值
 *    2.做analyze之后，返回集合统计信息实际值
 *    3.集合统计信息部分过期，返回集合统计信息的过期值
 *    3.集合统计信息过期，返回集合统计信息的过期值
 *    4.truncate之后，返回集合统计信息默认值
 *
 **************************************************************************************************/
testConf.skipStandAlone = true ;
testConf.skipOneGroup = true ;
testConf.clOpt = { ShardingKey: { no:1 }, AutoSplit: true }
testConf.clName = COMMCLNAME + "_collectionStat_3"
main( test );

function test( testPara ){
  // 未做analyze，显示集合统计信息默认值
  checkCollectionStat( testPara.testCL, testConf.clName, 1, true, false, 10, 400, 400, 2, 160000 ) ;

  // 做analyze之后，显示集合统计信息的实际值
  db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } ) ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, false, false, 10, 0, 0, 0, 0 ) ;

  // 向集合中插入记录，使集合统计信息部分过期
  var data = [] ;
  for( var i = 0; i < 100000; i++ ){
    data.push({a:1,b:1});
  }
  testPara.testCL.insert( data ) ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, false, true, 10, 0, 0, 0, 0 ) ;

  // analyze，更新缓存中的集合统计信息
  db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } ) ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, false, false, 10, 200, 100000, 86, 3600000 ) ;

  // truncate，清空缓存中的集合统计信息
  testPara.testCL.truncate() ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, true, false, 10, 400, 400, 2, 160000 ) ;

  // 向集合中插入记录，使集合统计信息全部过期
  db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } ) ;
  var data = [] ;
  for( var i = 0; i < 100000; i++ ){
    data.push({a:i,b:1});
  }
  testPara.testCL.insert( data ) ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, false, true, 10, 0, 0, 0, 0 ) ;

  // analyze，更新缓存中的集合统计信息
  db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } ) ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, false, false, 10, 200, 100000, 86, 3600000 ) ;

  // truncate，清空缓存中的集合统计信息
  testPara.testCL.truncate() ;
  checkCollectionStat( testPara.testCL, testConf.clName, 1, true, false, 10, 400, 400, 2, 160000 ) ;
}