# aleth-key

`aleth-key` is a CLI tool that allows you to interact with the Ethereum wallet. With it you can list, inspect, create, delete and modify keys and inspect, create and sign transactions.

### Keys and Wallets

When using Ethereum you will own one or more `keys`. These are special files that allow you access to a particular account. Such access might allow you to spend funds, register a name or transfer an asset. Keys are standardised and compatible across major clients. They are always protected by password-based encryption. Also they do not directly identify the actual account that the key represents. To determine this, the key must be decrypted through providing the correct password.

In cpp-ethereum, your *wallet* keeps a track of each key that you own along with what *address* it represents. An address is just way of referring to a particular *account* in Ethereum. It, too, is protected by a password, which is generally provided when the client begins.

While all clients have keys, some do not have wallets; these clients typically store the address in the key in plain view. This substantially reduces privacy.

### Creating a Wallet

We'll assume you have not yet run a client such as `eth` or anything in the Aleth series of clients. If you have, you should skip this section.

To create a wallet, run `aleth-key` with the `createwallet` command:

```
> aleth-key createwallet
Please enter a MASTER passphrase to protect your key store (make it strong!): 
```

You'll be asked for a "master" passphrase. This protects your privacy and acts as a default password for any keys. You'll need to confirm it by entering the same text again.

### Listing the Keys in your Wallet

We can list the keys within the wallet simply by using the `list` command:

```
> aleth-key list
No keys found.
```

We haven't yet created any keys, and it's telling us so! Let's create one.

### Creating your First Key

One of the nice things about Ethereum is that creating a key is tantamount to creating an account. You don't need to tell anybody else you're doing it, you don't even need to be connected to the Internet. Of course your new account will not contain any Ether. But it'll be yours and you can be certain that without your key and your password, nobody else can ever access it.

To create a key, we use the `new` command. To use it we must pass a name - this is the name we'll give to this account in the wallet. Let's call it "test":

```
> aleth-key new test
Enter a passphrase  with which to secure this account (or nothing to use the master passphrase):
```

It will prompt you to enter a passphrase to protect this key. If you just press enter, it'll use the default "master" passphrase. Typically this means you won't need to enter the passphrase for the key when you want to use the account (since it remembers the master passphrase). In general, you should try to use a different passphrase for each key since it prevents one compromised passphrase from giving access to other accounts. However, out of convenience you might decide that for low-security accounts to use the same passphrase.

Here, let's give it the incredibly imaginitive passphrase of `123`.

Once you enter a passphrase, it'll ask you to confirm it by entering again. Enter `123` a second time.

Because you gave it its own passphrase, it'll also ask you to provide a hint for this password which will be displayed to you whenever it asks you to enter it. The hint is stored in the wallet and is itself protected by the master passphrase. Enter the truly awful hint of `321 backwards`.

```
> aleth-key new test
Enter a passphrase with which to secure this account (or nothing to use the master passphrase): 
Please confirm the passphrase by entering it again: 
Enter a hint to help you remember this passphrase: 321 backwards
Created key 055dde03-47ff-dded-8950-0fe39b1fa101
  Name: test
  Password hint: 321 backwards
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

Notice the last two lines there. One is the ICAP address, the other is the raw hexadecimal address. The latter is an older representation of address that you'll sometimes see and is being phased out in favour of the shorter ICAP address which also includes a checksum to avoid problems with mistyping. All normal (aka *direct*) ICAP addresses begin with `XE` so you should be able to recognise them easily.

Notice also that the key has another identifier after `Created key`. This is known as the UUID. This is a unique identifer for the key that has absolutely nothing to do with the account itself. Knowing it does nothing to help an attacker discover who you are on the network. It also happens to be the filename for the key, which you can find in either `~/.web3/keys` (Mac or Linux) or `$HOME/AppData/Web3/keys` (Windows).

Now let's make sure it worked properly by listing the keys in the wallet:

```
> aleth-key list
055dde03-47ff-dded-8950-0fe39b1fa101 0092e965… XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ  test
```

It reports one key on each line (for a total of one key here). In this case our key is stored in a file `055dde...` and has an ICAP address beginning `XE472EVK...`. Not especially easy things to remember so rather helpful that it has its proper name, `test`, too.

### ICAP or Raw hex?

You might see addresses passed as hex-only strings, especially with old software. These are dangerous since they don't include a checksum or special code to detect typos. You should generally try to keep clear of them.

Occasionally, however, it's important to convert between the two. `aleth-key` provides the `inspect` command for this purpose. When passed any address, file or UUID, it will tell you information about it including both formats of address.

For example, to get it to tell us about our account, we might use:

```
> aleth-key inspect test
test (0092e965…)
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

We could just as easily use the ICAP `XE472EVK...` or raw hex `0092e965...`:

```
> aleth-key inspect XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
test (0092e965…)
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
> aleth-key inspect 0092e965928626f8880629cec353d3fd7ca5974f
test (0092e965…)
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

### Backing up Your Keys

You should always back up your keys! Any backup solution that protects your home directory should also protect your keys (since that's where they live). However for added piece of mind make an explicit backup of your keys by copying the contents of the `~/.web3/keys` (Mac or Linux, or `$HOME/AppData/Web3/keys` for Windows) to an external disk. You might also open the files in a text editor, print them and keep them in a lawyer's safe for additional piece of mind. **If they get lost, nobody can help you!**

### Decoding a Transaction

Here's an unsigned transaction. It authorises the donation of 1 ether to me:

```
ec80850ba43b74008252089400be78bf8a425471eca0cf1d255118bc080abf95880de0b6b3a7640000801b8080
```

On its own, it won't do much. We can see this by decoding it in `aleth-key`:

```
> aleth-key decode ec80850ba43b74008252089400be78bf8a425471eca0cf1d255118bc080abf95880de0b6b3a7640000801b8080
Transaction 705d490edc318b50223efa7bb9c19d65f05c3c527e4f8e60535b46a2ed128706
  type: message
  to: XE6934MX3U67M48MPHYMC1A1X306AFKEXH (00be78bf…)
  data: none
  from: <unsigned>
  value: 1 ether (1000000000000000000 wei)
  nonce: 0
  gas: 21000
  gas price: 50 Gwei (50000000000 wei)
  signing hash: f2790ed53c803ee882c892e1d9715181dfc93780d755fbe4ffefd90701e15c31
```

Note that it states the transaction is `<unsigned>` to the right of `from:`. This means that at present it's useless. Signing it would make it useful (to me, at least, since it'd make me one Ether richer), or dangerous (to you if you didn't want to give me that Ether).

### Signing a Transaction

`aleth-key` can be used to sign a pre-existing, but unsigned, transaction (it can also create a transaction and sign it itself). In this case, the transaction is actually harmless anyway since we're signing with the key of a fresh account that has no Ether to be transferred.

The command we'll use is `sign`. To use it we must identify the account with which we wish to sign. This can be the ICAP (`XE472EVK...`), the hex address (`0092e965...`), the UUID (`055dde...`), the key file or simply the plain old name (`test`). Secondly you must describe transaction it should sign. This can be done through passing the hex or through a file containing the hex.

```
> aleth-key sign test ec80850ba43b74008252089400be78bf8a425471eca0cf1d255118bc080abf95880de0b6b3a7640000801b8080
Enter passphrase for key (hint:321 backwards): 
```

It will ask you for the passphrase from earlier, along with the ludicrously transparent hint. Enter `123`, the correct answer and it will provide you with the unsigned transaction (`a37c58...`), a `:` and the signed transaction (`f86c80...`):

```
a37c588c853dc20bbaef53b680e23642a03122897bbb9a53d25d0d8f3665a94f: f86c80850ba43b74008252089400be78bf8a425471eca0cf1d255118bc080abf95880de0b6b3a7640000801ca07638c34170f3e04313bbb6c5bfc10a0c665200515a1aa5e922c7ae6c0dd085faa079ab46048e643bb4042bcb22da86d2646eb0b727f23aa3e165102b824563c70d
```

Let's make sure it worked by decoding it.

```
> aleth-key decode f86c80850ba43b74008252089400be78bf8a425471eca0cf1d255118bc080abf95880de0b6b3a7640000801ca07638c34170f3e04313bbb6c5bfc10a0c665200515a1aa5e922c7ae6c0dd085faa079ab46048e643bb4042bcb22da86d2646eb0b727f23aa3e165102b824563c70d
Transaction a37c588c853dc20bbaef53b680e23642a03122897bbb9a53d25d0d8f3665a94f
  type: message
  to: XE6934MX3U67M48MPHYMC1A1X306AFKEXH (00be78bf…)
  data: none
  from: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ (0092e965…)
  value: 1 ether (1000000000000000000 wei)
  nonce: 0
  gas: 21000
  gas price: 50 Gwei (50000000000 wei)
  signing hash: f2790ed53c803ee882c892e1d9715181dfc93780d755fbe4ffefd90701e15c31
  v: 1
  r: 7638c34170f3e04313bbb6c5bfc10a0c665200515a1aa5e922c7ae6c0dd085fa
  s: 79ab46048e643bb4042bcb22da86d2646eb0b727f23aa3e165102b824563c70d
```

Being a signed transaction, it has the three fields at the end (`v`, `r` and `s`) and, importantly, the address from whom the transaction is sent (`from:`). You'll notice that the sender address (`XE472EVK...`) is indeed ours from before!

The signed transaction can be sent in an e-mail in a similar way to how you might send a cheque in the mail. It can also be placed on the network to enact it; through the web3 API web3.sendRawTransaction

### Killing an Account

Let's now delete our key we've made. Deleting a key actually actually deletes the underlying file. After doing this there's no going back (unless you have a backup). To avoid losing anything, we're first going to back up our account. First, let's copy the key file somewhere safe:

```
> mkdir ~/backup-keys
> cp ~/.web3/keys/* ~/backup-keys
```

or, for Windows:

```
> md $HOME/backup-keys
> copy $HOME/AppData/Web3/keys/*.* $HOME/backup-keys
```

Now, we'll delete the key with the `kill` command:

```
> aleth-key kill test
1 key(s) deleted.
```

And bang! It's gone.

Check by calling `list`:


```
> aleth-key list
No keys found.
```

### Restoring an Account from a Backup

Now let's support we made a horrible mistake and want to recover the account. Luckily we made a backup!

We could simply copy it back into the original `keys` directory. This would indeed make the key "available", however it would only be identifiable by its UUID (the filename minus the `.json`). This is a bit of a pain.

Better would be to reimport it into the wallet, which makes it addressable by its ICAP and hex, and gives it a name and password hint to boot. To do this, we need to use the `import` command, which takes the file and the name of the key:

```
> aleth-key import ~/backup-keys/* test
```

or, for Windows:

```
> aleth-key import $HOME/backup-keys/*.* test
```

Here it will need to know the passphrase for the key, mainly to determine the address of the key for placing into the wallet. There's no hint now because the wallet doesn't know anything about it. Enter the `123` passphrase.

It will then ask you to provide a hint (assuming it's different to the master password, which ours is). Enter the same hint.

```
Enter the passphrase for the key:
Enter a hint to help you remember the key's passphrase: 321 backwards
Imported key 055dde03-47ff-dded-8950-0fe39b1fa101
  Name: test
  Password hint: 321 backwards
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

Finally it will tell you that all went well and the key is reimported. We should recognise our address by now with the `XE472EVK...`.

To double-check, we can list the keys:

```
> aleth-key list     
055dde03-47ff-dded-8950-0fe39b1fa101 0092e965… XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ  test
```

All restored!

### Importing a key from another client (e.g. Geth)

Because our keys all share the same format it's really easy to import keys from other clients like Geth. In fact it's exactly the same process as restoring a key from a previous backup as we did in the last step.

If we assume we have a geth key at `mygeth-key.json`, then to import it to use `aleth`, simply use:

```
> aleth-key import mygeth-key.json "My Old Geth Key"
```

It will prompt you for your passphrase to ascertain the address for the key.


### Changing the Password

Security people reckon that it is prudent to change your password regularly. You can do so easily with `aleth-key` using the `recode` command (which actually does a whole lot more, but that's advanced usage).

To do so, simply pass in the name(s) of any keys whose passwords you wish to change. Let's change our key's password:

```
> aleth-key recode test
Enter old passphrase for key 'test' (hint: 321 backwards):
```

So it begins by asking for your key's old passphrase. Enter in the correct answer `123`.

It will then ask you for the new password (enter `321`) followed by a confirmation (enter the same) and a password hint (`123 backwards`). 

```
Enter new passphrase for key 'test': 
Please confirm the passphrase by entering it again: 
Enter a hint to help you remember this passphrase: 123 backwards
Re-encoded key 'test' successfully.
```

You'll finally get a confirmation that the re-encoding took place; your key is now encrypted by the new password.

## The Rest

There's much more to discover with `aleth-key`; it provides a suite of commands for playing with "bare" secrets, those not in the wallet (the `listbare`, `newbare`, ... commands), and allows keys to be imported without actually ever being decrypted (`importwithaddress`) and conversion between ICAP and hex (`inspectbare`).

Options allow you to alter transactions before you sign them and even create transactions from scratch. You can also configure the method by which keys are encrypted, changing the encryption function or its parameters.

See `aleth-key --help` for more information. Enjoy!

