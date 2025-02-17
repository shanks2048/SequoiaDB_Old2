##名称##

execUpdate - 执行除 select 以外的 SQL 语句

##语法##

**db.execUpdate(\<other sql\>)**

##类别##

Sdb

##描述##

该函数用于执行 SQL 除 select 以外的其它语句。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

向集合 sample.employee 中插入新的记录

```lang-javascript
> db.execUpdate("insert into sample.employee(name,age) values('zhangshang', 30)")
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md