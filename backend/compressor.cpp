#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <zstd.h>

std::vector<char> readFile(const std::string &fileName) {
    
    std::ifstream inFile(fileName, std::ios_base::binary | std::ios_base::ate);
    
    if (!inFile) {
        throw std::runtime_error("Error opening input file: " + fileName);
    }

    std::streamsize fileSize = inFile.tellg();
    inFile.seekg(0, std::ios_base::beg);

    std::vector<char> buffer(fileSize);
    inFile.read(buffer.data(), fileSize);

    return buffer;
}

void writeFile(const std::string &fileName, const std::vector<char> &buffer) {
    
    std::ofstream outFile(fileName, std::ios_base::binary);
    
    if (!outFile) {
        throw std::runtime_error("Error opening output file: " + fileName);
    }

    outFile.write(buffer.data(), buffer.size());
}

void compressFile(const std::string &inputFileName, const std::string &outputFileName){
    
    std::vector<char> buffer = readFile(inputFileName);

    size_t compressedBound = ZSTD_compressBound(buffer.size());
    std::vector<char> compressedBuffer(compressedBound);

    size_t compressedSize = ZSTD_compress(compressedBuffer.data(), compressedBound, buffer.data(), buffer.size(), 1);
    if (ZSTD_isError(compressedSize)) {
        throw std::runtime_error("Compression failed: " + std::string(ZSTD_getErrorName(compressedSize)));
    }

    compressedBuffer.resize(compressedSize);
    writeFile(outputFileName, compressedBuffer);

    std::cout << "File compressed successfully!" << std::endl;
}

void decompressFile(const std::string &inputFileName, const std::string &outputFileName) {
    std::vector<char> compressedBuffer = readFile(inputFileName);

    unsigned long long decompressedSize = ZSTD_getFrameContentSize(compressedBuffer.data(), compressedBuffer.size());
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error("Invalid compressed file");
    } else if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("Original size unknown");
    }

    std::vector<char> decompressedBuffer(decompressedSize);

    size_t actualDecompressedSize = ZSTD_decompress(decompressedBuffer.data(), decompressedSize, compressedBuffer.data(), compressedBuffer.size());
    if (ZSTD_isError(actualDecompressedSize)) {
        throw std::runtime_error("Decompression failed: " + std::string(ZSTD_getErrorName(actualDecompressedSize)));
    }

    writeFile(outputFileName, decompressedBuffer);

    std::cout << "File decompressed successfully!" << std::endl;
}
double calculateFileSize(const std::string& fileName){
    std::ifstream file(fileName, std::ios::binary);
    file.seekg(0, std::ios::end);
    double sizeOfFile = (double)file.tellg();
    return sizeOfFile;
}

double compressionRatio(const std::string& compressedFileName, const std::string& originalFileName){
    double originalFileSize = calculateFileSize(originalFileName);
    double compressedFileSize = calculateFileSize(compressedFileName);
    double ratio = (originalFileSize) / (compressedFileSize);
    return ratio;
}
int main(){
    std::string inputFileName = "input.txt";
    std::string compressedFileName = "compressed.zst";
    std::string decompressedFileName = "decompressed.txt";

    compressFile(inputFileName, compressedFileName);
    decompressFile(compressedFileName, decompressedFileName);

    double ratio = compressionRatio(compressedFileName, inputFileName);
    std::cout << "Compression ratio: " << ratio << std::endl;

    return 0;
}