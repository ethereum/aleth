#pragma once
#include "SafeHttpServer.h"
#include "ModularServer.h"
#include "IpcServer.h"

template <class I, class... Is>
class RPCServer: public ModularServer<I, Is...>
{
public:
	RPCServer<I, Is...>(I* _i, Is*... _is): ModularServer<I, Is...>(_i, _is...)
	{
		// todo: use sensible***
		m_httpConnectorId = this->addConnector(new dev::SafeHttpServer(8545, "", "", 4));
		m_ipcConnectorId = this->addConnector(new dev::IpcServer("geth"));
	}

	void enableHttp(bool _enabled)
	{
		if (_enabled)
			httpConnector()->StartListening();
		else
			httpConnector()->StopListening();
	}

	void enableIpc(bool _enabled)
	{
		if (_enabled)
			ipcConnector()->StartListening();
		else
			ipcConnector()->StopListening();
	}

	dev::SafeHttpServer* httpConnector() const { return static_cast<dev::SafeHttpServer*>(this->connector(m_httpConnectorId)); }
	dev::IpcServer* ipcConnector() const { return static_cast<dev::IpcServer*>(this->connector(m_ipcConnectorId)); }

private:
	unsigned m_httpConnectorId;
	unsigned m_ipcConnectorId;
};
