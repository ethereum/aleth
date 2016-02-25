# eth: The Command-Line Interface Client

**Note: This chapter is for getting started with the command-line client `eth`. If you are interested only in AlethZero, move on to the next chapter.**

`eth` or **++eth** as it is sometimes referred to, is the TurboEthereum CLI client. To use it, you should open a terminal on your system. `eth` normally just runs in the background. If you want an interactive console (you do!), it has an option: `console` (it has two others - `import` and `export`. We'll get to those later. So now, start `eth`:

```
eth console
```

You'll see a little bit of information:

```
(++)Ethereum
Beware. You're entering the Frontier!
16:13:52|  Id: ##59650f8a…
16:13:58|  Opened blockchain DB. Latest: #d4e56740… (rebuild not needed)
16:13:58|  Opened state DB.
```

## Using the Testnet

There are two Ethereum "networks": the *mainnet* (the current version of which is called "Frontier") and the *testnet* (currently called "Morden"). They're independent of each other. The only difference between the two is that ether is essentially gratis on the testnet. By default, you'll connect to the mainnet. If you want to connect to the Morden testnet instead, start `eth` with the `--testnet` option instead:

```
eth console --testnet
```

## Setting your Master Password

After this information, the first thing it will do is ask you for a master password.

```
Please enter a MASTER password to protect your key store (make it strong!):
```

The master password is a password that protects your privacy and acts as a default security measure for your various Ethereum identities. Even with access to your computer nobody can work out who your online Ethereum addresses are without this password. It's also a default security password for your other keys, if you don't want to be remembering too many password. Anyway, it's the first line of defence and aught to be strong.

Enter a password, preferably taking into account [sage advice on password creation](https://xkcd.com/936/). Then when it asks for a confirmation...

```
Please confirm the password by entering it again: 
```

...enter it again.

It will pause shortly while it figures out your network environment and starts it all up. After a little while, you'll see some information on the software as well as on the account it created for you.

## Your First Account

This is your newly created default account (or 'identity'. I use the words interchangeably). In my case, it was the account that begins with `XE712F44`. This is an *ICAP code*, similar to an IBAN code that you might have used when doing banking transfers. You and only you have the special *secret key* for this account. It's guarded by the password you just typed. Don't ever tell anyone your password or they'll be able to send ether from this account and use it for nefarious means.

```
Transaction Signer: XE712F44QOZBKNLD20DLAEE8O2YJ7XRGP4 (be5af9b0-9917-b9bc-8f95-65cb9f042052 - 0093503f)
Mining Beneficiary: XE712F44QOZBKNLD20DLAEE8O2YJ7XRGP4 (be5af9b0-9917-b9bc-8f95-65cb9f042052 - 0093503f)
```

`eth` is nice. It tells you that any transactions you do will come from your account beginning with `XE712F44`. Similarly by default, if you mine successfully with the inbuilt miner, the proceeds will go into the same account.

You'll notice that there are two other codes in parentheses. The first is the *UUID* of the key. This is a code, only used on your computer, which allows us to identify which file the key is stored in without giving any any information of what account the key is for. In this case, the UUID begins with `be5af9b0`. 

The second piece of information that is parenthesised is the first few digits of the hex key. Older clients and Ethereum software depend on this to identify accounts. We don't use it any more because it's longer and doesn't have any way of determining if an address is invalid, so errors with mistyping can easily have major consequences.

Let's check that you do indeed have the key file for this account!

### Find that Key!

If you're using Linux or MacOS, open another terminal and navigate to `~/.web3/keys`. This is where all of your keys are stored. Enter `ls` and make sure there's a file that corresponds to the account's UUID. 

For Windows users, just use Explorer to navigate into your home folder's AppData/Web3/keys directory (you might need to enable Show Hidden Files to get there).

If you don't find a file with the same name as the UUID, then something is terribly wrong (out of disk space, possibly)! Get yourself on the forums and ask before going any further.

## Syncing up

You'll now start seeing a little bit of information as it tries to connect to the network. You might see a line like:

```
18:25:31|p2p  Hello: ++eth-v0.9.40-727666c2/EthDEV Server Frontier//RelWithDebInfo-Linux/g++/JIT V[ 4 ] ##979b7fa2… (eth,61) 30303
```

This is it telling you that it's managed to contact another node. After a little while it will begin to synchronise to the network. This will probably give you an awful lot of messages. If there are too many for you to handle, reduce them by changing the verbosity. We can set the verbosity to zero (the lowest and quietest) by typing:

```
web3.admin.setVerbosity(0)
```

It'll reply `true` to tell you that all is fine:

```
> web3.admin.setVerbosity(0)
true
```

As it synchronises, the latest block number will constantly rise, usually rather fast. Once it is synchronised, it'll still rise but much more slowly - at around 1 block every 15 seconds.

You can check its progress by using the console to get the latest block number. To do this, type:

```
web3.eth.blockNumber
```

You'll end up with something like:

```
> web3.eth.blockNumber
11254
```

## Got ETH?

You can easily check to see if you have ether in your account using the `eth.getBalance` function of `web3`. For this to work you'll need the address of which to get the balance. In my case, the address is the aforementioned `XE712F44QOZBKNLD20DLAEE8O2YJ7XRGP4`:

```
> web3.eth.getBalance("XE712F44QOZBKNLD20DLAEE8O2YJ7XRGP4")
0
```

That's not much, but then it is after all a newly cerated account. Let's query the balance of an account that actually has some funds, the Ethereum Foundation wallet:

```
> web3.eth.getBalance("XE86PXQKKKORDZQ1RWT9LGUGYZ1U57A56Y2")
11901464239480000000000000
```

Ooh, rather a lot more. The answer is given in Wei, the lowest denomination of ether. To work out what this is in sensible terms, use `web3.fromWei` and provide a sensible unit, e.g. `grand` (a grand, for those unfamiliar with English slang, is one thousand Ether):

```
> web3.fromWei(web3.eth.getBalance("de0b295669a9fd93d5f28d9ec85e40f4cb697bae"), 'grand')
11901.46423948
```

Wow that's nearly 12 million ether.


## And Finally...

Aside from the full power of Javascript, there are loads of functions you can use in the console; to see them just type `web3`.

When you're done playing, simply type `web3.admin.exit()` to exit `eth`.



