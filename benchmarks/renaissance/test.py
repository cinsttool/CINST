import os
import subprocess
from timeit import default_timer as timer
import sys
jar_name = 'renaissance.jar'
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
env['LD_PRELOAD'] = f'{env["JLITE_ROOT"]}/lib/libpreload.so'
with open("err.log", "a") as f:
    # for size in ["default"]:
        for case in testcases:
            print(f"{case}", flush=True)
            f.flush()            
            bolt_base_prefix = f'java -agentpath:{env["JLITE_ROOT"]}/lib/libagent.so="base" -Xbootclasspath/a:{env["JLITE_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            bolt_use_prefix = f'java -agentpath:{env["JLITE_ROOT"]}/lib/libagent.so="use" -Xbootclasspath/a:{env["JLITE_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            prefix='/usr/bin/time -v '
            suffix = f"-Xmx1024m -Xms256m -XX:+UseG1GC -jar {jar_name} -o thread_count=1 -o spark_thread_limit=1 {case} -r 1"
            print(f"[{timer()}] get candidate", flush=True)
            if not os.path.exists(f'{env["JLITE_PROJECT_ROOT"]}/benchmarks/candidates/renaissance/{case}-candidate.txt'):
                res = run(f"{prefix}{bolt_base_prefix} -jar {jar_name} -o thread_count=16 {case} -r 1",
                          env=env)
                res = run('''
                    cd `ls -l -t | grep data- |head -n 1 |awk '{print $NF}'`;
                    ../freq_new | awk -v FS=',' '{print $2}' > ../candidate.txt;
                          ''')
                run(f'cp candidate.txt {case}-candidate.txt')
            else:
                run(f'cp {env["JLITE_PROJECT_ROOT"]}/benchmarks/candidates/renaissance/{case}-candidate.txt candidate.txt')

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
                print(f"[{timer()}] run ort_use", flush=True)
                st = timer()
                res = run(f"{prefix}{bolt_use_prefix} {suffix} ", env=env)
                # if res.returncode != 0:
                #     f.write(f"[ERROR] {case[0]}\n")
                #     subprocess.run([f"rm -rf data-*"], shell=True)
                #     continue
                ed = timer()
                with open(f"{run_st}/ort_use-{case}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)

                ort_use_time = ed - st
                with open(f"{run_st}/time.log", "a") as fout:
                    fout.write(f"{case}, {orgi_time}, {ort_time}, {ort_use_time}, {time_mark}\n")
                run("rm -rf data-*")
                print(f"[{timer()}] end", flush=True)

            cmds = [
                "ort.sh -jar {jar} -s small -t 1 {case}",
                "cd `ll -s=date | grep data- |tail -n 1 |awk '{print $NF}'`",
                "owner.sh",
                "freq_new_query.py",
                "cp candidate.txt ..",
                "cd ..",
                "java -jar {jar} -s large -t 1 {case}",
                "ort.sh -jar {jar} -s large -t 1 {case}",
                "ort_use.sh -jar {jar} -s large -t 1 {case}",
            ]
