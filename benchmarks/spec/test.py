import os
import subprocess
from timeit import default_timer as timer
import sys
jar_name = './SPECjvm2008.jar'
with open('tasks.txt') as f:
    testcases = f.readlines()
    testcases = [x.strip() for x in testcases]
# testcases = testcases[1:]

times_to_run = int(sys.argv[1])

def run(cmd, *args, **kwargs):
    return subprocess.run([cmd], *args, **kwargs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
# testcases = []
# for c in case_in_1:
#     testcases.append((c, case_in_1[c], "dacapo-evaluation-git-0f83db5d_1.jar"))
# for c in case_in_org:
#     testcases.append((c, case_in_org[c], "dacapo-evaluation-git-0f83db5d.jar"))
# testcases.sort(key=lambda x: 10 if x[0] in ['sunflow', 'graphchi', 'h2'] else 1)
run_st = str(timer())
os.mkdir(run_st)
with open("log_dir.txt", "w") as f:
    f.write(run_st)
env = os.environ.copy()
env['LD_PRELOAD'] = f'{env["CINST_ROOT"]}/lib/libpreload.so'
with open("err.log", "a") as f:
    # for size in ["default"]:
        for case in testcases:
            print(f"{case}", flush=True)
            f.flush()            
            bolt_base_prefix = f'java -agentpath:{env["CINST_ROOT"]}/lib/libagent.so="container" -Xbootclasspath/a:{env["CINST_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            prefix='/usr/bin/time -v '
            suffix = f"-Xmx1024m -Xms256m -XX:+UseG1GC -jar {jar_name} -i 1 -wt 20 -bt 1 -ops 5 {case} -ikv"

            for i in range(times_to_run):
                time_mark = timer()
                subprocess.run([f"rm -rf data-*"], shell=True)
                print(f"[{timer()}] run java", flush=True)
                st = timer()
                res = run(f"{prefix}java {suffix}")
                if res.returncode != 0:
                    f.write(f"[ERROR] {case}\n")
                    subprocess.run([f"rm -rf data-*"], shell=True)
                    continue
                ed = timer()
                with open(f"{run_st}/java-{case}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)

                orgi_time = ed - st

                subprocess.run([f"rm -rf data-*"], shell=True)
                print(f"[{timer()}] run ort", flush=True)

                st = timer()
                res = run(f"{prefix}{bolt_base_prefix} {suffix}", env=env)
                # if res.returncode != 0:
                #     f.write(f"[ERROR] {case[0]}\n")
                #     subprocess.run([f"rm -rf data-*"], shell=True)
                #     continue
                ed = timer()

                ort_time = ed - st
                with open(f"{run_st}/ort-{case}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)
                subprocess.run([f"rm -rf data-*"], shell=True)