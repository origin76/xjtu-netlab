#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <server.hpp>
#include <socket.hpp>
#include "configparser.hpp"

using namespace std;

class CGIHandler {
public:
    CGIHandler(std::string r):root(r) {};

    void handle(const HttpRequest& req, HttpResponse& res) {
        string scriptPath = this->root + req.getPath(); // 去掉路径中的前导斜杠
        std::cout << scriptPath << std::endl;
        // 检查脚本是否存在
        if (!fileExists(scriptPath)) {
            res.setStatus(404, "Not Found");
            res.setBody("Script not found");
            return;
        }

        // 设置环境变量
        setenv("REQUEST_METHOD", req.getMethod().c_str(), 1);
        setenv("CONTENT_LENGTH", to_string(req.getBody().size()).c_str(), 1);
        setenv("CONTENT_TYPE", req.getHeader("Content-Type").c_str(), 1);

        // 创建管道
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            res.setStatus(500, "Internal Server Error");
            res.setBody("Failed to create pipe");
            return;
        }

        // 创建子进程
        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            res.setStatus(500, "Internal Server Error");
            res.setBody("Failed to fork");
            return;
        }

        if (pid == 0) {
            // 子进程
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            // 执行 CGI 脚本
            execl(scriptPath.c_str(), scriptPath.c_str(), (char*)nullptr);
            exit(1);
        } else {
            // 父进程
            close(pipefd[1]);
            char buffer[4096];
            string output;
            ssize_t bytes_read;

            while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                output += buffer;
            }
            close(pipefd[0]);

            // 等待子进程结束
            int status;
            waitpid(pid, &status, 0);

            // 设置响应
            res.setStatus(200, "OK");
            res.setHeader("Content-Type", "text/html");
            res.setBody(output);
        }
    }


private:
    static bool fileExists(const string& path) {
        return access(path.c_str(), F_OK) == 0;
    }

    std::string root;
};