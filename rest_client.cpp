#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#include "nlohmann/json.hpp"

// Include as_tuple to match the pattern used in websocket_session.cpp
#include <boost/asio/as_tuple.hpp>

// Command line argument structure
struct Config {
    std::string api_key = "";
    std::string model = "llama-3.3-70b";
    std::string prompt = "Hello, world!";
    int max_tokens = 100;
    bool streaming = false;
};

// Simple command line argument parser
Config parse_arguments(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg.find("--api_key=") == 0) {
            config.api_key = arg.substr(10);
        } else if (arg.find("--model=") == 0) {
            config.model = arg.substr(8);
        } else if (arg.find("--prompt=") == 0) {
            config.prompt = arg.substr(9);
        } else if (arg.find("--max_tokens=") == 0) {
            config.max_tokens = std::stoi(arg.substr(13));
        } else if (arg == "--stream" || arg == "--streaming") {
            config.streaming = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --api_key=KEY        API key for authentication (required)\n";
            std::cout << "  --model=MODEL        Model to use for inference (default: llama-3.3-70b)\n";
            std::cout << "  --prompt=TEXT        Prompt to send to the API (default: Hello, world!)\n";
            std::cout << "  --max_tokens=N       Maximum number of tokens to generate (default: 100)\n";
            std::cout << "  --stream, --streaming Enable streaming mode for real-time response\n";
            std::cout << "  --help, -h           Show this help message\n";
            exit(0);
        }
    }
    
    return config;
}

// Simple logging functions
void log_error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void log_info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

// Helper function to parse Server-Sent Events (SSE)
std::vector<std::string> parse_sse_chunks(const std::string& data) {
    std::vector<std::string> chunks;
    std::istringstream stream(data);
    std::string line;
    std::string current_chunk;
    
    while (std::getline(stream, line)) {
        if (line.empty()) {
            // Empty line indicates end of chunk
            if (!current_chunk.empty()) {
                chunks.push_back(current_chunk);
                current_chunk.clear();
            }
        } else if (line.find("data: ") == 0) {
            // Extract data after "data: "
            std::string data_content = line.substr(6);
            if (data_content != "[DONE]") {
                current_chunk += data_content;
            }
        }
    }
    
    // Add the last chunk if it exists
    if (!current_chunk.empty()) {
        chunks.push_back(current_chunk);
    }
    
    return chunks;
}

// Helper function to extract content from streaming response
std::string extract_streaming_content(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            auto& choice = j["choices"][0];
            if (choice.contains("delta") && choice["delta"].contains("content")) {
                return choice["delta"]["content"];
            }
        }
    } catch (const std::exception& e) {
        // Ignore JSON parsing errors for malformed chunks
    }
    return "";
}


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

// Performs an HTTPS POST request to api.cerebras.ai and prints the response
net::awaitable<void> do_rest_call(const Config& config) {
  if (config.api_key.empty()) {
    log_error("API key is required. Please provide --api_key flag.");
    co_return;
  }

  // The io_context is required for all I/O
  auto ioc = co_await net::this_coro::executor;

  // The SSL context is required, and holds certificates
  ssl::context ctx{ssl::context::tlsv12_client};

  // This holds the root certificate used for verification
  ctx.set_default_verify_paths();

  // Verify the remote server's certificate
  ctx.set_verify_mode(ssl::verify_peer);

  // These objects perform our I/O
  tcp::resolver resolver(ioc);
  net::ssl::stream<tcp::socket> stream(ioc, ctx);

  // Look up the domain name
  auto [ec_resolve, endpoints] = co_await resolver.async_resolve(
      "api.cerebras.ai", "443", net::as_tuple(net::use_awaitable));
  if (ec_resolve) {
    log_error("Failed to resolve api.cerebras.ai: " + ec_resolve.message());
    co_return;
  }
  // Connect to the server
  auto& socket = beast::get_lowest_layer(stream);
  auto [ec_connect, endpoint] = co_await net::async_connect(
      socket, endpoints, net::as_tuple(net::use_awaitable));
  if (ec_connect) {
    log_error("Failed to connect to api.cerebras.ai: " + ec_connect.message());
    co_return;
  }

  // Set up the HTTP request
  json request_body;
  request_body["model"] = config.model;
  request_body["messages"] =
      json::array({{{"role", "user"}, {"content", config.prompt}}});
  request_body["max_tokens"] = config.max_tokens;
  request_body["stream"] = config.streaming;

  // ADD THIS BLOCK - Set SNI Hostname (Server Name Indication)
  if (!SSL_set_tlsext_host_name(stream.native_handle(), "api.cerebras.ai")) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
    log_error("Failed to set SNI hostname: " + ec.message());
    co_return;
  }

  // Perform the SSL handshake (this call was already here)
  co_await stream.async_handshake(ssl::stream_base::client, net::use_awaitable);

  // Set up an HTTP POST request message
  http::request<http::string_body> req{http::verb::post, "/v1/chat/completions",
                                       11};
  req.set(http::field::host, "api.cerebras.ai");
  req.set(http::field::content_type, "application/json");
  req.set(http::field::authorization, "Bearer " + config.api_key);
  req.body() = request_body.dump();
  req.prepare_payload();

  // Send the HTTP request to the remote host
  co_await http::async_write(stream, req, net::use_awaitable);

  // This buffer is used for reading and must be persisted
  beast::flat_buffer buffer;

  if (config.streaming) {
    // Handle streaming response
    log_info("Streaming response:");
    std::cout << "\n--- Streaming Response ---\n";
    
    // Read the complete streaming response
    http::response<http::string_body> res;
    co_await http::async_read(stream, buffer, res, net::use_awaitable);
    
    // Check if we got a streaming response
    if (res.result() == http::status::ok) {
      std::string response_body = res.body();
      
      // Parse all SSE chunks from the response
      auto chunks = parse_sse_chunks(response_body);
      
      // Process each chunk (similar to Node.js: for await (const chunk of stream))
      for (const auto& chunk : chunks) {
        // Extract content from this chunk
        std::string content = extract_streaming_content(chunk);
        if (!content.empty()) {
          std::cout << content << std::flush; // Print without newline for streaming effect
        }
      }
    } else {
      // Non-200 response, show the error
      log_info("Response: " + res.body());
    }
    
    std::cout << "\n--- End of Stream ---\n";
  } else {
    // Handle non-streaming response (original behavior)
    http::response<http::string_body> res;
    
    // Receive the HTTP response
    co_await http::async_read(stream, buffer, res, net::use_awaitable);
    
    // Write the response to stdout
    log_info("Response: " + res.body());
  }

  // Gracefully close the stream
  beast::error_code ec;
  stream.shutdown(ec);
  if (ec == net::error::eof) {
    // Rationale:
    // https://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    ec = {};
  }
  if (ec) {
    log_error("Shutdown error: " + ec.message());
  }

  co_return;
}

int main(int argc, char* argv[]) {
  // Parse command line arguments
  Config config = parse_arguments(argc, argv);

  // The io_context is required for all I/O
  net::io_context ioc;
  auto completion_handler = [](std::exception_ptr e) {
    if (e) {
      // An unhandled exception escaped the coroutine.
      // We can rethrow and catch it here.
      try {
        std::rethrow_exception(e);
      } catch (const std::exception& ex) {
        log_error("Coroutine finished with an exception: " + std::string(ex.what()));
      }
    } else {
      // Coroutine finished successfully.
      log_info("Coroutine finished successfully.");
    }
  };
  // Launch the asynchronous operation
  net::co_spawn(ioc, do_rest_call(config), completion_handler);

  // Run the I/O service
  try {
    ioc.run();
  } catch (const boost::system::system_error& se) {
    log_error("Listener: " + se.code().message());
  } catch (const std::exception& e) {
    log_error("Listener exception: " + std::string(e.what()));
  }
  log_info("Done!");
  return EXIT_SUCCESS;
}
