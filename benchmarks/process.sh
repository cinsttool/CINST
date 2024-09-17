rm -rf ../logs
mkdir -p ../logs
cp -r dacapo/`cat dacapo/log_dir.txt` ../logs/dacapo
cp -r renaissance/`cat renaissance/log_dir.txt` ../logs/renaissance
cp -r spec/SPECjvm2008/`cat spec/SPECjvm2008/log_dir.txt` ../logs/spec
cd ../scripts
./process_log.sh

python3 ./overhead.py
python3 ./overhead.py 2
mv *.pdf ../benchmarks
