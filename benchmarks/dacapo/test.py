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
env['LD_PRELOAD'] = f'{env["JLITE_ROOT"]}/lib/libpreload.so'
with open("err.log", "a") as f:
    for size in ["small"]:
        for case in testcases:
            print(f"{case[0]}-{size}", flush=True)
            bolt_base_prefix = f'java -agentpath:{env["JLITE_ROOT"]}/lib/libagent.so="base" -Xbootclasspath/a:{env["JLITE_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            bolt_use_prefix = f'java -agentpath:{env["JLITE_ROOT"]}/lib/libagent.so="use" -Xbootclasspath/a:{env["JLITE_ROOT"]}/../agent-jar-with-dependencies.jar -Xverify:none'
            f.flush()
            # if case[1] == '8':
            #     prefix=". ~/.javarc; /usr/bin/time -v "
            # else:
            #     prefix=". ~/.java11rc; /usr/bin/time -v "
            prefix="/usr/bin/time -v "
            suffix = f"-Xmx1024m -Xms256m -XX:+UseG1GC -jar {case[2]} -t 1 {case[0]}"
            print(f"find file {case[0]}-candidate.txt")
            if not os.path.exists(f'{env["JLITE_PROJECT_ROOT"]}/benchmarks/candidates/dacapo/{case[0]}-candidate.txt'):
                print(f"[{timer()}] get first ort", flush=True)
                run("touch candidate.txt")
                res = subprocess.run([f"{prefix}{bolt_base_prefix} {suffix} -s small"], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    env=env)
                print(res.stderr.decode())
                print(res.stdout.decode())
                # if res.returncode != 0:
                #     f.write(f"[ERROR] {case[0]}\n")
                #     subprocess.run([f"rm -rf data-*"], shell=True)
                #     continue
                print(f"[{timer()}] get candidate", flush=True)
                res = run('''
                    cd `ls -l -t | grep data- |head -n 1 |awk '{print $NF}'`;
                    owner.sh;
                    freq_new_query.py;
                    cp candidate.txt ..
                          ''')
                if res.returncode != 0:
                    f.write(f"[ERROR] {case[0]}\n")
                    subprocess.run([f"rm -rf data-*"], shell=True)
                    print(res.stderr)
                    continue
                run(f'cp candidate.txt {case[0]}-candidate.txt')
            else:
                run(f'cp {env["JLITE_PROJECT_ROOT"]}/benchmarks/candidates/dacapo/{case[0]}-candidate.txt candidate.txt')
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
                print(f"[{timer()}] run ort_use", flush=True)
                st = timer()
                res = run(f"{prefix}{bolt_use_prefix} {suffix} -s {size}",
                                     env=env
                          )

                # if res.returncode != 0:
                #     f.write(f"[ERROR] {case[0]}\n")
                #     subprocess.run([f"rm -rf data-*"], shell=True)
                #     continue
                ed = timer()
                with open(f"{st_marker}/ort_use-{case[0]}-{size}-{time_mark}.log", 'wb') as f1:
                    if (res.stdout != None):
                        f1.write(res.stdout)
                    if (res.stderr != None):
                        f1.write(res.stderr)

                ort_use_time = ed - st
                with open(f"{st_marker}/time.log", "a") as fout:
                    fout.write(f"{case[0]}, {orgi_time}, {ort_time}, {ort_use_time}, {size}, {time_mark}\n")
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
