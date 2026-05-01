# Quick Start

## 1) Start server

```bash
cd cinst
./run_server.sh
```

## 2) Run instrumented Java app

```bash
LD_PRELOAD=/path/to/cinst/install/lib/libpreload.so \
LD_LIBRARY_PATH=/path/to/cinst/install/lib/ \
java -agentpath:/path/to/cinst/install/lib/libagent.so \
     -javaagent:/path/to/cinst/agent-jar-with-dependencies.jar[="<args>"] \
     [-jar] <JAVA_APP>
```

Optional bootstrap mode for tracing standard library internals:

```bash
-Xbootclasspath/a:/path/to/CINST/agent-jar-with-dependencies.jar
```

## 3) Analyze generated trace

```bash
cd data-<pid>
bash build_container.sh
python3 NGC_query.py
```
