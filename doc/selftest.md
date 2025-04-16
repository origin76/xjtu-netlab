#### 支持Cookie（见RFC 2109）的基本机制，实现典型的网站登录

测试过程：使用浏览器的Private模式（没有任何缓存），访问我们的服务器，然后会发现浏览器保留了cookie。

![1744785622503.png](https://s2.loli.net/2025/04/16/aYOhUKEry7flXSj.png)

#### 支持基本的缓存处理
通过服务端来实现，使用map来记录哪些文件已经被读取，哪些没有。对于已经读取过（缓存)）的文件，会直接发送而无需再次读取；

初次访问某个文件（未缓存）：
![image.png](https://s2.loli.net/2025/04/16/utbvBTaDh1KOdU4.png)

再次访问：
![image.png](https://s2.loli.net/2025/04/16/zpXeKfyUsHDIYLF.png)

#### 可配置Web服务器的监听地址、监听端口和虚拟路径
我们的server采用了独立的配置文件配置的方式：
![image.png](https://s2.loli.net/2025/04/16/Yw2MpcRDi6dbkG5.png)
在server启动时，会自动读取配置文件来选择合适的参数：
![image.png](https://s2.loli.net/2025/04/16/2CZmiPzNAl9GcTf.png)

#### 能够多线程处理并发的请求，或采取其他方法正确处理多个并发连接。
基于线程池实现，同时处理多个请求。测试方式，（真）同时打开多个浏览器tab，检查服务端连接情况：
![image.png](https://s2.loli.net/2025/04/16/4nOZLkjNIWumGcS.png)
![image.png](https://s2.loli.net/2025/04/16/VXeWrSfJ6NwljAG.png)

#### 对于无法成功定位文件的请求，根据错误原因，作相应错误提示。支持一定的异常情况处理能力。
使用`terminal`连接server，并请求不存在的文件：
![image.png](https://s2.loli.net/2025/04/16/TX41uxJpobDZBjy.png)
返回了file not found

浏览器访问：
![image.png](https://s2.loli.net/2025/04/16/ZHGqEhSNcUTL8sa.png)

服务端记录：
![image.png](https://s2.loli.net/2025/04/16/kOX2cU1RLZEFCmg.png)

#### 在服务器端的日志中记录每一个请求（如IP地址、端口号和HTTP请求命令行，及应答码等）。

GET请求：
![image.png](https://s2.loli.net/2025/04/16/dUL3a5XDqiGvHzY.png)


POST请求：
![image.png](https://s2.loli.net/2025/04/16/1btUwAKupIPRqDN.png)

HEAD请求：
![image.png](https://s2.loli.net/2025/04/16/BFJ2TgIHO6A8uSR.png)

#### 