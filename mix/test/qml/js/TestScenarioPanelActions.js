function rebuild()
{
	mainApplication.mainContent.scenarioPanel.bc.rebuildBtn.build()
	wait(3000)
}

function addTx()
{
	mainApplication.mainContent.scenarioPanel.bc.addTxBtn.addTx()
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog
	ts.waitForRendering(transactionDialog, 3000);
}

function addBlock()
{
	mainApplication.mainContent.scenarioPanel.bc.addBlockBtn.addBlockClicked()
	wait(3000)
}

function newAccount()
{
	mainApplication.mainContent.scenarioPanel.bc.newAccountBtn.newAccount()
	clickElement(mainApplication, 2434, 790);
}

function newScenario()
{
	mainApplication.mainContent.scenarioPanel.load.addScenarioBtn.add()
	wait(3000)
}

function restore()
{
	mainApplication.mainContent.scenarioPanel.load.restoreScenarioBtn.restore()
}

function save()
{
	mainApplication.mainContent.scenarioPanel.load.saveScenarioBtn.save()
}

function duplicate()
{
	mainApplication.mainContent.scenarioPanel.load.duplicateScenarioBtn.duplicate()
}

function editTitle()
{
	mainApplication.mainContent.scenarioPanel.load.scenarioNameEditAction.edit()
}

function saveTitle()
{
	mainApplication.mainContent.scenarioPanel.load.scenarioNameEditAction.save()
}

function editTx(blockIndex, txIndex)
{
	mainApplication.mainContent.scenarioPanel.bc.blockChainControl.editTx(blockIndex, txIndex)
}

function debugTx(blockIndex, txIndex)
{
	mainApplication.mainContent.scenarioPanel.bc.blockChainControl.debugTx(blockIndex, txIndex)
}

/* Tx Panel */
function selectSendEtherTrType(trDialog)
{
	clickElement(trDialog, 243, 60);
}

function selectCreationTxType(trDialog)
{
	clickElement(trDialog, 243, 90);
}

function selectExecuteTxType(trDialog)
{
	clickElement(trDialog, 243, 125);
}

function applyTx(trDialog)
{
	trDialog.updateAction.save()
	if (trDialog.execute)
	{
		if (!ts.waitForSignal(mainApplication.clientModel, "runComplete()", 5000))
			fail("Error running transaction");
	}
}

function fillParamInput(trDialog, index, value)
{
	clickElement(trDialog, 226, 220 + (36 * index));
	ts.typeString(value, trDialog);
}

function fillCtrlParamInput(trDialog, index, value)
{
	clickElement(trDialog, 226, 187 + (36 * index));
	ts.typeString(value, trDialog);
}

function selectParamInput(trDialog, index, value)
{
	trDialog.paramsCtrl.getItem(index).select(value)
}


