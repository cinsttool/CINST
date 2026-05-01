# Installation

## Dependencies

- CMake >= 3.0
- GCC >= 9.0
- JDK 8
- (for overhead evaluation) JDK 11, Python 3 with scientific stack

## Build steps

```bash
cd cinst
export JAVA_HOME=<JAVA_HOME_FOR_JDK8>
./build.sh
source env.sh
```

## Runtime prerequisites

- Ensure `container_methods.txt` is available in the execution directory.
- Start the CINST server before running target applications.
