import os
import sys
import subprocess
import re
from multiprocessing import Pool

class ATPGLS:
    name = "ATPG-LS"

    def analyse(output_file):
        # print("anal:" + output_file)
        content = open(output_file, "r").read()
        p1 = re.compile(r'final coverage: (\d+/\d+)', re.S)

        if len(p1.findall(content)) == 0:
            return("ERROR", "ERROR", "ERROR", "ERROR")
        
        cov = p1.findall(content)[0]
        
        dt = float(cov.split('/')[0])
        udt = int(cov.split('/')[1])

        coverage = "%.3f" % (dt / udt * 100)
        num_faults = udt
        
        p3 = re.compile(r'\) pattern\s*:\s*(\d+)', re.S)
        pattern = p3.findall(content)[-1]

        p4 = re.compile(r'Execution time: (\d+) milliseconds', re.S)
        time = p4.findall(content)[0]

        time = float(time) / 1000

        return (coverage, pattern, time, num_faults)    

def run_single_task(args):
    bench_file, seed = args
    filename = os.path.basename(bench_file)
    output_file = f"/tmp/{filename}seed_{seed}"
    
    # 运行可执行文件并将输出重定向到临时文件
    cmd = f"./build/src/atpg -i {bench_file} --seed={seed} > {output_file}"
    subprocess.run(cmd, shell=True)
    
    return output_file

def main():
    if len(sys.argv) != 3:
        print("用法: ./run_parallel.py <bench文件路径> <线程数>")
        sys.exit(1)
        
    bench_file = sys.argv[1]
    num_threads = int(sys.argv[2])
    
    # 生成所有任务参数
    tasks = [(bench_file, seed) for seed in range(num_threads)]
    
    # 使用进程池并行执行
    with Pool(num_threads) as pool:
        output_files = pool.map(run_single_task, tasks)
        
    # 整理结果
    print(f"处理文件: {bench_file}")
    print("-" * 50)

    max_coverage = -1
    max_pattern = -1
    max_time = -1
    max_num_faults = -1

    content = ""
    
    for output_file in output_files:
        with open(output_file) as f:
            (coverage, pattern, time, num_faults) = ATPGLS.analyse(output_file)
            print(f"seed {output_file.split('_')[-1]} 的结果:")
            print(f"coverage: {coverage}, pattern: {pattern}, time: {time}, num_faults: {num_faults}")

            if float(coverage) > max_coverage:
                max_coverage = float(coverage)
                max_pattern = pattern
                max_time = time
                max_num_faults = num_faults
                content = open(output_file, "r").read()
            elif float(coverage) == max_coverage and int(pattern) < int(max_pattern):
                max_pattern = int(pattern)
                max_time = time
                max_num_faults = num_faults
                content = open(output_file, "r").read()
        
        # 删除临时文件
        os.remove(output_file)

    print(content)

if __name__ == "__main__":
    main()
