#pragma once
#include <string>
class Compressor {
public:
    static std::string gzip(const std::string& input);
    static std::string deflate(const std::string& input);
};
