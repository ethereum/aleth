wget https://nodejs.org/dist/v8.3.0/node-v8.3.0-linux-x64.tar.xz && \
tar -xvf node-v8.3.0-linux-x64.tar.xz && \
sudo mv node-v8.3.0-linux-x64/bin/* /usr/bin/ && \
sudo mv node-v8.3.0-linux-x64/lib/* /usr/lib/ && \
sudo npm install -g jsonschema && \
# install lllc
git clone --recursive https://github.com/ethereum/solidity.git && \
cd solidity && \
./scripts/install_deps.sh && \
mkdir build && \
cd build && \
cmake .. && \
make && \
sudo mv lllc/lllc /usr/bin
