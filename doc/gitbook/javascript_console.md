# JavaScript console

Mix exposes the following objects into the global window context

web3 - Ethereum JavaScript API

contracts: A collection of contract objects. A key to the collection is the contract name. A value is an object with the following properties:

contract: contract object instance (created as in web3.eth.contract)

address: contract address from the last deployed state (see below)

interface: contract ABI

Check the JavaScript API Reference for further information.

###Using the JS console to add transactions and local calls

In case the name of the contract is "Sample" with a function named "set", it is possible to make a transaction to call "set" by writing:

    contracts["Sample"].contract.set(14)

If a call can be made this will be done by writing:

    contracts["Sample"].contract.get.call()
    

It also possible to use all properties and function of the web3 object:
https://github.com/ethereum/wiki/wiki/JavaScript-API




