#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "liboai.h"

// Command line argument structure
struct Config {
    std::string api_key = "";
    std::string model = "llama-3.3-70b";
    std::string prompt = "Hello, world!";
    int max_tokens = 100;
    bool streaming = false;
};

// Simple command line argument parser using boost::program_options
Config parse_arguments(int argc, char* argv[]) {
    namespace po = boost::program_options;
    
    Config config;
    
    try {
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "Show this help message")
            ("api_key", po::value<std::string>(&config.api_key), "API key for authentication (required)")
            ("model", po::value<std::string>(&config.model)->default_value("llama-3.3-70b"), "Model to use for inference")
            ("prompt", po::value<std::string>(&config.prompt)->default_value("Hello, world!"), "Prompt to send to the API")
            ("max_tokens", po::value<int>(&config.max_tokens)->default_value(100), "Maximum number of tokens to generate")
            ("stream", po::bool_switch(&config.streaming)->default_value(false), "Enable streaming mode for real-time response");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << desc << "\n";
            exit(0);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command line arguments: " << e.what() << std::endl;
        exit(1);
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

// Performs a chat completion request using liboai and prints the response
void do_rest_call(const Config& config) {
    if (config.api_key.empty()) {
        log_error("API key is required. Please provide --api_key flag.");
        return;
    }

    try {
        // Initialize liboai with Cerebras API endpoint
        liboai::OpenAI oai("https://api.cerebras.ai/v1");
        
        // Set the API key for authorization
        bool auth_success = oai.auth.SetKey(config.api_key);
        if (!auth_success) {
            log_error("Failed to set API key.");
            return;
        }
        
        // Create a conversation object
        liboai::Conversation convo;
        
        // Add the user prompt to the conversation
        bool prompt_success = convo.AddUserData(config.prompt);
        if (!prompt_success) {
            log_error("Failed to add user prompt to conversation.");
            return;
        }
        
        if (config.streaming) {
            // Handle streaming response
            log_info("Streaming response:");
            std::cout << "\n--- Streaming Response ---\n";
            
            // Create streaming callback function
            auto stream_callback = [](std::string data, intptr_t userdata, liboai::Conversation& convo) -> bool {
                // Process the streaming data
                std::string delta_content;
                bool completed = false;
                
                // Append stream data to conversation
                if (convo.AppendStreamData(data, delta_content, completed)) {
                    if (!delta_content.empty()) {
                        std::cout << delta_content << std::flush;
                    }
                }
                
                // Return true to continue streaming, false to stop
                return !completed;
            };
            
            // Make the streaming API call
            liboai::Response response = oai.ChatCompletion->create(
                config.model,
                convo,
                std::nullopt,  // function_call
                std::nullopt,  // temperature
                std::nullopt,  // top_p
                std::nullopt,  // n
                stream_callback,  // stream callback
                std::nullopt,  // stop
                config.max_tokens  // max_tokens
            );
            
            std::cout << "\n--- End of Stream ---\n";
        } else {
            // Handle non-streaming response
            liboai::Response response = oai.ChatCompletion->create(
                config.model,
                convo,
                std::nullopt,  // function_call
                std::nullopt,  // temperature
                std::nullopt,  // top_p
                std::nullopt,  // n
                std::nullopt,  // stream
                std::nullopt,  // stop
                config.max_tokens  // max_tokens
            );
            
            // Extract and print the response content
            if (response.raw_json.contains("choices") && !response.raw_json["choices"].empty()) {
                std::string content = response.raw_json["choices"][0]["message"]["content"];
                log_info("Response: " + content);
            } else {
                log_error("No response content received.");
            }
        }
    } catch (const std::exception& e) {
        log_error("API call failed: " + std::string(e.what()));
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    Config config = parse_arguments(argc, argv);
    
    // Make the API call
    do_rest_call(config);
    
    log_info("Done!");
    return EXIT_SUCCESS;
}
