# Reproducibility

## Recommended reporting metadata

When publishing results, include:

- repository URL and commit hash,
- OS/kernel/toolchain versions,
- JVM versions and flags,
- benchmark run count,
- and post-processing command sequence.

## Minimal reproducibility script path

```bash
# build tool
cd cinst
export JAVA_HOME=<JAVA_HOME_FOR_JDK8>
./build.sh
source env.sh

# run benchmarks
cd ../
. env.sh
cd benchmarks
./download.sh
./run.sh <times-to-run>
./process.sh
```

## Determinism notes

Absolute determinism is not required for systems artifacts, but variance should be reported and controlled via:

- multiple runs,
- fixed hardware configuration,
- and stable software environments.
