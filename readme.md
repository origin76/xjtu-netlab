**请使用linux环境开发**

```
wget https://archives.boost.io/release/1.82.0/source/boost_1_82_0.tar.bz2
```

```
sudo apt-get install libssl-dev
```

## 目标

1、基于标准的HTTP/1.1协议（RFC 2616等），协议支持方面的要求：

A. 基本要求：

+ [ ]  支持GET、HEAD和POST三种请求方法
+ [x]  支持URI的"%HEXHEX"编码，如对 http://abc.com:80/~smith/ 和 http://ABC.com/%7Esmith/ 两种等价的URI能够正确处理；
+ [ ]  正确给出应答码（如200，304，100，404，500等）；
+ [ ]  支持Connection: Keep-Alive和Connection: Close两种连接模式。

B. 高级要求：

+ [ ]  支持HTTPS，
+ [ ]  支持分块传输编码(Chunked Transfer Encoding)，
+ [ ]  支持gzip等内容编码；
+ [ ]  支持Cookie（见RFC 2109）的基本机制，实现典型的网站登录；
+ [ ]  支持基本的缓存处理；
+ [ ]  支持基于POST方法的文件上传。

2、服务器的基本要求包括：

+ [x] 可配置Web服务器的监听地址、监听端口和虚拟路径。

+ [x] 能够多线程处理并发的请求，或采取其他方法正确处理多个并发连接。

+ [ ] 对于无法成功定位文件的请求，根据错误原因，作相应错误提示。支持一定的异常情况处理能力。

+ [x] 服务可以启动和关闭。

+ [ ] 在服务器端的日志中记录每一个请求（如IP地址、端口号和HTTP请求命令行，及应答码等）。

3、服务器的高级要求是支持CGI todo

+ [ ] CGI（Common Gateway
  Interface）并不是HTTP协议的一部分，而是一个独立的标准，用于定义Web服务器如何与外部程序（如脚本或可执行文件）交互以生成动态内容。CGI的主要目的是允许Web服务器调用外部程序来处理HTTP请求并生成响应。参见RFC
  3875。

## Usage(Tests)

- Get请求：可以使用如下命令在终端中手动使用HTTP协议发送GET请求
  ```shell
  openssl s_client -connect 127.0.0.1:8080 
  ``` 
- 上传文件：许可的上传url和server端保存路径通过 [`config.ini`](./config.ini) 进行配置。
- 转发：类似nginx的proxy_pass，尚未实现，可以通过 [`config.ini`](./config.ini) 配置前缀