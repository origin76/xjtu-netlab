#include <iostream>
#include <server.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <zlib.h>

using namespace std;

class GzipHandler {
public:
    static bool compress(const string &input, string &output) {
        // 创建Gzip压缩流
        gzFile gzFile = gzopen("compressed.gz", "wb");
        if (!gzFile) {
            return false;
        }

        // 写入数据
        gzwrite(gzFile, input.data(), input.size());

        // 关闭Gzip流
        if (gzclose(gzFile) != Z_OK) {
            return false;
        }

        // 读取压缩后的数据
        ifstream ifs("compressed.gz", ios::binary);
        if (!ifs) {
            return false;
        }

        ifs.seekg(0, ios::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, ios::beg);
        output.resize(size);
        ifs.read(&output[0], size);
        ifs.close();

        // 删除临时文件
        remove("compressed.gz");

        return true;
    }

    static bool decompress(const string &input, string &output) {
        uLongf destLen = input.size() * 2; // 初始猜测
        output.resize(destLen);

        int ret = uncompress((Bytef *) output.data(), &destLen, (const Bytef *) input.data(), input.size());

        if (ret != Z_OK) {
            return false;
        }

        output.resize(destLen);
        return true;
    }
};
