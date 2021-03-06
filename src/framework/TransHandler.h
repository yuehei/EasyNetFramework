/*
 * TransHandler.h
 *
 *  Created on: 2013-5-3
 *      Author: LiuYongJin
 */

#ifndef _FRAMEWORK_TRANS_HANDLER_H_
#define _FRAMEWORK_TRANS_HANDLER_H_
#include <assert.h>
#include <map>
using std::map;

#include "Logger.h"
#include "EventServer.h"

#include "IProtocolFactory.h"

namespace easynet
{

class IAppInterface;
class TransHandler:public IEventHandler
{
typedef map<int32_t, ProtocolContext*> FDMap;
public:
	TransHandler(IAppInterface *app_interface)
		:m_AppInterface(app_interface)
	{
		assert(m_AppInterface != NULL);
	}

//基类接口方法
public:
	//时钟超时
	void OnTimeout(uint64_t now_time);
	//错误事件
	void OnEventError(int32_t fd, uint64_t nowtime_ms, ERROR_CODE code);
	//可读事件
	ERROR_CODE OnEventRead(int32_t fd, uint64_t now_time);
	//可写事件
	ERROR_CODE OnEventWrite(int32_t fd, uint64_t now_time);
private:
	//接收数据
	// @param fd          : socket fd
	// @param buffer      : 接收数据缓冲区
	// @param buffer_size : 缓冲区大小
	// @param need_size   : 预期接收的数据大小
	// @return            : 成功返回读取的字节数,错误返回-1,对端关闭返回-2
	int32_t ReadData(int32_t fd, char *buffer, uint32_t buffer_size, uint32_t need_size);

private:
	FDMap m_RecvFdMap;
	FDMap m_SendFdMap;
	IAppInterface     *m_AppInterface;
private:
	DECL_LOGGER(logger);
};

inline
//时钟超时
void TransHandler::OnTimeout(uint64_t now_time)
{
	assert(0);    //没有时钟
}

}//namespace
#endif //_FRAMEWORK_TRANS_HANDLER_H_

