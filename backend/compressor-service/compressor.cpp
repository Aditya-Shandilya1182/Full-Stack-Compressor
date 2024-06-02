#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <zstd.h>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

std::pair<std::vector<char>, size_t> Compress(const std::vector<char>& data) {
    size_t compressed_bound = ZSTD_compressBound(data.size());
    std::vector<char> compressed_data(compressed_bound);
    size_t compressed_size = ZSTD_compress(compressed_data.data(), compressed_bound, data.data(), data.size(), 1);
    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error("Compression failed: " + std::string(ZSTD_getErrorName(compressed_size)));
    }
    compressed_data.resize(compressed_size);
    return std::make_pair(compressed_data, data.size());
}

std::vector<char> Decompress(const std::vector<char>& data) {
    unsigned long long decompressed_size = ZSTD_getFrameContentSize(data.data(), data.size());
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error("Decompression failed: Not a valid compressed frame");
    } else if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("Decompression failed: Original size unknown");
    }

    std::vector<char> decompressed_data(decompressed_size);
    size_t actual_decompressed_size = ZSTD_decompress(decompressed_data.data(), decompressed_size, data.data(), data.size());
    if (ZSTD_isError(actual_decompressed_size)) {
        throw std::runtime_error("Decompression failed: " + std::string(ZSTD_getErrorName(actual_decompressed_size)));
    }
    if (actual_decompressed_size != decompressed_size) {
        throw std::runtime_error("Decompressed size does not match the expected size");
    }
    return decompressed_data;
}

void handleCompress(http_request request) {
    request.extract_vector().then([request](std::vector<unsigned char> body) {
        std::vector<char> buffer(body.begin(), body.end());
        try {
            auto [compressed_data, original_size] = Compress(buffer);
            std::vector<unsigned char> compressed_data_unsigned(compressed_data.begin(), compressed_data.end());
            web::json::value json_response;
            json_response[U("message")] = web::json::value::string(U("File compressed successfully."));
            json_response[U("compressedData")] = web::json::value::string(utility::conversions::to_base64(compressed_data_unsigned));
            json_response[U("originalSize")] = web::json::value::number(original_size);
            request.reply(status_codes::OK, json_response);
        } catch (const std::exception& e) {
            request.reply(status_codes::InternalError, e.what());
        }
    }).wait();
}

void handleDecompress(http_request request) {
    request.extract_json().then([request](json::value body) {
        try {
            auto compressed_data_str = body[U("compressedData")].as_string();
            std::vector<unsigned char> compressed_data_unsigned = utility::conversions::from_base64(compressed_data_str);
            std::vector<char> compressed_data(compressed_data_unsigned.begin(), compressed_data_unsigned.end());

            auto decompressed_data = Decompress(compressed_data);

            http_response response(status_codes::OK);
            response.set_body(Concurrency::streams::bytestream::open_istream(std::vector<unsigned char>(decompressed_data.begin(), decompressed_data.end())));
            response.headers().add(U("Content-Disposition"), U("attachment; filename=decompressed.txt"));
            response.headers().set_content_type(U("application/octet-stream"));
            request.reply(response);
        } catch (const std::exception& e) {
            request.reply(status_codes::InternalError, e.what());
            std::cout << e.what() << std::endl;
        }
    }).wait();
}


void handleRequest(http_request req) {
    auto path = uri::split_path(uri::decode(req.relative_uri().path()));
    if (!path.empty()) {
        if (path[0] == U("compress")) {
            handleCompress(req);
        } else if (path[0] == U("decompress")) {
            handleDecompress(req);
        } else {
            req.reply(status_codes::NotFound, U("Not Found"));
        }
    } else {
        req.reply(status_codes::NotFound, U("Not Found"));
    }
}

int main() {
    http_listener listener("http://localhost:8081");
    listener.support(methods::POST, handleRequest);

    try {
        listener.open().wait();
        std::cout << "Listening on http://localhost:8081" << std::endl;
        std::cin.get();
        listener.close().wait();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}



//g++ -o compressor compressor.cpp -lssl -lcrypto -lzstd -lcpprest -lboost_system -lpthread