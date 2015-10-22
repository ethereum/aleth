#pragma once

#include <string>
#include <map>
#include <vector>
#include <jsonrpccpp/common/procedure.h>
#include <jsonrpccpp/server/iprocedureinvokationhandler.h>
#include <jsonrpccpp/server/abstractserverconnector.h>
#include <jsonrpccpp/server/requesthandlerfactory.h>

template <class I> using AbstractMethodPointer = void(I::*)(Json::Value const& _parameter, Json::Value& _result);
template <class I> using AbstractNotificationPointer = void(I::*)(Json::Value const& _parameter);

template <template <class I> class C, class I>
class ProcedureBinding
{
public:
	using CallPointer = C<I>;

	ProcedureBinding(jsonrpc::Procedure const& _procedure, CallPointer _call)
	: m_procedure(_procedure), m_call(_call) {}

	jsonrpc::Procedure const& procedure() const { return m_procedure; }
	CallPointer call() const { return m_call; }

private:
	jsonrpc::Procedure m_procedure;
	CallPointer m_call;
};

template <class I> using MethodBinding = ProcedureBinding<AbstractMethodPointer, I>;
template <class I> using NotificationBinding = ProcedureBinding<AbstractNotificationPointer, I>;

template <class I>
class ServerInterface
{
public:
	using MethodPointer = AbstractMethodPointer<I>;
	using NotificationPointer = AbstractNotificationPointer<I>;
	using Methods = std::vector<MethodBinding<I>>;
	using Notifications = std::vector<NotificationBinding<I>>;

	Methods const& methods() const { return m_methods; }
	Notifications const& notifications() const { return m_notifications; }

protected:
	void bindAndAddMethod(jsonrpc::Procedure const& _proc, MethodPointer _pointer) { m_methods.emplace_back(_proc, _pointer); }
	void bindAndAddNotification(jsonrpc::Procedure const& _proc, NotificationPointer _pointer) { m_notifications.emplace_back(_proc, _pointer); }

private:
	Methods m_methods;
	Notifications m_notifications;
};

template <class... Is>
class ModularServer: public jsonrpc::IProcedureInvokationHandler
{
public:
	ModularServer(jsonrpc::AbstractServerConnector* _connector)
	: m_connection(_connector), m_handler(jsonrpc::RequestHandlerFactory::createProtocolHandler(jsonrpc::JSONRPC_SERVER_V2, *this))
	{
		m_connection->SetHandler(m_handler.get());
	}

	bool StartListening() { return m_connection->StartListening(); }
	bool StopListening() { return m_connection->StopListening(); }

	virtual void HandleMethodCall(jsonrpc::Procedure& _proc, const Json::Value& _input, Json::Value& _output) override {}
	virtual void HandleNotificationCall(jsonrpc::Procedure& _proc, const Json::Value& _input) override {}

private:
	std::unique_ptr<jsonrpc::AbstractServerConnector> m_connection;

protected:
	std::unique_ptr<jsonrpc::IProtocolHandler> m_handler;
};

template <class I, class... Is>
class ModularServer<I, Is...> : public ModularServer<Is...>
{
public:
	using MethodPointer = AbstractMethodPointer<I>;
	using NotificationPointer = AbstractNotificationPointer<I>;

	ModularServer<I, Is...>(jsonrpc::AbstractServerConnector* _connector, I* _i, Is*... _is): ModularServer<Is...>(_connector, _is...), m_interface(_i)
	{
		for (auto const& method: m_interface->methods())
		{
			m_methods[method.procedure().GetProcedureName()] = method.call();
			this->m_handler->AddProcedure(method.procedure());
		}
		
		for (auto const& notification: m_interface->notifications())
		{
			m_notifications[notification.procedure().GetProcedureName()] = notification.call();
			this->m_handler->AddProcedure(notification.procedure());
		}
	}

	virtual void HandleMethodCall(jsonrpc::Procedure& _proc, const Json::Value& _input, Json::Value& _output) override
	{
		auto pointer = m_methods.find(_proc.GetProcedureName());
		if (pointer != m_methods.end())
			(m_interface.get()->*(pointer->second))(_input, _output);
		else
			ModularServer<Is...>::HandleMethodCall(_proc, _input, _output);
	}

	virtual void HandleNotificationCall(jsonrpc::Procedure& _proc, const Json::Value& _input) override
	{
		auto pointer = m_notifications.find(_proc.GetProcedureName());
		if (pointer != m_notifications.end())
			(m_interface.get()->*(pointer->second))(_input);
		else
			ModularServer<Is...>::HandleNotificationCall(_proc, _input);
	}

private:
	std::unique_ptr<I> m_interface;
	std::map<std::string, MethodPointer> m_methods;
	std::map<std::string, NotificationPointer> m_notifications;
};
