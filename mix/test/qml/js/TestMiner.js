Qt.include("TestScenarioPanelActions.js")

function test_getDefaultMiner()
{
	newProject();
	newScenario()
	rebuild()
	var scenario = mainApplication.mainContent.scenarioPanel.bc.model
	compare(scenario.miner.secret, "cb73d9408c4720e230387d956eb0f829d8a4dd2c1055f96257167e14e7169074")
}

function test_selectMiner()
{
	newProject();
	mainApplication.projectModel.stateListModel.editState(0);
	var account = mainApplication.projectModel.stateDialog.newAccAction.add();
	account = mainApplication.projectModel.stateDialog.newAccAction.add();
	mainApplication.projectModel.stateDialog.minerComboBox.currentIndex = 2;
	ts.waitForRendering(mainApplication.projectModel.stateDialog.minerComboBox, 3000);
	mainApplication.projectModel.stateDialog.acceptAndClose();
	var state = mainApplication.projectModel.stateListModel.get(0);
	compare(state.miner.secret, account.secret);
}

function test_mine()
{
	newProject()
	newScenario()
	rebuild()
	addBlock()
	waitForMining();
	wait(1000); //there need to be at least 1 sec diff between block times
	var blocks = mainApplication.mainContent.scenarioPanel.bc.model.blocks
	tryCompare(blocks, "length", 2)
	tryCompare(blocks[1], 'status', 'pending')
	tryCompare(blocks[0], 'status', 'mined')
	addBlock()
	waitForMining();
	wait(1000); //there need to be at least 1 sec diff between block times
	blocks = mainApplication.mainContent.scenarioPanel.bc.model.blocks
	tryCompare(blocks, "length", 3)
	tryCompare(blocks[2], 'status', 'pending')
	tryCompare(blocks[1], 'status', 'mined')
}

