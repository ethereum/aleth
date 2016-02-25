# Dapps deployment


This feature allows users to deploy the current project as a Dapp in the main blockchain.
This will deploy contracts and register frontend resources.

The deployment process includes three steps: 
 
 - **Deploy contract**:
This step will deploy contracts in the main blockchain.

 - **Package dapp**:
This step is used to package and upload frontend resources.

 - **Register**:
To render the Dapp, the Ethereum browser (Mist or AlethZero) needs to access this package. This step will register the URL where the resources are stored.

To Deploy your Dapp, Please follow these instructions:

Click on `Deploy`, `Deploy to Network`.
This modal dialog displays three parts (see above):
 
 - **Deploy contract**

 - *Select Scenario*

"Ethereum node URL" is the location where a node is running, there must be a node running in order to initiate deployment.

"Pick Scenario to deploy" is a mandatory step. Mix will execute transactions that are in the selected scenario (all transactions except transactions that are not related to contract creation or contract call). Mix will display all the transactions in the panel below with all associated input parameters.

"Gas Used": depending on the selected scenario, Mix will display the total gas used.

 - *Deploy Scenario*

"Deployment account" allow selecting the account that Mix will use to execute transactions.

"Gas Price" shows the default gas price of the network. You can also specify a different value. 

"Deployment cost": depending on the value of the gas price that you want to use and the selected scenario. this will display the amount ether that the deployment need.

"Deployed Contract": before any deployment this part is empty. This will be filled once the deployment is finished by all contract addresses that have been created.

"Verifications". This will shows the number of verifications (number of blocks generated on top of the last block which contains the last deployed transactions). Mix keep track of all the transactions. If one is missing (unvalidated) it will be displayed in this panel.  

- **Package dapp**

- *Generate local package*

The action "Generate Package" will create the package.dapp in the specified folder

"Local package Url" the content of this field can be pasted directly in AlethZero in order to use the dapp before uploading it.

- *Upload and share package*

This step has to be done outside of Mix. package.dapp file has to be hosted by a server in order to be available by all users.

"Copy Base64" will copy the base64 value of the package to the clipboard.

"Host in pastebin.com" will open pastebin.com in a browser (you can then host your package as base64).

- **Package dapp**

"Root Registrar address" is the account address of the root registrar contract

"Http URL" is the url where resources are hosted (pastebin.com or similar)

"Ethereum URL" is the url that users will use in AlethZero or Mist to access your dapp.

"Formatted Ethereum URL" is the url that users will use in AlethZero or Mist to access your dapp.

"Gas Price" shows the default gas price of the network. You can also specify a different value.

"Registration Cost" will display the amount of ether you need to register your dapp url.


