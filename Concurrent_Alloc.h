#define _CRT_SECURE_NO_WARNINGS  1
#pragma once
#include "Public.h"
#include "Thread_Cache.h"



inline static void* ConcurrentAlloc(size_t size)//����̲߳��������ڴ�
{
	if (size > Max_Byte)
	{
		//��pagecache����
		return malloc(size);
	}
	else//ÿ���߳�����Ҫ���ҵ������Լ���ThreadCache����ʱ�򣬾�������TLS
	{
		if (tlslist == nullptr)
		{
			//cout << std::this_thread::get_id() << endl;//��ȡ�߳�id
			tlslist = new ThreadCache;
			//cout << tlslist << endl;
		}
		//cout << tlslist << endl;


		return tlslist->Allocate(size);
	}
}

inline static void ConcurrentFree(void* ptr, size_t size)//�����ͷ��ڴ�
{
	if (size > Max_Byte)
	{
		free(ptr);
	}
	else
	{
		tlslist->Deallocate(ptr, size);
	}
}