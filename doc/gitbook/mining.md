# Mining on Ethereum

Mining is a common term for securing the Ethereum network and validating new transactions in exchange for a small payment. Anyone can mine, though it really helps if you can a good GPU. How often you are paid out depends on who else is mining and how much mining power (read: computation power) your hardware has.

We use a custom-made algorithm named Ethash, a combination of the Hashimoto and Dagger algorithms, designed by Tim Hughes, Vitalik Buterin and Matthew Wampler-Doty. It is memory-bandwidth-hard making is an excellent candidate for GPU mining but a bad candidate for custom hardware. We plan on switching to a proof-of-stake algorithm inover the course of the next 9 months with the Serenity release of Ethereum.

Because the algorithm is memory hard, you'll need 2GB of RAM per GPU with which you wish to mine, at least for the forseeable future. (The dataset starts at 1GB and grows every few days, so you might be able to get away with 1.5GB for the first few months, if such graphics cards exist.)

ASICs and FPGAs is be strongly discouraged by being rendered financially inefficient, which was confirmed in an independent audit. Don't expect to see them on the market, and if you do, proceed with extreme caution.

## Setting things up on Linux

For this quick guide, you'll need Ubuntu 14.04 or 15.04 and the fglrx graphics drivers. You an use NVidia drivers and other platforms, too, but you'll have to find your own way to getting a working OpenCL install with them.

If you're on 15.04, Go to "Software and Updates > Additional Drivers" and set it to "Using video drivers for the AMD graphics accelerator from fglrx". Once the drivers are installed and in use, you're all set, go to the next section!

If you're on 14.04, go to "Software and Updates > Additional Drivers" and set it to "Using video drivers for the AMD graphics accelerator from fglrx". Unfortunately, for some of you this will not work due to a known bug in Ubuntu 14.04.02 preventing you from switching to the proprietary graphics drivers required to GPU mine. 

So, if you encounter this bug, and before you do anything else, go to "Software and updates > Updates" and select "Pre-released updates trusty proposed". Then, go back to "Software and Updates > Additional Drivers" and set it to "Using video drivers for the AMD graphics accelerator from fglrx"). Reboot. 

Once rebooted, it's well worth having a check that the drivers have now indeed been installed correctly.

Whatever you do, if you are on 14.04.02 do not alter the drivers or the drivers configuration once set. For example, the usage of `aticonfig --initial` can and likely will 'break' your setup. If you accidentally alter their configuration, you'll need to de-install the drivers, reboot, reinstall the drivers and reboot. 

# Mining with eth

Mining on Ethereum with `eth` is simple. If you need to mine with a single GPU then, just running` eth` will be sufficient. If not you can use a combination of `eth` and `ethminer`. This works on all platforms, though Linux is usually the easiest to set up.

## Mining on a single GPU

In order to mine on a single GPU all that needs to be done is to run `eth` with the following arguments:

```
eth -i -v 1 -a 0xcadb3223d4eebcaa7b40ec5722967ced01cfc8f2 --client-name "OPTIONALNAMEHERE" -x 50 -m on -G
```

- `-i` Requests an interactive javascript console so that we can interact with the client
- `-v 1` Set verbosity to 1. Let's not get spammed by messages.
- `-a YOURWALLETADDRESS` Set the coinbase, where the mining rewards will go to. The above address is just an example. This argument is really important, make sure to not make a mistake in your wallet address or you will receive no ether payout.
- `--client-name "OPTIONAL"` Set an optional client name to identify you on the network
- `-x 50` Request a high amount of peers. Helps with finding peers in the beginning.
- `-m on` Actually launch with mining on.
- `-G` set GPU mining on.

While the client is running you can interact with it using the [interactive console](interactive_console.md).

## Mining on multiple GPUs

Mining with multiple GPUs and `eth` is very similar to mining with [geth and multiple GPUs](http://ethereum.gitbooks.io/frontier-guide/content/gpu.html#gpu-mining-with-ethminer). 

1. Ensure that an eth++ node is running with your coinbase address properly set:
 ```
 eth -i -v 1 -a 0xcadb3223d4eebcaa7b40ec5722967ced01cfc8f2 --client-name "OPTIONALNAMEHERE" -x 50 -j
 ```
 Notice that we also added the `-j` argument so that the client can have the JSON-RPC server enabled to communicate with the ethminer instances. Additionally we removed the mining related arguments since `ethminer` will now do the mining for us.
 
2. For each of your GPUs execute a different ethminer instance:
 ```
 ethminer --no-precompute -G --opencl-device XX  
 ```
 Where `XX` is an index number corresponding to the openCL device you want the ethminer to use. 
 
 In order to easily get a list of OpenCL devices you can execute `ethminer --list-devices` which will
provide a list of all devices OpenCL can detect, with also some additional information per device. Below is a sample output:
 ```
 [0] GeForce GTX 770
        CL_DEVICE_TYPE: GPU
        CL_DEVICE_GLOBAL_MEM_SIZE: 4286345216
        CL_DEVICE_MAX_MEM_ALLOC_SIZE: 1071586304
        CL_DEVICE_MAX_WORK_GROUP_SIZE: 1024

 ```
 Finally the `--no-precompute` argument requests that the ethminers don't create the [DAG](https://github.com/ethereum/wiki/wiki/Ethash-DAG) of the next epoch ahead of time.
 
## Benchmarking

Mining power tends to scale with memory bandwidth. Our implementation is written in OpenCL, which is typically supported better by AMD GPUs over NVidia. Empirical evidence confirms that AMD GPUs offer a better mining performance in terms of price than their NVidia counterparts. R9 290x appears to be the best card at present. 

To benchmark a single-device setup you can use `ethminer` in benchmarking mode through the `-M` option:

```
ethminer -G -M
```

If you have many devices and you'll like to benchmark each individually, you can use the `--opencl-device` option similarly to the previous section:

```
ethminer -G -M --opencl-device XX
```
Use `ethminer --list-devices` to list possible numbers to substitute for the `XX`.
