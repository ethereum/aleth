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
/** @file Transact.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

// Make sure boost/asio.hpp is included before windows.h.
#include <boost/asio.hpp>

#include "Transact.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <liblll/Compiler.h>
#include <liblll/CodeFragment.h>
#include <libsolidity/CompilerStack.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/AST.h>
#include <libsolidity/SourceReferenceFormatter.h>
#include <libnatspec/NatspecExpressionEvaluator.h>
#include <libethereum/Client.h>
#include <libethereum/Utility.h>
#if ETH_SERPENT
#include <libserpent/funcs.h>
#include <libserpent/util.h>
#endif
#include "Debugger.h"
#include "ui_Transact.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

Transact::Transact(Context* _c, QWidget* _parent):
	QDialog(_parent),
	ui(new Ui::Transact),
	m_context(_c)
{
	ui->setupUi(this);

	initUnits(ui->gasPriceUnits);
	initUnits(ui->valueUnits);
	ui->valueUnits->setCurrentIndex(6);
	ui->gasPriceUnits->setCurrentIndex(4);
	ui->gasPrice->setValue(10);
	on_destination_currentTextChanged(QString());
}

Transact::~Transact()
{
	delete ui;
}

void Transact::setEnvironment(QList<dev::KeyPair> _myKeys, dev::eth::Client* _eth, NatSpecFace* _natSpecDB)
{
	m_myKeys = _myKeys;
	m_ethereum = _eth;
	m_natSpecDB = _natSpecDB;
}

bool Transact::isCreation() const
{
	return ui->destination->currentText().isEmpty() || ui->destination->currentText() == "(Create Contract)";
}

u256 Transact::fee() const
{
	return ui->gas->value() * gasPrice();
}

u256 Transact::value() const
{
	if (ui->valueUnits->currentIndex() == -1)
		return 0;
	return ui->value->value() * units()[units().size() - 1 - ui->valueUnits->currentIndex()].first;
}

u256 Transact::gasPrice() const
{
	if (ui->gasPriceUnits->currentIndex() == -1)
		return 0;
	return ui->gasPrice->value() * units()[units().size() - 1 - ui->gasPriceUnits->currentIndex()].first;
}

u256 Transact::total() const
{
	return value() + fee();
}

void Transact::updateDestination()
{
	cwatch << "updateDestination()";
	QString s;
	for (auto i: ethereum()->addresses())
		if ((s = m_context->pretty(i)).size())
			// A namereg address
			if (ui->destination->findText(s, Qt::MatchExactly | Qt::MatchCaseSensitive) == -1)
				ui->destination->addItem(s);
	for (int i = 0; i < ui->destination->count(); ++i)
		if (ui->destination->itemText(i) != "(Create Contract)" && !m_context->fromString(ui->destination->itemText(i)))
			ui->destination->removeItem(i--);
}

void Transact::updateFee()
{
	ui->fee->setText(QString("(gas sub-total: %1)").arg(formatBalance(fee()).c_str()));
	auto totalReq = total();
	ui->total->setText(QString("Total: %1").arg(formatBalance(totalReq).c_str()));

	bool ok = false;
	for (auto i: m_myKeys)
		if (ethereum()->balanceAt(i.address()) >= totalReq)
		{
			ok = true;
			break;
		}
	ui->send->setEnabled(ok);
	QPalette p = ui->total->palette();
	p.setColor(QPalette::WindowText, QColor(ok ? 0x00 : 0x80, 0x00, 0x00));
	ui->total->setPalette(p);
}

void Transact::on_destination_currentTextChanged(QString)
{
	if (ui->destination->currentText().size() && ui->destination->currentText() != "(Create Contract)")
		if (Address a = m_context->fromString(ui->destination->currentText()))
			ui->calculatedName->setText(m_context->render(a));
		else
			ui->calculatedName->setText("Unknown Address");
	else
		ui->calculatedName->setText("Create Contract");
	rejigData();
//	updateFee();
}

static std::string toString(TransactionException _te)
{
	switch (_te)
	{
	case TransactionException::Unknown: return "Unknown error";
	case TransactionException::InvalidSignature: return "Permanent Abort: Invalid transaction signature";
	case TransactionException::InvalidNonce: return "Transient Abort: Invalid transaction nonce";
	case TransactionException::NotEnoughCash: return "Transient Abort: Not enough cash to pay for transaction";
	case TransactionException::OutOfGasBase: return "Permanent Abort: Not enough gas to consider transaction";
	case TransactionException::BlockGasLimitReached: return "Transient Abort: Gas limit of block reached";
	case TransactionException::BadInstruction: return "VM Error: Attempt to execute invalid instruction";
	case TransactionException::BadJumpDestination: return "VM Error: Attempt to jump to invalid destination";
	case TransactionException::OutOfGas: return "VM Error: Out of gas";
	case TransactionException::OutOfStack: return "VM Error: VM stack limit reached during execution";
	case TransactionException::StackUnderflow: return "VM Error: Stack underflow";
	default:; return std::string();
	}
}

#if ETH_SOLIDITY
static string getFunctionHashes(dev::solidity::CompilerStack const& _compiler, string const& _contractName)
{
	string ret = "";
	auto const& contract = _compiler.getContractDefinition(_contractName);
	auto interfaceFunctions = contract.getInterfaceFunctions();

	for (auto const& it: interfaceFunctions)
	{
		ret += it.first.abridged();
		ret += " :";
		ret += it.second->getDeclaration().getName() + "\n";
	}
	return ret;
}
#endif

static tuple<vector<string>, bytes, string> userInputToCode(string const& _user, bool _opt)
{
	string lll;
	string solidity;
	bytes data;
	vector<string> errors;
	if (_user.find_first_not_of("1234567890abcdefABCDEF\n\t ") == string::npos && _user.size() % 2 == 0)
	{
		std::string u = _user;
		boost::replace_all_copy(u, "\n", "");
		boost::replace_all_copy(u, "\t", "");
		boost::replace_all_copy(u, " ", "");
		data = fromHex(u);
	}
#if ETH_SOLIDITY
	else if (sourceIsSolidity(_user))
	{
		dev::solidity::CompilerStack compiler(true);
		try
		{
//				compiler.addSources(dev::solidity::StandardSources);
			data = compiler.compile(_user, _opt);
			solidity = "<h4>Solidity</h4>";
			solidity += "<pre>var " + compiler.defaultContractName() + " = web3.eth.contract(" + QString::fromStdString(compiler.getInterface()).replace(QRegExp("\\s"), "").toHtmlEscaped().toStdString() + ");</pre>";
			solidity += "<pre>" + QString::fromStdString(compiler.getSolidityInterface()).toHtmlEscaped().toStdString() + "</pre>";
			solidity += "<pre>" + QString::fromStdString(getFunctionHashes(compiler, "")).toHtmlEscaped().toStdString() + "</pre>";
		}
		catch (dev::Exception const& exception)
		{
			ostringstream error;
			solidity::SourceReferenceFormatter::printExceptionInformation(error, exception, "Error", compiler);
			errors.push_back("Solidity: " + error.str());
		}
		catch (...)
		{
			errors.push_back("Solidity: Uncaught exception");
		}
	}
#endif
#if ETH_SERPENT
	else if (sourceIsSerpent(_user))
	{
		try
		{
			data = dev::asBytes(::compile(_user));
		}
		catch (string const& err)
		{
			errors.push_back("Serpent " + err);
		}
	}
#endif
	else
	{
		data = compileLLL(_user, _opt, &errors);
		if (errors.empty())
		{
			auto asmcode = compileLLLToAsm(_user, _opt);
			lll = "<h4>LLL</h4><pre>" + QString::fromStdString(asmcode).toHtmlEscaped().toStdString() + "</pre>";
		}
	}
	return make_tuple(errors, data, lll + solidity);
}

string Transact::natspecNotice(Address _to, bytes const& _data)
{
	if (ethereum()->codeAt(_to, PendingBlock).size())
	{
		string userNotice = m_natSpecDB->getUserNotice(ethereum()->postState().codeHash(_to), _data);
		if (userNotice.empty())
			return "Destination contract unknown.";
		else
		{
			NatspecExpressionEvaluator evaluator;
			return evaluator.evalExpression(QString::fromStdString(userNotice)).toStdString();
		}
	}
	else
		return "Destination not a contract.";
}

void Transact::rejigData()
{
	if (!ethereum())
		return;

	// Determine how much balance we have to play with...
	auto s = findSecret(value() + ethereum()->gasLimitRemaining() * gasPrice());
	auto b = ethereum()->balanceAt(KeyPair(s).address(), PendingBlock);

	m_allGood = true;
	QString htmlInfo;

	auto bail = [&](QString he) {
		m_allGood = false;
		ui->send->setEnabled(false);
		ui->code->setHtml(he + htmlInfo);
	};

	// Determine m_info.
	if (isCreation())
	{
		string info;
		vector<string> errors;
		tie(errors, m_data, info) = userInputToCode(ui->data->toPlainText().toStdString(), ui->optimize->isChecked());
		if (errors.size())
		{
			// Errors determining transaction data (i.e. init code). Bail.
			QString htmlErrors;
			for (auto const& i: errors)
				htmlErrors.append("<div class=\"error\"><span class=\"icon\">ERROR</span> " + QString::fromStdString(i).toHtmlEscaped() + "</div>");
			bail(htmlErrors);
			return;
		}
		htmlInfo = QString::fromStdString(info) + "<h4>Code</h4>" + QString::fromStdString(disassemble(m_data)).toHtmlEscaped();
	}
	else
	{
		m_data = parseData(ui->data->toPlainText().toStdString());
		htmlInfo = "<h4>Dump</h4>" + QString::fromStdString(dev::memDump(m_data, 8, true));
	}

	htmlInfo += "<h4>Hex</h4>" + QString(Div(Mono)) + QString::fromStdString(toHex(m_data)) + "</div>";

	// Determine the minimum amount of gas we need to play...
	qint64 baseGas = (qint64)Transaction::gasRequired(m_data, 0);
	qint64 gasNeeded = 0;

	if (b < value() + baseGas * gasPrice())
	{
		// Not enough - bail.
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> No single account contains enough for paying even the basic amount of gas required.</div>");
		return;
	}
	else
		gasNeeded = (qint64)min<bigint>(ethereum()->gasLimitRemaining(), ((b - value()) / gasPrice()));

	// Dry-run execution to determine gas requirement and any execution errors
	Address to;
	ExecutionResult er;
	if (isCreation())
		er = ethereum()->create(s, value(), m_data, gasNeeded, gasPrice());
	else
	{
		to = m_context->fromString(ui->destination->currentText());
		er = ethereum()->call(s, value(), to, m_data, gasNeeded, gasPrice());
	}
	gasNeeded = (qint64)(er.gasUsed + er.gasRefunded);
	htmlInfo = QString("<div class=\"info\"><span class=\"icon\">INFO</span> Gas required: %1 total = %2 base, %3 exec [%4 refunded later]</div>").arg(gasNeeded).arg(baseGas).arg(gasNeeded - baseGas).arg((qint64)er.gasRefunded) + htmlInfo;

	if (er.excepted != TransactionException::None)
	{
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> " + QString::fromStdString(toString(er.excepted)) + "</div>");
		return;
	}
	if (er.codeDeposit == CodeDeposit::Failed)
	{
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> Code deposit failed due to insufficient gas; " + QString::fromStdString(toString(er.gasForDeposit)) + " GAS &lt; " + QString::fromStdString(toString(er.depositSize)) + " bytes * " + QString::fromStdString(toString(c_createDataGas)) + "GAS/byte</div>");
		return;
	}

	// Add Natspec information
	if (!isCreation())
		htmlInfo = "<div class=\"info\"><span class=\"icon\">INFO</span> " + QString::fromStdString(natspecNotice(to, m_data)).toHtmlEscaped() + "</div>" + htmlInfo;

	// Update gas
	if (ui->gas->value() == ui->gas->minimum())
	{
		ui->gas->setMinimum(gasNeeded);
		ui->gas->setValue(gasNeeded);
	}
	else
		ui->gas->setMinimum(gasNeeded);

	updateFee();

	ui->code->setHtml(htmlInfo);
	ui->send->setEnabled(m_allGood);
}

Secret Transact::findSecret(u256 _totalReq) const
{
	if (!ethereum())
		return Secret();

	Secret best;
	u256 bestBalance = 0;
	for (auto const& i: m_myKeys)
	{
		auto b = ethereum()->balanceAt(i.address(), PendingBlock);
		if (b >= _totalReq)
			return i.secret();
		if (b > bestBalance)
			bestBalance = b, best = i.secret();
	}
	return best;
}

void Transact::on_send_clicked()
{
	Secret s = findSecret(value() + fee());
	auto b = ethereum()->balanceAt(KeyPair(s).address(), PendingBlock);
	if (!s || b < value() + fee())
	{
		QMessageBox::critical(this, "Transaction Failed", "Couldn't make transaction: no single account contains at least the required amount.");
		return;
	}

	if (isCreation())
	{
		// If execution is a contract creation, add Natspec to
		// a local Natspec LEVELDB
		ethereum()->submitTransaction(s, value(), m_data, ui->gas->value(), gasPrice());
#if ETH_SOLIDITY
		string src = ui->data->toPlainText().toStdString();
		if (sourceIsSolidity(src))
			try
			{
				dev::solidity::CompilerStack compiler(true);
				m_data = compiler.compile(src, ui->optimize->isChecked());
				for (string const& s: compiler.getContractNames())
				{
					h256 contractHash = compiler.getContractCodeHash(s);
					m_natSpecDB->add(contractHash, compiler.getMetadata(s, dev::solidity::DocumentationType::NatspecUser));
				}
			}
			catch (...) {}
#endif
	}
	else
		ethereum()->submitTransaction(s, value(), m_context->fromString(ui->destination->currentText()), m_data, ui->gas->value(), gasPrice());
	close();
}

void Transact::on_debug_clicked()
{
	Secret s = findSecret(value() + fee());
	auto b = ethereum()->balanceAt(KeyPair(s).address(), PendingBlock);
	if (!s || b < value() + fee())
	{
		QMessageBox::critical(this, "Transaction Failed", "Couldn't make transaction: no single account contains at least the required amount.");
		return;
	}

	try
	{
		State st(ethereum()->postState());
		Transaction t = isCreation() ?
			Transaction(value(), gasPrice(), ui->gas->value(), m_data, st.transactionsFrom(dev::toAddress(s)), s) :
			Transaction(value(), gasPrice(), ui->gas->value(), m_context->fromString(ui->destination->currentText()), m_data, st.transactionsFrom(dev::toAddress(s)), s);
		Debugger dw(m_context, this);
		Executive e(st, ethereum()->blockChain(), 0);
		dw.populate(e, t);
		dw.exec();
	}
	catch (dev::Exception const& _e)
	{
		QMessageBox::critical(this, "Transaction Failed", "Couldn't make transaction. Low-level error: " + QString::fromStdString(diagnostic_information(_e)));
		// this output is aimed at developers, reconsider using _e.what for more user friendly output.
	}
}
