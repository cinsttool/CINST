# CINST Artifact

## Build and Use of CINST

### Dependencies

To build CINST, you need the following dependencies installed:

- CMake >= 3.0
- GCC >= 9.0
- JDK 8

### Build & Install

```
cd cinst
export JAVA_HOME=<JAVA_HOME_FOR_JDK8>
./build.sh
```

Load the environment to use the installed CINST:

```
cd cinst
# load the environment variables
source env.sh
```

### How to use CINST

To use CINST, you need to complete each step of the **Build & Install**
Usage:

```
cd cinst
./run_server.sh

LD_PRELOAD=/path/to/cinst/install/lib/libpreload.so LD_LIBRARY_PATH=/path/to/cinst/install/lib/ java -agentpath:/path/to/cinst/install/lib/libagent.so -javaagent:/path/to/cinst/agent-jar-with-dependencies.jar[="<args>"] [-jar] <JAVA_APP>
```

A file `container_methods.txt` that contains the containers and methods you want to trace. Format is as followed.

```
java/util/List,add(Ljava/lang/Object;)Z
java/util/List,addAll(Ljava/util/Collection;)Z
```

We provide a representative file in this repo, please put this file in the execution directory.

#### Bootstrap mode

Only bootstrap mode can trace some java standard library. 

Add java argument `-Xbootclasspath/a:/path/to/CINST/agent-jar-with-dependencies.jar` and you can enable bootstrap mode.

After running of CINST, a directory named `data-<pid>` is generated.

### Analysis Scripts

Before analysis, trace needs to be processed. Analysis related scripts can be found at `cinst/scripts`.

Take Never-get Container pattern as example:

Usage:
```
cd data-<pid>
bash build_container.sh
python3 NGC_query.py
```

## Overhead Evaluation

It may take long time to run overhead evaluation.

### Requirements

- Jdk 8
- Jdk 11
- python3 
  - numpy
  - scipy
  - matplotlib



### Preparation of Benchmarks

```
cd <root-of-project>
. env.sh
cd benchmarks
./download.sh
```

### Run of Benchmarks

```
export JAVA8_HOME=<JAVA_HOME_OF_JDK8>
export JAVA11_HOME=<JAVA_HOME_OF_JDK11>
cd <root-of-project>
. env.sh
cd benchmarks
./run.sh <times-to-run>
```

### Process of Log File

```
cd <root-of-project>
. env.sh
cd benchmarks
./process.sh
```

Two pdf file will be generated.