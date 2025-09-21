# Cerebras REST Client

A C++ client for making inference calls to the Cerebras API endpoint with support for both streaming and non-streaming modes.

## Features

- ✅ **Non-streaming mode**: Complete JSON response at once
- ✅ **Streaming mode**: Real-time response as it's generated
- ✅ **No external dependencies**: Uses only Boost and nlohmann/json
- ✅ **Command-line interface**: Easy to use with various parameters
- ✅ **SSL/TLS support**: Secure HTTPS communication

## Prerequisites

### Required Libraries

1. **Boost libraries** (version 1.70+)
2. **nlohmann/json** (JSON library)
3. **OpenSSL** (for SSL/TLS support)
4. **liboai** (OpenAI API library - included as submodule)

### Installation

#### macOS (using Homebrew)
```bash
brew install boost nlohmann-json openssl
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libboost-all-dev nlohmann-json3-dev libssl-dev
```

### Submodule Initialization

If cloning this repository, initialize the liboai submodule:

```bash
git submodule update --init --recursive
```

## Building

### Method 1: Using CMake (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# The binary will be created at: build/bin/rest_client
```

### Method 2: Direct Compilation

```bash
g++ -std=c++20 -o rest_client rest_client.cpp \
    -lssl -lcrypto \
    -lnlohmann_json::nlohmann_json \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib
```

## Usage

### Basic Usage

```bash
./bin/rest_client --api_key=YOUR_API_KEY --prompt="Hello, world!"
```

### All Parameters

```bash
./bin/rest_client \
  --api_key="your-cerebras-api-key" \
  --model="llama-3.3-70b" \
  --prompt="What is artificial intelligence?" \
  --max_tokens=100 \
  --stream
```

### Parameter Descriptions

- `--api_key`: (Required) Your Cerebras API key
- `--model`: (Optional) Model name, defaults to "llama-3.3-70b"
- `--prompt`: (Optional) Input prompt, defaults to "Hello, world!"
- `--max_tokens`: (Optional) Maximum tokens to generate, defaults to 100
- `--stream`, `--streaming`: (Optional) Enable streaming mode for real-time response
- `--help`, `-h`: Show help message

## Examples

### Non-Streaming Mode (Default)

```bash
# Basic usage
./bin/rest_client --api_key=YOUR_API_KEY --prompt="What is AI?"

# With custom parameters
./bin/rest_client \
  --api_key=YOUR_API_KEY \
  --model="llama-3.3-70b" \
  --prompt="Explain quantum computing" \
  --max_tokens=200
```

**Output:**
```
[INFO] Response: {"id":"chatcmpl-123","object":"chat.completion","created":1234567890,"model":"llama-3.3-70b","choices":[{"index":0,"message":{"role":"assistant","content":"Quantum computing is a type of computation..."},"finish_reason":"stop"}],"usage":{"prompt_tokens":5,"completion_tokens":50,"total_tokens":55}}
[INFO] Coroutine finished successfully.
[INFO] Done!
```

### Streaming Mode

```bash
# Enable streaming
./bin/rest_client --api_key=YOUR_API_KEY --prompt="What is AI?" --stream

# With custom parameters
./bin/rest_client \
  --api_key=YOUR_API_KEY \
  --model="llama-3.3-70b" \
  --prompt="Explain machine learning" \
  --max_tokens=150 \
  --stream
```

**Output:**
```
[INFO] Streaming response:

--- Streaming Response ---
**Machine Learning** is a subset of artificial intelligence that focuses on algorithms and statistical models that enable computers to learn and make decisions from data without being explicitly programmed for every task.

Key concepts include:
* **Supervised Learning**: Learning from labeled examples
* **Unsupervised Learning**: Finding patterns in unlabeled data
* **Reinforcement Learning**: Learning through trial and error

--- End of Stream ---
[INFO] Coroutine finished successfully.
[INFO] Done!
```

## Expected Output

### Non-Streaming Mode
- Complete JSON response from the Cerebras API
- Usage statistics (tokens used)
- Connection status and any errors

### Streaming Mode
- Real-time text output as it's generated
- Visual indicators for stream start/end
- Same error handling as non-streaming mode

## Troubleshooting

### Common Issues

1. **SSL/TLS errors**: Ensure your system has proper SSL certificates
2. **Connection refused**: Check your internet connection and API endpoint
3. **Authentication errors**: Verify your API key is correct
4. **Compilation errors**: Ensure all dependencies are properly installed

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
