#!/usr/bin/env python3
"""
Script to convert benchmark JSONL files to the format required by the C++ benchmark binary.

Converts:
- "text" -> "prompt"
- "token_length" -> "max_tokens" 
- Adds random "temperature" between 0.2 and 0.7

Usage:
    python convert_benchmark.py input_file.jsonl output_file.jsonl
"""

import json
import random
import sys
import argparse
from pathlib import Path


def convert_benchmark_file(input_file: str, output_file: str) -> None:
    """
    Convert a benchmark JSONL file to the format required by the C++ benchmark.
    
    Args:
        input_file: Path to input JSONL file
        output_file: Path to output JSONL file
    """
    converted_count = 0
    
    try:
        with open(input_file, 'r', encoding='utf-8') as infile, \
             open(output_file, 'w', encoding='utf-8') as outfile:
            
            for line_num, line in enumerate(infile, 1):
                line = line.strip()
                if not line:
                    continue
                    
                try:
                    # Parse the JSON line
                    data = json.loads(line)
                    
                    # Convert the format
                    converted_data = {
                        "prompt": data["text"],
                        "max_tokens": int(data["token_length"]),
                        "temperature": round(random.uniform(0.2, 0.7), 1)
                    }
                    
                    # Write the converted line
                    outfile.write(json.dumps(converted_data) + '\n')
                    converted_count += 1
                    
                except json.JSONDecodeError as e:
                    print(f"Warning: Skipping invalid JSON on line {line_num}: {e}")
                    continue
                except KeyError as e:
                    print(f"Warning: Missing required field {e} on line {line_num}")
                    continue
                except Exception as e:
                    print(f"Warning: Error processing line {line_num}: {e}")
                    continue
    
    except FileNotFoundError:
        print(f"Error: Input file '{input_file}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    
    print(f"Successfully converted {converted_count} entries from '{input_file}' to '{output_file}'")


def main():
    parser = argparse.ArgumentParser(
        description="Convert benchmark JSONL files to C++ benchmark format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python convert_benchmark.py datasets/benchmark_1K.jsonl datasets/benchmark_1K_converted.jsonl
  python convert_benchmark.py datasets/benchmark_8K.jsonl datasets/benchmark_8K_converted.jsonl
  python convert_benchmark.py datasets/benchmark_16K.jsonl datasets/benchmark_16K_converted.jsonl
        """
    )
    
    parser.add_argument('input_file', help='Input JSONL file to convert')
    parser.add_argument('output_file', help='Output JSONL file path')
    parser.add_argument('--seed', type=int, default=None, 
                       help='Random seed for reproducible temperature values')
    
    args = parser.parse_args()
    
    # Set random seed if provided
    if args.seed is not None:
        random.seed(args.seed)
        print(f"Using random seed: {args.seed}")
    
    # Validate input file exists
    if not Path(args.input_file).exists():
        print(f"Error: Input file '{args.input_file}' does not exist")
        sys.exit(1)
    
    # Create output directory if it doesn't exist
    output_path = Path(args.output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Convert the file
    convert_benchmark_file(args.input_file, args.output_file)


if __name__ == "__main__":
    main()
