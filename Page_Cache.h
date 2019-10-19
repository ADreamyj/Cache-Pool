#define _CRT_SECURE_NO_WARNINGS  1

#pragma once

#include"Public.h"

//Page_Cache：叫做页缓存，是一种更高级别的缓存，存储内存是
//以页为单位及分配的，当Central_Cache中没有缓存对象的时候，
//就会从Page_Cache中分配出一定数量的page，并切割成定长大小
//的小内存快，分配给Central_Cache,Page_Cache会回收
//Central_Cache满足对象条件的span对象，并且合并相邻的页
//组成更大的页，解决内存碎片的问题。


//PageCache也要设计成单例模式

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* NewSpan(size_t n);//申请一个新的span

	//获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	//把空闲span释放回PageCache,并与相邻的span进行合并
	void ReleaseSpanToPageCache(Span* span);

	PageCache(const PageCache&) = delete;//拷贝构造函数声明为delete函数
	PageCache& operator=(const PageCache&) = delete;//赋值函数声明为delete函数

private:
	PageCache(){}//构造函数私有化
	static PageCache _inst;
private:
	SpanList _spanlist[Npages];
	std::map<PageID, Span*> _idspanmap;
};