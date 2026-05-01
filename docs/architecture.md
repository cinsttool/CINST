# Architecture

## High-level components

CINST includes:

- runtime instrumentation components (agent + preload/native),
- trace generation for each instrumented process (`data-<pid>`),
- analysis pipeline scripts for pattern queries and reporting.

## End-to-end workflow

1. Build CINST and load environment variables.
2. Start the CINST server.
3. Run the target Java application with instrumentation.
4. Collect generated trace directory (`data-<pid>`).
5. Build analysis data artifacts.
6. Run query scripts for targeted behavior patterns.
