#include "Page_Cache.h"

PageCache PageCache::_inst;//����ģʽ��.cpp�н��ж���

Span* PageCache::NewSpan(size_t n)//����һ���µ�span
{
	assert(n < Npages);
	if (!_spanlist[n].Empty())
	{
		return _spanlist[n].PopFront();
	}

	for (size_t i = n + 1; i < Npages; i++)
	{
		if (!_spanlist[i].Empty())
		{
			//ֻҪ�У���Ҫ�����з�
			Span* span = _spanlist[i].PopFront();
			Span* split = new Span;

			split->_pageid = span->_pageid + n;
			split->_npage = span->_npage - n;
			span->_npage = n;

			_spanlist[split->_npage].PushFront(split);
			return span;
		}
	}
	//��ϵͳ����һ��128ҳ��span
	Span* span = new Span;
	void* ptr = VirtualAlloc(0, (Npages - 1)*(1 << Pageshift),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);//��ϵͳ����,����һ���ӿڡ�

	span->_pageid = (PageID)ptr >> (Pageshift);//ҳ��=��ַ/1ҳ�Ĵ�С  ��ַ=ҳ��*1ҳ�Ĵ�С
	span->_npage = Npages - 1;
	for (size_t i = 0; i < span->_npage; i++)
	{
		_idspanmap[span->_pageid + i] = span;
	}

	_spanlist[span->_npage].PushFront(span);
	return NewSpan(n);
}

//��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PageID id = (PageID)obj >> Pageshift;
	auto it = _idspanmap.find(id);
	if (it != _idspanmap.end())
	{
		return it->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

//�ѿ���span�ͷŻ�PageCache,�������ڵ�span���кϲ�
void PageCache::ReleaseSpanToPageCache(Span* cur)
{
	//��ǰ�ϲ�
	while (1)
	{
		//����128ҳ�ģ��Ͳ��úϲ���
		if (cur->_npage >= Npages - 1)
		{
			break;
		}
		PageID curid = cur->_pageid;//��ǰҳ��span
		PageID previd = curid - 1;//ǰһҳ��span
		auto it = _idspanmap.find(previd);

		//û���ҵ�
		if (it == _idspanmap.end())
		{
			break;
		}
		//ǰһ��span������
		if (it->second->usecount != 0)
		{
			break;
		}

		//��ʾǰһ��span���ڶ��ҿ���
		Span* prev = it->second;
		//�Ȱ�prev���������Ƴ�
		_spanlist[prev->_npage].Erase(prev);
		//�ϲ�
		prev->_npage += cur->_npage;

		//�ϲ�֮����Ҫ�ĵ�ӳ���ϵ����������Ұָ������,�ͷ�˭����Ҫ�ĵ�˭��ӳ���ϵ
		for (PageID i = 0; i < cur->_npage; i++)
		{
			_idspanmap[cur->_pageid + i] = prev;
		}
		delete cur;

		//������ǰ�ϲ�
		cur = prev;
	}

	//���ϲ�
	while (1)
	{
		//����128ҳ�ģ��Ͳ��úϲ���
		if (cur->_npage >= Npages - 1)
		{
			break;
		}

		PageID curid = cur->_pageid;//��ǰҳ
		PageID nextid = curid + cur->_npage;//��һ��ҳ
		//std::map<PageID,Span*>::iterator it = _idspanmap.find(nextid);
		auto it = _idspanmap.find(nextid);

		//û���ҵ�
		if (it == _idspanmap.end())
		{
			break;
		}
		//�ҵ��ˣ����ǲ�����
		if (it->second->usecount != 0)
		{
			break;
		}
		//�ҵ��ˣ��ҿ��У����кϲ�
		Span* next = it->second;
		_spanlist[next->_npage].Erase(next);
		cur->_npage += next->_npage;
		//�ĵ�next��id->span��ӳ���ϵ
		for (PageID i = 0; i < next->_npage; i++)
		{
			_idspanmap[next->_pageid + i] = cur;
		}
		delete next;

	}
	cur->_list = nullptr;
	cur->objsize = 0;
	_spanlist[cur->_npage].PushFront(cur);
}