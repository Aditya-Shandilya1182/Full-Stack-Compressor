#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <zstd.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

std::vector<char> ReadFile(const std::string& file_name) {
  std::ifstream in_file(file_name, std::ios_base::binary | std::ios_base::ate);
  if (!in_file) {
    throw std::runtime_error("Error opening input file: " + file_name);
  }
  std::streamsize file_size = in_file.tellg();
  in_file.seekg(0, std::ios_base::beg);
  std::vector<char> buffer(file_size);
  in_file.read(buffer.data(), file_size);
  return buffer;
}

void WriteFile(const std::string& file_name, const std::vector<char>& buffer) {
  std::ofstream out_file(file_name, std::ios_base::binary);
  if (!out_file) {
    throw std::runtime_error("Error opening output file: " + file_name);
  }
  out_file.write(buffer.data(), buffer.size());
}

std::vector<char> Compress(const std::vector<char>& data) {
    
    size_t compressed_bound = ZSTD_compressBound(data.size());
    
    std::vector<char> compressed_data(compressed_bound);
    
    size_t compressed_size = ZSTD_compress(compressed_data.data(), compressed_bound, data.data(), data.size(), 1);
    
    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error("Compression failed: " + std::string(ZSTD_getErrorName(compressed_size)));
    }

    compressed_data.resize(compressed_size);
    
    return compressed_data;
}

void handleDecompress(web::http::http_request req){

}

void handleCompress(web::http::http_request request) {
    
    request.content_ready().then([request](web::http::http_request req) {
        
        auto fileStream = std::make_shared<Concurrency::streams::istream>(req.body());
        
        Concurrency::streams::container_buffer<std::vector<unsigned char>> inBuffer;
        
        fileStream->read_to_end(inBuffer).then([fileStream, inBuffer, request](size_t) {
            
            std::vector<char> buffer(inBuffer.collection().begin(), inBuffer.collection().end());
            
            try {
                
                auto compressed_data = Compress(buffer);
                web::http::http_response response(web::http::status_codes::OK);
                response.set_body(Concurrency::streams::bytestream::open_istream(std::vector<unsigned char>(compressed_data.begin(), compressed_data.end())));
                response.headers().add(U("Content-Disposition"), U("attachment; filename=compressed.zst"));
                response.headers().set_content_type(U("application/octet-stream"));
                request.reply(response);

            } catch (const std::exception& e) {
                request.reply(web::http::status_codes::InternalError, e.what());
            }
        }).wait();

    }).wait();
}


void handleRequest(web::http::http_request req){
    auto path = web::uri::split_path(web::uri::decode(req.relative_uri().path()));
    if (!path.empty()) {
    if (path[0] == U("compress")) {
      handleCompress(req);
    } else if (path[0] == U("decompress")) {
      handleDecompress(req);
    } else {
      req.reply(web::http::status_codes::NotFound, U("Not Found"));
    }
  } else {
    req.reply(web::http::status_codes::NotFound, U("Not Found"));
  }
}

int main(){
    
    web::http::experimental::listener::http_listener listener("http://localhost:8081");
    listener.support(web::http::methods::POST, handleRequest);

    try{
        listener.open().wait();
        std::cout << "Listening on http://localhost:8081" << std::endl;
        std::cin.get();
        listener.close().wait();
    }catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}