import os
import subprocess
from timeit import default_timer as timer
import sys
times_to_run = int(sys.argv[1])

cases = [
"avrora",
"batik",
"biojava",
"eclipse",
"fop",

"kafka",
"luindex",
"lusearch",
"pmd",
"sunflow",
"xalan",
"zxing",
]

def run(cmd, *args, **kwargs):
    return subprocess.run([cmd], *args, **kwargs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
testcases = []
for c in cases:
    testcases.append((c, -1, "dacapo-23.11-chopin.jar"))
testcases.sort(key=lambda x: 10 if x[0] in ['sunflow', 'graphchi', 'h2'] else 1)
st_marker = str(timer())
os.mkdir(st_marker)
with open("log_dir.txt", "w") as f:
    f.write(st_marker)
env = os.environ.copy()
env['LD_PRELOAD'] = f'{env["CINST_ROOT"]}/lib/libpreload.so'
with open("err.log", "a") as f:
    for size in ["small"]:
        for case in testcases:
            print(f"{case[0]}-{size}", flush=True)
            bolt_base_prefix = f'java -agentpath:{env["CINST_ROOT"]}/lib/libagent.so="container" -Xbootclasspath/a:{env["CINST_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            f.flush()
            # if case[1] == '8':
            #     prefix=". ~/.javarc; /usr/bin/time -v "
            # else:
            #     prefix=". ~/.java11rc; /usr/bin/time -v "
            prefix="/usr/bin/time -v "
            suffix = f"-Xmx1024m -Xms256m -XX:+UseG1GC -jar {case[2]} -t 1 {case[0]}"
            for i in range(times_to_run):
                time_mark = timer()
                subprocess.run([f"rm -rf data-*"], shell=True)
                print(f"[{timer()}] run java", flush=True)
                st = timer()
                res = run(f"{prefix}java {suffix} -s {size}")
                if res.returncode != 0:
                    f.write(f"[ERROR] {case[0]}\n")
                    print(res.stderr.decode())
                    print(res.stdout.decode())
                    subprocess.run([f"rm -rf data-*"], shell=True)
                    continue
                ed = timer()
                with open(f"{st_marker}/java-{case[0]}-{size}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)

                orgi_time = ed - st
                subprocess.run([f"rm -rf data-*"], shell=True)
                print(f"[{timer()}] run ort", flush=True)

                st = timer()
                res = run(f"{prefix}{bolt_base_prefix} {suffix} -s {size}",
                                     env=env)
                
                # if res.returncode != 0:
                #     f.write(f"[ERROR] {case[0]}\n")
                #     subprocess.run([f"rm -rf data-*"], shell=True)
                #     continue
                ed = timer()

                ort_time = ed - st
                with open(f"{st_marker}/ort-{case[0]}-{size}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)

                subprocess.run([f"rm -rf data-*"], shell=True)
