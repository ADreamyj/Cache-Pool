#define _CRT_SECURE_NO_WARNINGS  1

#include "Thread_Cache.h"
#include "Central_Cache.h"
#include "Public.h"

//从中心缓存获取对象,就相当于把Central_Cache的内存放到thread_Cache;
void* ThreadCache::Fetch_From_Central_Cache(size_t index, size_t size)
{
	//从中心缓存获取批量的对象
	FreeList* freelist = &_FreeList[index];//找到链表中的某个位置
	size_t maxsize = freelist->MaxSize();
	size_t numtomove = min(Size::NumMoveSize(size), maxsize);
	void* start = nullptr;
	void* end = nullptr;
	size_t batchsize = CentralCache::GetInstance()->FetchRangeObj(start, end, numtomove, size);//向中心cache申请批量的内存，并返回实际拿到的对象个数

	if (batchsize > 1)
	{
		freelist->PushRange(NEXT_OBJ(start), end, batchsize - 1);//把剩下的(batchsize-1)个对象挂回到自由链表,剩余的一个要返回出去，申请内存用。
	}

	if (batchsize >= freelist->MaxSize())
	{
		freelist->SetMaxSize(maxsize + 1);
	}

	return start;//返回开始的内存。
}



//申请内存对象
void* ThreadCache::Allocate(size_t size)
{
	size_t index = Size::Index(size);
	//用Size这个类算传入的size所占的空间的大小，下标，下标是第几个就找第几个对象
	//之后调push，掉pop处理的就是下标这个位置。这就是面向对象。
	FreeList* freelist = &_FreeList[index];
	if (!freelist->Empty())//如果自由链表不为空
	{
		return freelist->Pop();
	}
	else//如果自由链表为空
	{
		return Fetch_From_Central_Cache(index, Size::Roundup(size));
	}
}

//释放内存对象
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t index = Size::Index(size);
	FreeList* freelist = &_FreeList[index];
	freelist->Push(ptr);

	//满足条件(释放回一个批量的内存)，释放回中心缓存
	if (freelist->Size() >= freelist->MaxSize())
	{
		//ListTooLong(freelist, size);
	}

}

//释放对象时，如果链表过长，则对象将放回到中心缓存
void ThreadCache::ListTooLong(FreeList* list, size_t size)
{
	void* start = list->PopRange();
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}