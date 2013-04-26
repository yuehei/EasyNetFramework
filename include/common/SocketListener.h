/*
 * socket_listener.h
 *
 *  Created on: Mar 15, 2013
 *      Author: LiuYongJin
 */

#ifndef _COMMON_SOCKET_LISTENER_H_
#define _COMMON_SOCKET_LISTENER_H_

#include <stdint.h>

#include "SocketInterface.h"
#include "Logger.h"

namespace easynet
{

//用于监听的socket
class SocketListener:public ISocket
{
public:
	SocketListener(uint32_t backlog=128)    //backlog:等待被accept的最大链接个数
		:m_backlog(backlog)
	{}
	SocketListener(int32_t port, char *addr, bool block, uint32_t backlog=128)
		:m_backlog(backlog)
		,ISocket(-1, addr, port, block)
	{}

	virtual ~SocketListener(){}

	//忽略等待时间wait_ms
	bool open(int32_t /*wait_ms*/);

private:
	uint32_t m_backlog;
private:
	DECL_LOGGER(logger);
};

}//namespace

#endif //_COMMON_SOCKET_LISTENER_H_
