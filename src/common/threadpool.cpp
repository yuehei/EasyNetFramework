/*
 * threadpool.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: LiuYongJin
 */

#include <common/threadpool.h>

IMPL_LOGGER(ThreadPool, logger);

ThreadPool::ThreadPool(uint32_t thread_num)
	:m_threads(NULL)
	,m_thread_num(0)
	,m_size(thread_num)
{
	if(m_size > 0)
		m_threads = (Thread**)calloc(m_size, sizeof(Thread*));
	assert(m_threads != NULL);
}

ThreadPool::~ThreadPool()
{

}

bool ThreadPool::start()
{
	uint32_t i;
	for(i=0; i<m_size; ++i)
	{
		m_threads[m_thread_num] = create_thread();
		if(m_threads[m_thread_num] == NULL)
			LOG4CPLUS_ERROR(logger, "create thread failed.");
		else
			++m_thread_num;
	}

	return true;
}

