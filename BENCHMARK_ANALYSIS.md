# Benchmark Results Analysis

This directory contains scripts to analyze benchmark results and calculate performance percentiles.

## Scripts

### `analyze_benchmark_results.py`

Analyzes benchmark results JSON files and calculates percentiles for performance metrics.

**Features:**
- Calculates P50, P90, P95, P99, and P100 percentiles for all metrics
- Extracts metrics from `api_time_info` and `api_usage` blocks
- Outputs results to console and optionally saves to CSV
- Handles large JSON files efficiently

**Metrics Analyzed:**
- **Time Metrics (seconds):**
  - `queue_time`: Time spent in queue
  - `prompt_time`: Time to process the prompt
  - `completion_time`: Time to generate completion
  - `total_time`: Total API time
  - `queue_plus_prompt_time`: Combined queue and prompt time

- **Token Metrics:**
  - `prompt_tokens`: Number of tokens in the prompt
  - `completion_tokens`: Number of tokens in the completion
  - `total_tokens`: Total tokens used

## Usage

### Basic Analysis
```bash
python3 analyze_benchmark_results.py benchmark_1k_results.json
```

### Analysis with CSV Output
```bash
python3 analyze_benchmark_results.py benchmark_1k_results.json --output-csv analysis_results.csv
```

## Output Format

### Console Output
The script provides:
1. **Detailed Percentile Analysis**: P50, P90, P95, P99, P100 for each metric
2. **Summary Statistics**: Total requests, failures, duration, throughput, and token usage

### CSV Output
The CSV file contains a table with:
- **Metric**: Name of the performance metric
- **P50, P90, P95, P99, P100**: Percentile values for each metric

## Example Output

This is for 100 overall requests in `datasets/benchmark_1K_converted_2.jsonl` with 5 concurrent requests.

```
================================================================================
BENCHMARK RESULTS ANALYSIS
================================================================================

QUEUE TIME:
----------------------------------------
  P50:     0.308756
  P90:     0.939962
  P95:     1.406565
  P99:     2.034422
  P100:     2.112814

PROMPT TIME:
----------------------------------------
  P50:     0.217224
  P90:     0.275567
  P95:     0.293915
  P99:     0.324401
  P100:     0.330093

COMPLETION TIME:
----------------------------------------
  P50:     0.235411
  P90:     0.414430
  P95:     0.450377
  P99:     0.556289
  P100:     0.598075

TOTAL TIME:
----------------------------------------
  P50:     0.847628
  P90:     1.493491
  P95:     1.937206
  P99:     2.663364
  P100:     2.762684

QUEUE PLUS PROMPT TIME:
----------------------------------------
  P50:     0.521345
  P90:     1.195415
  P95:     1.593250
  P99:     2.363943
  P100:     2.385737

PROMPT TOKENS:
----------------------------------------
  P50:  8002.000000
  P90:  9053.900000
  P95:  9273.250000
  P99:  9633.010000
  P100:  9832.000000

COMPLETION TOKENS:
----------------------------------------
  P50:   803.000000
  P90:   905.700000
  P95:   927.100000
  P99:   963.200000
  P100:   983.000000

TOTAL TOKENS:
----------------------------------------
  P50:  8805.000000
  P90:  9959.600000
  P95: 10202.150000
  P99: 10596.210000
  P100: 10815.000000

================================================================================
SUMMARY STATISTICS
================================================================================
Total Requests: 100
Total Failures: 0
Total Duration: 26.37 seconds
Requests/Second: 3.79
Total Prompt Tokens: 797,394
Total Completion Tokens: 79,975
Total Tokens: 877,369

Results saved to: analysis_results.csv
```

## Dependencies

- Python 3.6+
- numpy (for percentile calculations)

## Error Handling

The script handles:
- Missing or invalid JSON files
- Missing metric fields in completion entries
- Empty or malformed data gracefully

## Performance

The script is optimized for large JSON files and processes data efficiently using numpy for statistical calculations.
