#include "Page_Cache.h"

PageCache PageCache::_inst;//单例模式在.cpp中进行定义

Span* PageCache::NewSpan(size_t n)//申请一个新的span
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
			//只要有，就要进行切分
			Span* span = _spanlist[i].PopFront();
			Span* split = new Span;

			split->_pageid = span->_pageid + n;
			split->_npage = span->_npage - n;
			span->_npage = n;

			_spanlist[split->_npage].PushFront(split);
			return span;
		}
	}
	//向系统申请一个128页的span
	Span* span = new Span;
	void* ptr = VirtualAlloc(0, (Npages - 1)*(1 << Pageshift),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);//向系统申请,这是一个接口。

	span->_pageid = (PageID)ptr >> (Pageshift);//页号=地址/1页的大小  地址=页号*1页的大小
	span->_npage = Npages - 1;
	for (size_t i = 0; i < span->_npage; i++)
	{
		_idspanmap[span->_pageid + i] = span;
	}

	_spanlist[span->_npage].PushFront(span);
	return NewSpan(n);
}

//获取从对象到span的映射
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

//把空闲span释放回PageCache,并与相邻的span进行合并
void PageCache::ReleaseSpanToPageCache(Span* cur)
{
	//向前合并
	while (1)
	{
		//超过128页的，就不用合并啦
		if (cur->_npage >= Npages - 1)
		{
			break;
		}
		PageID curid = cur->_pageid;//当前页的span
		PageID previd = curid - 1;//前一页的span
		auto it = _idspanmap.find(previd);

		//没有找到
		if (it == _idspanmap.end())
		{
			break;
		}
		//前一个span不空闲
		if (it->second->usecount != 0)
		{
			break;
		}

		//表示前一个span存在而且空闲
		Span* prev = it->second;
		//先把prev从链表中移除
		_spanlist[prev->_npage].Erase(prev);
		//合并
		prev->_npage += cur->_npage;

		//合并之后，需要改掉映射关系，否则会出现野指针问题,释放谁，就要改掉谁的映射关系
		for (PageID i = 0; i < cur->_npage; i++)
		{
			_idspanmap[cur->_pageid + i] = prev;
		}
		delete cur;

		//继续向前合并
		cur = prev;
	}

	//向后合并
	while (1)
	{
		//超过128页的，就不用合并啦
		if (cur->_npage >= Npages - 1)
		{
			break;
		}

		PageID curid = cur->_pageid;//当前页
		PageID nextid = curid + cur->_npage;//下一个页
		//std::map<PageID,Span*>::iterator it = _idspanmap.find(nextid);
		auto it = _idspanmap.find(nextid);

		//没有找到
		if (it == _idspanmap.end())
		{
			break;
		}
		//找到了，但是不空闲
		if (it->second->usecount != 0)
		{
			break;
		}
		//找到了，且空闲，进行合并
		Span* next = it->second;
		_spanlist[next->_npage].Erase(next);
		cur->_npage += next->_npage;
		//改掉next的id->span的映射关系
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