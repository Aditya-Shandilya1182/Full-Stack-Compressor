#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <zstd.h>

std::vector<uint8_t> readFile(const std::string &fileName){
    
    std::ifstream file(fileName, std::ios::binary);
    
    if(!file){
        throw std::runtime_error("Failed to open file!");
    }

    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>())
}

void writeFile(const std::string& filename, const std::vector<uint8_t>& data){
    
    std::ofstream file(filename, std::ios::binary);
    
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}
