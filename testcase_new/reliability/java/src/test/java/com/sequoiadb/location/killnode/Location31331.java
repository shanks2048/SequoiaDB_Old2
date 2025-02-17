package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @Description seqDB-31331:集合设置ReplSize为3，主位置多数派优先，PrimaryLocation中备节点全部异常
 * @Author liuli
 * @Date 2023.05.05
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.05
 * @version 1.10
 */
@Test(groups = "location")
public class Location31331 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl = null;
    private String csName = "cs_31331";
    private String clName = "cl_31331";
    private String primaryLocation = "guangzhou.nansha_31331";
    private String sameCityLocation = "guangzhou.panyu_31331";
    private String offsiteLocation = "shenzhan.nanshan_31331";
    private List< BSONObject > batchRecords;
    private int recordNum = 200000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 3 );
        option1.put( "ConsistencyStrategy", 3 );
        option1.put( "Group", SdbTestBase.expandGroupNames.get( 0 ) );
        dbcl = dbcs.createCollection( clName, option1 );
    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupNames.get( 0 );
        ReplicaGroup group = sdb.getReplicaGroup( groupName );

        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止 PrimaryLocation 中的备节点
        for ( BasicBSONObject primaryLocationSlaveNode : primaryLocationSlaveNodes ) {
            String nodeName = primaryLocationSlaveNode.getString( "hostName" )
                    + ":" + primaryLocationSlaveNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        batchRecords = CommLib.insertData( dbcl, recordNum );
        // 校验数据已经同步到具有亲和性的Location
        LocationUtils.checkRecordSync( csName, clName, recordNum,
                sameCityLocationNodes );

        for ( BasicBSONObject primaryLocationSlaveNode : primaryLocationSlaveNodes ) {
            String nodeName = primaryLocationSlaveNode.getString( "hostName" )
                    + ":" + primaryLocationSlaveNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.start();
        }

        // 集群环境恢复后校验数据
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
