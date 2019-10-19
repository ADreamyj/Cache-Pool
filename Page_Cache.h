#define _CRT_SECURE_NO_WARNINGS  1

#pragma once

#include"Public.h"

//Page_Cache������ҳ���棬��һ�ָ��߼���Ļ��棬�洢�ڴ���
//��ҳΪ��λ������ģ���Central_Cache��û�л�������ʱ��
//�ͻ��Page_Cache�з����һ��������page�����и�ɶ�����С
//��С�ڴ�죬�����Central_Cache,Page_Cache�����
//Central_Cache�������������span���󣬲��Һϲ����ڵ�ҳ
//��ɸ����ҳ������ڴ���Ƭ�����⡣


//PageCacheҲҪ��Ƴɵ���ģʽ

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* NewSpan(size_t n);//����һ���µ�span

	//��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	//�ѿ���span�ͷŻ�PageCache,�������ڵ�span���кϲ�
	void ReleaseSpanToPageCache(Span* span);

	PageCache(const PageCache&) = delete;//�������캯������Ϊdelete����
	PageCache& operator=(const PageCache&) = delete;//��ֵ��������Ϊdelete����

private:
	PageCache(){}//���캯��˽�л�
	static PageCache _inst;
private:
	SpanList _spanlist[Npages];
	std::map<PageID, Span*> _idspanmap;
};