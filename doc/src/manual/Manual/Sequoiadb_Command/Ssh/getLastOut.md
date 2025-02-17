##名称##

getLastOut - 获取上次命令执行的返回结果

##语法##

**getLastOut()**

##类别##

Ssh

##描述##

获取上次命令执行的返回结果。

##参数##

无

##返回值##

返回上次命令执行的返回结果。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 使用 SSH 方式连接主机。

    ```lang-javascript
    > var ssh = new Ssh( "192.168.20.71", "sdbadmin", "sdbadmin", 22 )
    ```

* 执行命令。

    ```lang-javascript
    > ssh.exec( "ls /opt/sequoiadb/file" )
    file1
    file2
    file3
    ```

* 获取上次命令执行的返回结果。

    ```lang-javascript
    > ssh.getLastOut()
    file1
    file2
    file3
    ```
