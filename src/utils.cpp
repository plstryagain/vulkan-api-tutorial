#include "utils.h"

#include <fstream>

namespace utils
{

std::vector<char> readFile(const std::string& file_path)
{
    std::ifstream file(file_path, std::fstream::ate | std::fstream::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + file_path);
    }
    size_t file_size = file.tellg();
    std::vector<char> content;
    content.resize(file_size);
    file.seekg(0);
    file.read(content.data(), file_size);
    file.close();
    return content;
}
    
} // namespace utils

