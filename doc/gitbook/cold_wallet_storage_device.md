# Cold Wallet Storage Device

A Cold Wallet Storage Device (CWSD) is a device (duh) used to store keys and sign transactions which never touches the internet, or indeed any communications channels excepting those solely for basic user interaction. The use of such a device is pretty much necessary for storing any large sum of value or other blockchain-based asset, promise or instrument. For example, a device like this has been used for operating blockchain-based keys worth many millions of dollars.

For this how-to, we'll assume that the CWSD is a simple Ubuntu-based computer (a netbook works pretty well) with TurboEthereum preinstalled as per the first chapter; I will assume that you've taken the proper precautions to avoid any malware getting on to the machine (though without an internet connection, there's not too much damage malware can realistically cause).

### Kill the network

The first thing to do is to make sure you've disabled any network connection, wireless or otherwise. Maybe compile a kernel without ICP/IP and Bluetooth, maybe just destroy or remove the network hardware of the computer. It is this precaution that puts the 'C' in CWSD.

### Generate the keys

The next thing to do is to generate the key (or keys) that this machine will store. Run `ethkey` to create a wallet and then again to make as many keys as you would like to use. You can always make more later. For now I'll make one:

```
> ethkey createwallet
Please enter a MASTER passphrase to protect your key store (make it strong!): password
Please confirm the passphrase by entering it again: password
> ethkey new supersecret
Enter a passphrase with which to secure this account (or nothing to use the master passphrase): password
Please confirm the passphrase by entering it again: password
Enter a hint to help you remember this passphrase: just 'password'
Created key 055dde03-47ff-dded-8950-0fe39b1fa101
  Name: supersecret
  Password hint: just 'password'
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

It will prompt for a password and confirmation for both commands. I'm just going to use the password "password" for both.

This "supersecret" key has an address of `XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ`. 

### Signing with the keys

Signing with the keys can happen in two ways: The first is to export a transaction to sign from e.g. AlethZero, perhaps saving to a USB pendrive. Let's assume that is what we have done and we have the hex-encoded transaction at `/mnt/paygav.tx`.

In order to sign this transaction we just need a single `ethkey` invocation:

```
> ethkey sign supersecret /tmp/paygav.tx
```

It will prompt you for the passphrase and finally place the signed hex in a file `/mnt/paygav.tx.signed`. Easy. If we just want to copy and paste the hex (we're too paranoid to use pen drives!) then we would just do:

```
> echo "<hex-encoded transaction here>" | ethkey sign supersecret
```

At which it will ask for your passphrase and spit out the hex of the signed transaction.

Alternatively, if we don't yet have an unsigned transaction, but we actually want to construct a transactions locally, we can do that too.

Let's assume our "supersecret" account has received some ether in the meantime and we want to pay somebody 2.1 grand of this ether (2100 ether for those not used to my English colloquialisms). That's easy, too.

```
> ethkey sign supersecret --tx-dest <destination address> --tx-gas 55000 --tx-gasprice 50000000000 --tx-value 2100000000000000000 --tx-nonce 0
```

Note the `--tx-value` (the amount to transfer) and the `--tx-gasprice` (the price we pay for a single unit of gas) must be specified in Wei, hence the large numbers there. `--tx-nonce` only needs to be specified if it's not the first transaction sent from this account.

### Importing the key

You may want to eventually import the key to your everyday device. This may be to use it directly there or simply to facilitate the creation of unsigned transactions for later signing on the CWSD. Assuming you have a strong passphrase, importing the key on to a hot device itself should not compromise the secret's safety too much (though obviously it's materially less secure than being on a physically isolated machine).

To do this, simply copy the JSON file(s) in your `~/.web3/keys` path to somewhere accessible on your other (non-CWSD) computer. Let's assume this other computer now has our "supersecret" key at `/mnt/supersecret.json`. There are two ways of importing it into your Ethereum wallet. The first is simplest:

```
> ethkey import /mnt/supersecret.json supersecret
Enter the passphrase for the key: password
Enter a hint to help you remember the key's passphrase: just 'password'
Imported key 055dde03-47ff-dded-8950-0fe39b1fa101
  Name: supersecret
  Password hint: just 'password'
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

A key can only be added to the wallet whose address is known; to figure out the address, `ethkey` will you to type your passphrase.

This is less than ideal since if the machine is actually compromised (perhaps with a keylogger), then an attacker could slurp up your passphrase and key JSON and be able to fraudulently use that account as they pleased. Ouch.

A more secure way, especially if you're not planning on using the key directly from this hot machine in the near future, is to provide the address manually on import. It won't ask you for the passphrase and thus potentially compromise the secret's integrity (assuming the machine is actually compromised in the first place!).

To do this, I would remember the "supersecret" account was `XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ` and tell `ethkey` as such while importing:

```
> ethkey importwithaddress XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ supersecret
Enter a hint to help you remember the key's passphrase: just 'password'
Imported key 055dde03-47ff-dded-8950-0fe39b1fa101
  Name: supersecret
  Password hint: just 'password'
  ICAP: XE472EVKU3CGMJF2YQ0J9RO1Y90BC0LDFZ
  Raw hex: 0092e965928626f8880629cec353d3fd7ca5974f
```

In both cases, we'll be able to see the key in e.g. AlethZero as one of our own, though we will not be able to sign with it without entering the passphrase. Assuming you never enter the passphrase on the hot machine (but rather do all signing on the CWSD) then you should be reasonably safe. Just be warned that the security of the secret is lieing on the network security of your hot machine *and* the strength of your key's passphrase. I really wouldn't count on the former.