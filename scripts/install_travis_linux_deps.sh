set -e
# install lllc
git clone --recursive https://github.com/ethereum/solidity.git
cd solidity
./scripts/install_deps.sh
mkdir build
cd build
cmake ..
make
sudo mv lllc/lllc /usr/bin
