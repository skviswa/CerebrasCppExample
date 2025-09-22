#!/usr/bin/env python3
"""
Script to convert all benchmark JSONL files to the format required by the C++ benchmark binary.

This script converts all benchmark files in the datasets directory:
- benchmark_1K.jsonl -> benchmark_1K_converted.jsonl
- benchmark_8K.jsonl -> benchmark_8K_converted.jsonl  
- benchmark_16K.jsonl -> benchmark_16K_converted.jsonl

Usage:
    python3 convert_all_benchmarks.py [--seed SEED]
"""

import subprocess
import sys
import argparse
from pathlib import Path


def convert_all_benchmarks(seed: int = 42) -> None:
    """
    Convert all benchmark JSONL files to the required format.
    
    Args:
        seed: Random seed for reproducible temperature values
    """
    datasets_dir = Path("datasets")
    
    # Define the benchmark files to convert
    benchmark_files = [
        "benchmark_1K.jsonl",
        "benchmark_8K.jsonl", 
        "benchmark_16K.jsonl"
    ]
    
    converted_count = 0
    failed_count = 0
    
    for benchmark_file in benchmark_files:
        input_path = datasets_dir / benchmark_file
        output_path = datasets_dir / f"{benchmark_file.replace('.jsonl', '_converted.jsonl')}"
        
        if not input_path.exists():
            print(f"Warning: Input file '{input_path}' not found, skipping...")
            failed_count += 1
            continue
        
        try:
            print(f"Converting {input_path} -> {output_path}...")
            
            # Run the conversion script
            result = subprocess.run([
                sys.executable, "convert_benchmark.py", 
                str(input_path), str(output_path), 
                "--seed", str(seed)
            ], capture_output=True, text=True, check=True)
            
            print(f"✓ {result.stdout.strip()}")
            converted_count += 1
            
        except subprocess.CalledProcessError as e:
            print(f"✗ Failed to convert {input_path}: {e.stderr}")
            failed_count += 1
        except Exception as e:
            print(f"✗ Error converting {input_path}: {e}")
            failed_count += 1
    
    print(f"\nConversion Summary:")
    print(f"  Successfully converted: {converted_count} files")
    print(f"  Failed: {failed_count} files")
    
    if failed_count > 0:
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Convert all benchmark JSONL files to C++ benchmark format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 convert_all_benchmarks.py
  python3 convert_all_benchmarks.py --seed 123
        """
    )
    
    parser.add_argument('--seed', type=int, default=42,
                       help='Random seed for reproducible temperature values (default: 42)')
    
    args = parser.parse_args()
    
    print(f"Converting all benchmark files with seed: {args.seed}")
    print("=" * 50)
    
    convert_all_benchmarks(args.seed)


if __name__ == "__main__":
    main()
