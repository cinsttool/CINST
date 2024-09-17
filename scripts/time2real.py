import os
import sys
os.chdir(sys.argv[1])
def convert_file(input_file, output_file):
    #with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
    with open(input_file, 'r') as infile:
        outfile=sys.stdout
        for line in infile:
            # 分割每一行的数据
            parts = line.strip().split(', ')
            
            # 提取数据
            name = parts[0]
            time1 = parts[1]
            time2 = parts[2]
            time3 = parts[3]
            marker = parts[-1]
            
            # 将数据写入输出文件
            outfile.write(f"java, {name}, {time1}, {marker}\n")
            outfile.write(f"ort, {name}, {time2}, {marker}\n")
            outfile.write(f"ort_use, {name}, {time3}, {marker}\n")

if __name__ == "__main__":
    input_file = "time.log"  # 输入文件名
    output_file = "real.log"  # 输出文件名

    convert_file(input_file, output_file)
