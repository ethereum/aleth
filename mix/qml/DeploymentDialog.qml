import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.1
import QtQuick.Controls.Styles 1.3
import org.ethereum.qml.QEther 1.0
import "js/TransactionHelper.js" as TransactionHelper
import "js/ProjectModel.js" as ProjectModelCode
import "js/QEtherHelper.js" as QEtherHelper
import "."


Window {

	id: modalDeploymentDialog
	modality: Qt.ApplicationModal
	width: 735
	height: 320
	maximumWidth: width
	minimumWidth: width
	maximumHeight: height
	minimumHeight: height
	visible: false
	property alias applicationUrlEth: applicationUrlEth.text
	property alias applicationUrlHttp: applicationUrlHttp.text
	property string urlHintContract: urlHintAddr.text
	property string packageHash
	property string packageBase64
	property string eth: registrarAddr.text
	property string currentAccount
	property alias gasToUse: gasToUseInput.text

	color: Style.generic.layout.backgroundColor

	function close()
	{
		visible = false;
	}

	function open()
	{
		modalDeploymentDialog.setX((Screen.width - width) / 2);
		modalDeploymentDialog.setY((Screen.height - height) / 2);
		visible = true;

		var requests = [{
							//accounts
							jsonrpc: "2.0",
							method: "eth_accounts",
							params: null,
							id: 0
						}];

		TransactionHelper.rpcCall(requests, function(arg1, arg2)
		{
			modelAccounts.clear();
			var ids = JSON.parse(arg2)[0].result;
			requests = [];
			for (var k in ids)
			{
				modelAccounts.append({ "id": ids[k] })
				requests.push({
								  //accounts
								  jsonrpc: "2.0",
								  method: "eth_balanceAt",
								  params: [ids[k]],
								  id: k
							  });
			}

			if (ids.length > 0)
				currentAccount = modelAccounts.get(0).id;

			TransactionHelper.rpcCall(requests, function (request, response){
				var balanceRet = JSON.parse(response);
				for (var k in balanceRet)
				{
					var ether = QEtherHelper.createEther(balanceRet[k].result, QEther.Wei);
					comboAccounts.balances.push(ether.format());
				}
				balance.text = comboAccounts.balances[0];
			});
		});
	}

	function stopForInputError(inError)
	{
		errorDialog.text = "";
		if (inError.length > 0)
		{
			errorDialog.text = qsTr("The length of a string cannot exceed 32 characters.\nPlease verify the following value(s):\n\n")
			for (var k in inError)
				errorDialog.text += inError[k] + "\n";
			errorDialog.open();
			return true;
		}
		return false;
	}

	function pad(h)
	{
		// TODO move this to QHashType class
		while (h.length < 64)
		{
			h = '0' + h;
		}
		return h;
	}

	function waitForTrCountToIncrement(callBack)
	{
		poolLog.callBack = callBack;
		poolLog.k = -1;
		poolLog.elapsed = 0;
		poolLog.start();
	}

	Timer
	{
		id: poolLog
		property var callBack
		property int k: -1
		property int elapsed
		interval: 500
		running: false
		repeat: true
		onTriggered: {
			elapsed += interval;
			var requests = [];
			var jsonRpcRequestId = 0;
			requests.push({
							  jsonrpc: "2.0",
							  method: "eth_countAt",
							  params: [ currentAccount ],
							  id: jsonRpcRequestId++
						  });
			TransactionHelper.rpcCall(requests, function (httpRequest, response){
				response = response.replace(/,0+/, ''); // ==> result:27,00000000
				var count = JSON.parse(response)[0].result
				if (k < parseInt(count) && k > 0)
				{
					stop();
					callBack(1);
				}
				else if (elapsed > 25000)
				{
					stop();
					callBack(-1);
				}
				else
					k = parseInt(JSON.parse(response)[0].result);
			})
		}
	}

	SourceSansProRegular
	{
		id: lightFont
	}

	Column
	{
		spacing: 5
		anchors.fill: parent
		anchors.margins: 10
		ColumnLayout
		{
			id: containerDeploy
			Layout.fillWidth: true
			Layout.preferredHeight: 500
			RowLayout
			{
				Rectangle
				{
					Layout.preferredWidth: 357
					DefaultLabel
					{
						text: qsTr("Deployment")
						font.family: lightFont.name
						font.underline: true
						anchors.centerIn: parent
					}
				}

				Button
				{
					action: displayHelpAction
					iconSource: "qrc:/qml/img/help.png"
				}

				Action {
					id: displayHelpAction
					tooltip: qsTr("Help")
					onTriggered: {
						Qt.openUrlExternally("https://github.com/ethereum/wiki/wiki/Mix:-The-DApp-IDE#deployment-to-network")
					}
				}

				Button
				{
					action: openFolderAction
					iconSource: "qrc:/qml/img/openedfolder.png"
				}

				Action {
					id: openFolderAction
					enabled: deploymentDialog.packageBase64 !== ""
					tooltip: qsTr("Open Package Folder")
					onTriggered: {
						fileIo.openFileBrowser(projectModel.deploymentDir);
					}
				}

				Button
				{
					action: b64Action
					iconSource: "qrc:/qml/img/b64.png"
				}

				Action {
					id: b64Action
					enabled: deploymentDialog.packageBase64 !== ""
					tooltip: qsTr("Copy Base64 conversion to ClipBoard")
					onTriggered: {
						clipboard.text = deploymentDialog.packageBase64;
					}
				}

				Button
				{
					action: exitAction
					iconSource: "qrc:/qml/img/exit.png"
				}

				Action {
					id: exitAction
					tooltip: qsTr("Exit")
					onTriggered: {
						close()
					}
				}
			}

			GridLayout
			{
				columns: 2
				width: parent.width

				DefaultLabel
				{
					text: qsTr("Root Registrar address:")
				}

				DefaultTextField
				{
					Layout.preferredWidth: 350
					id: registrarAddr
				}

				DefaultLabel
				{
					text: qsTr("Account used to deploy:")
				}

				Rectangle
				{
					width: 300
					height: 25
					color: "transparent"
					ComboBox {
						id: comboAccounts
						property var balances: []
						onCurrentIndexChanged : {
							if (modelAccounts.count > 0)
							{
								currentAccount = modelAccounts.get(currentIndex).id;
								balance.text = balances[currentIndex];
							}
						}
						model: ListModel {
							id: modelAccounts
						}
					}

					DefaultLabel
					{
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: comboAccounts.right
						anchors.leftMargin: 20
						id: balance;
					}
				}

				DefaultLabel
				{
					text: qsTr("Amount of gas to use for each contract deployment: ")
				}

				DefaultTextField
				{
					text: "20000"
					Layout.preferredWidth: 350
					id: gasToUseInput
				}

				DefaultLabel
				{
					text: qsTr("Ethereum Application URL: ")
				}

				Rectangle
				{
					Layout.fillWidth: true
					height: 25
					color: "transparent"
					DefaultTextField
					{
						width: 200
						id: applicationUrlEth
						onTextChanged: {
							appUrlFormatted.text = ProjectModelCode.formatAppUrl(text).join('/');
						}
					}

					DefaultLabel
					{
						id: appUrlFormatted
						anchors.verticalCenter: parent.verticalCenter;
						anchors.left: applicationUrlEth.right
						font.italic: true
						font.pointSize: Style.absoluteSize(-1)
					}
				}
			}

			RowLayout
			{
				Layout.fillWidth: true
				Rectangle
				{
					Layout.preferredWidth: 357
					color: "transparent"
				}

				Button
				{
					id: deployButton
					action: runAction
					iconSource: "qrc:/qml/img/run.png"
				}

				Action {
					id: runAction
					tooltip: qsTr("Deploy contract(s) and Package resources files.")
					onTriggered: {
						var inError = [];
						var ethUrl = ProjectModelCode.formatAppUrl(applicationUrlEth.text);
						for (var k in ethUrl)
						{
							if (ethUrl[k].length > 32)
								inError.push(qsTr("Member too long: " + ethUrl[k]) + "\n");
						}
						if (!stopForInputError(inError))
						{
							if (contractRedeploy.checked)
								deployWarningDialog.open();
							else
								ProjectModelCode.startDeployProject(false);
						}
					}
				}

				CheckBox
				{
					anchors.left: deployButton.right
					id: contractRedeploy
					enabled: Object.keys(projectModel.deploymentAddresses).length > 0
					checked: Object.keys(projectModel.deploymentAddresses).length == 0
					text: qsTr("Deploy Contract(s)")
					anchors.verticalCenter: parent.verticalCenter
				}
			}
		}

		Rectangle
		{
			width: parent.width
			height: 1
			color: "#5891d3"
		}

		ColumnLayout
		{
			id: containerRegister
			Layout.fillWidth: true
			Layout.preferredHeight: 500
			RowLayout
			{
				Layout.preferredHeight: 25
				Rectangle
				{
					Layout.preferredWidth: 356
					DefaultLabel
					{
						text: qsTr("Registration")
						font.family: lightFont.name
						font.underline: true
						anchors.centerIn: parent
					}
				}
			}

			GridLayout
			{
				columns: 2
				Layout.fillWidth: true

				DefaultLabel
				{
					Layout.preferredWidth: 355
					text: qsTr("URL Hint contract address:")
				}

				DefaultTextField
				{
					Layout.preferredWidth: 350
					id: urlHintAddr
					enabled: rowRegister.isOkToRegister()
				}

				DefaultLabel
				{
					Layout.preferredWidth: 355
					text: qsTr("Web Application Resources URL: ")
				}

				DefaultTextField
				{
					Layout.preferredWidth: 350
					id: applicationUrlHttp
					enabled: rowRegister.isOkToRegister()
				}
			}

			RowLayout
			{
				id: rowRegister
				Layout.fillWidth: true

				Rectangle
				{
					Layout.preferredWidth: 357
					color: "transparent"
				}

				function isOkToRegister()
				{
					return Object.keys(projectModel.deploymentAddresses).length > 0 && deploymentDialog.packageHash !== "";
				}

				Button {
					action: registerAction
					iconSource: "qrc:/qml/img/note.png"
				}

				Action {
					id: registerAction
					enabled: rowRegister.isOkToRegister()
					tooltip: qsTr("Register hosted Web Application.")
					onTriggered: {
						if (applicationUrlHttp.text === "" || deploymentDialog.packageHash === "")
						{
							deployDialog.title = text;
							deployDialog.text = qsTr("Please provide the link where the resources are stored and ensure the package is aleary built using the deployment step.")
							deployDialog.open();
							return;
						}
						var inError = [];
						if (applicationUrlHttp.text.length > 32)
							inError.push(qsTr(applicationUrlHttp.text));
						if (!stopForInputError(inError))
							ProjectModelCode.registerToUrlHint();
					}
				}
			}
		}
	}

	MessageDialog {
		id: deployDialog
		standardButtons: StandardButton.Ok
		icon: StandardIcon.Warning
	}

	MessageDialog {
		id: errorDialog
		standardButtons: StandardButton.Ok
		icon: StandardIcon.Critical
	}
}
