//Test case to cover Mix tutorial
Qt.include("TestScenarioPanelActions.js")

function test_tutorial()
{
	newProject();
	editContract(
		"contract Rating {\r" +
		"	function setRating(bytes32 _key, uint256 _value) {\r" +
		"		ratings[_key] = _value;\r" +
		"	}\r" +
		"	mapping (bytes32 => uint256) public ratings;\r" +
		"}\r"
	);
	editHtml(
	"<!doctype>\r" +
	"<html>\r" +
	"<head>\r" +
	"<script type='text/javascript'>\r" +
	"function getRating() {\r" +
	"	var param = document.getElementById('query').value;\r" +
	"	var res = contracts['Rating'].contract.ratings(param);\r" +
	"	document.getElementById('queryres').innerText = res;\r" +
	"}\r" +
	"function setRating() {\r" +
	"	var key = document.getElementById('key').value;\r" +
	"	var value = parseInt(document.getElementById('value').value);\r" +
	"	var res = contracts['Rating'].contract.setRating(key, value);\r" +
	"}\r" +
	"</script>\r" +
	"</head>\r" +
	"<body bgcolor='#E6E6FA'>\r" +
	"	<h1>Ratings</h1>\r" +
	"	<div>\r" +
	"		Store:\r" +
	"		<input type='string' id='key'>\r" +
	"		<input type='number' id='value'>\r" +
	"		<button onclick='setRating()'>Save</button>\r" +
	"		</div>\r" +
	"		<div>\r" +
	"		Query:\r" +
	"		<input type='string' id='query' onkeyup='getRating()'>\r" +
	"		<div id='queryres'></div>\r" +
	"	</div>\r" +
	"</body>\r" +
	"</html>\r"
	);

	newScenario()
	rebuild()
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog;
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("setRating")
	fillParamInput(transactionDialog, 0, "Titanic")
	fillParamInput(transactionDialog, 1, "2")
	applyTx(transactionDialog)
	clickElement(mainApplication.mainContent.webView.webView, 1, 1);
	mainApplication.mainContent.webView.reload()
	ts.waitForSignal(mainApplication.mainContent.webView, "ready()", 5000);
	ts.typeString("\t\t\t\t");
	ts.typeString("Titanic");
	wait(1)
	mainApplication.mainContent.webView.getContent();
	ts.waitForSignal(mainApplication.mainContent.webView, "webContentReady()", 5000);
	var body = mainApplication.mainContent.webView.webContent;
	verify(body.indexOf("<div id=\"queryres\">2</div>") != -1, "Web content not updated")
}
