Qt.include("TestScenarioPanelActions.js")

function test_contractRename()
{
	newProject();
	tryCompare(mainApplication.mainContent.projectNavigator.sections.itemAt(0).model.get(0), "name", "Contract");
	editContract("contract Renamed {}");
	newScenario()
	rebuild()
	waitForExecution();
	tryCompare(mainApplication.mainContent.scenarioPanel.bc.model.blocks[0].transactions[0], "contractId", "Renamed");
	tryCompare(mainApplication.mainContent.projectNavigator.sections.itemAt(0).model.get(0), "name", "Renamed");
	editTx(0,0)
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog;
	tryCompare(transactionDialog, "contractId", "Renamed");
	transactionDialog.close();
}

function test_multipleWebPages()
{
	newProject();
	editHtml("<html><body><a href=\"page1.html\">page1</a></body></html>");
	createHtml("page1.html", "<html><body><div id='queryres'>Fail</div></body><script>if (web3) document.getElementById('queryres').innerText='OK'</script></html>");
	newScenario()
	clickElement(mainApplication.mainContent.webView.webView, 1, 1);
	ts.typeString("\t\r");
	wait(300); //TODO: use a signal in qt 5.5
	mainApplication.mainContent.webView.getContent();
	ts.waitForSignal(mainApplication.mainContent.webView, "webContentReady()", 5000);
	var body = mainApplication.mainContent.webView.webContent;
	verify(body.indexOf("<div id=\"queryres\">OK</div>") != -1, "Web content not updated")
}

function test_multipleContractsSameFile()
{
	newProject();
	editContract(
	"contract C1 {}\r" +
	"contract C2 {}\r" +
	"contract C3 {}\r");
	newScenario()
	rebuild()
	wait(1)
	var bc = mainApplication.mainContent.scenarioPanel.bc.model;
	tryCompare(bc.blocks[0].transactions, "length", 3);
	tryCompare(bc.blocks[0].transactions[0], "contractId", "C1");
	tryCompare(bc.blocks[0].transactions[1], "contractId", "C2");
	tryCompare(bc.blocks[0].transactions[2], "contractId", "C3");
}

function test_deleteFile()
{
	newProject();
	var path = mainApplication.projectModel.projectPath;
	createHtml("page1.html", "<html><body><div id='queryres'>Fail</div></body><script>if (web3) document.getElementById('queryres').innerText='OK'</script></html>");
	createHtml("page2.html", "<html><body><div id='queryres'>Fail</div></body><script>if (web3) document.getElementById('queryres').innerText='OK'</script></html>");
	createHtml("page3.html", "<html><body><div id='queryres'>Fail</div></body><script>if (web3) document.getElementById('queryres').innerText='OK'</script></html>");
	mainApplication.projectModel.removeDocument("page2.html");
	mainApplication.projectModel.closeProject(function(){});
	mainApplication.projectModel.loadProject(path);
	var doc = mainApplication.projectModel.getDocument("page2.html");
	verify(doc === undefined, "page2.html has not been removed");
}
