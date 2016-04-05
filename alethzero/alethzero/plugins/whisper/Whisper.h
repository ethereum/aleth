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
/** @file Whisper.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <QString>
#include <QList>
#include <QSet>
#include <libethcore/Common.h>
#include "Plugin.h"

namespace Ui
{
class Whisper;
}

namespace dev
{
namespace aleth
{
namespace zero
{

class Whisper: public QObject, public Plugin
{
	Q_OBJECT

public:
	Whisper(ZeroFace* _m);

private slots:
	void onPostClicked();
	void onForgetTopicsClicked();
	void onForgetDestinationsClicked();

private:
	void readSettings(QSettings const&) override;
	void writeSettings(QSettings&) override;
	void refreshWhisper();
	void addNewId(QString _ids);
	void noteTopic(QString const& _topic);
	void onNewIdentityTriggered();

	Ui::Whisper* m_ui;
	QList<KeyPair> m_myIdentities;
};

}
}
}
