# Cerebras Throughput Test Client

A C++ client for testing throughput performance when making inference calls to the Cerebras API endpoint. This tool supports concurrent requests and detailed performance statistics collection.

## Features

- ✅ **Non-streaming mode**: Complete JSON response at once
- ✅ **Streaming mode**: Real-time response as it's generated
- ✅ **Simplified dependencies**: Uses only Boost and liboai
- ✅ **Command-line interface**: Easy to use with various parameters
- ✅ **SSL/TLS support**: Secure HTTPS communication

## Prerequisites

### Required Libraries

1. **Boost libraries** (version 1.70+)
2. **liboai** (OpenAI API library - included as submodule)

### Installation

#### macOS (using Homebrew)
```bash
brew install boost
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libboost-all-dev
```

### Submodule Initialization

If cloning this repository, initialize the liboai submodule:

```bash
git submodule update --init --recursive
```

## Building

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# The binary will be created at: build/bin/throughput_test
```

## Usage

### Throughput Test Usage

```bash
./bin/throughput_test \
  --api_key="your-cerebras-api-key" \
  --input_file="requests.jsonl" \
  --concurrent_requests=5 \
  --output_file="results.json"
```

### Throughput Test Parameters

- `--api_key`: (Required) Your Cerebras API key
- `--api_endpoint`: (Optional) API endpoint URL, defaults to "https://api.cerebras.ai/v1"
- `--model`: (Optional) Model name to use, defaults to "llama-3.3-70b"
- `--input_file`: (Required) Path to JSONL file containing completion requests
- `--concurrent_requests`: (Optional) Number of concurrent threads, defaults to 10
- `--output_file`: (Optional) Path to output JSON stats file, defaults to "throughput_stats.json"
- `--help`, `-h`: Show help message

### JSONL File Format

The input file should contain one JSON object per line with the following structure:

```json
{"prompt": "Your prompt here", "max_tokens": 100, "temperature": 0.7}
{"prompt": "Another prompt", "max_tokens": 150, "temperature": 0.5}
```

Each JSON object can contain:
- `prompt`: (Required) The input prompt string
- `max_tokens`: (Optional) Maximum tokens to generate
- `temperature`: (Optional) Sampling temperature for the model
- `stream`: (Optional) Enable streaming mode for real-time response (defaults to true)

## Examples

### Basic Throughput Test

```bash
# Run throughput test with sample requests
./bin/throughput_test \
  --api_key=YOUR_API_KEY \
  --input_file=datasets/sample_requests.jsonl \
  --concurrent_requests=5 \
  --output_file=results.json
```

### Custom Parameters

```bash
# Run with custom endpoint and model
./bin/throughput_test \
  --api_key=YOUR_API_KEY \
  --api_endpoint="https://api.cerebras.ai/v1" \
  --model="llama-3.3-70b" \
  --input_file="my_requests.jsonl" \
  --concurrent_requests=20 \
  --output_file="performance_results.json"
```

**Sample Output:**
```json
{
  "overall_stats": {
    "total_duration_seconds": 15.234,
    "total_prompt_tokens": 1250,
    "total_completion_tokens": 25000,
    "total_tokens": 26250,
    "total_number_requests": 11,
    "total_number_failures": 0,
    "requests_per_second": 0.722,
    "start_time": 1758574038.123456,
    "end_time": 1758574053.357456
  },
  "completions": [
    {
      "input": {
        "prompt": "Summarize Harry Potter Series. I have 10 minutes to read it.",
        "max_tokens": 5000,
        "temperature": 0.5
      },
      "output_text": "Harry Potter is a series of seven fantasy novels written by J.K. Rowling...",
      "success": true,
      "error_message": "",
      "total_duration_seconds": 2.345,
      "ttft_duration_seconds": 0.123,
      "number_of_chunks": 45,
      "start_time": 1758574038.123456,
      "ttft_time": 1758574038.246456,
      "end_time": 1758574040.468456,
      "api_usage": {
        "prompt_tokens": 120,
        "completion_tokens": 2500,
        "total_tokens": 2620
      },
      "api_time_info": {
        "queue_time": 0.001,
        "prompt_time": 0.050,
        "completion_time": 2.294,
        "total_time": 2.345,
        "created": 1758574038
      }
    }
  ]
}
```

## Expected Output

The throughput test generates detailed performance statistics including:
- Overall test duration and requests per second
- Token usage statistics (prompt, completion, and total tokens)
- Individual request timing (total duration, time to first token)
- Success/failure counts and error messages
- API timing information (queue time, prompt processing time, completion time)

## Troubleshooting

### Common Issues

1. **SSL/TLS errors**: Ensure your system has proper SSL certificates
2. **Connection refused**: Check your internet connection and API endpoint
3. **Authentication errors**: Verify your API key is correct
4. **Compilation errors**: Ensure all dependencies are properly installed
5. **No valid requests found**: Check that your input JSONL file contains valid JSON objects

### Build Issues

If you encounter compilation errors:

1. **Missing Boost libraries**:
   ```bash
   brew install boost  # macOS
   sudo apt-get install libboost-all-dev  # Ubuntu
   ```

2. **Missing nlohmann/json**:
   ```bash
   brew install nlohmann-json  # macOS
   sudo apt-get install nlohmann-json3-dev  # Ubuntu
   ```

3. **Missing OpenSSL**:
   ```bash
   brew install openssl  # macOS
   sudo apt-get install libssl-dev  # Ubuntu
   ```

## API Reference

For more information about the Cerebras API:
- [Cerebras Developer Documentation](https://docs.cerebras.ai/)
- [API Reference](https://docs.cerebras.ai/api-reference/)

## License

This project follows the same license as the parent repository.
