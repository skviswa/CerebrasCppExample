# Benchmark File Conversion

This directory contains scripts to convert benchmark JSONL files from HuggingFace format to the format required by the C++ benchmark binary.

## File Format Conversion

The conversion transforms the following fields:

| Original Field | Converted Field | Description |
|----------------|-----------------|-------------|
| `text` | `prompt` | The input text for the model |
| `token_length` | `max_tokens` | Maximum number of tokens to generate |
| *(missing)* | `temperature` | Random value between 0.2 and 0.7 |

## Scripts

### `convert_benchmark.py`
Converts a single benchmark JSONL file.

**Usage:**
```bash
python3 convert_benchmark.py input_file.jsonl output_file.jsonl [--seed SEED]
```

**Examples:**
```bash
# Convert a single file
python3 convert_benchmark.py datasets/benchmark_1K.jsonl datasets/benchmark_1K_converted.jsonl

# Convert with specific seed for reproducible results
python3 convert_benchmark.py datasets/benchmark_1K.jsonl datasets/benchmark_1K_converted.jsonl --seed 123
```

### `convert_all_benchmarks.py`
Converts all benchmark files at once.

**Usage:**
```bash
python3 convert_all_benchmarks.py [--seed SEED]
```

**Examples:**
```bash
# Convert all files with default seed (42)
python3 convert_all_benchmarks.py

# Convert all files with custom seed
python3 convert_all_benchmarks.py --seed 123
```

## Input Files

The scripts expect these benchmark files in the `datasets/` directory:
- `benchmark_1K.jsonl` (100 entries)
- `benchmark_8K.jsonl` (100 entries) 
- `benchmark_16K.jsonl` (100 entries)

## Output Files

The converted files will be created as:
- `datasets/benchmark_1K_converted.jsonl`
- `datasets/benchmark_8K_converted.jsonl`
- `datasets/benchmark_16K_converted.jsonl`

## Running the C++ Benchmark

After conversion, you can run the C++ benchmark with the converted files:

```bash
./build/bin/benchmark \
  --api_key="your-cerebras-api-key" \
  --input_file="datasets/benchmark_1K_converted.jsonl" \
  --concurrent_requests=5 \
  --output_file="benchmark_1K_results.json"
```

## Notes

- The `temperature` values are randomly generated between 0.2 and 0.7
- Using the same `--seed` parameter ensures reproducible temperature values
- The scripts handle JSON parsing errors gracefully and report warnings
- All converted files maintain the same number of entries as the original files
