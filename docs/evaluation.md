# Evaluation

## Benchmark repository

Benchmark automation is available under `benchmarks/` for:

- benchmark download/setup,
- repeated runs,
- and post-processing of logs into report outputs.

## Suggested evaluation sections

1. Experimental setup (hardware, software versions, run counts).
2. Research questions (overhead, detection quality, workload sensitivity).
3. Results (runtime impact and discovered patterns).
4. Validity considerations (measurement noise, benchmark representativeness).

## Reproduction commands

```bash
. env.sh
cd benchmarks
./download.sh
./run.sh <times-to-run>
./process.sh
```
