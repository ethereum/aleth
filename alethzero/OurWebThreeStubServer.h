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
/** @file OurWebThreeStubServer.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <QtCore/QObject>
#include <libdevcore/CommonJS.h>
#include <libdevcrypto/Common.h>
#include <libweb3jsonrpc/WebThreeStubServer.h>

class OurWebThreeStubServer: public QObject, public WebThreeStubServer
{
	Q_OBJECT

public:
	OurWebThreeStubServer(jsonrpc::AbstractServerConnector* _conn, dev::WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts);

	virtual std::string shh_newIdentity() override;

signals:
	void onNewId(QString _s);
};
