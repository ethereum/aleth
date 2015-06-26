/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file NetworkDeployment.js
 * @author Arkadiy Paronyan arkadiy@ethdev.com
 * @author Yann yann@ethdev.com
 * @date 2015
 * Ethereum IDE client.
 */
.import org.ethereum.qml.QSolidityType 1.0 as QSolidityType

Qt.include("TransactionHelper.js")
Qt.include("QEtherHelper.js")


var jsonRpcRequestId = 1;
function deployProject(force) {
	saveAll(); //TODO: ask user
	deploymentDialog.open();
}

function startDeployProject(erasePrevious)
{
	var date = new Date();
	var deploymentId = date.toLocaleString(Qt.locale(), "ddMMyyHHmmsszzz");
	if (!erasePrevious)
	{
		finalizeDeployment(deploymentId, projectModel.deploymentAddresses);
		return;
	}

	var jsonRpcUrl = "http://127.0.0.1:8080";
	console.log("Deploying " + deploymentId + " to " + jsonRpcUrl);
	deploymentStarted();

	var ctrAddresses = {};
	var state = retrieveState(projectModel.deployedState);
	console.log(JSON.stringify(state));
	if (!state)
	{
		var txt = qsTr("Unable to find state " + projectModel.deployedState);
		deploymentError(txt);
		console.log(txt);
		return;
	}
	executeTr(0, state, ctrAddresses, function (){
		finalizeDeployment(deploymentId, ctrAddresses);
	});
}

function checkPathCreationCost(callBack)
{
	var dappUrl = formatAppUrl(deploymentDialog.applicationUrlEth);
	checkEthPath(dappUrl, true, function(success, cause) {
		if (!success)
		{
			switch (cause)
			{
			case "rootownedregistrar_notexist":
				deploymentError(qsTr("Owned registrar does not exist under the global registrar. Please create one using DApp registration."));
				break;
			case "ownedregistrar_creationfailed":
				deploymentError(qsTr("The creation of your new owned registrar fails. Please use DApp registration to create one."));
				break;
			case "ownedregistrar_notowner":
				deploymentError(qsTr("You are not the owner of this registrar. You cannot register your Dapp here."));
				break;
			default:
				break;
			}
		}
		else
		{
			deploymentStepChanged(qsTr("Your Dapp can be registered here."));
			callBack((dappUrl.length - 1) * (deploymentDialog.ownedRegistrarDeployGas + deploymentDialog.ownedRegistrarSetSubRegistrarGas) + deploymentDialog.ownedRegistrarSetContentHashGas);
		}
	});
}

function gasUsed()
{
	var gas = 0;
	var gasCosts = clientModel.gasCosts;
	for (var g in gasCosts)
	{
		gas += gasCosts[g];
		console.log(" gasCost " + gasCosts[g]);
	}
	return gas;
}

function retrieveState(state)
{
	for (var k = 0; k < projectModel.stateListModel.count; k++)
	{
		if (projectModel.stateListModel.get(k).title === state)
			return projectModel.stateListModel.get(k);
	}
	return null;
}

function replaceParamToken(paramsDef, params, ctrAddresses)
{
	var retParams = {};
	for (var k in paramsDef)
	{
		var value = "";
		if (params[paramsDef[k].name] !== undefined)
		{
			value = params[paramsDef[k].name];
			if (paramsDef[k].type.category === 4 && value.indexOf("<") === 0)
			{
				value = value.replace("<", "").replace(">", "");
				value = ctrAddresses[value];
			}
		}
		retParams[paramsDef[k].name] = value;
	}
	return retParams;
}

function getFunction(ctrName, functionId)
{
	if (codeModel.contracts[ctrName] === undefined)
		return null;
	if (ctrName === functionId)
		return codeModel.contracts[ctrName].contract.constructor;
	else
	{
		for (var j in codeModel.contracts[ctrName].contract.functions)
		{
			if (codeModel.contracts[ctrName].contract.functions[j].name === functionId)
				return codeModel.contracts[ctrName].contract.functions[j];
		}
	}
}

function executeTr(trIndex, state, ctrAddresses, callBack)
{
	var tr = state.transactions.get(trIndex);
	var func = getFunction(tr.contractId, tr.functionId);
	if (!func)
		executeTrNextStep(trIndex, state, ctrAddresses, callBack);
	else
	{
		var gasCost = clientModel.toHex(clientModel.gasCosts[trIndex]);
		var rpcParams = { "from": deploymentDialog.currentAccount, "gas": "0x" + gasCost };
		var params = replaceParamToken(func.parameters, tr.parameters, ctrAddresses);
		var encodedParams = clientModel.encodeParams(params, tr.contractId, tr.functionId);

		if (state.contractId === state.functionId)
			rpcParams.code = codeModel.contracts[tr.contractId].codeHex + encodedParams.join("");
		else
			rpcParams.data = func.hash + encodedParams.join("");

		var requests = [{
							jsonrpc: "2.0",
							method: "eth_sendTransaction",
							params: [ rpcParams ],
							id: jsonRpcRequestId
						}];

		rpcCall(requests, function (httpCall, response){
			var txt = qsTr(tr.contractId + "." + tr.functionId + "() ...")
			deploymentStepChanged(txt);
			console.log(txt);
			if (tr.contractId === tr.functionId)
			{
				ctrAddresses[tr.contractId] = JSON.parse(response)[0].result
				ctrAddresses[tr.contractId + " - " + trIndex] = JSON.parse(response)[0].result //get right ctr address if deploy more than one contract of same type.
			}
			deploymentDialog.waitForTrCountToIncrement(function(status) {
				if (status === -1)
					trCountIncrementTimeOut();
				else
					executeTrNextStep(trIndex, state, ctrAddresses, callBack)
			});
		});
	}
}

function executeTrNextStep(trIndex, state, ctrAddresses, callBack)
{
	trIndex++;
	if (trIndex < state.transactions.count)
		executeTr(trIndex, state, ctrAddresses, callBack);
	else
		callBack();
}

function gasPrice(callBack, error)
{
	var requests = [{
						jsonrpc: "2.0",
						method: "eth_gasPrice",
						params: [],
						id: jsonRpcRequestId
					}];
	rpcCall(requests, function (httpCall, response){
		callBack(JSON.parse(response)[0].result)
	}, function(message){
		error(message)
	});
}

function finalizeDeployment(deploymentId, addresses) {
	deploymentStepChanged(qsTr("Packaging application ..."));
	var deploymentDir = projectPath + deploymentId + "/";
	projectModel.deploymentDir = deploymentDir;
	fileIo.makeDir(deploymentDir);
	for (var i = 0; i < projectListModel.count; i++) {
		var doc = projectListModel.get(i);
		if (doc.isContract)
			continue;
		if (doc.isHtml) {
			//inject the script to access contract API
			//TODO: use a template
			var html = fileIo.readFile(doc.path);
			var insertAt = html.indexOf("<head>")
			if (insertAt < 0)
				insertAt = 0;
			else
				insertAt += 6;
			html = html.substr(0, insertAt) +
					"<script src=\"deployment.js\"></script>" +
					html.substr(insertAt);
			fileIo.writeFile(deploymentDir + doc.fileName, html);
		}
		else
			fileIo.copyFile(doc.path, deploymentDir + doc.fileName);
	}
	//write deployment js
	var deploymentJs =
			"// Autogenerated by Mix\n" +
			"contracts = {};\n";
	for (var c in codeModel.contracts)  {
		var contractAccessor = "contracts[\"" + codeModel.contracts[c].contract.name + "\"]";
		deploymentJs += contractAccessor + " = {\n" +
				"\tinterface: " + codeModel.contracts[c].contractInterface + ",\n" +
				"\taddress: \"" + addresses[c] + "\"\n" +
				"};\n" +
				contractAccessor + ".contractClass = web3.eth.contract(" + contractAccessor + ".interface);\n" +
				contractAccessor + ".contract = new " + contractAccessor + ".contractClass(" + contractAccessor + ".address);\n";
	}
	fileIo.writeFile(deploymentDir + "deployment.js", deploymentJs);
	deploymentAddresses = addresses;
	saveProject();

	var packageRet = fileIo.makePackage(deploymentDir);
	deploymentDialog.packageHash = packageRet[0];
	deploymentDialog.packageBase64 = packageRet[1];
	deploymentDialog.localPackageUrl = packageRet[2] + "?hash=" + packageRet[0];

	var applicationUrlEth = deploymentDialog.applicationUrlEth;

	applicationUrlEth = formatAppUrl(applicationUrlEth);
	deploymentStepChanged(qsTr("Registering application on the Ethereum network ..."));
	checkEthPath(applicationUrlEth, false, function (success) {
		if (!success)
			return;
		deploymentComplete();
		deployResourcesDialog.text = qsTr("Register Web Application to finalize deployment.");
		deployResourcesDialog.open();
	});
}

function checkEthPath(dappUrl, checkOnly, callBack)
{
	if (dappUrl.length === 1)
	{
		// convenient for dev purpose, should not be possible in normal env.
		if (!checkOnly)
			reserve(deploymentDialog.eth, function() {
				registerContentHash(deploymentDialog.eth, callBack); // we directly create a dapp under the root registrar.
			});
		else
			callBack(true);
	}
	else
	{
		// the first owned registrar must have been created to follow the path.
		var str = clientModel.encodeStringParam(dappUrl[0]);
		var requests = [];
		requests.push({
						  //subRegistrar()
						  jsonrpc: "2.0",
						  method: "eth_call",
						  params: [ { "gas": "0xffff", "from": deploymentDialog.currentAccount,  "to": '0x' + deploymentDialog.eth, "data": "0xe1fa8e84" + str }, "pending" ],
						  id: jsonRpcRequestId++
					  });
		rpcCall(requests, function (httpRequest, response) {
			var res = JSON.parse(response);
			var addr = normalizeAddress(res[0].result);
			if (addr.replace(/0+/g, "") === "")
			{
				var errorTxt = qsTr("Path does not exists " + JSON.stringify(dappUrl) + ". Please register using Registration Dapp. Aborting.");
				deploymentError(errorTxt);
				console.log(errorTxt);
				callBack(false, "rootownedregistrar_notexist");
			}
			else
			{
				dappUrl.splice(0, 1);
				checkRegistration(dappUrl, addr, callBack, checkOnly);
			}
		});
	}
}

function isOwner(addr, callBack)
{
	var requests = [];
	requests.push({
					  //getOwner()
					  jsonrpc: "2.0",
					  method: "eth_call",
					  params: [ { "from": deploymentDialog.currentAccount, "to": '0x' + addr, "data": "0xb387ef92" }, "pending" ],
					  id: jsonRpcRequestId++
				  });
	rpcCall(requests, function (httpRequest, response) {
		var res = JSON.parse(response);
		callBack(normalizeAddress(deploymentDialog.currentAccount) === normalizeAddress(res[0].result));
	});
}

function checkRegistration(dappUrl, addr, callBack, checkOnly)
{
	isOwner(addr, function(ret){
		if (!ret)
		{
			var errorTxt = qsTr("You are not the owner of " + dappUrl[0] + ". Aborting");
			deploymentError(errorTxt);
			console.log(errorTxt);
			callBack(false, "ownedregistrar_notowner");
		}
		else
			continueRegistration(dappUrl, addr, callBack, checkOnly);
	});
}

function continueRegistration(dappUrl, addr, callBack, checkOnly)
{
	if (dappUrl.length === 1)
	{
		if (!checkOnly)
			registerContentHash(addr, callBack); // We do not create the register for the last part, just registering the content hash.
		else
			callBack(true);
	}
	else
	{
		var txt = qsTr("Checking " + JSON.stringify(dappUrl));
		deploymentStepChanged(txt);
		console.log(txt);
		var requests = [];
		var registrar = {}
		var str = clientModel.encodeStringParam(dappUrl[0]);


		requests.push({
						  //register()
						  jsonrpc: "2.0",
						  method: "eth_call",
						  params: [ { "from": deploymentDialog.currentAccount, "to": '0x' + addr, "data": "0x5a3a05bd"  + str }, "pending" ],
						  id: jsonRpcRequestId++
					  });

		rpcCall(requests, function (httpRequest, response) {
			var res = JSON.parse(response);
			var nextAddr = normalizeAddress(res[0].result);
			var errorTxt;
			if (res[0].result === "0x")
			{
				errorTxt = qsTr("Error when creating new owned registrar. Please use the registration Dapp. Aborting");
				deploymentError(errorTxt);
				console.log(errorTxt);
				callBack(false, "ownedregistrar_creationfailed");
			}
			else if (nextAddr.replace(/0+/g, "") !== "")
			{
				dappUrl.splice(0, 1);
				checkRegistration(dappUrl, nextAddr, callBack, checkOnly);
			}
			else
			{
				if (checkOnly)
				{
					callBack(true);
					return;
				}
				var txt = qsTr("Registering sub domain " + dappUrl[0] +  " ...");
				console.log(txt);
				deploymentStepChanged(txt);
				//current registrar is owned => ownedregistrar creation and continue.
				requests = [];
				var gasCost = clientModel.toHex(deploymentDialog.ownedRegistrarDeployGas);
				requests.push({
								  jsonrpc: "2.0",
								  method: "eth_sendTransaction",
								  params: [ { "from": deploymentDialog.currentAccount, "gas": "0x" + gasCost, "code": "0x600080547fffffffffffffffffffffffff000000000000000000000000000000000000000016331781556105cd90819061003990396000f3007c010000000000000000000000000000000000000000000000000000000060003504630198489281146100b257806321f8a721146100e45780632dff6941146100ee5780633b3b57de1461010e5780635a3a05bd1461013e5780635fd4b08a146101715780637dd564111461017d57806389a69c0e14610187578063b387ef92146101bb578063b5c645bd146101f4578063be99a98014610270578063c3d014d6146102a8578063d93e7573146102dc57005b73ffffffffffffffffffffffffffffffffffffffff600435166000908152600160205260409020548060005260206000f35b6000808052602081f35b600435600090815260026020819052604090912001548060005260206000f35b600435600090815260026020908152604082205473ffffffffffffffffffffffffffffffffffffffff1680835291f35b600435600090815260026020908152604082206001015473ffffffffffffffffffffffffffffffffffffffff1680835291f35b60008060005260206000f35b6000808052602081f35b60005461030c9060043590602435903373ffffffffffffffffffffffffffffffffffffffff908116911614610569576105c9565b60005473ffffffffffffffffffffffffffffffffffffffff168073ffffffffffffffffffffffffffffffffffffffff1660005260206000f35b600435600090815260026020819052604090912080546001820154919092015473ffffffffffffffffffffffffffffffffffffffff9283169291909116908273ffffffffffffffffffffffffffffffffffffffff166000528173ffffffffffffffffffffffffffffffffffffffff166020528060405260606000f35b600054610312906004359060243590604435903373ffffffffffffffffffffffffffffffffffffffff90811691161461045457610523565b6000546103189060043590602435903373ffffffffffffffffffffffffffffffffffffffff90811691161461052857610565565b60005461031e90600435903373ffffffffffffffffffffffffffffffffffffffff90811691161461032457610451565b60006000f35b60006000f35b60006000f35b60006000f35b60008181526002602090815260408083205473ffffffffffffffffffffffffffffffffffffffff16835260019091529020548114610361576103e1565b6000818152600260205260408082205473ffffffffffffffffffffffffffffffffffffffff169183917ff63780e752c6a54a94fc52715dbc5518a3b4c3c2833d301a204226548a2a85459190a360008181526002602090815260408083205473ffffffffffffffffffffffffffffffffffffffff16835260019091528120555b600081815260026020819052604080832080547fffffffffffffffffffffffff00000000000000000000000000000000000000009081168255600182018054909116905590910182905582917fa6697e974e6a320f454390be03f74955e8978f1a6971ea6730542e37b66179bc91a25b50565b600083815260026020526040902080547fffffffffffffffffffffffff00000000000000000000000000000000000000001683179055806104bb57827fa6697e974e6a320f454390be03f74955e8978f1a6971ea6730542e37b66179bc60006040a2610522565b73ffffffffffffffffffffffffffffffffffffffff8216837ff63780e752c6a54a94fc52715dbc5518a3b4c3c2833d301a204226548a2a854560006040a373ffffffffffffffffffffffffffffffffffffffff821660009081526001602052604090208390555b5b505050565b600082815260026020819052604080832090910183905583917fa6697e974e6a320f454390be03f74955e8978f1a6971ea6730542e37b66179bc91a25b5050565b60008281526002602052604080822060010180547fffffffffffffffffffffffff0000000000000000000000000000000000000000168417905583917fa6697e974e6a320f454390be03f74955e8978f1a6971ea6730542e37b66179bc91a25b505056" } ],
								  id: jsonRpcRequestId++
							  });

				rpcCall(requests, function(httpRequest, response) {
					var newCtrAddress = normalizeAddress(JSON.parse(response)[0].result);
					requests = [];
					var txt = qsTr("Please wait " + dappUrl[0] + " is registering ...");
					deploymentStepChanged(txt);
					console.log(txt);
					deploymentDialog.waitForTrCountToIncrement(function(status) {
						if (status === -1)
						{
							trCountIncrementTimeOut();
							return;
						}
						var crLevel = clientModel.encodeStringParam(dappUrl[0]);
						var gasCost = clientModel.toHex(deploymentDialog.ownedRegistrarSetSubRegistrarGas);
						requests.push({
										  //setRegister()
										  jsonrpc: "2.0",
										  method: "eth_sendTransaction",
										  params: [ { "from": deploymentDialog.currentAccount, "gas": "0x" + gasCost, "to": '0x' + addr, "data": "0x89a69c0e" + crLevel + deploymentDialog.pad(newCtrAddress) } ],
										  id: jsonRpcRequestId++
									  });

						rpcCall(requests, function(request, response){
							dappUrl.splice(0, 1);
							checkRegistration(dappUrl, newCtrAddress, callBack);
						});
					});
				});
			}
		});
	}
}

function trCountIncrementTimeOut()
{
	var error = qsTr("Something went wrong during the deployment. Please verify the amount of gas for this transaction and check your balance.")
	console.log(error);
	deploymentError(error);
}

function reserve(registrar, callBack)
{
	var txt = qsTr("Making reservation in the root registrar...");
	deploymentStepChanged(txt);
	console.log(txt);
	var requests = [];
	var paramTitle = clientModel.encodeStringParam(projectModel.projectTitle);
	requests.push({
				  //reserve()
				  jsonrpc: "2.0",
				  method: "eth_sendTransaction",
				  params: [ { "from": deploymentDialog.currentAccount, "gas": "0xfffff", "to": '0x' + registrar, "data": "0x432ced04" + paramTitle } ],
				  id: jsonRpcRequestId++
			  });
	rpcCall(requests, function (httpRequest, response) {
		callBack();
	});
}


function registerContentHash(registrar, callBack)
{
	var txt = qsTr("Finalizing Dapp registration ...");
	deploymentStepChanged(txt);
	console.log(txt);
	var requests = [];
	var paramTitle = clientModel.encodeStringParam(projectModel.projectTitle);
	var gasCost = clientModel.toHex(deploymentDialog.ownedRegistrarSetContentHashGas);
	requests.push({
					  //setContent()
					  jsonrpc: "2.0",
					  method: "eth_sendTransaction",
					  params: [ { "from": deploymentDialog.currentAccount, "gas": "0x" + gasCost, "to": '0x' + registrar, "data": "0xc3d014d6" + paramTitle + deploymentDialog.packageHash } ],
					  id: jsonRpcRequestId++
				  });
	rpcCall(requests, function (httpRequest, response) {
		callBack(true);
	});
}

function registerToUrlHint()
{
	deploymentStepChanged(qsTr("Registering application Resources (" + deploymentDialog.applicationUrlHttp) + ") ...");

	urlHintAddress(function(urlHint){
		var requests = [];
		var paramUrlHttp = clientModel.encodeStringParam(deploymentDialog.applicationUrlHttp);
		var gasCost = clientModel.toHex(deploymentDialog.urlHintSuggestUrlGas);
		requests.push({
						  //urlHint => suggestUrl
						  jsonrpc: "2.0",
						  method: "eth_sendTransaction",
						  params: [ {  "to": '0x' + urlHint, "from": deploymentDialog.currentAccount, "gas": "0x" + gasCost, "data": "0x584e86ad" + deploymentDialog.packageHash + paramUrlHttp } ],
						  id: jsonRpcRequestId++
					  });

		rpcCall(requests, function (httpRequest, response) {
			deploymentComplete();
		});
	});
}

function urlHintAddress(callBack)
{
	var requests = [];
	var urlHint = clientModel.encodeStringParam("urlhint");
	requests.push({
					  //registrar: get UrlHint addr
					  jsonrpc: "2.0",
					  method: "eth_call",
					  params: [ {  "to": '0x' + deploymentDialog.eth, "from": deploymentDialog.currentAccount, "data": "0x3b3b57de" + urlHint }, "pending" ],
					  id: jsonRpcRequestId++
				  });

	rpcCall(requests, function (httpRequest, response) {
		var res = JSON.parse(response);
		callBack(normalizeAddress(res[0].result));
	});
}

function normalizeAddress(addr)
{
	addr = addr.replace('0x', '');
	if (addr.length <= 40)
		return addr;
	var left = addr.length - 40;
	return addr.substring(left);
}

function formatAppUrl(url)
{
	if (url.toLowerCase().lastIndexOf("/") === url.length - 1)
		url = url.substring(0, url.length - 1);
	if (url.toLowerCase().indexOf("eth://") === 0)
		url = url.substring(6);
	if (url.toLowerCase().indexOf(projectModel.projectTitle + ".") === 0)
		url = url.substring(projectModel.projectTitle.length + 1);
	if (url === "")
		return [projectModel.projectTitle];

	var ret;
	if (url.indexOf("/") === -1)
		ret = url.split('.').reverse();
	else
	{
		var slash = url.indexOf("/");
		var left = url.substring(0, slash);
		var leftA = left.split(".");
		leftA.reverse();

		var right = url.substring(slash + 1);
		var rightA = right.split('/');
		ret = leftA.concat(rightA);
	}
	if (ret[0].toLowerCase() === "eth")
		ret.splice(0, 1);
	ret.push(projectModel.projectTitle);
	return ret;
}
