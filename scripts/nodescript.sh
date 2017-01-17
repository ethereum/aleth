timeout=2000
nodeGenesisfile="
{\n
        \"sealEngine\": \"NoProof\",\n
        \"params\": {\n
            \"accountStartNonce\": \"0x00\",\n
            \"frontierCompatibilityModeLimit\": \"0xffffffff\",\n
            \"maximumExtraDataSize\": \"0x0400\",\n
            \"tieBreakingGas\": false,\n
            \"minGasLimit\": \"125000\",\n
            \"gasLimitBoundDivisor\": \"0x0400\",\n
            \"minimumDifficulty\": \"0x020000\",\n
            \"difficultyBoundDivisor\": \"0x0800\",\n
            \"durationLimit\": \"0x08\",\n
            \"blockReward\": \"0x14D1120D7B160000\",\n
            \"registrar\": \"5e70c0bbcd5636e0f9f9316e9f8633feb64d4050\",\n
            \"networkID\" : \"0x25\"\n
        },\n
        \"genesis\": {\n
            \"nonce\": \"0x000000000000002a\",\n
            \"difficulty\": \"0x2000\",\n
            \"mixHash\": \"0x0000000000000000000000000000000000000000000000000000000000000000\",\n
            \"author\": \"0x0000000000000000000000000000000000000000\",\n
            \"timestamp\": \"0x00\",\n
            \"parentHash\": \"0x0000000000000000000000000000000000000000000000000000000000000000\",\n
            \"extraData\": \"0x\",\n
            \"gasLimit\": \"0x2fefd8\"\n
        },\n
        \"accounts\": {\n
            \"0000000000000000000000000000000000000001\": { \"wei\": \"1\", \"precompiled\": { \"name\": \"ecrecover\", \"linear\": { \"base\": 3000, \"word\": 0 } } },\n
            \"0000000000000000000000000000000000000002\": { \"wei\": \"1\", \"precompiled\": { \"name\": \"sha256\", \"linear\": { \"base\": 60, \"word\": 12 } } },\n
            \"0000000000000000000000000000000000000003\": { \"wei\": \"1\", \"precompiled\": { \"name\": \"ripemd160\", \"linear\": { \"base\": 600, \"word\": 120 } } },\n
            \"0000000000000000000000000000000000000004\": { \"wei\": \"1\", \"precompiled\": { \"name\": \"identity\", \"linear\": { \"base\": 15, \"word\": 3 } } },\n
            \"[ADDRESS]\": { \"wei\": \"100000000000000000000\" }\n
        }\n
}";

startScript="function sleep(ms) { return new Promise(resolve => setTimeout(resolve, ms));}\nvar result;\n";
endScript="sleep($timeout).then(() => {\n
     console.log('Result: ' + result);\n
     process.exit(0);\n
});"

getNodeIdScript=$startScript"web3.admin.getNodeInfo(function(err,res){result = res.id;})\n"$endScript;
getNewAccountScript=$startScript"web3.personal.newAccount(\"123\", function(err,res){result = res;})\n"$endScript;
mineBlocksScript=$startScript"web3.test.mineBlocks(1, function(err,res){result = res;})\n"$endScript;
sendTransactionScript=$startScript"web3.eth.sendTransaction({from:\"[ADDRESS]\", to:\"0x543B2eA3B34790eddE2321038B540F555c5756b1\",value:10000000000000000000},function(err,res){result=res;})\n"$endScript;
unlockAccountScript=$startScript"web3.personal.unlockAccount(\"[ADDRESS]\", \"123\", 1000000, function(err,res){result=res;})\n"$endScript;
getBalanceScript=$startScript"web3.eth.getBalance(\"[ADDRESS]\", function(err,res){result=res;})\n"$endScript;
getPeerCountScript=$startScript"web3.net.getPeerCount(function(err, res){ result=res; })\n"$endScript;
getBlockNumberScript=$startScript"web3.eth.getBlockNumber(function(err, res){ result=res; })\n"$endScript;
getBlockHashScript=$startScript"web3.eth.getBlock(\"latest\", function(err, res){ result=res.hash; })\n"$endScript;
mineBlocksScript=$startScript"web3.test.mineBlocks(1,function(err, res){ result=res; })\n"$endScript;
addPeerScript=$startScript"web3.admin.addPeer(\"enode://[PEER]\", function(err, res){ result=res; })\n"$endScript;


startEth()
{
$ETH --private "chain" -d "$FULL_WORK_DIRECTORY/ethnode1" --config "$FULL_WORK_DIRECTORY/nodeGenesis.json" --ipcpath "$FULL_WORK_DIRECTORY/ethnode1/geth.ipc" --ipc --listen 30305 --test &> /dev/null &
debug "$ETH --private \"chain\" -d \"$FULL_WORK_DIRECTORY/ethnode1\" --config \"$FULL_WORK_DIRECTORY/nodeGenesis.json\" --ipcpath \"$FULL_WORK_DIRECTORY/ethnode1/geth.ipc\" --ipc --listen 30305 --test"
sleep 5
}

startEth2()
{
$ETH --private "chain" -d "$FULL_WORK_DIRECTORY/ethnode2" --config "$FULL_WORK_DIRECTORY/nodeGenesis.json" --ipcpath "$FULL_WORK_DIRECTORY/ethnode2/geth.ipc" --ipc --listen 30306 --test &> /dev/null &
sleep 5
}

debug()
{
if [ $DEBUG == "debug" ]; then
   echo $1
fi
}

notef()
{
if [ $DEBUG == "debug" ]; then
   echo $1
else
   echo -e $1"\c"
fi
}

check()
{
if [ $1 == $2 ]; then
    echo "OK"
else
    echo "FAILED"
fi
}


#environment variables
ETH=$1
DEBUG=$2
VERSION="0.0.1"
WORK_DIRECTORY="scripts"
FULL_WORK_DIRECTORY=$PWD/$WORK_DIRECTORY
NODEPATH=./ethereum-console/node/bin/node
MAINJSPATH=./ethereum-console/main.js
IPC1=$FULL_WORK_DIRECTORY/ethnode1/geth.ipc
IPC2=$FULL_WORK_DIRECTORY/ethnode2/geth.ipc
PROPAGATION_WAIT=5
exec 2> /dev/null  #disable error notification in bash (use manual)

if [ ! $ETH ]; then
echo "Provide eth path! Usage: (./nodescript /ethereum/build/eth/eth)"
exit
fi
if [ ! -f $ETH ]; then
    echo "Eth executable was not found at: $ETH"
    exit
fi

debug "setting up ethereum-console and node"
if [ ! -d "ethereum-console" ]; then
git clone https://github.com/winsvega/ethereum-console.git &> /dev/null
cd ethereum-console
git checkout nodestandalone &> /dev/null
cd ..
fi

debug "Generate temp files and scripts"
if [ ! -d $WORK_DIRECTORY ]; then
   mkdir $WORK_DIRECTORY
fi
echo -e $getNodeIdScript > ./$WORK_DIRECTORY/getNodeIdScript.js
echo -e $getNewAccountScript > ./$WORK_DIRECTORY/getNewAccountScript.js
echo -e $mineBlocksScript > ./$WORK_DIRECTORY/mineBlocksScript.js
echo -e $getPeerCountScript > ./$WORK_DIRECTORY/getPeerCountScript.js
echo -e $getBlockNumberScript > ./$WORK_DIRECTORY/getBlockNumberScript.js
echo -e $getBlockHashScript > ./$WORK_DIRECTORY/getBlockHashScript.js
echo -e $mineBlocksScript > ./$WORK_DIRECTORY/mineBlocksScript.js


#Generate new account 
notef "TEST_generateAccount "
debug "Starting eth Instance..."
startEth
pid=$!
debug "Eth Instance1 ID: "$pid
debug "Generate new account"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getNewAccountScript.js)
NEW_ACCOUNT_0X=${STR#*"Result: "} #before=${s%%"$sep"*}
NEW_ACCOUNT=$(echo $NEW_ACCOUNT_0X | sed "s/0x//g")
debug $NEW_ACCOUNT_0X
check ${#NEW_ACCOUNT_0X} 42
kill -KILL $pid &> /dev/null

debug "Configure custom genesis file and scripts with new account"
nodeGenesisfile=$(echo $nodeGenesisfile | sed "s/\[ADDRESS\]/$NEW_ACCOUNT/g")
echo -e $nodeGenesisfile > ./$WORK_DIRECTORY/nodeGenesis.json
sendTransactionScript=$(echo $sendTransactionScript | sed "s/\[ADDRESS\]/$NEW_ACCOUNT_0X/g")
echo -e $sendTransactionScript > ./$WORK_DIRECTORY/sendTransactionScript.js
unlockAccountScript=$(echo $unlockAccountScript | sed "s/\[ADDRESS\]/$NEW_ACCOUNT_0X/g")
echo -e $unlockAccountScript > ./$WORK_DIRECTORY/unlockAccountScript.js
getBalanceScript=$(echo $getBalanceScript | sed "s/\[ADDRESS\]/$NEW_ACCOUNT_0X/g")
echo -e $getBalanceScript > ./$WORK_DIRECTORY/getBalanceScript.js


#Test addPeer function
notef "TEST_addPeer "
debug "Starting eth Instance1..."
startEth
pid=$!
debug "Eth Instance1 ID: "$pid
debug "Starting eth Instance2..."
startEth2
pid2=$!
debug "Eth Instance2 ID: "$pid2
debug "Add Instance1 as a peer to Instance2"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getNodeIdScript.js)
NODEID=${STR#*"Result: "}
NODEID=$NODEID"@127.0.0.1:30305"
debug "instance1 ID: "$NODEID
addPeerScript=$(echo $addPeerScript | sed "s/\[PEER\]/$NODEID/g")
echo -e $addPeerScript > ./$WORK_DIRECTORY/addPeerScript.js
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2  ./$WORK_DIRECTORY/addPeerScript.js)
debug "Get peer count of Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getPeerCountScript.js)
STR=${STR#*"Result: "}
check $STR 1


#Test mineBlocks function
notef "TEST_mineBlocks "
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Mine a block on Instance1..."
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/mineBlocksScript.js)
debug "Wait for block propagation"
sleep $PROPAGATION_WAIT
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR1=${STR#*"Result: "}
debug $STR1
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR2=${STR#*"Result: "}
debug $STR2
check $STR1 $STR2

#Test mineBlocks function
notef "TEST_mineBlocks2 "
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Mine a block on Instance1..."
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/mineBlocksScript.js)
debug "Wait for block propagation"
sleep $PROPAGATION_WAIT
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR1=${STR#*"Result: "}
debug $STR1
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR2=${STR#*"Result: "}
debug $STR2
check $STR1 $STR2

#Test sendTransaction function
notef "TEST_sendTransaction "
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBalanceScript.js)
STR=${STR#*"Result: "}
debug "Initial balance of account on instance1 = "$STR
debug "Unlock account on Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/unlockAccountScript.js)
debug "Send Transaction from Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/sendTransactionScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Wait for transaction propagation"
sleep $PROPAGATION_WAIT
debug "Mine Blocks on Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/mineBlocksScript.js)
debug "Wait for block propagation"
sleep $PROPAGATION_WAIT
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBalanceScript.js)
STR=${STR#*"Result: "}
debug "Balance on Instance 1 = "$STR
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBalanceScript.js)
STR=${STR#*"Result: "}
debug "Balance after on Instance2 = "$STR
check $STR 89999580000000000000


#Test mineBlocks viceversa function
notef "TEST_mineBlocks3 "
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Mine a block on Instance2..."
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/mineBlocksScript.js)
debug "Wait for block propagation"
sleep $PROPAGATION_WAIT
debug "Latest Block on Instance1: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR1=${STR#*"Result: "}
debug $STR1
debug "Latest Block on Instance2: "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBlockHashScript.js)
STR2=${STR#*"Result: "}
debug $STR2
check $STR1 $STR2


#Test sendTransaction function
notef "TEST_sendTransaction2 "
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/getBalanceScript.js)
STR=${STR#*"Result: "}
debug "Initial balance of account on instance1 = "$STR
debug "Unlock account on Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/unlockAccountScript.js)
debug "Send Transaction from Instance1"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC1 ./$WORK_DIRECTORY/sendTransactionScript.js)
STR=${STR#*"Result: "}
debug $STR
debug "Wait for transaction propagation"
sleep $PROPAGATION_WAIT
debug "Mine Blocks on Instance2"
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/mineBlocksScript.js)
debug "Wait for block propagation"
sleep $PROPAGATION_WAIT
STR=$($NODEPATH $MAINJSPATH ipc://$IPC2 ./$WORK_DIRECTORY/getBalanceScript.js)
STR=${STR#*"Result: "}
debug "Balance after transaction and block from Instance2 = "$STR
check $STR 79999160000000000000

rm -r $FULL_WORK_DIRECTORY
kill -KILL $pid &> /dev/null
kill -KILL $pid2 &> /dev/null
