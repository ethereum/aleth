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
/** @file TransactDialog.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

// Make sure boost/asio.hpp is included before windows.h.
#include <boost/asio.hpp>

#include "TransactDialog.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>
#include <liblll/Compiler.h>
#include <liblll/CodeFragment.h>

#if ETH_SOLIDITY
#include <libsolidity/interface/CompilerStack.h>
#include <libsolidity/parsing/Scanner.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/interface/SourceReferenceFormatter.h>
#endif // ETH_SOLIDITY

#include <libethereum/Client.h>
#include <libethereum/Utility.h>
#include <libethcore/ICAP.h>
#include <libethcore/KeyManager.h>
#include <libaleth/Debugger.h>
#include "ui_TransactDialog.h"

using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

TransactDialog::TransactDialog(QWidget* _parent, AlethFace* _aleth):
	QDialog(_parent),
	m_ui(new Ui::TransactDialog),
	m_aleth(_aleth)
{
	m_ui->setupUi(this);

	qRegisterMetaType<dev::eth::ExecutionResult>("dev::eth::ExecutionResult");
	connect(this, &TransactDialog::gasEstimationProgress, this, &TransactDialog::updateBounds, Qt::QueuedConnection);
	connect(this, &TransactDialog::gasEstimationComplete, this, &TransactDialog::finaliseBounds, Qt::QueuedConnection);

	resetGasPrice();
	setValueUnits(m_ui->valueUnits, m_ui->value, 0);

	on_destination_currentTextChanged(QString());
}

TransactDialog::~TransactDialog()
{
	m_estimationFuture.waitForFinished();
	delete m_ui;
}

void TransactDialog::setEnvironment(AddressHash const& _accounts, dev::eth::Client*, NatSpecFace* _natSpecDB)
{
	m_accounts = _accounts;
	m_natSpecDB = _natSpecDB;

	auto old = m_ui->from->currentIndex();
	m_ui->from->clear();
	for (auto const& address: m_accounts)
	{
		u256 b = ethereum()->balanceAt(address, PendingBlock);
		QString s = QString("%2: %1").arg(formatBalance(b).c_str()).arg(QString::fromStdString(aleth()->toReadable(address)));
		m_ui->from->addItem(s);
	}
	updateDestination();
	if (old > -1 && old < m_ui->from->count())
		m_ui->from->setCurrentIndex(old);
	else if (m_ui->from->count())
		m_ui->from->setCurrentIndex(0);
}

void TransactDialog::resetGasPrice()
{
	setValueUnits(m_ui->gasPriceUnits, m_ui->gasPrice, aleth()->ethereum()->gasPricer()->bid());
}

bool TransactDialog::isCreation() const
{
	return m_ui->destination->currentText().isEmpty() || m_ui->destination->currentText() == "(Create Contract)";
}

u256 TransactDialog::fee() const
{
	return gas() * gasPrice();
}

u256 TransactDialog::gas() const
{
	return m_ui->gas->value() == -1 ? (qint64)m_upperBound : m_ui->gas->value();
}

u256 TransactDialog::value() const
{
	if (m_ui->valueUnits->currentIndex() == -1)
		return 0;
	return m_ui->value->value() * units()[units().size() - 1 - m_ui->valueUnits->currentIndex()].first;
}

u256 TransactDialog::gasPrice() const
{
	if (m_ui->gasPriceUnits->currentIndex() == -1)
		return 0;
	return m_ui->gasPrice->value() * units()[units().size() - 1 - m_ui->gasPriceUnits->currentIndex()].first;
}

u256 TransactDialog::total() const
{
	return value() + fee();
}

void TransactDialog::updateDestination()
{
	// TODO: should be a Qt model.
	m_ui->destination->clear();
	m_ui->destination->addItem("(Create Contract)");
	QMultiMap<QString, QString> in;
	for (Address const& a: aleth()->allKnownAddresses())
		in.insert(QString::fromStdString(aleth()->toReadable(a)), QString::fromStdString(a.hex()));
	for (auto i = in.begin(); i != in.end(); ++i)
		m_ui->destination->addItem(i.key(), i.value());

}

void TransactDialog::updateFee()
{
//	ui->fee->setText(QString("(gas sub-total: %1)").arg(formatBalance(fee()).c_str()));
	auto totalReq = total();
	m_ui->total->setText(QString("Total: %1").arg(formatBalance(totalReq).c_str()));

	bool ok = false;
	for (auto const& i: m_accounts)
		if (ethereum()->balanceAt(i) >= totalReq)
		{
			ok = true;
			break;
		}
//	ui->send->setEnabled(ok);
	QPalette p = m_ui->total->palette();
	p.setColor(QPalette::WindowText, QColor(ok ? 0x00 : 0x80, 0x00, 0x00));
	m_ui->total->setPalette(p);
}

void TransactDialog::on_destination_currentTextChanged(QString)
{
	if (m_ui->destination->currentText().size() && m_ui->destination->currentText() != "(Create Contract)")
	{
		auto p = toAccount();
		if (p.first)
			m_ui->calculatedName->setText(QString::fromStdString(aleth()->toReadable(p.first)));
		else
			m_ui->calculatedName->setText("Unknown Address");

		if (!p.second.empty())
		{
			m_data = p.second;
			m_ui->data->setPlainText(QString::fromStdString("0x" + toHex(m_data)));
			m_ui->data->setEnabled(false);
		}
		else if (!m_ui->data->isEnabled())
		{
			m_data.clear();
			m_ui->data->setPlainText("");
			m_ui->data->setEnabled(true);
		}
	}
	else
		m_ui->calculatedName->setText("Create Contract");
	rejigData();
	//	updateFee();
}

void TransactDialog::on_copyUnsigned_clicked()
{
	auto a = fromAccount();
	u256 nonce = m_ui->autoNonce->isChecked() ? ethereum()->countAt(a, PendingBlock) : m_ui->nonce->value();

	Transaction t;
	if (isCreation())
		// If execution is a contract creation, add Natspec to
		// a local Natspec LEVELDB
		t = Transaction(value(), gasPrice(), gas(), m_data, nonce);
	else
		// TODO: cache like m_data.
		t = Transaction(value(), gasPrice(), gas(), toAccount().first, m_data, nonce);
	qApp->clipboard()->setText(QString::fromStdString(toHex(t.rlp())));
}

/*static std::string toString(TransactionException _te)
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
}*/

#if ETH_SOLIDITY
static string getFunctionHashes(dev::solidity::CompilerStack const& _compiler, string const& _contractName)
{
	string ret = "";
	auto const& contract = _compiler.contractDefinition(_contractName);
	auto interfaceFunctions = contract.interfaceFunctions();

	for (auto const& it: interfaceFunctions)
	{
		ret += it.first.abridged();
		ret += " :";
		ret += it.second->declaration().name() + "\n";
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
		auto scannerFromSourceName = [&](string const& _sourceName) -> solidity::Scanner const& { return compiler.scanner(_sourceName); };
		try
		{
			if (!compiler.compile(_user, _opt))
			{
				for (auto const& error: compiler.errors())
				{
					ostringstream errorStr;
					solidity::SourceReferenceFormatter::printExceptionInformation(errorStr, *error, "Error", scannerFromSourceName);
					errors.push_back("Solidity: " + errorStr.str());
				}
			}
			else
			{
				solidity = "<h4>Solidity</h4>";
				solidity += "<pre>var " + compiler.defaultContractName() + " = web3.eth.contract(" + QString::fromStdString(compiler.interface()).replace(QRegExp("\\s"), "").toHtmlEscaped().toStdString() + ");</pre>";
				solidity += "<pre>" + QString::fromStdString(compiler.solidityInterface()).toHtmlEscaped().toStdString() + "</pre>";
				solidity += "<pre>" + QString::fromStdString(getFunctionHashes(compiler, "")).toHtmlEscaped().toStdString() + "</pre>";
				LinkerObject const& obj = compiler.object();
				if (obj.linkReferences.empty())
					data = obj.bytecode;
				else
					errors.push_back("Solidity: Compilation resulted in unlinked object.");
			}
		}
		catch (dev::Exception const& exception)
		{
			ostringstream error;
			solidity::SourceReferenceFormatter::printExceptionInformation(error, exception, "Error", scannerFromSourceName);
			errors.push_back("Solidity: " + error.str());
		}
		catch (...)
		{
			errors.push_back("Solidity: Uncaught exception");
		}
	}
#endif // ETH_SOLIDITY
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

pair<Address, bytes> TransactDialog::toAccount()
{
	pair<Address, bytes> p;
	if (!isCreation())
	{
		if (!m_ui->destination->currentData().isNull() && m_ui->destination->currentText() == m_ui->destination->itemText(m_ui->destination->currentIndex()))
			p.first = Address(m_ui->destination->currentData().toString().trimmed().toStdString());
		else
			p = aleth()->readAddress(m_ui->destination->currentText().trimmed().toStdString());
	}
	return p;
}

void TransactDialog::updateBounds()
{
	m_ui->minGas->setValue(m_lowerBound);
	m_ui->maxGas->setValue(m_upperBound);
	double oran = m_startUpperBound - m_startLowerBound;
	double nran = m_upperBound - m_lowerBound;
	int x = int(log2(oran / nran) * 100.0 / log2(oran * 2));
	m_ui->progressGas->setValue(x);
	m_ui->progressGas->setVisible(true);
	m_ui->gas->setSpecialValueText(QString("Auto (%1 gas)").arg(m_upperBound));
}

void TransactDialog::finaliseBounds(ExecutionResult const& _er)
{
	EVMSchedule schedule;	// TODO: populate properly

	quint64 baseGas = (quint64)Transaction::gasRequired(isCreation(), &m_data, m_aleth->ethereum()->evmSchedule(), 0);
	m_ui->progressGas->setVisible(false);

	quint64 executionGas = m_upperBound - baseGas;
	QString htmlInfo = QString("<div class=\"info\"><span class=\"icon\">INFO</span> Gas required: %1 total = %2 base, %3 exec [%4 refunded later]</div>").arg(m_upperBound).arg(baseGas).arg(executionGas).arg((qint64)_er.gasRefunded);

	auto bail = [&](QString he) {
		m_ui->send->setEnabled(false);
		m_ui->code->setHtml(he + htmlInfo + m_dataInfo);
	};

	auto s = fromAccount();
	auto b = ethereum()->balanceAt(s, PendingBlock);

	if (b < value() + baseGas * gasPrice())
	{
		// Not enough - bail.
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> Account doesn't contain enough for paying even the basic amount of gas required.</div>");
		return;
	}
	if ((qint64)m_upperBound > ethereum()->gasLimitRemaining())
	{
		// Not enough - bail.
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> Gas remaining in block isn't enough to allow the gas required.</div>");
		return;
	}
	if (_er.excepted != TransactionException::None)
	{
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> " + QString::fromStdString(toString(_er.excepted)) + "</div>");
		return;
	}
	if (_er.codeDeposit == CodeDeposit::Failed)
	{
		bail("<div class=\"error\"><span class=\"icon\">ERROR</span> Code deposit failed due to insufficient gas; " + QString::fromStdString(toString(_er.gasForDeposit)) + " GAS &lt; " + QString::fromStdString(toString(_er.depositSize)) + " bytes * " + QString::fromStdString(toString(schedule.createDataGas)) + "GAS/byte</div>");
		return;
	}

	updateFee();
	m_ui->code->setHtml(htmlInfo + m_dataInfo);
	m_ui->send->setEnabled(true);
}

void TransactDialog::determineGasRequirements()
{
	if (m_estimationFuture.isRunning())
	{
		m_needsEstimation = true;
		return;
	}
	// Determine the minimum amount of gas we need to play...

	Address from = fromAccount();
	Address to = toAccount().first;

	m_startLowerBound = 0;
	m_startUpperBound = 0;

	m_estimationFuture = QtConcurrent::run([this, from, to]()
	{
		GasEstimationCallback callback = [this](GasEstimationProgress const& _p)
		{
			m_lowerBound = (qint64)_p.lowerBound;
			m_upperBound = (qint64)_p.upperBound;
			if (m_startLowerBound == 0)
				m_startLowerBound = m_lowerBound;
			if (m_startUpperBound == 0)
				m_startUpperBound = m_upperBound;

			emit gasEstimationProgress();
		};
		do
		{
			m_needsEstimation = false;
			ExecutionResult er = ethereum()->estimateGas(from, value(), to, m_data, Invalid256, gasPrice(), PendingBlock, callback).second;
			emit gasEstimationComplete(er);
		}
		while (m_needsEstimation);
	});
}

void TransactDialog::rejigData()
{
	if (!ethereum())
		return;

	// Determine how much balance we have to play with...
	//findSecret(value() + ethereum()->gasLimitRemaining() * gasPrice());
	auto s = fromAccount();
	if (!s)
		return;

	QString htmlInfo;

	auto bail = [&](QString he) {
		m_ui->send->setEnabled(false);
		m_dataInfo = he + htmlInfo;
		m_ui->code->setHtml(m_dataInfo);
	};

	// Determine m_info.
	if (isCreation())
	{
		string info;
		vector<string> errors;
		tie(errors, m_data, info) = userInputToCode(m_ui->data->toPlainText().toStdString(), m_ui->optimize->isChecked());
		if (errors.size())
		{
			// Errors determining transaction data (i.e. init code). Bail.
			QString htmlErrors;
			for (auto const& i: errors)
				htmlErrors.append("<div class=\"error\"><span class=\"icon\">ERROR</span><pre>" + QString::fromStdString(i).toHtmlEscaped() + "</pre></div>");
			bail(htmlErrors);
			return;
		}
		htmlInfo = QString::fromStdString(info) + "<h4>Code</h4>" + QString::fromStdString(eth::disassemble(m_data)).toHtmlEscaped();
	}
	else
	{
		m_data = parseData(m_ui->data->toPlainText().toStdString());
		htmlInfo = "<h4>Dump</h4>" + QString::fromStdString(dev::memDump(m_data, 8, true));
	}

	htmlInfo += "<h4>Hex</h4>" + QString(ETH_HTML_DIV(ETH_HTML_MONO)) + QString::fromStdString(toHex(m_data)) + "</div>";

	determineGasRequirements();

	m_dataInfo = htmlInfo;
	m_ui->code->setHtml(m_dataInfo);
	m_ui->send->setEnabled(true);
}

Secret TransactDialog::findSecret(u256 _totalReq) const
{
	if (!ethereum())
		return Secret();

	Address best;
	u256 bestBalance = 0;
	for (auto const& i: m_accounts)
	{
		auto b = ethereum()->balanceAt(i, PendingBlock);
		if (b >= _totalReq)
		{
			best = i;
			break;
		}
		if (b > bestBalance)
			bestBalance = b, best = i;
	}
	return aleth()->retrieveSecret(best);
}

Address TransactDialog::fromAccount()
{
	if (m_ui->from->currentIndex() < 0 || m_ui->from->currentIndex() >= (int)m_accounts.size())
		return Address();
	auto it = m_accounts.begin();
	std::advance(it, m_ui->from->currentIndex());
	return *it;
}

void TransactDialog::updateNonce()
{
	u256 n = ethereum()->countAt(fromAccount(), PendingBlock);
	m_ui->nonce->setMaximum((unsigned)n);
	m_ui->nonce->setMinimum(0);
	m_ui->nonce->setValue((unsigned)n);
}

void TransactDialog::on_send_clicked()
{
//	Secret s = findSecret(value() + fee());
	u256 nonce = m_ui->autoNonce->isChecked() ? ethereum()->countAt(fromAccount(), PendingBlock) : m_ui->nonce->value();
	auto a = fromAccount();
	auto b = ethereum()->balanceAt(a, PendingBlock);

	if (!a || b < value() + fee())
	{
		QMessageBox::critical(nullptr, "Transaction Failed", "Couldn't make transaction: account doesn't contain at least the required amount.", QMessageBox::Ok);
		return;
	}

	Secret s = aleth()->retrieveSecret(a);
	if (!s)
		return;

	if (isCreation())
	{
		// If execution is a contract creation, add Natspec to
		// a local Natspec LEVELDB
		ethereum()->submitTransaction(s, value(), m_data, gas(), gasPrice(), nonce);
#if ETH_SOLIDITY
		string src = m_ui->data->toPlainText().toStdString();
		if (sourceIsSolidity(src))
			try
			{
				dev::solidity::CompilerStack compiler(true);
				if (compiler.compile(src, m_ui->optimize->isChecked()))
				{
					LinkerObject obj = compiler.object();
					if (obj.linkReferences.empty())
					{
						m_data = obj.bytecode;
						for (string const& s: compiler.contractNames())
						{
							h256 contractHash = compiler.contractCodeHash(s);
							if (contractHash != h256())
								m_natSpecDB->add(contractHash, compiler.metadata(s, dev::solidity::DocumentationType::NatspecUser));
						}
					}
				}
			}
			catch (...) {}
#endif
	}
	else
		// TODO: cache like m_data.
		ethereum()->submitTransaction(s, value(), toAccount().first, m_data, gas(), gasPrice(), nonce);
	close();
}

void TransactDialog::on_debug_clicked()
{
//	Secret s = findSecret(value() + fee());
	Address from = fromAccount();
	auto b = ethereum()->balanceAt(from, PendingBlock);
	if (!from || b < value() + fee())
	{
		QMessageBox::critical(this, "Transaction Failed", "Couldn't make transaction: account doesn't contain at least the required amount.");
		return;
	}

	try
	{
		eth::Block postState(ethereum()->postState());
		Transaction t = isCreation() ?
			Transaction(value(), gasPrice(), gas(), m_data, postState.transactionsFrom(from)) :
			Transaction(value(), gasPrice(), gas(), toAccount().first, m_data, postState.transactionsFrom(from));
		t.forceSender(from);
		Debugger dw(aleth(), this);
		Executive e(postState, ethereum()->blockChain(), 0);
		dw.populate(e, t);
		dw.exec();
	}
	catch (dev::Exception const& _e)
	{
		QMessageBox::critical(this, "Transaction Failed", "Couldn't make transaction. Low-level error: " + QString::fromStdString(diagnostic_information(_e)));
		// this output is aimed at developers, reconsider using _e.what for more user friendly output.
	}
}
