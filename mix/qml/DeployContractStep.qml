import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.3
import Qt.labs.settings 1.0
import "js/TransactionHelper.js" as TransactionHelper
import "js/NetworkDeployment.js" as NetworkDeploymentCode
import "js/QEtherHelper.js" as QEtherHelper
import org.ethereum.qml.QEther 1.0

Rectangle {
	property variant paramsModel: []
	property variant worker
	property variant gas: []
	color: "#E3E3E3E3"
	anchors.fill: parent
	id: root

	property int labelWidth: 150

	function show()
	{
		visible = true
		contractList.currentIndex = 0
		contractList.change()
		accountsModel.clear()
		for (var k in worker.accounts)
		{
			accountsModel.append(worker.accounts[k])
		}

		if (worker.accounts.length > 0)
			worker.currentAccount = worker.accounts[0].id

		if (projectModel.deployBlockNumber !== -1)
		{
			worker.verifyHashes(projectModel.deploymentTrHashes, function (bn, trLost)
			{
				root.updateVerification(bn, trLost)
			});
		}
		deployedAddresses.refresh()
		worker.renewCtx()
	}

	function updateVerification(blockNumber, trLost)
	{
		verificationLabel.text = blockNumber - projectModel.deployBlockNumber
		if (trLost.length > 0)
		{
			verificationLabel.text += "\n" + qsTr("Transactions lost") + "\n"
			for (var k in trLost)
			{
				verificationLabel.text += trLost[k] + "\n"
			}
		}
	}

	RowLayout
	{
		anchors.fill: parent
		anchors.margins: 10
		ColumnLayout
		{
			anchors.top: parent.top
			Layout.preferredWidth: parent.width * 0.40 - 20
			Layout.fillHeight: true
			id: scenarioList

			Label
			{
				Layout.fillWidth: true
				text: qsTr("Pick Scenario to deploy")
			}

			ComboBox
			{
				id: contractList
				Layout.preferredWidth: parent.width - 20
				model: projectModel.stateListModel
				textRole: "title"
				onCurrentIndexChanged:
				{
					if (root.visible)
						change()
				}

				function change()
				{
					trListModel.clear()
					if (currentIndex > -1)
					{
						for (var k = 0; k < projectModel.stateListModel.get(currentIndex).blocks.count; k++)
						{
							for (var j = 0; j < projectModel.stateListModel.get(currentIndex).blocks.get(k).transactions.count; j++)
							{
								trListModel.append(projectModel.stateListModel.get(currentIndex).blocks.get(k).transactions.get(j));
							}
						}
						for (var k = 0; k < trListModel.count; k++)
						{
							trList.itemAt(k).init()
						}
						ctrDeployCtrLabel.calculateContractDeployGas();
					}
				}
			}

			Rectangle
			{
				Layout.fillHeight: true
				Layout.preferredWidth: parent.width - 20
				id: trContainer
				color: "white"
				border.color: "#cccccc"
				border.width: 1
				ScrollView
				{
					anchors.fill: parent
					ColumnLayout
					{
						spacing: 0
						ListModel
						{
							id: trListModel
						}

						Repeater
						{
							id: trList
							model: trListModel
							ColumnLayout
							{
								Layout.fillWidth: true
								spacing: 5
								Layout.preferredHeight:
								{
									if (index > -1)
										return 20 + trListModel.get(index)["parameters"].count * 20
									else
										return 20
								}

								function init()
								{
									paramList.clear()
									if (trListModel.get(index).parameters)
									{
										for (var k in trListModel.get(index).parameters)
										{
											paramList.append({ "name": k, "value": trListModel.get(index).parameters[k] })
										}
									}
								}

								Label
								{
									id: trLabel
									Layout.preferredHeight: 20
									anchors.left: parent.left
									anchors.top: parent.top
									anchors.topMargin: 5
									anchors.leftMargin: 10
									text:
									{
										if (index > -1)
											return trListModel.get(index).label
										else
											return ""
									}
								}

								ListModel
								{
									id: paramList
								}

								Repeater
								{
									Layout.preferredHeight:
									{
										if (index > -1)
											return trListModel.get(index)["parameters"].count * 20
										else
											return 0
									}
									model: paramList
									Label
									{
										Layout.preferredHeight: 20
										anchors.left: parent.left
										anchors.leftMargin: 20
										text: name + "=" + value
										font.italic: true
									}
								}
							}
						}
					}
				}
			}
		}


		ColumnLayout
		{
			anchors.top: parent.top
			Layout.preferredHeight: parent.height - 25
			ColumnLayout
			{
				anchors.top: parent.top
				Layout.preferredWidth: parent.width * 0.60
				Layout.fillHeight: true
				id: deploymentOption
				spacing: 8

				Label
				{
					anchors.left: parent.left
					anchors.leftMargin: 105
					text: qsTr("Deployment options")
				}

				ListModel
				{
					id: accountsModel
				}

				RowLayout
				{
					Layout.fillWidth: true
					Rectangle
					{
						width: labelWidth
						Label
						{
							text: qsTr("Account")
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
						}
					}

					ComboBox
					{
						id: accountsList
						textRole: "id"
						model: accountsModel
						Layout.preferredWidth: 235
						onCurrentTextChanged:
						{
							worker.currentAccount = currentText
							accountBalance.text = worker.balance(currentText).format()
							console.log(worker.balance(currentText).format())
						}
					}

					Label
					{
						id: accountBalance
					}
				}

				RowLayout
				{
					Layout.fillWidth: true
					Rectangle
					{
						width: labelWidth
						Label
						{
							text: qsTr("Gas Price")
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
						}
					}

					Ether
					{
						id: gasPriceInput
						displayUnitSelection: true
						displayFormattedValue: true
						edit: true
					}

					Connections
					{
						target: gasPriceInput
						onValueChanged:
						{
							ctrDeployCtrLabel.calculateContractDeployGas()
						}
						onAmountChanged:
						{
							ctrDeployCtrLabel.setCost()
						}
						onUnitChanged:
						{
							ctrDeployCtrLabel.setCost()
						}
					}

					Connections
					{
						target: worker
						id: gasPriceLoad
						property bool loaded: false
						onGasPriceLoaded:
						{
							gasPriceInput.value = QEtherHelper.createEther(worker.gasPriceInt.value(), QEther.Wei)
							gasPriceLoad.loaded = true
							ctrDeployCtrLabel.calculateContractDeployGas()
						}
					}
				}

				RowLayout
				{
					id: ctrDeployCtrLabel
					Layout.fillWidth: true
					property int cost
					function calculateContractDeployGas()
					{
						if (!root.visible)
							return;
						var sce = projectModel.stateListModel.getState(contractList.currentIndex)
						worker.estimateGas(sce, function(gas) {
							if (gasPriceLoad.loaded)
							{
								root.gas = gas
								cost = 0
								for (var k in gas)
								{
									cost += gas[k]
								}
								setCost()
							}
						});
					}

					function setCost()
					{
						var ether = QEtherHelper.createBigInt(cost);
						var gasTotal = ether.multiply(gasPriceInput.value);
						gasToUseInput.value = QEtherHelper.createEther(gasTotal.value(), QEther.Wei, parent);
					}

					Rectangle
					{
						width: labelWidth
						Label
						{
							text: qsTr("Cost Estimate")
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
						}
					}

					Ether
					{
						id: gasToUseInput
						displayUnitSelection: false
						displayFormattedValue: true
						edit: false
						Layout.preferredWidth: 350
					}
				}

				RowLayout
				{
					id: deployedRow
					Layout.fillWidth: true
					Rectangle
					{
						width: labelWidth
						Label
						{
							id: labelAddresses
							text: qsTr("Deployed Contracts")
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
						}
					}

					ColumnLayout
					{
						anchors.top: parent.top
						anchors.topMargin: 1
						ListModel
						{
							id: deployedAddrModel
						}

						Repeater
						{
							id: deployedAddresses
							model: deployedAddrModel
							function refresh()
							{
								deployedAddrModel.clear()
								deployedRow.visible = Object.keys(projectModel.deploymentAddresses).length > 0
								for (var k in projectModel.deploymentAddresses)
								{
									if (k.indexOf("-") !== -1) // this is an contract instance. ctr without - are the last deployed (to support old project)
										deployedAddrModel.append({ id: k, value: projectModel.deploymentAddresses[k]})
								}
							}

							Rectangle
							{
								Layout.preferredHeight: 20
								Layout.preferredWidth: 235
								color: "transparent"
								Label
								{
									id: labelContract
									width: 112
									elide: Text.ElideRight
									text: index > -1 ? deployedAddrModel.get(index).id : ""
								}

								TextField
								{
									width: 123
									anchors.verticalCenter: parent.verticalCenter
									anchors.left: labelContract.right
									text:  index > - 1 ? deployedAddrModel.get(index).value : ""
								}
							}
						}
					}
				}

				RowLayout
				{
					id: verificationRow
					Layout.fillWidth: true
					visible: Object.keys(projectModel.deploymentAddresses).length > 0
					Rectangle
					{
						width: labelWidth
						Label
						{
							text: qsTr("Verifications")
							anchors.right: parent.right
							anchors.verticalCenter: parent.verticalCenter
						}
					}

					Label
					{
						id: verificationLabel
						maximumLineCount: 20
					}
				}
			}

			Rectangle
			{
				Layout.preferredWidth: parent.width
				Layout.alignment: Qt.BottomEdge
				Button
				{
					anchors.right: parent.right
					text: qsTr("Deploy Contracts")
					onClicked:
					{
						projectModel.deployedScenarioIndex = contractList.currentIndex
						NetworkDeploymentCode.deployContracts(root.gas, function(addresses, trHashes)
						{
							projectModel.deploymentTrHashes = trHashes
							worker.verifyHashes(trHashes, function (nb, trLost)
							{
								projectModel.deployBlockNumber = nb
								projectModel.saveProject()
								root.updateVerification(nb, trLost)
							})
							projectModel.deploymentAddresses = addresses
							projectModel.saveProject()
							deployedAddresses.refresh()
						});
					}
				}
			}
		}
	}

}

