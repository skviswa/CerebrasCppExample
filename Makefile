# Makefile for Cerebras REST Client
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic
INCLUDES = -I/opt/homebrew/include -I/usr/local/include
LIBS = -L/opt/homebrew/lib -lssl -lcrypto -lboost_program_options

# Target executable
TARGET = rest_client
SOURCE = rest_client.cpp

# Default target
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCE) $(LIBS)

# Clean target
clean:
	rm -f $(TARGET)

# Install dependencies (macOS with Homebrew)
install-deps:
	brew install boost abseil nlohmann-json openssl

# Test target
test: $(TARGET)
	./$(TARGET) --api_key=test --prompt="Hello, world!"

.PHONY: all clean install-deps test
