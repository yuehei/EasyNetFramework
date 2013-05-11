/*
 * IAppInterface.cpp
 *
 *  Created on: 2013-5-9
 *      Author: LiuYongJin
 */
#include <assert.h>
#include <netinet/in.h>

#include "IAppInterface.h"
#include "EventServerEpoll.h"
#include "KVDataProtocolFactory.h"

namespace easynet
{
IMPL_LOGGER(IAppInterface, logger);

#define GetCurTime(now) do{                  \
	struct timeval tv;                        \
	gettimeofday(&tv, NULL);                  \
	now = tv.tv_sec*1000+tv.tv_usec/1000;     \
}while(0)

IAppInterface::IAppInterface()
	:m_EventServer(NULL)
	,m_ProtocolFactory(NULL)
	,m_TransHandler(NULL)
{}

IAppInterface::~IAppInterface()
{
	if(m_EventServer != NULL)
		delete m_EventServer;
	if(m_ProtocolFactory != NULL)
		delete m_ProtocolFactory;
	if(m_TransHandler != NULL)
		delete m_TransHandler;
}

bool IAppInterface::SendProtocol(int32_t fd, ProtocolContext *context)
{
	if(fd<0 || context==NULL)
		return false;
	if(context->time_out > 0)
	{
		uint64_t now;
		GetCurTime(now);
		context->expire_time = now+context->time_out;
	}

	SendMap::iterator it = m_SendMap.find(fd);
	if(it == m_SendMap.end())
	{
		ProtocolList temp_list;
		std::pair<SendMap::iterator, bool> result = m_SendMap.insert(std::make_pair(fd, temp_list));
		if(result.second == false)
		{
			LOG_ERROR(logger, "add protocol to list failed. fd="<<fd<<" context="<<context);
			return false;
		}
		it = result.first;
	}

	ProtocolList *protocol_list = &it->second;
	/*ProtocolList::iterator list_it = protocol_list->begin();
	for(; list_it!=protocol_list->end(); ++list_it)    //按超时时间点从小到大排列
	{
		ProtocolContext *temp_context = *list_it;
		if(context->expire_time < temp_context->expire_time)
			break;
	}
	protocol_list->insert(list_it, context);
	*/
	protocol_list->push_back(context);    //直接添加到队列尾

	//添加可写事件监控
	IEventServer *event_server = GetEventServer();
	assert(event_server != NULL);
	TransHandler* trans_handler = GetTransHandler();
	assert(trans_handler != NULL);
	int32_t time_out = GetIdleTimeout();
	if(!event_server->AddEvent(fd, ET_WRITE, trans_handler, time_out))
	{
		LOG_ERROR(logger, "add write event to event_server failed when send protocol. fd="<<fd<<" context="<<context);
		return false;
	}
	return true;
}

ProtocolContext* IAppInterface::GetSendProtocol(int32_t fd)
{
	SendMap::iterator it = m_SendMap.find(fd);
	if(it == m_SendMap.end())
		return NULL;
	ProtocolList *protocol_list = &it->second;
	ProtocolContext *context = NULL;
	if(!protocol_list->empty())
	{
		context = protocol_list->front();
		protocol_list->pop_front();
	}

	if(protocol_list->empty())
		m_SendMap.erase(it);

	return context;
}

//获取EventServer的实例
IEventServer* IAppInterface::GetEventServer()
{
	if(m_EventServer == NULL)
		m_EventServer = new EventServerEpoll(10000);
	return m_EventServer;
}

//获取ProtocolFactory的实例
IProtocolFactory* IAppInterface::GetProtocolFactory()
{
	if(m_ProtocolFactory)
		m_ProtocolFactory = new KVDataProtocolFactory;
	return m_ProtocolFactory;
}

//获取传输handler
IEventHandler* IAppInterface::GetTransHandler()
{
	if(m_TransHandler == NULL)
		m_TransHandler = new TransHandler(this);
	return m_TransHandler;
}

bool IAppInterface::AcceptNewConnect(int32_t fd)
{
	const char *peer_ip = "_unknow ip_";
	int16_t peer_port = -1;
	struct sockaddr_in peer_addr;
	int socket_len = sizeof(peer_addr);

	if(getpeername(fd, (struct sockaddr*)&peer_addr, (socklen_t*)&socket_len) == 0)
	{
		peer_ip = inet_ntoa(peer_addr.sin_addr);
		peer_port = ntohs(peer_addr.sin_port);
	}

	LOG_DEBUG(logger, "accept new connect. fd="<<fd<<" peer_ip="<<peer_ip<<" peer_port="<<peer_port);
	IEventServer *event_server = GetEventServer();
	assert(event_server != NULL);
	TransHandler* trans_handler = GetTransHandler();
	assert(trans_handler != NULL);
	int32_t time_out = GetIdleTimeout();
	if(!event_server->AddEvent(fd, ET_PER_RD, trans_handler, time_out))
	{
		LOG_ERROR(logger, "add persist read event to event_server failed when send protocol. fd="<<fd);
		return false;
	}
	return true;
}

}//namespace
