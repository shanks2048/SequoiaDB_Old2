﻿/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using SequoiaDB.Bson;
using System.Collections.Generic;
using System.Text;

namespace DriverTest
{
    
    
    [TestClass()]
    public class DBCollectionTest
    {
        private TestContext testContextInstance;
        private static Config config = null;

        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection coll = null;
        string csName = "testfoo_cs";
        string cName = "testbar_cs";

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if (config == null)
                config = new Config(); 
        }

        [TestInitialize()]
        public void MyTestInitialize()
        {
            //BsonDefaults.MaxDocumentSize = 16 * 1024 * 1024;
            try
            {
                sdb = new Sequoiadb(config.conf.Coord.Address);
                sdb.Connect(config.conf.UserName, config.conf.Password);
                if (sdb.IsCollectionSpaceExist(csName))
                    sdb.DropCollectionSpace(csName);
                cs = sdb.CreateCollectionSpace(csName);
                coll = cs.CreateCollection(cName);
            }
            catch (BaseException e)
            {
                Console.WriteLine("Failed to Initialize in DBCollectionTest, ErrorType = {0}", e.ErrorType);
                Environment.Exit(0);
            }
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        #endregion


        [TestMethod()]
        public void BulkInsertTmpTest()
        {
            int times = 3;
            int bulkSize = 5000;
            for (int i = 0; i < times; i++)
            {
                List<BsonDocument> list = new List<BsonDocument>(bulkSize);
                for (int j = 0; j < bulkSize; j++)
                {
                    BsonDocument obj = new BsonDocument();
                    obj.Add("bbs", "725").
                            Add("csbh", 1817).
                            Add("cljg", "工作状态").
                            Add("sjym", "79H").
                            Add("wxbs", "WX1558").
                            Add("dmzbs", "DMZ2206").
                            Add("cxbz", 0).
                            Add("sjsj", new DateTime()).
                            Add("rksj", new DateTime());
                    list.Add(obj);
                }
                DateTime beginTime = DateTime.Now;
                coll.BulkInsert(list, 0);
                DateTime endTime = DateTime.Now;
                System.TimeSpan takes = endTime - beginTime;
                Console.WriteLine(String.Format("Times: {0}, tasks: {1}ms", i, takes.TotalMilliseconds));
            }
        }

        /// <summary>
        ///Testing for Insert
        ///</summary>
        [TestMethod()]
        public void InsertTest()
        {
            // insert
            BsonDocument insertor = new BsonDocument();
            string date = DateTime.Now.ToString();
            insertor.Add("operation", "Insert");
            insertor.Add("date", date);
            Object id = (ObjectId)coll.Insert(insertor);

            BsonDocument matcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("date", date);
            query.Matcher = matcher;
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            BsonDocument bson = cursor.Next();
            Assert.IsNotNull(bson);
            Assert.IsTrue(id.Equals(bson["_id"].AsObjectId));
            //Assert.IsTrue(id.)
        }

        /// <summary>
        ///Testing for Insert
        ///</summary>
        [TestMethod()]
        public void InsertTest_WithId()
        {
            int testTypeSize = 8;
            for (int i = 0; i < testTypeSize; i++)
            {
                // insert
                BsonDocument insertor = new BsonDocument();
                string date = DateTime.Now.ToString();
                insertor.Add("operation", "Insert");
                insertor.Add("date", date);
                switch (i)
                { 
                    case 0:
                        insertor.Add("_id", 3.14);
                        break;
                    case 1:
                        insertor.Add("_id", "abcdefg");
                        break;
                    case 2:
                        insertor.Add("_id", new BsonDocument("id", "id"));
                        break;
                    case 3:
                        insertor.Add("_id", ObjectId.GenerateNewId());
                        break;
                    case 4:
                        insertor.Add("_id", true);
                        break;
                    case 5:
                        insertor.Add("_id", 1234);
                        break;
                    case 6:
                        insertor.Add("_id", 10000L);
                        break;
                    case 7:
                        insertor.Add("_id", new BsonTimestamp(1000000000L));
                        break;
                    default:
                        continue;
                }
                coll.Delete(null);
                BsonValue value = coll.Insert(insertor);
                Object id = null;
                BsonType type = value.BsonType;
                if (type == BsonType.Double)
                    id = value.AsDouble;
                else if (type == BsonType.String)
                    id = value.AsString;
                else if (type == BsonType.Document)
                    id = value.AsBsonDocument;
                else if (type == BsonType.Array)
                    id = value.AsBsonArray;
                else if (type == BsonType.Binary)
                    id = value.AsBsonBinaryData;
                else if (type == BsonType.Undefined)
                    id = value.AsBsonUndefined;
                else if (type == BsonType.ObjectId)
                    id = value.AsObjectId;
                else if (type == BsonType.Boolean)
                    id = value.AsBoolean;
                else if (type == BsonType.DateTime)
                    id = value.AsDateTime;
                else if (type == BsonType.Null)
                    ;
                else if (type == BsonType.RegularExpression)
                    id = value.AsRegex;
                else if (type == BsonType.JavaScript)
                    id = value.AsBsonJavaScript;
                else if (type == BsonType.Symbol)
                    id = value.AsBsonSymbol;
                else if (type == BsonType.JavaScriptWithScope)
                    id = value.AsBsonJavaScriptWithScope;
                else if (type == BsonType.Int32)
                    id = value.AsInt32;
                else if (type == BsonType.Timestamp)
                    id = value.AsBsonTimestamp;
                else if (type == BsonType.Decimal)
                    id = value.AsBsonDecimal;
                else if (type == BsonType.Int64)
                    id = value.AsInt64;
                else if (type == BsonType.MinKey)
                    id = value.AsBsonMinKey;
                else if (type == BsonType.MaxKey)
                    id = value.AsBsonMaxKey;

                BsonDocument matcher = new BsonDocument();
                DBQuery query = new DBQuery();
                matcher.Add("date", date);
                query.Matcher = matcher;
                DBCursor cursor = coll.Query(query);
                Assert.IsNotNull(cursor);
                BsonDocument bson = cursor.Next();
                Assert.IsNotNull(bson);

                BsonValue ret = bson.GetValue("_id");
                type = ret.BsonType;
                if (type == BsonType.Double)
                    Assert.IsTrue(id.Equals(ret.AsDouble));
                else if (type == BsonType.String)
                    Assert.IsTrue(id.Equals(ret.AsString));
                else if (type == BsonType.Document)
                    Assert.IsTrue(id.Equals(ret.AsBsonDocument));
                else if (type == BsonType.Array)
                    Assert.IsTrue(id.Equals(ret.AsBsonArray));
                else if (type == BsonType.Binary)
                    Assert.IsTrue(id.Equals(ret.AsBsonBinaryData));
                else if (type == BsonType.Undefined)
                    Assert.IsTrue(id.Equals(ret.AsBsonUndefined));
                else if (type == BsonType.ObjectId)
                    Assert.IsTrue(id.Equals(ret.AsObjectId));
                else if (type == BsonType.Boolean)
                    Assert.IsTrue(id.Equals(ret.AsBoolean));
                else if (type == BsonType.DateTime)
                    Assert.IsTrue(id.Equals(ret.AsDateTime));
                else if (type == BsonType.Null)
                    Assert.IsTrue(id == null);
                else if (type == BsonType.RegularExpression)
                    Assert.IsTrue(id.Equals(ret.AsRegex));
                else if (type == BsonType.JavaScript)
                    Assert.IsTrue(id.Equals(ret.AsBsonJavaScript));
                else if (type == BsonType.Symbol)
                    Assert.IsTrue(id.Equals(ret.AsBsonSymbol));
                else if (type == BsonType.JavaScriptWithScope)
                    Assert.IsTrue(id.Equals(ret.AsBsonJavaScriptWithScope));
                else if (type == BsonType.Int32)
                    Assert.IsTrue(id.Equals(ret.AsInt32));
                else if (type == BsonType.Timestamp)
                    Assert.IsTrue(id.Equals(ret.AsBsonTimestamp));
                else if (type == BsonType.Decimal)
                    Assert.IsTrue(id.Equals(ret.AsBsonDecimal));
                else if (type == BsonType.Int64)
                    Assert.IsTrue(id.Equals(ret.AsInt64));
                else if (type == BsonType.MinKey)
                    Assert.IsTrue(id.Equals(ret.AsBsonMinKey));
                else if (type == BsonType.MaxKey)
                    Assert.IsTrue(id.Equals(ret.AsBsonMaxKey));
            }
        }

        /// <summary>
        ///Testing for DBCollection Operation: insert, delete, update, query
        ///</summary>
        [TestMethod()]
        public void OperationTest()
        {
            // Insert
            BsonDocument insertor = new BsonDocument();
            insertor.Add("Last Name", "Lin");
            insertor.Add("First Name", "Hetiu");
            insertor.Add("Address", "SYSU");
            BsonDocument sInsertor = new BsonDocument();
            sInsertor.Add("Phone", "10086");
            sInsertor.Add("EMail", "hetiu@yahoo.com.cn");
            insertor.Add("Contact", sInsertor);
            ObjectId insertID = (ObjectId)coll.Insert(insertor);
            Assert.IsNotNull(insertID);

            // Update
            DBQuery query = new DBQuery();
            BsonDocument updater = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            updater.Add("Age", 25);
            modifier.Add("$set", updater);
            matcher.Add("First Name", "Hetiu");
            query.Matcher = matcher;
            query.Modifier = modifier;
            coll.Update(query);

            // Query
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            BsonDocument bson = cursor.Next();
            Assert.IsNotNull(bson);
            Assert.IsTrue(bson["First Name"].AsString.Equals("Hetiu"));
            Assert.IsTrue(bson["Age"].AsInt32.Equals(25));

            // Delete
            BsonDocument drop = new BsonDocument();
            drop.Add("Last Name", "Lin");
            coll.Delete(drop);
            query.Matcher = drop;
            cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNull(bson);
        }

        /// <summary>
        ///Testing for Query
        ///</summary>
        [TestMethod()]
        public void QueryTest()
        {
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "Query");
                insertor.Add("date", date);
                coll.Insert(insertor);
            }
            BsonDocument matcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("operation", "Query");
            query.Matcher = matcher;
            query.ReturnRowsCount = 5;
            query.SkipRowsCount = 5;
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 5);
        }

        [TestMethod()]
        public void QueryWithFlagsTest()
        {
            DBCursor cursor = null;
            BsonDocument hint = new BsonDocument();
            hint.Add("", "$id");
            // case 1
            cursor = coll.Query(null, null, null, null, 0, -1, 
                DBQuery.FLG_QUERY_WITH_RETURNDATA);
            // case 2
            cursor = coll.Query(null, null, null, hint, 0, -1, 
                DBQuery.FLG_QUERY_WITH_RETURNDATA | DBQuery.FLG_QUERY_FORCE_HINT);
            // case 3
            cursor = coll.Query(null, null, null, hint, 0, -1, 
                DBQuery.FLG_QUERY_WITH_RETURNDATA | DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_PARALLED);
            // case 4
            cursor = coll.Query(null, null, null, hint, 0, -1,
                DBQuery.FLG_QUERY_EXPLAIN | DBQuery.FLG_QUERY_MODIFY);
        }

        [TestMethod()]
        public void QueryOneTest()
        {
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "Query");
                insertor.Add("date", date);
                coll.Insert(insertor);
            }
            BsonDocument matcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("operation", "Query");
            query.Matcher = matcher;
            query.ReturnRowsCount = 5;
            query.SkipRowsCount = 5;
            query.Flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            sdb.CloseAllCursors();
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 5);
        }

        /// <summary>
        ///Testing for Indexes: create index, get indexes, drop index
        ///</summary>
        [TestMethod()]
        public void IndexesTest()
        {
            // Insert
            BsonDocument insertor = new BsonDocument();
            insertor.Add("Last Name", "Lin");
            insertor.Add("First Name", "Hetiu");
            insertor.Add("Address", "SYSU");
            BsonDocument sInsertor = new BsonDocument();
            sInsertor.Add("Phone", "10086");
            sInsertor.Add("EMail", "hetiu@yahoo.com.cn");
            insertor.Add("Contact", sInsertor);
            ObjectId insertID = (ObjectId)coll.Insert(insertor);
            Assert.IsNotNull(insertID);
            
            // Create Index with default sort buffer size
            BsonDocument key = new BsonDocument();
            key.Add("Last Name", 1);
            key.Add("First Name", 1);
            string name = "index_name_default";
            coll.CreateIndex(name, key, false, false);

            // Create Index without sort buffer
            BsonDocument key2 = new BsonDocument();
            key2.Add("Last Name2", 1);
            key2.Add("First Name2", 1);
            string name2 = "index_name_without_buffer";
            coll.CreateIndex(name2, key2, true, true, 0);
            
            // Create Index with user-defined sort buffer
            BsonDocument key3 = new BsonDocument();
            key3.Add("Last Name3", 1);
            key3.Add("First Name3", 1);
            string name3 = "index_name_with_user-defined_buffer";
            coll.CreateIndex(name3, key3, true, true, 128);

            // Create Index, expect -6 exception
            BsonDocument key4 = new BsonDocument();
            key3.Add("Last Name4", 1);
            key3.Add("First Name4", 1);
            string name4 = "index_name_with_error";
            try
            {
                coll.CreateIndex(name4, key4, true, true, -1);
                Assert.IsTrue(false);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(e.ErrorCode == new BaseException("SDB_INVALIDARG").ErrorCode);
            }
            
            // create index with options
            BsonDocument key5 = new BsonDocument();
            key5.Add("Last Name5", 1);
            key5.Add("First Name5", -1);
            string name5 = "index_name_with_options";
            BsonDocument options5 = new BsonDocument();
            options5.Add("Unique", true);
            options5.Add("Enforced", true);
            options5.Add("NotNull", false);
            options5.Add("SortBufferSize", 100);
            coll.CreateIndex(name5, key5, options5);
            DBCursor cursor5 = coll.GetIndex(name5);
            Assert.IsNotNull(cursor5);
            BsonDocument index5 = cursor5.Next();
            Assert.IsNotNull(index5);
            Console.WriteLine("index5 is: " + index5);
            Assert.IsTrue(index5["IndexDef"].AsBsonDocument["NotNull"].AsBoolean.Equals(false));
            BsonDocument key6 = new BsonDocument();
            key6.Add("Last Name6", 1);
            key6.Add("First Name6", -1);
            coll.CreateIndex(name5 + "123", key6, null);

            // Get Indexes
            DBCursor cursor = coll.GetIndex(name);
            Assert.IsNotNull(cursor);
            BsonDocument index = cursor.Next();
            Assert.IsNotNull(index);
            Assert.IsTrue(index["IndexDef"].AsBsonDocument["name"].AsString.Equals(name));

            // Get Indexes
            DBCursor cursor2 = coll.GetIndex(name2);
            Assert.IsNotNull(cursor2);
            BsonDocument index2 = cursor2.Next();
            Assert.IsNotNull(index2);
            Assert.IsTrue(index2["IndexDef"].AsBsonDocument["name"].AsString.Equals(name2));
            
            // Get Indexes
            DBCursor cursor3 = coll.GetIndex(name3);
            Assert.IsNotNull(cursor3);
            BsonDocument index3 = cursor3.Next();
            Assert.IsNotNull(index3);
            Assert.IsTrue(index3["IndexDef"].AsBsonDocument["name"].AsString.Equals(name3));

            // Drop Index
            coll.DropIndex(name);
            cursor = coll.GetIndex(name);
            Assert.IsNotNull(cursor);
            index = cursor.Next();
            Assert.IsNull(index);

            coll.DropIndex(name2);
            cursor = coll.GetIndex(name2);
            Assert.IsNotNull(cursor);
            index = cursor.Next();
            Assert.IsNull(index);

            coll.DropIndex(name3);
            cursor = coll.GetIndex(name3);
            Assert.IsNotNull(cursor);
            index = cursor.Next();
            Assert.IsNull(index);
        }

        [TestMethod()]
        public void GetIndexStatTest()
        {
            string indexName = "GetIndexStatTest";
            string errorIndex = "errorGetIndexStatTest";
            BsonDocument indexDef = new BsonDocument();
            indexDef.Add(indexName, 1);

            // case 1, index exist
            coll.CreateIndex(indexName, indexDef, null);
            sdb.Analyze();
            BsonDocument obj = coll.GetIndexStat(indexName);
            Assert.AreEqual(coll.FullName, obj.GetElement("Collection").Value);
            Assert.AreEqual(indexName, obj.GetElement("Index").Value);

            // case 2, index no exist
            try
            {
                coll.GetIndexStat(errorIndex);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(e.ErrorType == "SDB_IXM_STAT_NOTEXIST");
            }
        }

        [TestMethod()]
        public void GetCountTest()
        {
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "GetCount");
                insertor.Add("date", date);
                coll.Insert(insertor);
            }

            BsonDocument condition = new BsonDocument();
            condition.Add("operation", "GetCount");
            long count = coll.GetCount(condition);
            Assert.IsTrue(count == 10);
        }

        [TestMethod()]
        public void BulkInsertTest()
        {
            long count = 0;
            List<BsonDocument> insertor = new List<BsonDocument>();
            /// default
            for (int i = 0; i < 10; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("operation", "BulkInsert");
                obj.Add("date", DateTime.Now.ToString());
                insertor.Add(obj);
            }
            coll.BulkInsert(insertor, 0);
            BsonDocument condition = new BsonDocument();
            condition.Add("operation", "BulkInsert");
            count = coll.GetCount(condition);
            Assert.IsTrue(count == 10);

            /// set EnsureOID to be false
            insertor.Clear();
            coll.EnsureOID = false;
            for (int i = 0; i < 10; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("operation", "BulkInsert");
                obj.Add("date", DateTime.Now.ToString());
                insertor.Add(obj);
            }
            coll.BulkInsert(insertor, 0);
            count = coll.GetCount(condition);
            Assert.IsTrue(count == 20);
            /// set EnsureOID to be true
            insertor.Clear();
            coll.EnsureOID = true;
            for (int i = 0; i < 10; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("operation", "BulkInsert");
                obj.Add("date", DateTime.Now.ToString());
                insertor.Add(obj);
            }
            coll.BulkInsert(insertor, 0);
            count = coll.GetCount(condition);
            Assert.IsTrue(count == 30);
        }

        [TestMethod()]
        public void UpdateInEmptyCLTest()
        {
            BsonDocument updater = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            BsonDocument tempMatcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("operation", new BsonDocument("$et", "Update"));
            query.Matcher = matcher;

            long count = coll.GetCount(matcher);
            Assert.IsTrue(count == 0);

            // update but insert
            updater.Add("operation", "Update");
            updater.Add("type", "Insert");
            modifier.Add("$set", updater);
            tempMatcher.Add("type", new BsonDocument("$et", "Insert"));
            coll.Update(matcher, modifier, hint);
            count = coll.GetCount(tempMatcher);
            Assert.IsTrue(count == 0);
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            BsonDocument rtn = cursor.Next();
            Assert.IsNull(rtn);
        }

        [TestMethod()]
        public void UpsertTest()
        {
            BsonDocument updater = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            BsonDocument tempMatcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("operation", new BsonDocument("$et", "Upsert"));
            query.Matcher = matcher;

            long count = coll.GetCount(matcher);
            Assert.IsTrue(count == 0);

            // update but insert
            updater.Add("operation", "Upsert");
            updater.Add("type", "Insert");
            modifier.Add("$set", updater);
            tempMatcher.Add("type",new BsonDocument("$et","Insert"));
            coll.Upsert(matcher, modifier, hint);
            //coll.Upsert(matcher, null, null);
            count = coll.GetCount(tempMatcher);
            Assert.IsTrue(count == 1);
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            BsonDocument rtn = cursor.Next();
            Assert.IsNotNull(rtn);
            Assert.IsTrue(rtn["type"].AsString.Equals("Insert"));

            // update
            updater = new BsonDocument();
            updater.Add("type", "Update");
            modifier = new BsonDocument();
            modifier.Add("$set", updater);
            coll.Upsert(matcher, modifier, hint);

            cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            count = coll.GetCount(matcher);
            Assert.IsTrue(count == 1);
            rtn = cursor.Next();
            Assert.IsTrue(rtn["type"].AsString.Equals("Update"));
        }

        [TestMethod()]
        public void InsertChineseTest()
        {
            BsonDocument insertor = new BsonDocument();
            insertor.Add("姓名", "林海涛");
            insertor.Add("年龄", 24);
            insertor.Add("id", 2001);

            // an embedded bson object
            BsonDocument phone = new BsonDocument
                {
                    {"0", "10086"},
                    {"1", "10000"}
                };

            insertor.Add("电话", phone);

            ObjectId id = (ObjectId)coll.Insert(insertor);

            BsonDocument matcher = new BsonDocument();
            DBQuery query = new DBQuery();
            matcher.Add("姓名", "林海涛");
            query.Matcher = matcher;
            DBCursor cursor = coll.Query(query);
            Assert.IsNotNull(cursor);
            BsonDocument rtn = cursor.Next();
            Assert.IsNotNull(rtn);
            Assert.IsTrue(id.Equals(rtn["_id"].AsObjectId));
        }
/*
        [TestMethod()]
        public void RenameTest()
        {
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "Rename");
                insertor.Add("date", date);
                coll.Insert(insertor);
            }

            coll.Rename("RenameCL");
            BsonDocument condition = new BsonDocument();
            condition.Add("operation", "Rename");
            long count = coll.GetCount(condition);
            Assert.IsTrue(count == 10);    
        }
*/

/*
        [TestMethod()]
        public void SplitTest()
        {
            //string srcGroup = config.conf.Sets[0].SetName;
            //string destGroup = config.conf.Sets[1].SetName;
            string oRGName = "group1";
            string nRGName = config.conf.Groups[0].GroupName;
            string node1 = "ubuntu-dev1:51000";
            string node2 = config.conf.Groups[0].Nodes[0].HostName + ":"
                           + config.conf.Groups[0].Nodes[0].Port.ToString();
            //string node2 = config.conf.Sets[1].Nodes[0].HostName + ":"
            //               + config.conf.Sets[1].Nodes[0].Port.ToString();
            string hostName = config.conf.Groups[0].Nodes[0].HostName;
            int port = config.conf.Groups[0].Nodes[0].Port;
            string csName = "SplitCS";
            string cName = "SplitCL";
            string cFullName = csName+"."+cName;

            // test wether we are in cluster environment or not
            try
            { 
                sdb.GetList(SDBConst.SDB_LIST_GROUPS);
            }
            catch(Exception ex)
            {
                ex.ToString();
                return;
            }
            // create rg
            ReplicaGroup rg = sdb.GetReplicaGroup(nRGName);
            if (rg == null)
                rg = sdb.CreateReplicaGroup(nRGName);
            // create node
            ReplicaNode node = rg.GetNode(hostName, port);
            if (node == null)
            {
                string dbpath = config.conf.Groups[0].Nodes[0].DBPath;
                Dictionary<string, string> map = new Dictionary<string, string>();
                map.Add("diaglevel", config.conf.Groups[0].Nodes[0].DiagLevel);
                node = rg.CreateNode(hostName, port, dbpath, map);
            }
            rg.Start();
            // create cs
            CollectionSpace cs1 = sdb.GetCollecitonSpace(csName);
            if (cs1 == null)
                cs1 = sdb.CreateCollectionSpace(csName);
            // create cl
            DBCollection coll1 = cs1.GetCollection(cName);
            if (coll1 == null)
            {
                BsonDocument shardingKey = new BsonDocument();
                shardingKey.Add("operation", 1);
                BsonDocument options = new BsonDocument();
                options.Add("ShardingKey", shardingKey);
                string s = options.ToString();
                coll1 = cs1.CreateCollection(cName, options);
                ;
            }
            // prepare record
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "Split1");
                insertor.Add("date", date);
                coll1.Insert(insertor);
            }
            for (int i = 0; i < 10; ++i)
            {
                string date = DateTime.Now.ToString();
                BsonDocument insertor = new BsonDocument();
                insertor.Add("operation", "Split2");
                insertor.Add("date", date);
                coll1.Insert(insertor);
            }
            // get src and dest group
            string srcGroup = null;
            string destGroup = null;
            Sequoiadb cdb = new Sequoiadb(config.conf.Catalog.Address);
            cdb.Connect(config.conf.UserName, config.conf.Password);
            CollectionSpace scs = cdb.GetCollecitonSpace("SYSCAT");
            DBCollection scl = scs.GetCollection("SYSCOLLECTIONS");
            //BsonDocument query = new BsonDocument{ {"Name": cFullName} };
            //BsonDocument selector = new BsonDocument{ {"":"CataInfo"} };
            BsonDocument query = new BsonDocument();
            query.Add("Name", cFullName);
            BsonDocument selector = new BsonDocument();
            selector.Add("", "CataInfo");
            DBCursor cursor = scl.Query(query, null, null, null);
            BsonDocument obj = cursor.Next();
            BsonArray array = obj["CataInfo"].AsBsonArray;
            for( int i=0; i<array.Count; i++)
            {
                BsonDocument subObj = array[i].AsBsonDocument;
                srcGroup = subObj["GroupName"].AsString;
                if(srcGroup != null)
                    break;
            }
            if (srcGroup == null)
            {
                System.Console.WriteLine("Can't find the source group.");
                Assert.IsTrue(0 == 1);
            }
            if(srcGroup.Equals(nRGName))
               destGroup = oRGName;
            else
               destGroup = nRGName;
            // build up split condition
            BsonDocument condition = new BsonDocument();
            condition.Add("operation", "Split2");
            // split
            coll1.Split(srcGroup, destGroup, condition, null);
            // check up
            Sequoiadb sdb2 = null;
            CollectionSpace cs2 = null;
            DBCollection coll2 = null;
            if(destGroup == oRGName)
            {
                sdb2 = sdb.GetReplicaGroup(destGroup).GetNode(node1).Connect(config.conf.UserName, config.conf.Password);
                cs2 = sdb2.GetCollecitonSpace(csName);
                // sleep 1s
                System.Threading.Thread.Sleep(3000);
                coll2 = cs2.GetCollection(cName);
                long num = coll2.GetCount(condition);
                Assert.IsTrue(num == 10);
            }
            else if (destGroup == nRGName)
            {
                sdb2 = sdb.GetReplicaGroup(destGroup).GetNode(node2).Connect(config.conf.UserName, config.conf.Password);
                cs2 = sdb2.GetCollecitonSpace(csName);
                // sleep 1s
                System.Threading.Thread.Sleep(3000);
                coll2 = cs2.GetCollection(cName);
                long num = coll2.GetCount(condition);
                Assert.IsTrue( num == 10);
            }
            else
            {
                System.Console.WriteLine("Something wrong.");
                Assert.IsTrue( 0 == 1);
            }

            sdb.DropCollectionSpace(csName);
            sdb.RemoveReplicaGroup(nRGName);
            sdb2.Disconnect();
        }
 */
        [TestMethod()]
        public void AggregateTest()
        {
		    String[] command = new String[2];
		    command[0] = "{$match:{status:\"A\"}}";
		    command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}";
		    String[] record = new String[4];
		    record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}";
	        record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}";
	        record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}";
	        record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}";
	        // insert record into database
		    for ( int i=0; i<record.Length; i++)
		    {
			    BsonDocument obj = new BsonDocument();
			    obj = BsonDocument.Parse(record[i]);
                Console.WriteLine("Record is: "+obj.ToString());
			    coll.Insert(obj);
		    }
            List<BsonDocument> list = new List<BsonDocument>();
		    for ( int i=0; i<command.Length; i++)
		    {
                BsonDocument obj = new BsonDocument();
                obj = BsonDocument.Parse(command[i]);
			    list.Add(obj);
		    }

            DBCursor cursor = coll.Aggregate(list);
            int count = 0;
            while ( null != cursor.Next() )
            {
                Console.WriteLine("Result is: "+cursor.Current().ToString());
                String str = cursor.Current().ToString();
                count++ ;
            }
            Assert.IsTrue(2 == count);
        }

        [TestMethod()]
        public void GetQueryMetaTest()
        {
            try{
            // create cl
            DBCollection coll2 = null;
            string cName2 = "testbar2";
            if (cs.IsCollectionExist(cName2))
                cs.DropCollection(cName);
            coll2 = cs.CreateCollection(cName2);
            // create index
            coll2.CreateIndex("ageIndex", new BsonDocument("age", -1), false, false);
            // prepare record
            Random ro = new Random();
            int recordNum = 10000;
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < recordNum; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("Id",i);
                obj.Add("age", ro.Next(0, 100));
                obj.Add("date", DateTime.Now.ToString());
                insertor.Add(obj);
            }
            coll2.BulkInsert(insertor, 0);

            // TODO:

            // query
            BsonDocument subobj = new BsonDocument();
            BsonDocument query = new BsonDocument();
            query.Add("age", subobj);
            subobj.Add("$gt", 1);
            subobj.Add("$lt", 99);
            // hint
            BsonDocument hint = new BsonDocument();
            hint.Add("", "ageIndex");
            // orderBy
            BsonDocument orderBy = new BsonDocument();
            orderBy.Add("Indexblocks", 1);
            // execute getQueryMeta
            DBCursor cursor = coll2.GetQueryMeta(query, orderBy, null, 0, -1);
            DBCursor datacursor = null ;
            long count = 0;
            while (cursor.Next() != null)
            {
                BsonDocument temp = new BsonDocument();
                temp = cursor.Current();
                BsonDocument h = new BsonDocument();
                if ( temp.Contains("Indexblocks") && temp["Indexblocks"].IsBsonArray )
                {
                     h.Add("Indexblocks", temp["Indexblocks"].AsBsonArray);
                }
                datacursor = coll2.Query(null, null, null, h, 0, -1);
                while ( datacursor.Next() != null )
                {
                    count++;
                }
            }
            Assert.IsTrue(recordNum == count);
            }catch (BaseException e)
            {
                Console.WriteLine(e.ErrorType);
                return;
            }
        }

        [TestMethod()]
        public void AttachAndDetachCollectionTest()
        {
            try
            {
                // create main cl
                DBCollection mainCL = null;
                string mainCLName = "maincl";
                if (cs.IsCollectionExist(mainCLName))
                    cs.DropCollection(mainCLName);
                BsonDocument conf = new BsonDocument
                                    {
                                        {"IsMainCL", true},
                                        {"ShardingKey", new BsonDocument
                                                        {
                                                            {"id", 1}
                                                        } 
                                        },
                                        {"ReplSize", 0}
                                    };
                mainCL = cs.CreateCollection(mainCLName, conf);
                // create sub cl
                DBCollection subCL = null;
                string subCLName = "subcl";
                if (cs.IsCollectionExist(subCLName))
                    cs.DropCollection(subCLName);
                BsonDocument conf1 = new BsonDocument
                                    {
                                        {"ReplSize", 0}
                                    };
                subCL = cs.CreateCollection(subCLName, conf1);
                // case 1: test attachCollection
                // TODO:
                int num = 10;
                // mainCL attach subCL
                BsonDocument options = new BsonDocument
                {
                    {"LowBound", new BsonDocument{{"id", 0}}},
                    {"UpBound", new BsonDocument{{"id", num}}}
                };
                string subCLFullName = cs.Name+"."+ subCLName;
                mainCL.AttachCollection(subCLFullName, options);
                // insert some records
                List<BsonDocument> insertor = new List<BsonDocument>();
                for (int i = 0; i < num + 10; i++)
                {
                    BsonDocument obj = new BsonDocument();
                    obj.Add("id", i);
                    //insertor.Add(obj);
                    try
                    {
                        mainCL.Insert(obj);
                    }
                    catch (BaseException e)
                    {
                        int errno = e.ErrorCode;
                        Console.WriteLine(e.ErrorType);
                    }
                }
                try
                {
                    //mainCL.BulkInsert(insertor, 0);
                }
                catch (BaseException e)
                {
                    Assert.IsTrue(e.ErrorType.Equals("SDB_CAT_NO_MATCH_CATALOG"));
                }
                // check
                BsonDocument subobj = new BsonDocument();
                BsonDocument matcher = new BsonDocument();
                matcher.Add("id", subobj);
                subobj.Add("$gte", 0);
                subobj.Add("$lt", num);
                DBCursor cursor = mainCL.Query(matcher, null, null, null, 0, -1);
                long count = 0;
                while (cursor.Next() != null)
                {
                    count++;
                }
                Assert.IsTrue(num == count);
                BsonDocument subobj1 = new BsonDocument();
                BsonDocument matcher1 = new BsonDocument();
                matcher1.Add("id", subobj1);
                subobj1.Add("$gte", num);
                DBCursor cursor1 = mainCL.Query(matcher1, null, null, null, 0, -1);
                count = 0;
                while (cursor1.Next() != null)
                {
                    count++;
                }
                Assert.IsTrue(0 == count);
                // case 2: test detachCollection
                // TODO:
                mainCL.DetachCollection(subCLFullName);
                // check
                try
                {
                    BsonDocument obj = new BsonDocument("test", "test");
                    mainCL.Insert(obj);
                }
                catch (BaseException e)
                {
                    Assert.IsTrue(e.ErrorType.Equals("SDB_CAT_NO_MATCH_CATALOG"));
                    Console.WriteLine(e.ErrorType);
                }
            }
            catch (BaseException e)
            {
                Console.WriteLine(e.ErrorType);
                return;
            }
        }

        [TestMethod()]
        public void AlterCollectionTest()
        {
            DBCollection coll = null;
            BsonDocument options = null;
            string clName_alter = "alterCLTest";
            string fullName = null;
            int partition = 0;
            string type = null;
            int rs = -1;
            coll = cs.CreateCollection(clName_alter);
            fullName = coll.CollSpace.Name + "." + clName_alter;
            // alter collecton attrubute
            options = new BsonDocument {
                {"ReplSize", 0},
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 4096}
            };
            coll.Alter(options);
            // check
            BsonDocument matcher = new BsonDocument { {"Name", fullName} };
            DBCursor cur = sdb.GetSnapshot(8, matcher, null, null );
            BsonDocument obj = cur.Next();
            if (null == obj)
            {
                Assert.Fail();
            }
            try
            {
                rs = obj["ReplSize"].AsInt32;
                Assert.IsTrue(7 == rs);
                partition = obj["Partition"].AsInt32;
                Assert.IsTrue(4096 == partition);
                type = obj["ShardingType"].AsString;
                Assert.IsTrue(type.Equals("hash"));
            }
            catch(System.Exception)
            {
                Assert.Fail();
            }
        }

        [TestMethod()]
        public void EnableShardingTest()
        {
            DBCollection coll = null;
            BsonDocument options = null;
            string clName_alter = "shardingCLTest";
            coll = cs.CreateCollection(clName_alter);
            // alter collecton attrubute
            options = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 4096},
                {"AutoSplit", true}
            };
            coll.EnableSharding(options);
        }

        [TestMethod()]
        public void MultiAlterTest()
        {
            DBCollection coll = null;
            BsonDocument options = null;
            string clName_alter = "shardingCLTest";
            coll = cs.CreateCollection(clName_alter);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","enable sharding"},
                {"Args",new BsonDocument{
                    {"ShardingKey", new BsonDocument{{"a",1}}},
                    {"ShardingType", "hash"},
                    {"Partition", 4096},
                    {"AutoSplit", true}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"StrictDataMode", true}}}});
            // alter collecton attrubute
            options = new BsonDocument {{"Alter", alterArray }};
            coll.Alter(options);
        }

        [TestMethod()]
        public void QueryExplainAPITest()
        {
            int num = 100;
            int i = 0;
            string indexName = "QueryExpalinIndex";
            BsonDocument index = new BsonDocument
            {
                {"age", 1}
            };
            // create index
            coll.CreateIndex(indexName, index, false, false);
            // insert records
            for (; i < num; i++)
            {
                BsonDocument obj = new BsonDocument{
                    {"firstName", "John"},
                    {"lastName", "Smith"},
                    {"age", i}
                };
                coll.Insert(obj);
            }
            // query explain
            BsonDocument cond = new BsonDocument { { "age", new BsonDocument { { "$gt", 50 } } } };
            BsonDocument sel = new BsonDocument { { "age", "" } };
            BsonDocument orderBy = new BsonDocument { { "age", -1 } };
            BsonDocument hint = new BsonDocument { { "", indexName } };
            BsonDocument options = new BsonDocument { { "Run", true } };
            DBCursor cur = coll.Explain(cond, sel, orderBy, hint, 47, 3, 0, options);
            Assert.IsNotNull(cur);
            BsonDocument record = cur.Next();
            int indexRead = record["IndexRead"].AsInt32;
            Assert.IsTrue(50 == indexRead);
            int dataRead = record["DataRead"].AsInt32;
            // TODO: last time I set it to be 49, but now 
            // I set it to be 2, maybe some problem here
            Assert.IsTrue(2 == dataRead);
        }

        [TestMethod()]
        public void QueryExplainTest()
        {
            int num = 100;
            int i = 0;
            string indexName = "QueryExpalinIndex";
            BsonDocument index = new BsonDocument
            {
                {"age", 1}
            };
            // create index
            coll.CreateIndex(indexName, index, false, false);
            // insert records
            for (; i < num; i++)
            {
                BsonDocument obj = new BsonDocument{
                    {"firstName", "John"},
                    {"lastName", "Smith"},
                    {"age", i}
                };
                coll.Insert(obj);
            }
            // query explain
            BsonDocument cond = new BsonDocument { { "age", new BsonDocument { { "$gt", 50 } } } };
            BsonDocument sel = new BsonDocument { { "age", "" } };
            BsonDocument orderBy = new BsonDocument { { "age", -1 } };
            BsonDocument hint = new BsonDocument { { "", indexName } };
            BsonDocument options = new BsonDocument { { "Run", true } };
            DBCursor cur = coll.Explain(cond, sel, orderBy, hint, 47, 3, 0, options);
            Assert.IsNotNull(cur);
            BsonDocument record = cur.Next();
            int indexRead = record["IndexRead"].AsInt32;
            Assert.IsTrue(50 == indexRead);
            int dataRead = record["DataRead"].AsInt32;
            // TODO: last time, it's 49, but now it's 2
            Assert.IsTrue(2 == dataRead);
            int returnNum = record["ReturnNum"].AsInt32;
            // it seems not the result we expect.
            Assert.IsTrue(2 == returnNum);
        }

        [TestMethod()]
        public void TruncateTest()
        {
            int num = 100 ;
            long recordNum = 0;            
            for (int i = 0; i < num; i++)
            {
                BsonDocument obj = new BsonDocument { { "test_truncate", "test" } };
                coll.Insert(obj);
            }
            // test api
            coll.Truncate();
            // check
            recordNum = coll.GetCount(new BsonDocument());
            Assert.IsTrue(0 == recordNum);
        }

        [TestMethod()]
        public void QueryAndUpdate()
        {
            DBCursor cursor = null;
            BsonDocument condition;
            BsonDocument selector;
            BsonDocument orderBy;
            BsonDocument hint;
            BsonDocument update;
            BsonDocument tmp;
            string field1 = "testField1";
            string field2 = "testField2";
            string indexName1 = "test_index1";
            string indexName2 = "test_index2";
            int num = 100;
            int set_value = 100;
            int i = 0;
            BsonDocument index1 = new BsonDocument{{field1,1}};
            BsonDocument index2 = new BsonDocument{{field2,-1}};
            // create indexes
            coll.CreateIndex(indexName1,index1,false,false);
            coll.CreateIndex(indexName2,index2,false,false);
            for (i = 0; i < num; i++)
            {
                BsonDocument obj = new BsonDocument { { field1, i },{ field2, i } };
                coll.Insert(obj);
            }

            condition = new BsonDocument { { field1, new BsonDocument { { "$gte", 0 } } } };
            selector = new BsonDocument { { field2, "" } };
            orderBy = new BsonDocument { { field2, 1 } };
            hint = new BsonDocument { { "", indexName1 } };
            update = new BsonDocument { { "$set", new BsonDocument { { field2, set_value } } } };
            // test api
            // in case: use extend sort
            try
            {
                cursor = coll.QueryAndUpdate(condition, selector, orderBy, hint, update, 0, -1, 0, true);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(new BaseException("SDB_RTN_QUERYMODIFY_SORT_NO_IDX").ErrorCode == e.ErrorCode);
            }
            // in case: does not use extend sort
            try
            {
                hint = new BsonDocument { { "", indexName2 } };
                cursor = coll.QueryAndUpdate(condition, selector, orderBy, hint, update, 0, -1, 0, true);
            }
            catch (BaseException e)
            {
                Assert.Fail();
            }

            // check
            i = 0;
            while (null != (tmp = cursor.Next()))
            {
                i++;
                num = tmp[0].AsInt32;
                Assert.IsTrue(set_value == num);
            }
            Assert.IsTrue(100 == i);
        }

        [TestMethod()]
        public void QueryAndRemove()
        {
            DBCursor cursor = null;
            BsonDocument condition;
            BsonDocument selector;
            BsonDocument orderBy;
            BsonDocument hint;
            BsonDocument update;
            BsonDocument tmp;
            string field1 = "testField1";
            string field2 = "testField2";
            string indexName1 = "test_index1";
            string indexName2 = "test_index2";
            int num = 100;
            int set_value = 100;
            long recordNum = 0;
            int i = 0;
            BsonDocument index1 = new BsonDocument { { field1, 1 } };
            BsonDocument index2 = new BsonDocument { { field2, -1 } };
            // create indexes
            coll.CreateIndex(indexName1, index1, false, false);
            coll.CreateIndex(indexName2, index2, false, false);
            for (i = 0; i < num; i++)
            {
                BsonDocument obj = new BsonDocument { { field1, i }, { field2, i } };
                coll.Insert(obj);
            }

            condition = new BsonDocument { { field1, new BsonDocument { { "$gte", 0 } } } };
            selector = new BsonDocument { { field2, "" } };
            orderBy = new BsonDocument { { field2, 1 } };
            hint = new BsonDocument { { "", indexName1 } };
            update = new BsonDocument { { "$set", new BsonDocument { { field2, set_value } } } };
            // test api
            // in case: use extend sort
            try
            {
                cursor = coll.QueryAndRemove(condition, selector, orderBy, hint, 0, -1, 0);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(new BaseException("SDB_RTN_QUERYMODIFY_SORT_NO_IDX").ErrorCode == e.ErrorCode);
            }
            // in case: does not use extend sort
            try
            {
                hint = new BsonDocument { { "", indexName2 } };
                cursor = coll.QueryAndRemove(condition, selector, orderBy, hint, 50, 10, 0x00000080);
            }
            catch (BaseException e)
            {
                Assert.Fail();
            }

            // check
            i = 0;
            while (null != (tmp = cursor.Next()))
            {
                num = tmp[0].AsInt32;
                Assert.IsTrue((50 + i) == num);
                i++;
            }
            Assert.IsTrue(10 == i);
            i = 100;
            while ( 0 != (i--))
            {
                recordNum = coll.GetCount(condition);
                if (90 == recordNum)
                    break;
            }
            if (0 == i)
            {
                Assert.IsTrue(0 == recordNum);
            }
        }

        [TestMethod()]
        public void Create_And_Drop_Id_Index()
        {
            const string id_name = "$id";
            DBCursor cursor = null;
            BsonDocument doc = null;
            BsonDocument options = null;
            BsonValue value = null;

            // test
            coll.DropIdIndex();
            // check 
            cursor = coll.GetIndex(id_name);
            doc = cursor.Next();
            Assert.IsTrue(null == doc);

            // test
            options = new BsonDocument()
            {
                {"Offline", false}
            };
            coll.CreateIdIndex(options);
            cursor = coll.GetIndex(id_name);
            doc = null;
            doc = cursor.Next();
            Assert.IsTrue(null != doc);
            if (doc.TryGetValue("IndexDef", out value))
            {
                BsonValue v = null ;
                value.AsBsonDocument.TryGetValue("name", out v);
                Assert.IsTrue(id_name == v.AsString);
            }
            else
            {
                Assert.IsTrue(false);
            }
        }

        [TestMethod()]
        public void DuplicateKeyTest()
        {
            BsonDocument obj = new BsonDocument();
            BsonDecimal d = new BsonDecimal("12345.678956", 10, 5);
            String value = d.Value;
            int precision = d.Precision;
            int scale = d.Scale;
            BsonTimestamp ts = new BsonTimestamp(0,0);
            obj.Add("a", d);
            Console.WriteLine("insert obj is: " + obj.ToString());

            BsonValue idObj = coll.Insert(obj);
            Assert.IsNotNull(idObj);
            try
            {
                coll.Insert(obj);
                Assert.Fail();
            }
            catch (BaseException e)
            {
                Assert.AreEqual("SDB_IXM_DUP_KEY", e.ErrorType);
            }
            DBCursor cur = coll.Query();
            BsonDocument ret = cur.Next();
            Console.WriteLine("return ret is: " + ret.ToString());
        }

        [TestMethod()]
        public void CRUDTest()
        {
            // insert
            BsonDocument doc1 = new BsonDocument();
            BsonDocument doc2 = new BsonDocument();
            BsonDocument doc3 = new BsonDocument();
            BsonDocument doc4 = new BsonDocument();
            string filed1 = "id";
            string filed2 = "name";
            string filed3 = "address";

            doc1.Add(filed1, 1).Add(filed2, "nameaaa").Add(filed3, "addressaaa");
            doc2.Add(filed1, 2).Add(filed2, "namebbb").Add(filed3, "addressbbb");
            doc3.Add(filed1, 3).Add(filed2, "nameccc").Add(filed3, "addressccc");
            doc4.Add(filed1, 4).Add(filed2, "nameddd").Add(filed3, "addressddd");
            List<BsonDocument> list = new List<BsonDocument>();
            list.Add(doc1);
            list.Add(doc2);
            BsonValue idObj1 = coll.Insert(doc3);
            BsonValue idObj2 = coll.Insert(doc4);
            coll.BulkInsert(list, 0);
            Assert.IsNotNull(idObj1);
            Assert.IsNotNull(idObj2);
            Assert.AreNotEqual(idObj1, idObj2);
            coll.EnsureOID = true;
            list.Add(doc3);
            list.Add(doc4);
            coll.BulkInsert(list, SDBConst.FLG_INSERT_CONTONDUP);

            // query
            DBCursor cursor = coll.Query();
            int counter = 0;
            BsonDocument doc;
            while ((doc = cursor.Next()) != null)
            {
                counter++;
            }
            Assert.AreEqual(4, counter);

            counter = 0;
            cursor = coll.Query(null,null,new BsonDocument(filed1,1),null);
            while ((doc = cursor.Next()) != null)
            {
                BsonDocument originalDoc = list[counter++];
                Assert.AreEqual(originalDoc[filed1].AsInt32, doc[filed1].AsInt32);
                Assert.AreEqual(originalDoc[filed2].AsString, doc[filed2].AsString);
                Assert.AreEqual(originalDoc[filed3].AsString, doc[filed3].AsString);
            }
            Assert.AreEqual(4, counter);
            counter = 0;
            cursor = coll.Query(new BsonDocument(filed1,new BsonDocument("$gt",2)),
                new BsonDocument(filed2, ""), 
                new BsonDocument(filed1, -1),
                new BsonDocument("",null));
            doc = cursor.Next();
            Assert.AreEqual(doc4[filed2].AsString, doc[filed2].AsString);
            doc = cursor.Next();
            Assert.AreEqual(doc3[filed2].AsString, doc[filed2].AsString);
            doc = cursor.Next();
            Assert.IsNull(doc);

            // update
            coll.Update(new BsonDocument(filed1, new BsonDocument("$gt", 2)),
                new BsonDocument("$set", new BsonDocument(filed2, "abc")),
                new BsonDocument("", null));
            cursor = coll.Query(new BsonDocument(filed1, new BsonDocument("$gt", 2)),
                new BsonDocument(filed2, ""),
                new BsonDocument(filed1, -1),
                new BsonDocument("", null));
            doc = cursor.Next();
            Assert.AreEqual("abc", doc[filed2].AsString);
            doc = cursor.Next();
            Assert.AreEqual("abc", doc[filed2].AsString);
            doc = cursor.Next();
            Assert.IsNull(doc);

            // upsert
            coll.Upsert(new BsonDocument(filed1, new BsonDocument("$gt", 2)),
                new BsonDocument("$set", new BsonDocument(filed2, "cba")),
                new BsonDocument("","$id"));
            coll.Upsert(new BsonDocument(filed1, new BsonDocument("$gt", 20)),
                new BsonDocument("$set", new BsonDocument(filed2, "cba")),
                new BsonDocument("", "$id"));
            long count = coll.GetCount(new BsonDocument(filed2, new BsonDocument("$gt", "")), 
                new BsonDocument("", "$id"));
            Assert.AreEqual(5, count);

            // delete
            coll.Delete(new BsonDocument(filed1, new BsonDocument("$lt", 3)));
            cursor = coll.Query();
            doc = cursor.Next();
            Assert.AreEqual("cba", doc[filed2].AsString);
            doc = cursor.Next();
            Assert.AreEqual("cba", doc[filed2].AsString);
            doc = cursor.Next();
            Assert.AreEqual("cba", doc[filed2].AsString);
            doc = cursor.Next();
            Assert.IsNull(doc);
        }

        [TestMethod()]
        [Ignore] // jira_3357
        public void BigRecordTest()
        {
            string csName = "testfoo";
            string clName = "testbar";
            DBCollection cl = sdb.GetCollectionSpace(csName).GetCollection(clName);
            DBCursor cursor = cl.Query();
            BsonDocument record = cursor.Next();
            BsonValue bsonValue = record.GetValue("bindata");
            Assert.IsTrue(bsonValue.IsBsonBinaryData);
            BsonBinaryData binData = bsonValue.AsBsonBinaryData;
            byte[] bytes = binData.Bytes;
            int binSize = 15 * 1024 * 1024;
            for (int i = 0; i < binSize; ++i) 
            {
                Assert.AreEqual('a', (char)bytes[i]);
            }
        }

        [TestMethod()]
        public void jira_3353_invalidBinary() {
             List<BsonDocument> invalidDocs = new List<BsonDocument> 
            { 
                new BsonDocument("wrongType", new BsonDocument { { "$binary", "" }, { "$type", 1000 } }), 
                new BsonDocument("wrongBinStr", new BsonDocument { { "$binary", "aGVsbG8gd29ybGQ%%%%%="}, { "$type", 1 } }) 
            };
            coll.BulkInsert(invalidDocs, 0);
            byte[] array = Encoding.UTF8.GetBytes("");
            byte[] array2 = Encoding.UTF8.GetBytes("aGVsbG8gd29ybGQ%%%%%=");
            List<BsonDocument> invalidDocs2 = new List<BsonDocument> 
            { 
                new BsonDocument("wrongType", new BsonBinaryData(array, BsonBinarySubType.UserDefined)),
                new BsonDocument("wrongBinStr", new BsonBinaryData(array2, BsonBinarySubType.UserDefined))
            };
            coll.BulkInsert(invalidDocs2, 0);
        }

        [TestMethod]
        public void jira_3350_invalidDateTime()
        {
            long MILLISEC_PER_DAY =  24 * 3600 * 1000;
            List<BsonDocument> invalidDocs = new List<BsonDocument> { 
                new BsonDocument("0", new BsonDateTime(0)), 
                new BsonDocument("-1", new BsonDateTime(-1)), 
                new BsonDocument("-1 day", new BsonDateTime(-1 * MILLISEC_PER_DAY )), 
                new BsonDocument("overMax", new BsonDateTime(long.MaxValue)), 
                new BsonDocument("overMin", new BsonDateTime(long.MinValue)) 
            };
            coll.BulkInsert(invalidDocs, 0);
        }

        [TestMethod()]
        public void jira_3348_BsonTimestampTest()
        {
            long errorTimestamp1 = ((long)2177423999 << 32) + 0; // expect is 2038-12-31 23:59:59, but it is: -9094779208479014912L
            long errorTimestamp2 = (253402271999L << 32) + 0; // expect is 9999-12-31 23:59:59, but it is: -3429381062000640L
            int maxIntSec = 2147483647;
            int minIntSec = -2147483648;
            int sec0 = 0;
            int sec999999 = 999999;
            long timestamp1 = ((long)maxIntSec << 32) + 0;
            long timestamp2 = ((long)minIntSec << 32) + 0;
            BsonDocument insertor1 = new BsonDocument { { "a", new BsonTimestamp(timestamp1) }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", new BsonTimestamp(timestamp2) }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, sec999999) }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, sec0) }, { "b", 4 } };
            BsonDocument insertor5 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, sec999999) }, { "b", 5 } };
            BsonDocument insertor6 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, sec0) }, { "b", 6 } };
            coll.Insert(insertor1);
            coll.Insert(insertor2);
            coll.Insert(insertor3);
            coll.Insert(insertor4);
            coll.Insert(insertor5);
            coll.Insert(insertor6);
            DBCursor cursor = coll.Query();
            BsonDocument record;
            List<BsonDocument> bsonList = new List<BsonDocument>();
            while ((record = cursor.Next()) != null) 
            {
                bsonList.Add(record);
                Console.WriteLine("record is: " + record);
            }

            Assert.AreEqual(maxIntSec, bsonList[0].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[0].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[1].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[1].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[2].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(sec999999, bsonList[2].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[3].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(sec0, bsonList[3].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[4].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(sec999999, bsonList[4].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[5].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(sec0, bsonList[5].GetValue("a").AsBsonTimestamp.Increment);
        }

        [TestMethod]
        public void jira_3333()
        {
            DBCursor cursor = sdb.GetList(SDBConst.SDB_LIST_TASKS, null, null, null);
            BsonDocument doc;
            while ((doc = cursor.Next()) != null) 
            {
                Console.WriteLine("doc is: " + doc);
            }
        }

        [TestMethod]
        public void errorObjectTest()
        {
            // case 1:
            try
            {
                BsonDocument rule = new BsonDocument();
                rule.Add("$seta", 1);
                coll.Update(null, rule, null);
                Assert.Fail();
            }
            catch (BaseException e)
            {
                int errorCode = e.ErrorCode;
                String errorType = e.ErrorType;
                String errorMsg = e.Message;
                BsonDocument errorObj = e.ErrorObject;
                Console.WriteLine("error code is: " + errorCode);
                Console.WriteLine("error type is: " + errorType);
                Console.WriteLine("error msg  is: " + errorMsg);
                Console.WriteLine("error obj  is: " + errorObj);
                Assert.AreEqual("SDB_INVALIDARG", errorType);
                Assert.AreEqual(-6, errorCode);
                Assert.IsNotNull(errorObj);
            }
        }

        [TestMethod]
        public void errorObjectWithLobOperationTest()
        {
            ObjectId oid = new ObjectId(1, 1, 1, 1);
            DBLob lob = null;
            DBLob lob2 = null;

            // case 1:
            try
            {
                lob = coll.OpenLob(oid, DBLob.SDB_LOB_READ);
            }
            catch (BaseException e)
            {
                int errorCode = e.ErrorCode;
                String errorType = e.ErrorType;
                String errorMsg = e.Message;
                BsonDocument errorObj = e.ErrorObject;
                Console.WriteLine("error code is: " + errorCode);
                Console.WriteLine("error type is: " + errorType);
                Console.WriteLine("error msg  is: " + errorMsg);
                Console.WriteLine("error obj  is: " + errorObj);
            }
            
            // case 2:
            try
            {
                lob = coll.CreateLob(oid);
                lob.Close();
                lob = coll.OpenLob(oid, DBLob.SDB_LOB_WRITE);
                lob2 = coll.OpenLob(oid, DBLob.SDB_LOB_WRITE);

                lob.Lock(1, 10);
                lob2.Lock(1, 10);
            }
            catch (BaseException e)
            {
                int errorCode = e.ErrorCode;
                String errorType = e.ErrorType;
                String errorMsg = e.Message;
                BsonDocument errorObj = e.ErrorObject;
                Console.WriteLine("error code is: " + errorCode);
                Console.WriteLine("error type is: " + errorType);
                Console.WriteLine("error msg  is: " + errorMsg);
                Console.WriteLine("error obj  is: " + errorObj);
            }
            finally
            {
                if (lob != null)
                {
                    lob.Close();
                }
                if (lob2 != null)
                {
                    lob2.Close();
                }
            }
            
        }

        [TestMethod]
        public void invalidateCacheTest() 
        {
            BsonDocument options = new BsonDocument();           
            options.Add("Role", "coord");
            sdb.InvalidateCache(null);
            sdb.InvalidateCache(options);
        }

        [TestMethod]
        public void GetEmptyIndexTest()
        {
            String emptyIndexName = "aaaaaaaaa";
            BsonDocument record;
            // case 1:
            DBCursor cursor;
            cursor = coll.GetIndex(emptyIndexName);
            while((record = cursor.Next())!= null)
            {
                Console.Out.WriteLine("index is: " + record);
            }

            // case 2:
            Assert.IsFalse(coll.IsIndexExist(emptyIndexName));
            try {
                coll.GetIndexInfo(emptyIndexName);
                Assert.Fail();
            } catch (BaseException e) {
                Assert.AreEqual("SDB_IXM_NOTEXIST", e.ErrorType);
            }

            // case 3:
            String idIdxName = "$id";
            Assert.IsTrue(coll.IsIndexExist(idIdxName));
            BsonDocument indexObj = coll.GetIndexInfo(idIdxName);
            Assert. IsNotNull(indexObj);
            
            Console.Out.WriteLine("id index is: " + indexObj.ToString());
        }

        [TestMethod]
        public void BsonTimestampIncTest()
        {
            BsonTimestamp ts1 = new BsonTimestamp(10000, 1000000);
            Assert.AreEqual(10001, ts1.Timestamp);
            Assert.AreEqual(0, ts1.Increment);

            BsonTimestamp ts2 = new BsonTimestamp(10000, -1);
            Assert.AreEqual(9999, ts2.Timestamp);
            Assert.AreEqual(999999, ts2.Increment);

            BsonTimestamp ts3 = new BsonTimestamp(10000, 0);
            Assert.AreEqual(10000, ts3.Timestamp);
            Assert.AreEqual(0, ts3.Increment);

            BsonTimestamp ts4 = new BsonTimestamp(10000, 999999);
            Assert.AreEqual(10000, ts4.Timestamp);
            Assert.AreEqual(999999, ts4.Increment);

            int time = 1534942305;
            int inc = 123456789;
            int incSec = 123456789 / 1000000;
            int incMSec = 123456789 % 1000000;
            int incMSec2 = 1000000 - incMSec;
            BsonTimestamp ts5 = new BsonTimestamp(time, inc);
            Assert.AreEqual(time + incSec, ts5.Timestamp);
            Assert.AreEqual(incMSec, ts5.Increment);

            BsonTimestamp ts6 = new BsonTimestamp(time, -inc);
            Assert.AreEqual(time - incSec - 1, ts6.Timestamp);
            Assert.AreEqual(incMSec2, ts6.Increment);

            BsonTimestamp ts7 = new BsonTimestamp(-time, inc);
            Assert.AreEqual(-time + incSec, ts7.Timestamp);
            Assert.AreEqual(incMSec, ts7.Increment);

            BsonTimestamp ts8 = new BsonTimestamp(-time, -inc);
            Assert.AreEqual(-time - incSec - 1, ts8.Timestamp);
            Assert.AreEqual(incMSec2, ts8.Increment);

            BsonDocument doc = new BsonDocument();
            doc.Add("ts1", ts1);
            doc.Add("ts2", ts2);
            doc.Add("ts3", ts3);
            doc.Add("ts4", ts4);
            doc.Add("ts5", ts5);
            doc.Add("ts6", ts6);
            doc.Add("ts7", ts7);
            doc.Add("ts8", ts8);
            coll.Insert(doc);
            DBCursor cursor = coll.Query();
            BsonDocument result = cursor.Next();
            Console.WriteLine("result is: " + result);

        }

        [TestMethod]
        public void InsertRecordWithFlagTest()
        {
            BsonDocument record = new BsonDocument();
            record.Add("_id",1);
            record.Add("a",1);

            coll.Insert(record, SDBConst.FLG_INSERT_CONTONDUP);
            coll.Insert(record, SDBConst.FLG_INSERT_CONTONDUP);
            try
            {
                coll.Insert(record, 0);
            }
            catch (BaseException e)
            {
                Assert.AreEqual("SDB_IXM_DUP_KEY", e.ErrorType);
            }

        }

        [TestMethod()]
        public void IncrementFieldTest()
        {
            String field1 = "id1";
            String field2 = "id2";
            String field3 = "id3";
            BsonDocument options = new BsonDocument();
            options.Add("Field", "id1");
            // CreateAutoIncrement
            coll.CreateAutoIncrement(options);
            // check
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", cs.Name + "." + coll.Name);
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, null, null);
            BsonDocument result;
            try
            {
                result = cursor.Next();
            }
            finally
            {
                cursor.Close();
            }
            Assert.IsNotNull(result);
            BsonValue value = result.GetValue("AutoIncrement");
            BsonArray array = value.AsBsonArray;
            Assert.AreEqual(1, array.Count);

            BsonDocument options2 = new BsonDocument();
            options2.Add("Field", field2);
            BsonDocument options3 = new BsonDocument();
            options3.Add("Field", field3);
            List<BsonDocument> list = new List<BsonDocument>();
            list.Add(options2);
            list.Add(options3);
            coll.CreateAutoIncrement(list);
            // check
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, null, null);
            try
            {
                result = cursor.Next();
            }
            finally
            {
                cursor.Close();
            }
            Assert.IsNotNull(result);
            value = result.GetValue("AutoIncrement");
            array = value.AsBsonArray;
            Assert.AreEqual(3, array.Count);

            // DropAutoIncrement
            coll.DropAutoIncrement(field1);
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, null, null);
            try
            {
                result = cursor.Next();
            }
            finally
            {
                cursor.Close();
            }
            Assert.IsNotNull(result);
            value = result.GetValue("AutoIncrement");
            array = value.AsBsonArray;
            Assert.AreEqual(2, array.Count);

            List<String> list2 = new List<string>();
            list2.Add(field2);
            list2.Add(field3);
            coll.DropAutoIncrement(list2);
        }

        [TestMethod()]
        public void InsertAPITest()
        {
            long recordCount = 0;
            // case 1:
            BsonDocument doc1 = new BsonDocument();
            doc1.Add("a", 1);
            recordCount += 1;

            BsonValue oid1 = coll.Insert(doc1);
            Assert.IsTrue(oid1.IsObjectId);
            Console.WriteLine("doc1 is: {0}", doc1.ToString());

            // case 2:
            BsonDocument doc2 = new BsonDocument();
            doc2.Add("a", 2);
            doc2.Add("_id", 1);
            recordCount += 1;

            BsonValue oid2 = coll.Insert(doc2);
            Assert.IsTrue(oid2.IsInt32);
            Console.WriteLine("doc2 is: {0}", doc2.ToString());

            // case 3:
            BsonDocument doc3 = new BsonDocument();
            doc3.Add("a", 3);
            doc3.Add("_id", 3);
            recordCount += 1;

            BsonDocument result3 = coll.Insert(doc3, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.IsNotNull(result3);
            Assert.IsTrue(result3.Contains("_id"));
            Console.WriteLine("result3 is: {0}", result3.ToString());

            // case 4:
            BsonDocument doc4 = new BsonDocument();
            doc4.Add("a", 4);
            doc4.Add("_id", 4);
            recordCount += 1;

            BsonDocument result4 = coll.Insert(doc4, 0);
            Assert.IsNull(result4);
            BsonDocument result5 = coll.Insert(doc4, SDBConst.FLG_INSERT_CONTONDUP);
            Assert.IsNull(result5);
            BsonDocument result6 = coll.Insert(doc4, SDBConst.FLG_INSERT_CONTONDUP | SDBConst.FLG_INSERT_RETURN_OID);
            Assert.IsNotNull(result6);
            Assert.IsTrue(result6.Contains("_id"));
            Console.WriteLine("result6 is: {0}", result6.ToString());

            Assert.AreEqual(recordCount, coll.GetCount(null, null));
        }

        [TestMethod()]
        public void BulkInsertAPITest()
        {
            int recordCount = 0;
            List<BsonDocument> recordList1 = new List<BsonDocument>();
            recordList1.Add(new BsonDocument { { "a", 1 } });
            recordList1.Add(new BsonDocument { { "a", 2 } });
            recordCount += 2;

            List<BsonDocument> recordList2 = new List<BsonDocument>();
            recordList2.Add(new BsonDocument { { "a", 3 } });
            recordList2.Add(new BsonDocument { { "a", 4 } });
            recordCount += 2;

            List<BsonDocument> recordList3 = new List<BsonDocument>();
            recordList3.Add(new BsonDocument { { "a", 1 }, { "_id", 1 } });
            recordList3.Add(new BsonDocument { { "a", 2 }, { "_id", 2 } });
            recordCount += 2;

            List<BsonDocument> recordList4 = new List<BsonDocument>();
            recordList4.Add(new BsonDocument { { "a", 3 }, { "_id", 3 } });
            recordList4.Add(new BsonDocument { { "a", 4 }, { "_id", 4 } });
            recordList4.Add(new BsonDocument { { "a", 5 }, { "_id", 4 } });
            recordCount += 2;

            List<BsonDocument> recordList5 = new List<BsonDocument>();
            recordList5.Add(new BsonDocument { { "a", 3 }, { "_id", 5 } });
            recordList5.Add(new BsonDocument { { "a", 4 }, { "_id", 6 } });
            recordList5.Add(new BsonDocument { { "a", 5 }, { "_id", 7 } });
            recordCount += 3;

            // case 1:
            BsonDocument result1 = coll.Insert(recordList1, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.IsNotNull(result1);
            Console.WriteLine("result1 is: {0}", result1.ToString());

            // case 2:
            coll.EnsureOID = false;
            BsonDocument result2 = coll.Insert(recordList2, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.IsNotNull(result2);
            Console.WriteLine("result2 is: {0}", result2.ToString());

            // case 3:
            BsonDocument result3 = coll.Insert(recordList3, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.IsNotNull(result3);
            Console.WriteLine("result3 is: {0}", result3.ToString());

            // case 4:
            BsonDocument result4 = coll.Insert(recordList4, SDBConst.FLG_INSERT_RETURN_OID|SDBConst.FLG_INSERT_CONTONDUP);
            Assert.IsNotNull(result4);
            Console.WriteLine("result4 is: {0}", result4.ToString());

            // case 5:
            BsonDocument result5 = coll.Insert(recordList5, 0);
            Assert.IsNull(result5);

            // case 6:
            try
            {
                coll.Insert(new List<BsonDocument>(), 0);
            }
            catch (BaseException e) {
                Assert.AreEqual("SDB_INVALIDARG", e.ErrorType);
            }

            coll.BulkInsert(recordList5, SDBConst.FLG_INSERT_RETURN_OID | SDBConst.FLG_INSERT_CONTONDUP);

            Assert.AreEqual(recordCount, coll.GetCount(null, null));
        }


    }
}
