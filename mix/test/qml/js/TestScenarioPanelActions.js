function rebuild()
{
	mainApplication.mainContent.scenarioPanel.bc.rebuildBtn.build()
	wait(2000)
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

/* Tr Panel */
function selectSendEtherTrType(trDialog)
{
	clickElement(trDialog, 243, 60);
}

function selectCreationTrType(trDialog)
{
	clickElement(trDialog, 243, 90);
}

function selectExecuteTrType(trDialog)
{
	clickElement(trDialog, 243, 125);
}

function applyTx(trDialog)
{
	trDialog.updateAction.save()
	if (!ts.waitForSignal(mainApplication.clientModel, "runComplete()", 5000))
		fail("Error running transaction");
}

function fillParamInput(trDialog, index, value)
{
	clickElement(trDialog, 226, 220 + (36 * index));
	ts.typeString(value, trDialog);
}


