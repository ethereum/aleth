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
/** @file SendDialog.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "SendDialog.h"
#include <libethereum/Client.h>
#include "AlethFace.h"
#include "ui_SendDialog.h"
using namespace std;
using namespace dev;
using namespace eth;
using namespace aleth;

SendDialog::SendDialog(QWidget* _parent, AlethFace* _aleth):
	QDialog(_parent),
	m_ui(new Ui::SendDialog),
	m_aleth(_aleth)
{
	m_ui->setupUi(this);
	m_ui->send->setEnabled(false);
	m_ui->send->setStyleSheet("color: grey");
	adjustDialogWidth();
}

void SendDialog::adjustDialogWidth()
{
	QString str("0xd57b6c60f48f4187edf735f627400bf03e5c7f02"); //just some ethereum address
	QFontMetrics fm(m_ui->to->font());

	int width = fm.width(str) + m_ui->label->width();
	setMaximumWidth(width);
	setMinimumWidth(width);

	m_ui->to->setMinimumWidth(fm.width(str));
	m_ui->value->setMinimumWidth(fm.width(str));
}

SendDialog::~SendDialog()
{
	delete m_ui;
}

void SendDialog::on_value_textChanged(QString _s)
{
	if (_s.isEmpty())
	{
		m_value = Invalid256;
		updateProblem();
		return;
	}
	u256 mult = ether;
	for (auto const& i: units())
		if (_s.endsWith(QString::fromStdString(i.second)))
		{
			mult = i.first;
			_s.truncate(_s.size() - i.second.size());
			break;
		}

	_s = _s.trimmed();
	try
	{
		u256 r(_s.toStdString());
		m_value = r * mult;
	}
	catch (...)
	{
		try
		{
			bool ok;
			double d = _s.toDouble(&ok);
			if (ok && d >= 0)
			{
				for (; mult > 1; mult /= 1000)
					d *= 1000.0;
				m_value = (u256)round(d);
			}
			else
				m_value = Invalid256;
		}
		catch (...)
		{
			m_value = Invalid256;
		}
	}
	updateProblem();
}

void SendDialog::on_to_textChanged(QString _s)
{
	tie(m_to, m_data) = m_aleth->readAddress(_s.toStdString());
	updateProblem();
}

void SendDialog::updateProblem()
{
	if (m_value != Invalid256 && m_to)
	{
		m_ui->problem->setVisible(true);
		m_ui->problem->setStyleSheet("");
		m_ui->problem->setText(QString::fromStdString(
			formatBalance(m_value) +
			" to " +
			m_aleth->toReadable(m_to) +
			(m_data.empty() ? "" : (" with " + toHex(m_data))))
		);
		m_ui->send->setEnabled(true);
		m_ui->send->setStyleSheet("color: 'black'");
	}
	else
	{
		m_ui->problem->setStyleSheet("color: #880000");
		m_ui->problem->setVisible(false);
		m_ui->send->setEnabled(false);
		m_ui->send->setStyleSheet("color: grey");
	}

	if (!m_to)
	{
		m_ui->to->setStyleSheet("background: #ffcccc;");
		m_ui->problem->setStyleSheet("color: #880000");
		m_ui->problem->setText("Invalid address");
	}
	else
		m_ui->to->setStyleSheet("background: #ccffcc;");

	if (m_value == Invalid256)
	{
		m_ui->value->setStyleSheet("background: #ffcccc;");
		m_ui->problem->setStyleSheet("color: #880000");
		m_ui->problem->setText("Invalid value");
	}
	else
		m_ui->value->setStyleSheet("background: #ccffcc;");
}

void SendDialog::on_send_clicked()
{
	if (!m_to || m_value == Invalid256)
		updateProblem();
	else
		if (Secret sender = m_aleth->retrieveSecret(m_aleth->author()))
		{
			m_aleth->ethereum()->submitTransaction(sender, m_value, m_to, m_data);
			accept();
		}
}

void SendDialog::on_cancel_clicked()
{
	reject();
}
