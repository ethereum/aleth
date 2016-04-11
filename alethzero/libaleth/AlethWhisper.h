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
/** @file WebThreeServer.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <queue>
#include <QtCore/QObject>
#include <libdevcore/Guards.h>
#include <libethcore/CommonJS.h>
#include <libdevcrypto/Common.h>
#include <libweb3jsonrpc/Whisper.h>
#include "AccountHolder.h"

namespace dev
{

namespace aleth
{

class AccountHolder;

class AlethWhisper: public QObject, public rpc::Whisper
{
	Q_OBJECT

public:
	AlethWhisper(WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts): rpc::Whisper(_web3, _accounts) {}
	virtual std::string shh_newIdentity() override;

signals:
	void onNewId(QString _s);
};

}
}
