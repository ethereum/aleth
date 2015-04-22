Qt.include("QEtherHelper.js")

function defaultTransaction()
{
	return {
		value: createEther("0", QEther.Wei),
		functionId: "",
		gas: createBigInt("250000"),
		gasAuto: true,
		gasPrice: createEther("100000", QEther.Wei),
		parameters: {},
		stdContract: false
	};
}

function rpcCall(requests, callBack)
{
	var jsonRpcUrl = "http://localhost:8080";
	var rpcRequest = JSON.stringify(requests);
	var httpRequest = new XMLHttpRequest();
	httpRequest.open("POST", jsonRpcUrl, true);
	httpRequest.setRequestHeader("Content-type", "application/json");
	httpRequest.setRequestHeader("Content-length", rpcRequest.length);
	httpRequest.setRequestHeader("Connection", "close");
	httpRequest.onreadystatechange = function() {
		if (httpRequest.readyState === XMLHttpRequest.DONE) {
			if (httpRequest.status !== 200 || httpRequest.responseText === "")
			{
				var errorText = qsTr("Unable to initiate request to the live network. Please verify your ethereum node is up.") + qsTr(" Error status: ")  + httpRequest.status;
				console.log(errorText);
				deploymentError(errorText);
			}
			else
				callBack(httpRequest.status, httpRequest.responseText)
		}
	}
	httpRequest.send(rpcRequest);
}

