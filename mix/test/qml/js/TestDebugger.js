Qt.include("TestScenarioPanelActions.js")

function test_defaultTransactionSequence()
{
	newProject();
	editContract(
	"contract Contract {\r" +
	"	function Contract() {\r" +
	"		uint x = 69;\r" +
	"		uint y = 5;\r" +
	"		for (uint i = 0; i < y; ++i) {\r" +
	"			x += 42;\r" +
	"			z += x;\r" +
	"		}\r" +
	"	}\r" +
	"	uint z;\r" +
	"}\r"
	);
	newScenario()
	rebuild()
	var bc = mainApplication.mainContent.scenarioPanel.bc.model;
	tryCompare(bc.blocks, "length", 1);
	tryCompare(bc.blocks[0], "status", "pending");
	tryCompare(bc.blocks[0].transactions, "length", 1);

}

function test_transactionWithParameter()
{
	newProject();
	editContract(
	"contract Contract {\r" +
	"	function setZ(uint256 x) {\r" +
	"		z = x;\r" +
	"	}\r" +
	"	function getZ() returns(uint256) {\r" +
	"		return z;\r" +
	"	}\r" +
	"	uint z;\r" +
	"}\r"
	);
	newScenario()
	rebuild()
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog;
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("setZ");
	fillParamInput(transactionDialog, 0, "442")
	applyTx(transactionDialog)
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("getZ");
	applyTx(transactionDialog)
	var bc = mainApplication.mainContent.scenarioPanel.bc.model;
	tryCompare(bc.blocks, "length", 1);
	tryCompare(bc.blocks[0], "status", "pending");
	tryCompare(bc.blocks[0].transactions, "length", 3);
	tryCompare(bc.blocks[0].transactions[2], "returned", "(442)");
}

function test_constructorParameters()
{
	newProject();
	editContract(
	"contract Contract {\r" +
	"	function Contract(uint256 x) {\r" +
	"		z = x;\r" +
	"	}\r" +
	"	function getZ() returns(uint256) {\r" +
	"		return z;\r" +
	"	}\r" +
	"	uint z;\r" +
	"}\r"
	);
	newScenario()
	rebuild()
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog;
	editTx(0, 0)
	selectCreationTxType(transactionDialog)
	fillCtrlParamInput(transactionDialog, 0, "442")
	applyTx(transactionDialog)
	rebuild()
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("getZ");
	applyTx(transactionDialog)
	var bc = mainApplication.mainContent.scenarioPanel.bc.model
	tryCompare(bc.blocks, "length", 1);
	tryCompare(bc.blocks[0], "status", "pending");
	tryCompare(bc.blocks[0].transactions, "length", 2);
	tryCompare(bc.blocks[0].transactions[1], "returned", "(442)");
}

function test_arrayParametersAndStorage()
{
	newProject();
	editContract(
	"	contract ArrayTest {\r" +
	"		function setM(uint256[] x) external\r" +
	"		{\r" +
	"			m = x;\r" +
	"			s = 5;\r" +
	"			signed = 6534;\r" +
	"		}\r" +
	"		\r" +
	"		function setMV(uint72[5] x) external\r" +
	"		{\r" +
	"			mv = x;\r" +
	"			s = 42;\r" +
	"			signed = -534;\r" +
	"		}\r" +
	"		\r" +
	"		uint256[] m;\r" +
	"		uint72[5] mv;\r" +
	"		uint256 s;\r" +
	"		int48 signed;\r" +
	"	}\r");
	newScenario()
	rebuild()
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("setM")
	fillParamInput(transactionDialog, 0, "[4,5,6,2,10]")
	applyTx(transactionDialog)
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectFunction("setMV")
	fillParamInput(transactionDialog, 0, "[13,35,1,4,0]")
	applyTx(transactionDialog)
	wait(1)
	//debug setM
	debugTx(0,1)
	mainApplication.mainContent.debuggerPanel.debugSlider.value = mainApplication.mainContent.debuggerPanel.debugSlider.maximumValue
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "m", "4, 5, 6, 2, 10");
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "s", "5");
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "signed", "6534");
	mainApplication.mainContent.debuggerPanel.close()
	//debug setMV
	debugTx(0,2)
	mainApplication.mainContent.debuggerPanel.debugSlider.value = mainApplication.mainContent.debuggerPanel.debugSlider.maximumValue - 1
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "mv", "13, 35, 1, 4, 0");
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "s", "42");
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "signed", "-534");
	tryCompare(mainApplication.mainContent.debuggerPanel.solCallStack.listModel, 0, "setMV");
	mainApplication.mainContent.debuggerPanel.close()
}

function test_solidityDebugging()
{
	newProject();
	editContract(
	"contract Contract {\r " +
	"	function add(uint256 a, uint256 b) returns (uint256)\r " +
	"	{\r " +
	"		return a + b;\r " +
	"	}\r " +
	"	function Contract()\r " +
	"	{\r " +
	"		uint256 local = add(42, 34);\r " +
	"		sto = local;\r " +
	"	}\r " +
	"	uint256 sto;\r " +
	"}");
	newScenario()
	rebuild()
	wait(1)
	debugTx(0,0)
	tryCompare(mainApplication.mainContent.debuggerPanel.debugSlider, "maximumValue", 20);
	tryCompare(mainApplication.mainContent.debuggerPanel.debugSlider, "value", 0);
	mainApplication.mainContent.debuggerPanel.debugSlider.value = 13;
	tryCompare(mainApplication.mainContent.debuggerPanel.solCallStack.listModel, 0, "add");
	tryCompare(mainApplication.mainContent.debuggerPanel.solCallStack.listModel, 1, "Contract");
	tryCompare(mainApplication.mainContent.debuggerPanel.solLocals.item.value, "local", "0");
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "sto", undefined);
	mainApplication.mainContent.debuggerPanel.debugSlider.value = 19;
	tryCompare(mainApplication.mainContent.debuggerPanel.solStorage.item.value, "sto", "76");
}

function test_vmDebugging()
{
	newProject();
	editContract(
	"contract Contract {\r " +
	"	function add(uint256 a, uint256 b) returns (uint256)\r " +
	"	{\r " +
	"		return a + b;\r " +
	"	}\r " +
	"	function Contract()\r " +
	"	{\r " +
	"		uint256 local = add(42, 34);\r " +
	"		sto = local;\r " +
	"	}\r " +
	"	uint256 sto;\r " +
	"}");
	newScenario()
	rebuild()
	wait(1)
	debugTx(0,0)
	var setting = mainApplication.mainContent.debuggerPanel.assemblyMode
	mainApplication.mainContent.debuggerPanel.assemblyMode = true
	tryCompare(mainApplication.mainContent.debuggerPanel.debugSlider, "maximumValue", 44); // before was 41 ??
	tryCompare(mainApplication.mainContent.debuggerPanel.debugSlider, "value", 0);
	mainApplication.mainContent.debuggerPanel.debugSlider.value = 38; //before was 35 ??
	tryCompare(mainApplication.mainContent.debuggerPanel.vmCallStack.listModel, 0, mainApplication.clientModel.contractAddresses["Contract"].substring(2));
	tryCompare(mainApplication.mainContent.debuggerPanel.vmStorage.listModel, 0, "@ 0 (0x0)	 76 (0x4c)");
	tryCompare(mainApplication.mainContent.debuggerPanel.vmMemory.listModel, "length", 6); //before was 0 ??
	mainApplication.mainContent.debuggerPanel.assemblyMode = setting
}

function test_ctrTypeAsParam()
{
	newProject();
	editContract(
	"contract C1 {\r " +
	"	function get() returns (uint256)\r " +
	"	{\r " +
	"		return 159;\r " +
	"	}\r " +
	"}\r" +
	"contract C2 {\r " +
	"   C1 c1;\r " +
	"	function getFromC1() returns (uint256)\r " +
	"	{\r " +
	"		return c1.get();\r " +
	"	}\r " +
	"   function C2(C1 _c1)\r" +
	"	{\r " +
	"       c1 = _c1;\r" +
	"	}\r " +
	"}");
	newScenario()
	rebuild()
	wait(1)
	editTx(0,1)
	var transactionDialog = mainApplication.mainContent.scenarioPanel.bc.transactionDialog
	selectParamInput(transactionDialog, 0, "<C1 - 0>")
	applyTx(transactionDialog)
	addTx()
	selectExecuteTxType(transactionDialog)
	transactionDialog.selectRecipientAddress("<C2 - 1>");
	transactionDialog.selectFunction("getFromC1");
	applyTx(transactionDialog)
	rebuild()
	var bc = mainApplication.mainContent.scenarioPanel.bc.model;
	tryCompare(bc.blocks[0].transactions[2], "returned", "(159)");
}

