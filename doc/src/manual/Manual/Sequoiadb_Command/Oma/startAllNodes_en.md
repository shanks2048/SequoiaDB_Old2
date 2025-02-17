##NAME##

startAllNodes - start all node with the specified business name

##SYNOPSIS##

**oma.startAllNodes(\[businessName\])**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to start all nodes with the specified business name in the machine where the resource management node (sdbcm) is located.

##PARAMETERS##

businessName ( *string, optional* )

Business name

- This parameter is only used internally.
- If this parameter is not specified, all nodes of the machine where sdbcm is located will be started by default.
  
##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

Start all nodes with the business name "yyy".

```lang-javascript
> var oma = new Oma("localhost", 11790)
> oma.startAllNodes("yyy")  
Start sequoiadb(20000): Success
Start sequoiadb(40000): Success
Start sequoiadb(30020): Success
Start sequoiadb(50000): Success
Start sequoiadb(30010): Success
Start sequoiadb(30000): Success
Start sequoiadb(42000): Success
Start sequoiadb(41000): Success
Total: 8; Success: 8; Failed: 0
```


[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md