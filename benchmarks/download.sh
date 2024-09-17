cd dacapo
wget https://download.dacapobench.org/chopin/dacapo-23.11-chopin.zip
unzip dacapo-23.11-chopin.zip
cd ..

cd renaissance
wget https://github.com/renaissance-benchmarks/renaissance/releases/download/v0.14.2/renaissance-gpl-0.14.2.jar
mv renaissance-gpl-0.14.2.jar renaissance.jar
cd ..

cd spec
git clone https://github.com/connorimes/SPECjvm2008.git
cp ../test.py .
cp ../task.txt .
cd ..

