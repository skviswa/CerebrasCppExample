#!/usr/bin/env python3
"""
Script to analyze benchmark results JSON and calculate percentiles for performance metrics.

This script parses the benchmark results JSON file and calculates percentiles (P50, P90, P95, P99, P100)
for various performance metrics from api_time_info and api_usage blocks.

Usage:
    python3 analyze_benchmark_results.py input_file.json [--output-csv output.csv]
"""

import json
import csv
import sys
import argparse
import numpy as np
from pathlib import Path
from typing import Dict, List, Any


def calculate_percentiles(data: List[float], percentiles: List[float] = [50, 90, 95, 99, 100]) -> Dict[str, float]:
    """
    Calculate percentiles for a given dataset.
    
    Args:
        data: List of numeric values
        percentiles: List of percentile values to calculate
        
    Returns:
        Dictionary mapping percentile names to values
    """
    if not data:
        return {f"P{p}": 0.0 for p in percentiles}
    
    data_array = np.array(data)
    results = {}
    
    for p in percentiles:
        value = np.percentile(data_array, p)
        results[f"P{p}"] = round(value, 6)
    
    return results


def extract_metrics_from_completion(completion: Dict[str, Any]) -> Dict[str, float]:
    """
    Extract metrics from a single completion entry.
    
    Args:
        completion: Single completion entry from the JSON
        
    Returns:
        Dictionary containing extracted metrics
    """
    metrics = {}
    
    # Extract api_time_info metrics
    if "api_time_info" in completion:
        time_info = completion["api_time_info"]
        metrics["queue_time"] = time_info.get("queue_time", 0.0)
        metrics["prompt_time"] = time_info.get("prompt_time", 0.0)
        metrics["completion_time"] = time_info.get("completion_time", 0.0)
        metrics["total_time"] = time_info.get("total_time", 0.0)
        
        # Calculate queue_time + prompt_time
        metrics["queue_plus_prompt_time"] = metrics["queue_time"] + metrics["prompt_time"]
    
    # Extract api_usage metrics
    if "api_usage" in completion:
        usage_info = completion["api_usage"]
        metrics["prompt_tokens"] = float(usage_info.get("prompt_tokens", 0))
        metrics["completion_tokens"] = float(usage_info.get("completion_tokens", 0))
        metrics["total_tokens"] = float(usage_info.get("total_tokens", 0))
    
    return metrics


def analyze_benchmark_results(input_file: str, output_csv: str = None) -> Dict[str, Any]:
    """
    Analyze benchmark results and calculate percentiles.
    
    Args:
        input_file: Path to the benchmark results JSON file
        output_csv: Optional path to save results as CSV
        
    Returns:
        Dictionary containing analysis results
    """
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in file '{input_file}': {e}")
        sys.exit(1)
    
    # Extract completions
    completions = data.get("completions", [])
    if not completions:
        print("Error: No completions found in the JSON file")
        sys.exit(1)
    
    print(f"Analyzing {len(completions)} completion entries...")
    
    # Extract metrics from all completions
    all_metrics = {}
    metric_names = [
        "queue_time", "prompt_time", "completion_time", "total_time", 
        "queue_plus_prompt_time", "prompt_tokens", "completion_tokens", "total_tokens"
    ]
    
    # Initialize metric lists
    for metric in metric_names:
        all_metrics[metric] = []
    
    # Extract metrics from each completion
    for completion in completions:
        metrics = extract_metrics_from_completion(completion)
        for metric in metric_names:
            if metric in metrics:
                all_metrics[metric].append(metrics[metric])
    
    # Calculate percentiles for each metric
    results = {}
    percentiles = [50, 90, 95, 99, 100]
    
    for metric, values in all_metrics.items():
        if values:
            results[metric] = calculate_percentiles(values, percentiles)
        else:
            results[metric] = {f"P{p}": 0.0 for p in percentiles}
    
    # Print results
    print("\n" + "="*80)
    print("BENCHMARK RESULTS ANALYSIS")
    print("="*80)
    
    for metric, percentiles_data in results.items():
        print(f"\n{metric.upper().replace('_', ' ')}:")
        print("-" * 40)
        for p in percentiles:
            print(f"  P{p:2d}: {percentiles_data[f'P{p}']:>12.6f}")
    
    # Print summary statistics
    print("\n" + "="*80)
    print("SUMMARY STATISTICS")
    print("="*80)
    
    if "overall_stats" in data:
        overall = data["overall_stats"]
        print(f"Total Requests: {overall.get('total_number_requests', 'N/A')}")
        print(f"Total Failures: {overall.get('total_number_failures', 'N/A')}")
        print(f"Total Duration: {overall.get('total_duration_seconds', 'N/A'):.2f} seconds")
        print(f"Requests/Second: {overall.get('requests_per_second', 'N/A'):.2f}")
        print(f"Total Prompt Tokens: {overall.get('total_prompt_tokens', 'N/A'):,}")
        print(f"Total Completion Tokens: {overall.get('total_completion_tokens', 'N/A'):,}")
        print(f"Total Tokens: {overall.get('total_tokens', 'N/A'):,}")
    
    # Save to CSV if requested
    if output_csv:
        save_to_csv(results, output_csv, percentiles)
        print(f"\nResults saved to: {output_csv}")
    
    return results


def save_to_csv(results: Dict[str, Any], output_file: str, percentiles: List[int]) -> None:
    """
    Save results to CSV file.
    
    Args:
        results: Dictionary containing percentile results
        output_file: Path to output CSV file
        percentiles: List of percentile values
    """
    try:
        with open(output_file, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            
            # Write header
            header = ['Metric'] + [f'P{p}' for p in percentiles]
            writer.writerow(header)
            
            # Write data
            for metric, percentiles_data in results.items():
                row = [metric] + [percentiles_data[f'P{p}'] for p in percentiles]
                writer.writerow(row)
                
    except Exception as e:
        print(f"Error saving CSV file: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze benchmark results and calculate percentiles",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 analyze_benchmark_results.py benchmark_1k_results.json
  python3 analyze_benchmark_results.py benchmark_1k_results.json --output-csv results_analysis.csv
        """
    )
    
    parser.add_argument('input_file', help='Path to benchmark results JSON file')
    parser.add_argument('--output-csv', help='Path to save results as CSV file')
    
    args = parser.parse_args()
    
    # Validate input file exists
    if not Path(args.input_file).exists():
        print(f"Error: Input file '{args.input_file}' does not exist")
        sys.exit(1)
    
    # Analyze the results
    analyze_benchmark_results(args.input_file, args.output_csv)


if __name__ == "__main__":
    main()
