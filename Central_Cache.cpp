#define _CRT_SECURE_NO_WARNINGS  1
#include "Public.h"
#include "Central_Cache.h"
#include "Page_Cache.h"


CentralCache CentralCache::_inst;//�����ⶨ��

//��PageCache��ȡһ��span
Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t byte_size)
{
	Span* span = spanlist.Begin();
	while (span != spanlist.End())
	{
		if (span->_list != nullptr)//�ж�һ��span�Ƿ�Ϊ��
		{
			return span;
		}
		else
		{
			span = span->_next;
		}
	}


	//���Դ�׮
	//Span* newspan = new Span;
	//newspan->objsize = 16;
	//void* ptr = malloc(16 * 8);
	//void* cur = ptr;
	////�зֶ���
	//for (size_t i = 0; i < 7; i++)
	//{
	//	//���еĶ������������
	//	void* next = (char*)cur + 16;
	//	NEXT_OBJ(cur) = next;
	//	cur = next;
	//}

	//NEXT_OBJ(cur) = nullptr;
	//newspan->_list = ptr;


	//�ߵ������ʾspan�ǿյ�
	Span* newspan = PageCache::GetInstance()->NewSpan(Size::NumMovePage(byte_size));

	//��spanҳ�зֳ�һ��һ���Ķ��󣬲���������
	char* cur = (char*)(newspan->_pageid << Pageshift);
	char* end = cur + (newspan->_npage << Pageshift);

	newspan->_list = cur;
	newspan->objsize = byte_size;
	while (cur + byte_size < end)
	{
		char* next = cur + byte_size;
		NEXT_OBJ(cur) = next;
		cur = next;
	}
	NEXT_OBJ(cur) = nullptr;

	//��span���뵽��ǰ���������
	spanlist.PushFront(newspan);
	return newspan;
}

//�����Ļ����ȡһ�������Ķ��󷵻ظ�thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size)
{

	size_t index = Size::Index(byte_size);//����������С��Ӧ��һ���spanlist
	SpanList& spanlist = _spanlist[index];

	//����
	spanlist.Lock();
	Span* span = GetOneSpan(spanlist, byte_size);

	//��span�л�ȡһ����Χ�ڵĶ���
	size_t batchsize = 0;//ͳ�ƻ�ȡ����ĸ���
	void* prev = nullptr;
	void* cur = span->_list;
	for (size_t i = 0; i < n; i++)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		batchsize++;
		if (cur == nullptr)//�����Ҫ��ȡ�Ķ������n����ʵ���еĶ��󣬾�Ҫ�жϣ��Է�Խ��
			break;
	}
	start = span->_list;
	end = prev;
	span->_list = cur;//���߶������span��_listָ���µĵ�һ������
	span->usecount += batchsize;

	//���spanΪ�գ��Ͱ���β�嵽��󣬱��ַǿյ�span��ǰ��
	if (span->_list == nullptr)
	{
		spanlist.Erase(span);//ֻ���Ƴ������ǳ���ɾ��
		spanlist.PushBack(span);
	}

	//����
	spanlist.Unlock();
	return batchsize;

}

//��һ�������Ķ����ͷŵ�span���
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{

	size_t index = Size::Index(size);
	SpanList& spanlist = _spanlist[index];
	while (start)
	{
		void* next = NEXT_OBJ(start);
		//����
		spanlist.Lock();
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		//�ҵ���Ӧ��span֮�󣬽���������һ�span������ͷ�弴��
		NEXT_OBJ(start) = span->_list;
		span->_list = start;

		//��һ��span�Ķ���ȫ���ͷŻ���ʱ����span����pagecache�����ҽ���ҳ�ϲ�
		if (--span->usecount == 0)
		{
			spanlist.Erase(span);
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);

		}
		spanlist.Unlock();
		start = next;
	}
}