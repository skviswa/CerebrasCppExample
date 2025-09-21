#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <fstream>
#include <future>
#include <atomic>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include "liboai.h"


// Command line argument structure
struct CommandLineConfig {
    std::string api_key = "";
    std::string input_file = "";
    std::string output_file = "benchmark_results.json";
    std::string api_endpoint = "https://api.cerebras.ai/v1";
    std::string model = "llama-3.3-70b";
    int concurrent_requests = 10;
};

// Simple command line argument parser using boost::program_options
CommandLineConfig parse_arguments(int argc, char* argv[]) {
    namespace po = boost::program_options;
    
    CommandLineConfig config;
    
    try {
        po::options_description desc("Throughput Test Options");
        desc.add_options()
            ("help,h", "Show this help message")
            ("api_key", po::value<std::string>(&config.api_key), "API key for Cerebras authentication (required)")
            ("api_endpoint", po::value<std::string>(&config.api_endpoint)->default_value("https://api.cerebras.ai/v1"), "API endpoint URL")
            ("model", po::value<std::string>(&config.model)->default_value("llama-3.3-70b"), "Model to use for completions")
            ("input_file", po::value<std::string>(&config.input_file), "Path to JSONL file containing completion requests (required)")
            ("concurrent_requests", po::value<int>(&config.concurrent_requests)->default_value(10), "Number of concurrent requests")
            ("output_file", po::value<std::string>(&config.output_file)->default_value("throughput_stats.json"), "Path to output JSON stats file");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << desc << "\n";
            exit(0);
        }

        if (config.api_key.empty()) {
            std::cerr << "Error: API key is required. Please provide --api_key flag.\n";
            std::cerr << desc << "\n";
            exit(1);
        }

        if (config.input_file.empty()) {
            std::cerr << "Error: Input file is required. Please provide --input_file flag.\n";
            std::cerr << desc << "\n";
            exit(1);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command line arguments: " << e.what() << std::endl;
        exit(1);
    }
    
    return config;
}

// Load completion requests from JSONL file
std::vector<nlohmann::json> load_requests_from_jsonl(const std::string& filename) {
    std::vector<nlohmann::json> requests;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + filename);
    }
    
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        if (line.empty()) {
            continue;
        }
        
        try {
            nlohmann::json request = nlohmann::json::parse(line);
            requests.push_back(request);
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Warning: Failed to parse JSON on line " << line_number << ": " << e.what() << std::endl;
        }
    }
    
    file.close();
    std::cout << "[INFO] Loaded " + std::to_string(requests.size()) + " requests from " + filename << std::endl;
    return requests;
}

struct CompletionStats {
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ttft_time = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::time_point{};
    size_t number_of_chunks = 0;
    nlohmann::json input = nlohmann::json{};
    std::string output_text = "";
    bool success = true;
    std::string error_message = "";

    // Helper functions to calculate durations
    std::optional<double> get_total_duration() const {
        if (end_time.time_since_epoch().count() > 0 && start_time.time_since_epoch().count() > 0) {
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
            return duration.count();
        }
        return std::nullopt;
    }

    std::optional<double> get_ttft_duration() const {
        if (ttft_time.time_since_epoch().count() > 0 && start_time.time_since_epoch().count() > 0) {
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(ttft_time - start_time);
            return duration.count();
        }
        return std::nullopt;
    }

    // Helper functions to get timestamps in seconds since epoch
    std::optional<double> get_start_time() const {
        if (start_time.time_since_epoch().count() > 0) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                start_time.time_since_epoch()).count();
        }
        return std::nullopt;
    }

    std::optional<double> get_ttft_time() const {
        if (ttft_time.time_since_epoch().count() > 0) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                ttft_time.time_since_epoch()).count();
        }
        return std::nullopt;
    }

    std::optional<double> get_end_time() const {
        if (end_time.time_since_epoch().count() > 0) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                end_time.time_since_epoch()).count();
        }
        return std::nullopt;
    }

    // Usage information from API
    struct UsageDetails {
        size_t prompt_tokens = 0;
        size_t completion_tokens = 0;
        size_t total_tokens = 0;

        nlohmann::json to_json() const {
            return {
                {"prompt_tokens", prompt_tokens},
                {"completion_tokens", completion_tokens},
                {"total_tokens", total_tokens}
            };
        }
    };
    UsageDetails api_usage{};

    // Time information from API
    struct TimeInfo {
        double queue_time = 0.0;
        double prompt_time = 0.0;
        double completion_time = 0.0;
        double total_time = 0.0;
        long long created = 0;

        nlohmann::json to_json() const {
            return {
                {"queue_time", queue_time},
                {"prompt_time", prompt_time},
                {"completion_time", completion_time},
                {"total_time", total_time},
                {"created", created}
            };
        }
    };
    TimeInfo api_time_info{};

    nlohmann::json to_json() const {
        nlohmann::json completion_json;
        completion_json["input"] = input;
        completion_json["output_text"] = output_text;
        completion_json["success"] = success;
        completion_json["error_message"] = error_message;
        
        // Add duration information
        auto total_duration = get_total_duration();
        if (total_duration.has_value()) {
            completion_json["total_duration_seconds"] = total_duration.value();
        }
        
        auto ttft_duration = get_ttft_duration();
        if (ttft_duration.has_value()) {
            completion_json["ttft_duration_seconds"] = ttft_duration.value();
        }
        
        completion_json["number_of_chunks"] = number_of_chunks;
        
        // Add timestamp information in seconds since epoch
        auto start_time_seconds = get_start_time();
        if (start_time_seconds.has_value()) {
            completion_json["start_time"] = start_time_seconds.value();
        }
        
        auto ttft_time_seconds = get_ttft_time();
        if (ttft_time_seconds.has_value()) {
            completion_json["ttft_time"] = ttft_time_seconds.value();
        }
        
        auto end_time_seconds = get_end_time();
        if (end_time_seconds.has_value()) {
            completion_json["end_time"] = end_time_seconds.value();
        }
        
        // Add API usage details
        completion_json["api_usage"] = api_usage.to_json();
        
        // Add API time info
        completion_json["api_time_info"] = api_time_info.to_json();
        
        return completion_json;
    }
};

// Stats structure for overall performance metrics
struct OverallStats {
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::time_point{};
    size_t total_prompt_tokens = 0;
    size_t total_completion_tokens = 0;
    size_t total_tokens = 0;
    size_t total_number_requests = 0;
    size_t total_number_failures = 0;

    // Helper functions to calculate durations
    std::optional<double> get_total_duration() const {
        if (end_time.time_since_epoch().count() > 0 && start_time.time_since_epoch().count() > 0) {
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
            return duration.count();
        }
        return std::nullopt;
    }

    // Helper functions to get timestamps in seconds since epoch
    std::optional<double> get_start_time() const {
        if (start_time.time_since_epoch().count() > 0) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                start_time.time_since_epoch()).count();
        }
        return std::nullopt;
    }

    std::optional<double> get_end_time() const {
        if (end_time.time_since_epoch().count() > 0) {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                end_time.time_since_epoch()).count();
        }
        return std::nullopt;
    }

    nlohmann::json to_json() const {
        // Calculate duration from timestamps
        auto total_duration = get_total_duration();
        double total_duration_seconds = total_duration.value_or(0.0);
        
        double requests_per_second = total_duration_seconds > 0 ? total_number_requests / total_duration_seconds : 0.0;
        
        nlohmann::json overall_json = {
            {"total_duration_seconds", total_duration_seconds},
            {"total_prompt_tokens", total_prompt_tokens},
            {"total_completion_tokens", total_completion_tokens},
            {"total_tokens", total_tokens},
            {"total_number_requests", total_number_requests},
            {"total_number_failures", total_number_failures},
            {"requests_per_second", requests_per_second}
        };
        
        // Add timestamp information in seconds since epoch
        auto start_time_seconds = get_start_time();
        if (start_time_seconds.has_value()) {
            overall_json["start_time"] = start_time_seconds.value();
        }
        
        auto end_time_seconds = get_end_time();
        if (end_time_seconds.has_value()) {
            overall_json["end_time"] = end_time_seconds.value();
        }
        
        return overall_json;
    }
};

CompletionStats do_completion(const nlohmann::json& request, const liboai::OpenAI& oai, const std::string& model) {
    CompletionStats stats;
    stats.start_time = std::chrono::steady_clock::now();
    stats.input = request;

    // Buffer to accumulate streaming data chunks
    std::string data_buffer = "";

    liboai::Completions::StreamCallback stream_callback = [&stats, &data_buffer](std::string data, intptr_t userdata) -> bool {
        // Log the raw data received
        data_buffer += data;

        // Process complete lines from the buffer
        size_t pos = 0;
        while ((pos = data_buffer.find('\n')) != std::string::npos) {
            std::string line = data_buffer.substr(0, pos);
            data_buffer.erase(0, pos + 1);

            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \r\n"));
            line.erase(line.find_last_not_of(" \r\n") + 1);

            // Skip empty lines
            if (line.empty()) {
                continue;
            }

            // Handle SSE format - check for data: prefix
            if (line.substr(0, 5) == "data:") {
                std::string json_data = line.substr(5);
                // Trim whitespace after data: prefix
                json_data.erase(0, json_data.find_first_not_of(" "));
                json_data.erase(json_data.find_last_not_of(" ") + 1);

                // Handle [DONE] message
                if (json_data == "[DONE]") {
                    stats.end_time = std::chrono::steady_clock::now();
                    continue;
                }

                // Skip empty JSON data
                if (json_data.empty()) {
                    continue;
                }

                // Try to parse JSON and log any errors
                nlohmann::json chunk;
                try {
                    chunk = nlohmann::json::parse(json_data);
                } catch (const nlohmann::json::parse_error& e) {
                    std::cerr << "[ERROR] JSON parse error: " + std::string(e.what()) << std::endl;
                    std::cerr << "[ERROR] Failed data: '" + json_data + "'" << std::endl;
                    stats.success = false;
                    stats.error_message = e.what();
                    return false; // Stop streaming on parse error
                }

                // Extract content from delta or direct text
                if (chunk.contains("choices") && !chunk["choices"].empty()) {
                    auto& choice = chunk["choices"][0];
                    
                    // Handle streaming format with delta.content
                    if (choice.contains("delta")) {
                        auto& delta = choice["delta"];
                        if (delta.contains("content") && !delta["content"].is_null()) {
                            std::string content = delta["content"];
                            stats.output_text += content;
                        }
                    }
                    // Handle non-streaming format with direct text
                    else if (choice.contains("text") && !choice["text"].is_null()) {
                        std::string content = choice["text"];
                        stats.output_text += content;
                    }
                }

                // Record TTFT (Time To First Token) only if we have received actual content
                if (stats.number_of_chunks == 0 && !stats.output_text.empty()) {
                    stats.ttft_time = std::chrono::steady_clock::now();
                }
                stats.number_of_chunks++;

                // Extract usage information from final chunk
                if (chunk.contains("usage")) {
                    stats.api_usage.prompt_tokens = chunk["usage"].value("prompt_tokens", 0);
                    stats.api_usage.completion_tokens = chunk["usage"].value("completion_tokens", 0);
                    stats.api_usage.total_tokens = chunk["usage"].value("total_tokens", 0);
                }

                // Extract time information from final chunk
                if (chunk.contains("time_info")) {
                    stats.api_time_info.queue_time = chunk["time_info"].value("queue_time", 0.0);
                    stats.api_time_info.prompt_time = chunk["time_info"].value("prompt_time", 0.0);
                    stats.api_time_info.completion_time = chunk["time_info"].value("completion_time", 0.0);
                    stats.api_time_info.total_time = chunk["time_info"].value("total_time", 0.0);
                    stats.api_time_info.created = chunk["time_info"].value("created", 0);
                }
            }
            // Ignore other SSE event types (like event:, id:, retry:, etc.)
        }

        return true;
    };

    try {
        bool is_streaming = request.value("stream", true);

        liboai::Response response = oai.Completion->create(
            model,
            request.contains("prompt") ? std::make_optional(request["prompt"].get<std::string>()) : std::nullopt,
            request.contains("suffix") ? std::make_optional(request["suffix"].get<std::string>()) : std::nullopt,
            request.contains("max_tokens") ? std::make_optional(request["max_tokens"].get<uint16_t>()) : std::nullopt,
            request.contains("temperature") ? std::make_optional(request["temperature"].get<float>()) : std::nullopt,
            request.contains("top_p") ? std::make_optional(request["top_p"].get<float>()) : std::nullopt,
            request.contains("n") ? std::make_optional(request["n"].get<uint16_t>()) : std::nullopt,
            is_streaming ? std::make_optional(stream_callback) : std::nullopt,
            request.contains("logprobs") ? std::make_optional(request["logprobs"].get<uint8_t>()) : std::nullopt,
            request.contains("echo") ? std::make_optional(request["echo"].get<bool>()) : std::nullopt,
            request.contains("stop") ? std::make_optional(request["stop"].get<std::vector<std::string>>()) : std::nullopt,
            request.contains("presence_penalty") ? std::make_optional(request["presence_penalty"].get<float>()) : std::nullopt,
            request.contains("frequency_penalty") ? std::make_optional(request["frequency_penalty"].get<float>()) : std::nullopt,
            request.contains("best_of") ? std::make_optional(request["best_of"].get<uint16_t>()) : std::nullopt,
            request.contains("logit_bias") ? std::make_optional(request["logit_bias"].get<std::unordered_map<std::string, int8_t>>()) : std::nullopt,
            request.contains("user") ? std::make_optional(request["user"].get<std::string>()) : std::nullopt
        );
        stats.end_time = std::chrono::steady_clock::now();

        if (!is_streaming) {
            // Extract content from choices.text for non-streaming responses
            if (response.raw_json.contains("choices") && !response.raw_json["choices"].empty()) {
                auto& choice = response.raw_json["choices"][0];
                if (choice.contains("text") && !choice["text"].is_null()) {
                    stats.output_text = choice["text"].get<std::string>();
                }
            } else {
                // Fallback to response.content if no choices structure
                stats.output_text = response.content;
            }
            
            // Record TTFT only if we have actual content
            if (!stats.output_text.empty()) {
                stats.ttft_time = stats.end_time;
            }
            
            if (response.raw_json.contains("usage")) {
                stats.api_usage.prompt_tokens = response.raw_json["usage"].value("prompt_tokens", 0);
                stats.api_usage.completion_tokens = response.raw_json["usage"].value("completion_tokens", 0);
                stats.api_usage.total_tokens = response.raw_json["usage"].value("total_tokens", 0);
            }
            if (response.raw_json.contains("time_info")) {
                stats.api_time_info.queue_time = response.raw_json["time_info"].value("queue_time", 0.0);
                stats.api_time_info.prompt_time = response.raw_json["time_info"].value("prompt_time", 0.0);
                stats.api_time_info.completion_time = response.raw_json["time_info"].value("completion_time", 0.0);
                stats.api_time_info.total_time = response.raw_json["time_info"].value("total_time", 0.0);
                stats.api_time_info.created = response.raw_json["time_info"].value("created", 0);
            }
        }
    } catch (const std::exception& e) {
        stats.success = false;
        stats.error_message = e.what();
        stats.end_time = std::chrono::steady_clock::now();
        return stats;
    }
    return stats;
}
   
using Stats = std::pair<OverallStats, std::vector<CompletionStats>>;
Stats do_completions(const std::vector<nlohmann::json>& requests, int concurrent_requests, liboai::OpenAI& oai, const std::string& model) {
    OverallStats stats;
    std::vector<CompletionStats> all_completion_stats(requests.size());

    stats.start_time = std::chrono::steady_clock::now();

    // Create a thread pool-like approach using futures
    std::atomic<size_t> next_request_index{0};

    auto worker = [&]() {
        while (true) {
            size_t index = next_request_index.fetch_add(1);
            if (index >= requests.size()) {
                break;
            }
            all_completion_stats[index] = do_completion(requests[index], oai, model);
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < concurrent_requests; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& thread : threads) {
        thread.join();
    }

    stats.end_time = std::chrono::steady_clock::now();
    stats.total_number_requests = requests.size();

    for (const auto& completion_stats : all_completion_stats) {
        stats.total_prompt_tokens += completion_stats.api_usage.prompt_tokens;
        stats.total_completion_tokens += completion_stats.api_usage.completion_tokens;
        stats.total_tokens += completion_stats.api_usage.total_tokens;
        if (!completion_stats.success) {
            stats.total_number_failures++;
        }
    }

    return std::make_pair(stats, all_completion_stats);
}

void dump_stats_to_file(const Stats stats, const std::string& filename) {
    nlohmann::json output_json;
    
    // Add overall stats using the to_json method
    output_json["overall_stats"] = stats.first.to_json();
    
    // Add individual completion stats using the to_json method
    nlohmann::json completions_array = nlohmann::json::array();
    for (const auto& completion_stats : stats.second) {
        completions_array.push_back(completion_stats.to_json());
    }
    
    output_json["completions"] = completions_array;
    
    // Write to file
    std::ofstream output_file(filename);
    if (output_file.is_open()) {
        output_file << output_json.dump(4);
        output_file.close();
        std::cout << "[INFO] Statistics written to " + filename << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to open output file: " + filename << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    const auto config = parse_arguments(argc, argv);
    
    // Load requests from JSONL file
    const auto requests = load_requests_from_jsonl(config.input_file);
    if (requests.empty()) {
        std::cerr << "[ERROR] No valid requests found in input file" << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize liboai with the provided API key and endpoint
    liboai::OpenAI oai(config.api_endpoint);
    if (!oai.auth.SetKey(config.api_key)) {
        std::cerr << "[ERROR] Failed to set API key." << std::endl;
        return EXIT_FAILURE;
    }

    const auto stats = do_completions(requests, config.concurrent_requests, oai, config.model);

    // Dump stats to output file
    dump_stats_to_file(stats, config.output_file);
    
    std::cout << "[INFO] Done!" << std::endl;
    return EXIT_SUCCESS;
}
