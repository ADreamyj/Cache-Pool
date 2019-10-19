#define _CRT_SECURE_NO_WARNINGS  1

#pragma once

#include"Public.h"

//central_Cache�����Ļ����������߳�������ģ������������ԵĴ�
//Thread_Cache�л��ն��󣬱�����һ���߳�ռ��̫���ڴ档
//�������̵߳��ڴ�Խ����ﵽ�ڴ�����ڶ���߳��о�����ȵ�����
//central_Cache�Ǵ��ھ����ģ����Դ�����ȡ�ڴ��������Ҫ�����ģ�

//CentralCacheҪ���Ϊ����ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance()//��ȡһ��ʵ������
	{
		return &_inst;
	}
	//��page cache��ȡһ��span
	Span* GetOneSpan(SpanList& spanlist, size_t byte_size);

	//�����Ļ����ȡһ�������Ķ��󷵻ظ�thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t size);



	CentralCache(const CentralCache&) = delete;//��ֹ����
	CentralCache& operator=(const CentralCache&) = delete;//����ֵ

private:
	SpanList _spanlist[num];

	CentralCache(){}//��������˽�л�

	//����ģʽ�ڵ�һ�ε���ʱ�Ŵ���������̵߳�һ�ε���ʱ����Ҫ������һ����Ч�ʾͻή�ͣ����Բ�����ʹ������ģʽ
	static CentralCache _inst;//����д���Ƕ���ģʽ����������̰߳�ȫ�ģ�����main����֮ǰ�ʹ��������߳�����main����֮��Ŵ����ģ������Ļ����Ͳ��ü����ˣ�Ҳû��˫�ؼ��
};

//Central_Cache������ʹ�õ���ģʽ�������й�������ʹ�õ�ʱ����Ҫ������
//ÿ��thread_Cache��һ���߳����棬ÿ���̶߳�Ҫ��Central_Cache������ռ䡣

// 1.central_cache��������һ����ϣӳ���span�������������� 
// 2.ÿ��ӳ���С��empty span����һ�������У�nonempty span����һ�������� 
// 3.Ϊ�˱�֤ȫ��ֻ��Ψһ��central_cache������౻��Ƴ��˵���ģʽ�� 

//�����ڴ棺
//1. ��thread_cache��û���ڴ�ʱ���ͻ�������central_ache����һЩ�ڴ����
//central_cacheҲ��һ���� ϣӳ���freelist��freelist�й���span����span��ȡ����
//���thread_cache�������������Ҫ�����ġ� 
//2. central_cache��û�зǿյ�spanʱ���򽫿յ�span����һ����page_cache����һ��
//span���� span��������һЩ��ҳΪ��λ���ڴ棬�г���Ҫ���ڴ��С��������������
//�ҵ�span�С� 
//3. central_cache��span����һ��use_count������һ�������thread_cache����++use_count
//
//�ͷ��ڴ棺
//1. ��thread_cache���������߳����٣���Ὣ�ڴ��ͷŻ�central_cache�еģ���
//�Ż���ʱ--use_count�� ��use_count����0ʱ���ʾ���ж��󶼻ص���span����span
//�ͷŻ�page_cache��page_cache�� ���ǰ�����ڵĿ���ҳ���кϲ���

//����ռ䣬����ϵͳ��������һ��ϴ�Ŀռ䣬128ҳ����128ҳ��һ��span��Ҫһҳ��span
//�Ͱ�span�ֳ�1ҳspan��128ҳspan����֤span���������ģ�