wget https://nodejs.org/dist/v8.3.0/node-v8.3.0-linux-x64.tar.xz && \
tar -xvf node-v8.3.0-linux-x64.tar.xz && \
sudo mv node-v8.3.0-linux-x64/bin/* /usr/bin/ && \
sudo mv node-v8.3.0-linux-x64/lib/* /usr/lib/ && \
sudo npm install -g jsonschema
