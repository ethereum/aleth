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
/** @file DappLoader.cpp
 * @author Arkadiy Paronyan <arkadiy@ethdev.org>
 * @date 2015
 */

#include <algorithm>
#include <functional>
#include <json/json.h>
#include <boost/algorithm/string.hpp>
#include <QUrl>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMimeDatabase>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/CryptoPP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Client.h>
#include <libwebthree/Support.h>
#include <libwebthree/WebThree.h>
#include "DappLoader.h"
#include "AlethResources.hpp"
using namespace std;
using namespace dev;
using namespace crypto;
using namespace eth;
using namespace aleth;

struct DappLoaderChannel: public LogChannel { static const char* name(); static const int verbosity = 3; };
const char* DappLoaderChannel::name() { return EthOrange "Ð->"; }
#define cdapp clog(DappLoaderChannel)

namespace dev { namespace aleth { QString contentsOfQResource(std::string const& res); } }

DappLoader::DappLoader(QObject* _parent, WebThreeDirect* _web3):
	QObject(_parent), m_web3(_web3)
{
	connect(&m_net, &QNetworkAccessManager::finished, this, &DappLoader::downloadComplete);
}

DappLocation DappLoader::resolveAppUri(QString const& _uri)
{
	QUrl url(_uri);
	if (!url.scheme().isEmpty() && url.scheme() != "eth")
		throw dev::Exception(); //TODO:

	string dappuri = (url.host() + url.path()).toUtf8().toStdString();
	cdapp << "Resolving" << dappuri;
	strings domainParts = Support::decomposed(dappuri);
	cdapp << "Decomposed: " << domainParts;
	h256 contentHash = web3()->support()->content(dappuri);
	cdapp << "Content hash: " << contentHash;
	Address urlHint = web3()->support()->urlHint();
	cdapp << "URLHint address: " << urlHint;
	string contentUrl = web3()->support()->urlHint(contentHash);
	cdapp << "Suggested: " << contentUrl;

	string canon = boost::algorithm::join(domainParts, "/");
	QString path = QString::fromStdString(canon);
	QString contentUrlString = QString::fromStdString(toString(contentUrl));
	if (!contentUrlString.contains(":"))
		contentUrlString = "http://" + contentUrlString;
	cdapp << "Canon: " << canon << "; contentUrl: " << contentUrlString.toStdString();

	return DappLocation { "/", path, contentUrlString, contentHash };
}

QByteArray DappLoader::injectWeb3(QByteArray _page) const
{
	//inject web3 js
	QByteArray content = "<script>\n";
	content.append(jsCode());
	content.append(("web3.admin.setSessionKey('" + m_sessionKey + "');").c_str());
	content.append("</script>\n");
	content.append(_page);
	return content;
}

void DappLoader::downloadComplete(QNetworkReply* _reply)
{
	QUrl requestUrl = _reply->request().url();
	if (m_pageUrls.count(requestUrl) != 0)
	{
		QByteArray content = injectWeb3(_reply->readAll());
		QString contentType = _reply->header(QNetworkRequest::ContentTypeHeader).toString();
		if (contentType.isEmpty())
		{
			QMimeDatabase db;
			contentType = db.mimeTypeForUrl(requestUrl).name();
		}
		pageReady(content, contentType, requestUrl);
		return;
	}

	try
	{
		//try to interpret as rlp
		QByteArray data = _reply->readAll();
		_reply->deleteLater();

		h256 expected = m_uriHashes[requestUrl];
		bytes package(reinterpret_cast<unsigned char const*>(data.constData()), reinterpret_cast<unsigned char const*>(data.constData() + data.size()));
		cdapp << "Package arrived: " << package.size();
		cdapp << "Expecting (key): " << expected;
		Secp256k1PP::get()->decrypt(Secret(expected), package);
		h256 got = sha3(package);
		cdapp << "As binary, package contains: " << got;
		if (got != expected)
		{
			//try base64
			data = QByteArray::fromBase64(data);
			package = bytes(reinterpret_cast<unsigned char const*>(data.constData()), reinterpret_cast<unsigned char const*>(data.constData() + data.size()));
			Secp256k1PP::get()->decrypt(Secret(expected), package);
			got = sha3(package);
			cdapp << "As base-64, package contains: " << got;
			if (got != expected)
				throw dev::Exception() << errinfo_comment("Dapp content hash does not match");
		}

		RLP rlp(package);
		loadDapp(rlp);
		bytesRef(&package).cleanse();	// TODO: replace with bytesSec once the crypto API is up to it.
	}
	catch (...)
	{
		qWarning() << tr("Error downloading DApp: ") << boost::current_exception_diagnostic_information().c_str();
		emit dappError();
	}
}

void DappLoader::loadDapp(RLP const& _rlp)
{
	Dapp dapp;
	dapp.manifest = loadManifest(_rlp[0].toString());
	dapp.content = m_web3->swarm()->insertBundle(_rlp.data());

	for (ManifestEntry& m: dapp.manifest.entries)
	{
		bytes b;
		if (m.path == "/deployment.js")
			//inject web3 code
			b = bytes(jsCode().data(), jsCode().data() + jsCode().size()) + (bytes)m_web3->swarm()->get(m.hash);
		else if (m.path == "/")
			b = asBytes(boost::algorithm::replace_all_copy(asString((bytes)m_web3->swarm()->get(m.hash)), "\nweb3;\n", "\n" + jsCode().toStdString() + "\n"));
		else
			continue;
		dapp.content.push_back(m_web3->swarm()->put(b));
		m.hash = sha3(b);
	}
	emit dappReady(dapp);
}

QByteArray const& DappLoader::jsCode() const
{
	if (m_web3JsCache.isEmpty())
		m_web3JsCache = makeJSCode().toLatin1();
	return m_web3JsCache;
}

Manifest DappLoader::loadManifest(std::string const& _manifest)
{
	/// https://github.com/ethereum/go-ethereum/wiki/URL-Scheme
	Manifest manifest;
	Json::Reader jsonReader;
	Json::Value root;
	jsonReader.parse(_manifest, root, false);

	QMimeDatabase mimeDB;
	Json::Value entries = root["entries"];
	for (Json::ValueIterator it = entries.begin(); it != entries.end(); ++it)
	{
		Json::Value const& entryValue = *it;
		std::string path = entryValue["path"].asString();
		if (path.size() == 0 || path[0] != '/')
			path = "/" + path;
		std::string contentType = entryValue["contentType"].asString();
		if (contentType.empty())
		{
			if (path == "/")
				contentType = "text/html";
			else
			{
				auto mts = mimeDB.mimeTypesForFileName(QString::fromStdString(path));
				contentType = mts.isEmpty() ? "application/octet-stream" : mts[0].name().toStdString();
			}
		}
		h256 hash(entryValue["hash"].asString());
		unsigned httpStatus = entryValue["status"].asInt();
		manifest.entries.push_back(ManifestEntry{ path, hash, contentType, httpStatus });
	}
	return manifest;
}

void DappLoader::loadDapp(QString const& _uri)
{
	QUrl uri(_uri);
	QUrl contentUri;
	h256 hash;
	if (uri.path().endsWith(".dapp") && uri.query().startsWith("hash="))
	{
		contentUri = uri;
		QString query = uri.query();
		query.remove("hash=");
		if (!query.startsWith("0x"))
			query.insert(0, "0x");
		hash = jsToFixed<32>(query.toStdString());
	}
	else
	{
		DappLocation location = resolveAppUri(_uri);
		contentUri = location.contentUri;
		hash = location.contentHash;
		uri = contentUri;
	}
	QNetworkRequest request(contentUri);
	m_uriHashes[contentUri] = hash;
	m_net.get(request);
}

void DappLoader::loadPage(QString const& _uri)
{
	QUrl uri(_uri);
	QNetworkRequest request(uri);
	request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml,*/*");
	request.setRawHeader("User-Agent", "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.101 Safari/537.36");
	m_pageUrls.insert(uri);
	m_net.get(request);
}

QString DappLoader::makeJSCode()
{
	AlethResources resources;
	QString content;
	content += QString::fromStdString(resources.loadResourceAsString("web3"));
	content += "\n";
	content += QString::fromStdString(resources.loadResourceAsString("setup"));
	content += "\n";
	content += QString::fromStdString(resources.loadResourceAsString("admin"));
	content += "\n";
	content += R"ETHEREUM(
		web3._extend({
			property: "bzz",
			methods: [
				new web3._extend.Method({
					name: "put",
					call: 'bzz_put',
					params: 1,
					inputFormatter: [null],
					outputFormatter: null
				}),
				new web3._extend.Method({
					name: 'get',
					call: 'bzz_get',
					params: 1,
					inputFormatter: [null],
					outputFormatter: null
				})
			]
		});
	)ETHEREUM";
	return content;
}

