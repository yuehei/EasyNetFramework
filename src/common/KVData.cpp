/*
 * KVData.cpp
 *
 *  Created on: Apr 26, 2013
 *      Author: LiuYongJin
 */
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>

#include "KVData.h"

namespace easynet
{

IMPL_LOGGER(KVProtocol, logger);

#define KV_CAPACITY   1024    //初始化大小为1k

#define TYPE_UI8       0
#define TYPE_UI16      1
#define TYPE_UI32      2
#define TYPE_UI64      3
#define TYPE_BYTES    4
#define TYPE_BY0      4    //byte,多出0字节
#define TYPE_BY1      5    //byte,多出1字节,需要填充3字节
#define TYPE_BY2      6   //byte,多出2字节,需要填充2字节
#define TYPE_BY3      7   //byte,多出3字节,需要填充1字节

#define KeyType(k, t)   (k<<3|t)
#define ToKey(ky)       (ky>>3)
#define ToType(ky)      (ky&0x07)


#ifndef  htonll
#define  htonll(x)    (((uint64_t)(htonl((uint32_t)((x)&0xffffffff)))<<32) | htonl((uint32_t)(((x)>>32)&0xffffffff)))
#define  ntohll(x)    (((uint64_t)(ntohl((uint32_t)((x)&0xffffffff)))<<32) | ntohl((uint32_t)(((x)>>32)&0xffffffff)))
#endif//hton64

KVProtocol::KVProtocol()
	:m_Size(0)
	,m_Capacity(KV_CAPACITY)
{
	m_Buffer = calloc(m_Capacity, 1);
	assert(m_Buffer != NULL);
	m_Buffer[0] = 'K';
	m_Buffer[1] = 'V';
	m_Buffer[2] = 'D';
	m_Buffer[3] = 'T';
	m_Size = 4;
}

bool KVProtocol::_Set(uint16_t key, uint16_t type, const void *bytes, uint32_t size)
{
	uint32_t len = sizeof(uint32_t);   //key+type
	if(type > TYPE_UI16)
		len += sizeof(uint32_t);
	if(type == TYPE_UI64)
		len += sizeof(uint32_t);
	else if(type == TYPE_BYTES)
	{
		len += size;
		uint32_t temp = len%sizeof(uint32_t);
		type += temp;
		len += (sizeof(uint32_t)-temp)%sizeof(uint32_t);  //对齐,填充
	}

	if(m_Size+len>m_Capacity && !_ExpandCapacity(m_Size+len))
		return false;

	char *ptr = (char*)m_Buffer+m_Size;
	*(uint16_t*)ptr = htons(KeyType(key, type));
	ptr += sizeof(uint16_t);
	switch(type)
	{
	case TYPE_UI8:
		*ptr = *bytes;
		break;
	case TYPE_UI16:
	{
		uint16_t temp = *(uint16_t*)bytes;
		*(uint16_t)ptr =  htons(temp);  break;
	}
	case TYPE_UI32:
	{
		ptr += sizeof(uint16_t);
		uint32_t temp = *(uint32_t*)bytes;
		*(uint32_t)ptr = htonl(temp);
		break;
	}
	case TYPE_UI64:
	{
		ptr += sizeof(uint16_t);
		uint64_t temp = *(uint64_t*)bytes;
		*(uint64_t)ptr = htonll(temp);
		break;
	}
	case TYPE_BY0:
	case TYPE_BY1:
	case TYPE_BY2:
	case TYPE_BY3:
	{
		ptr += sizeof(uint16_t);
		*(uint32_t)ptr = htonl(size);
		ptr += sizeof(uint32_t);
		memcpy(ptr, bytes, size);
		break;
	}
	default:
		return false;
	}
	m_Size += len;
	return true;
}

bool KVProtocol::_ExpandCapacity(uint32_t need_size)
{
	uint32_t capacity = (1+need_size/KV_CAPACITY)*KV_CAPACITY;
	if(capacity-need_size < KV_CAPACITY/2)
		capacity += KV_CAPACITY;

	void *temp = realloc(m_Buffer, capacity);
	if(temp == NULL)
	{
		LOG_ERROR(logger, "KVData out of memory");
		return false;
	}
	m_Buffer = temp;
	m_Capacity = capacity;
	return true;
}

//8
bool KVProtocol::Set(uint16_t key, int8_t val)
{
	return _Set(key, TYPE_UI8, (const void*)&val, sizeof(uint8_t));
}
bool KVProtocol::Set(uint16_t key, uint8_t val)
{
	return _Set(key, TYPE_UI8, (const void*)&val, sizeof(uint8_t));
}

//16
bool KVProtocol::Set(uint16_t key, int16_t val)
{
	return _Set(key, TYPE_UI16, (const void*)&val, sizeof(uint16_t));
}
bool KVProtocol::Set(uint16_t key, uint16_t val)
{
	return _Set(key, TYPE_UI16, (const void*)&val, sizeof(uint16_t));
}

//32
bool KVProtocol::Set(uint16_t key, int32_t val)
{
	return _Set(key, TYPE_UI32, (const void*)&val, sizeof(uint32_t));
}
bool KVProtocol::Set(uint16_t key, uint32_t val)
{
	return _Set(key, TYPE_UI32, (const void*)&val, sizeof(uint32_t));
}

//64
bool KVProtocol::Set(uint16_t key, int64_t val)
{
	return _Set(key, TYPE_UI64, (const void*)&val, sizeof(uint64_t));
}
bool KVProtocol::Set(uint16_t key, uint64_t val)
{
	return _Set(key, TYPE_UI64, (const void*)&val, sizeof(uint64_t));
}

//bytes
bool KVProtocol::Set(uint16_t key, const void *bytes, uint32_t size)
{
	if(bytes==NULL || size==0)
		return false;
	return _Set(key, TYPE_BYTES, bytes, size);
}

bool KVProtocol::Set(uint16_t key, const string &str)
{
	if(str.size() == 0)
		return false;
	return _Set(key, TYPE_BYTES, (const void*)str.c_str(), str.size());
}

bool KVProtocol::UnPack()
{
	if(m_Size<4 || m_Buffer[0]!='K' || m_Buffer[1]!='V' || m_Buffer[2]!='D' || m_Buffer[3]!='T')
		return false;
	m_PosMap.clear();
	char *ptr_start = (char*)m_Buffer+4;
	char *ptr_end   = (char*)m_Buffer+m_Size;
	while(ptr_start < ptr_end)
	{
		void *pos = (void*)ptr_start;
		//key
		if(ptr_start+sizeof(uint32_t) > ptr_end)
			return false;
		uint16_t key_type = ntohs(*(uint16_t*)ptr_start);
		ptr_start += sizeof(uint32_t);     //next block
		uint16_t key = ToKey(key_type);    //key
		uint16_t type = ToType(key_type);  //type
		switch(type)
		{
		case TYPE_UI8:                 //8
		case TYPE_UI16:  break;       //16
		case TYPE_UI32:                //32
			ptr_start += sizeof(uint32_t); break;
		case TYPE_UI64:                //64
			ptr_start += sizeof(uint64_t); break;
		case TYPE_BY0: case TYPE_BY1: case TYPE_BY2: case TYPE_BY3:  //bytes
		{
			if(ptr_start+sizeof(uint32_t) > ptr_end)
				return false;
			uint32_t size = *(uint32_t)*ptr_start;
			size = ntohl(size);
			if(size == 0)
				return false;
			ptr_start += sizeof(uint32_t);
			type -= TYPE_BYTES;    //多出的字节数
			size += (sizeof(uint32_t)-type)%sizeof(uint32_t);  //对齐
			ptr_start += size;
			break;
		}
		default:
			return false;
		}

		PosMap::iterator it = m_PosMap.find(key);
		if(it == m_PosMap.end())
			m_PosMap.insert(std::make_pair(key, pos));
		else
			it->second = pos;
	}

	return true;
}

bool KVProtocol::_Get(uint16_t key, uint16_t type, void **bytes, uint32_t *size/*=NULL*/)
{
	if(bytes==NULL)
		return false;
	PosMap::iterator it = m_PosMap.find(key);
	if(it == m_PosMap.end())
		return false;

	char *ptr = it->second;
	uint16_t key_type = *(uint16_t*)ptr;
	key_type = ntohs(key_type);
	ptr += sizeof(uint16_t);
	uint16_t rkey = ToKey(key_type);
	uint16_t rtype = ToType(key_type);
	if(rtype > TYPE_BY3)
		return false;
	if(rtype>= TYPE_BY0 || rtype<=TYPE_BY3)
	{
		rtype = TYPE_BYTES;
		ptr += sizeof(uint16_t);
	}
	if(type != rtype)
		return false;

	if(type==TYPE_UI8)          //8
		*(uint8_t*)bytes = *(uint8_t*)ptr;
	else if(type==TYPE_UI16)  //16
	{
		uint16_t temp = *(uint16_t)ptr;
		*(uint16_t*)bytes = ntohs(temp);
	}
	else if(type==TYPE_UI32)  //32
	{
		uint32_t temp = *(uint32_t)ptr;
		*(uint32_t*)bytes = ntohl(temp);
	}
	else if(type==TYPE_UI64)  //64
	{
		uint64_t temp = *(uint64_t)ptr;
		*(uint64_t*)bytes = ntohll(temp);
	}
	else
	{
		if(size == NULL)
			return false;
		uint32_t temp = *(uint32_t)ptr;
		*size = ntohl(temp);
		if(*size == 0)
			return false;
		ptr += sizeof(uint32_t);
		*bytes = (void*)ptr;
	}
	return true;
}

//8
bool KVProtocol::Get(uint16_t key, int8_t *val)
{
	return _Get(key, TYPE_UI8, (void**)val);
}
bool KVProtocol::Get(uint16_t key, uint8_t *val)
{
	return _Get(key, TYPE_UI8, (void**)val);
}

//16
bool KVProtocol::Get(uint16_t key, int16_t *val)
{
	return _Get(key, TYPE_UI8, (void**)val);
}
bool KVProtocol::Get(uint16_t key, uint16_t *val)
{
	return _Get(key, TYPE_UI8, (void**)val);
}

//32
bool KVProtocol::Get(uint16_t key, int32_t *val)
{
	return _Get(key, TYPE_UI32, (void**)val);
}
bool KVProtocol::Get(uint16_t key, uint32_t *val)
{
	return _Get(key, TYPE_UI32, (void**)val);
}

//64
bool KVProtocol::Get(uint16_t key, int64_t *val)
{
	return _Get(key, TYPE_UI64, (void**)val);
}
bool KVProtocol::Get(uint16_t key, int64_t *val)
{
	return _Get(key, TYPE_UI64, (void**)val);
}

//bytes
bool KVProtocol::Get(uint16_t key, void **bytes, uint32_t *size)
{
	if(bytes == NULL || *bytes==NULL || size==NULL)
		return false;
	return _Get(key, TYPE_BYTES, bytes, size);
}
bool KVProtocol::Get(uint16_t key, string &str)
{
	void *bytes = NULL;
	uint32_t size = 0;
	if(!_Get(key, TYPE_BYTES, &bytes, &size))
		return false;
	str.assign((const char*)bytes, size);
	return true;
}


}//namespace