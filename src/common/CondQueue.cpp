/*
 * CondQueue.cpp
 *
 *  Created on: Mar 15, 2013
 *      Author: LiuYongJin
 */

#include <common/CondQueue.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

namespace easynet
{

CondQueue::CondQueue(uint32_t capacity, int32_t wait_ms)
	:m_wait_time(wait_ms)
	,m_array(NULL)
	,m_in(0)
	,m_out(0)
{
	pthread_mutex_init(&m_lock, NULL);
	pthread_cond_init(&m_cond, NULL);

	assert(capacity > 0);
	m_capacity = capacity+1;    //多出一个格子用来判断循环数组是否已满
	m_array = malloc(sizeof(void*)*m_capacity);

}

CondQueue::~CondQueue()
{
	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_cond);
	delete[] m_array;
	m_array = NULL;
}

bool CondQueue::Push(void *data)
{
	uint32_t temp;
	assert(data != NULL);
	pthread_mutex_lock(&m_lock);
	if((temp=(m_in+1)%m_capacity) == m_out)  //已满
	{
		pthread_mutex_unlock(&m_lock);
		return false;
	}
	m_array[m_in] = data;
	m_in = temp;

	pthread_mutex_unlock(&m_lock);
	pthread_cond_signal(&m_cond);
	return true;
}

void* CondQueue::Pop()
{
	void *data = NULL;
	pthread_mutex_lock(&m_lock);
	if(m_out == m_in)    //空
	{
		if(m_wait_time == 0)    //立刻返回
		{
			pthread_mutex_unlock(&m_lock);
			return NULL;
		}
		if(m_wait_time > 0)
		{
			struct timespec ts;
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint32_t temp = m_wait_time+tv.tv_usec/1000;
			ts.tv_sec = tv.tv_sec + temp/1000;
			ts.tv_nsec = (temp%1000)*1000000;
			pthread_cond_timedwait(&m_cond, &m_lock, &ts);
		}
		else
		{
			pthread_cond_wait(&m_cond, &m_lock);
		}
	}

	if(m_out != m_in)
	{
		data = m_array[m_out];
		m_out = (++m_out)%m_capacity;
	}

	pthread_mutex_unlock(&m_lock);
	return data;
}

uint32_t CondQueue::GetSize()
{
	uint32_t size = 0;
	pthread_mutex_lock(&m_lock);
	size = (m_in>m_out?m_in-m_out:m_in+m_capacity-m_out);
	pthread_mutex_unlock(&m_lock);
	return size;
}

uint32_t CondQueue::GetCapacity()
{
	uint32_t capacity = 0;
	pthread_mutex_lock(&m_lock);
	capacity = m_capacity-1;
	pthread_mutex_unlock(&m_lock);
	return capacity;
}

void CondQueue::SetWaitTime(int32_t wait_ms)
{
	pthread_mutex_lock(&m_lock);
	m_wait_time = wait_ms;
	pthread_mutex_unlock(&m_lock);
}


}//namespace