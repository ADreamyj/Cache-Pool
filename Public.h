#define _CRT_SECURE_NO_WARNINGS  1

//基于Page_Cache,Central_Cache,Thread_Cache的所有公用的头文件。



#pragma once

#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <assert.h>
#include <windows.h>
#include <algorithm>
#include <map>
#include <mutex>


using std::cout;
using std::endl;

const size_t Max_Byte = 64 * 1024;
const size_t num = 184;//自由链表中有这么多个自由链表
const size_t Npages = 129;//PageCache的最大页
const size_t Pageshift = 12;//2的12次方(4096=4k=1页)

//这个数组一个指针数组，每个数组里面存的是指针。
//每个指针指向的是一个链表。




inline static void*& NEXT_OBJ(void* obj)//获取当前对象的下一个对象，传引用
{
	return *((void**)obj);//32位平台上是4个字节，64位平台上是8个字节
}

class FreeList
{
public:
	void PushRange(void* start, void* end, size_t n)//插入批量的对象
	{
		NEXT_OBJ(end) = _list;
		start = _list;
		_size += n;
	}

	void* PopRange()
	{
		_size = 0;
		void* list = _list;
		_list = nullptr;

		return list;
	}

	void Push(void* obj)//插入一个对象
	{
		NEXT_OBJ(obj) = _list;// *((void**)obj) = _list; 先把它强转为void** ，然后
		_list = obj;
		_size++;
	}
	void* Pop()//弹出一个对象
	{
		/*void* next = *((void**)_list);
		void* cur = _list;
		_list = next;
		return cur;*/

		void* obj = _list;
		_list = NEXT_OBJ(obj);
		_size--;

		return obj;//返回被弹出的对象
	}
	bool Empty()
	{
		return _list == nullptr;
	}

	size_t Size()
	{
		return _size;
	}

	size_t MaxSize()
	{
		return _maxsize;
	}

	void SetMaxSize(size_t maxsize)
	{
		_maxsize = maxsize;
	}

private:
	void* _list = nullptr;//对象里面有一个指针，指向后面的对象块
	size_t _size = 0;//记录自由链表里面有多少个对象
	size_t _maxsize = 1;

};

class Size//算大小，即最后的下标位置
{ 
	//整体把内碎片浪费率控制在%12左右
	/* [1,128]                 8byte对齐     freelist[0,16) 128/8=16
	[129,1024]              16byte对齐      freelist[16,72) (1024-129)/16=56
	[1025,8*1024]            128byte对齐     freelist[72,128) (8*1024-1025)/128=56
	[8*1024+1,64*1024]       1024byte对齐    freelist[72,184)*/

public:
	inline static size_t Index(size_t size)//利用一个size计算，这个size在自由链表的哪个位置
	{
		//return _Index(size, 3);

		assert(size <= Max_Byte);
		//每个区间的链表
		static int group_array[4] = { 16, 56, 56, 56 };

		if (size <= 128)
		{
			return _Index(size, 3);//以8字节对齐
		}
		else if (size <= 1024)
		{
			return _Index(size - 128, 4) + group_array[0];//以16字节对齐
		}
		else if (size <= 8192)
		{
			return _Index(size - 1024, 7) + group_array[0] + group_array[1];//以128字节对齐
		}
		else //(size<= 65536)
		{
			return _Index(size - 8 * 1024, 10) + group_array[0] + group_array[1] + group_array[2];//以1024字节对齐
		}
	}

	//对齐大小的计算
	static inline size_t Roundup(size_t bytes)
	{
		assert(bytes <= Max_Byte);
		if (bytes <= 128)
		{
			return _Roundup(bytes, 3);//8字节对齐
		}
		else if (bytes <= 1024)
		{
			return _Roundup(bytes, 4);//16字节对齐
		}
		else if (bytes <= 8192)
		{
			return _Roundup(bytes, 7);//128字节对齐
		}
		else // (bytes<=65536)
		{
			return _Roundup(bytes, 10);//1024字节对齐
		}
	}
	//判断一次从中心缓存拿多少个对象，小对象，就多拿一点；大对象，就少拿一点
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = (int)(Max_Byte / size);
		if (num < 2)
		{
			num = 2;
		}
		if (num > 512)
		{
			num = 512;
		}
		return num;
	}

	//计算一次向PageCache获取多少页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;//一次批量移动的字节数
		npage >>= Pageshift;//向下取整
		if (npage == 0)
		{
			npage = 1;
		}
		return npage;
	}

	//inline static size_t Roundup(size_t size)//对齐到8字节的整数倍
	//{
	//	return _Roundup(size, 3);//这里之所以要封装对齐数，是因为考虑到对齐数越小，内存碎片问题越轻
	//}
private:
	inline static size_t _Index(size_t size, size_t align)//利用一个size计算，这个size在自由链表的哪个位置
	{
		//9-16(+7后)  --->16-23
		//return ((size+7)>>3)-1;
		size_t _align = 1 << align;//align = 3的话 _align = 8
		return ((size + _align - 1) >> align) - 1;
	}

	//对齐数越小，内存碎片越轻，浪费的越少，自由链表越多，这个要进行分段。
	inline static size_t _Roundup(size_t size, size_t align)//对齐到8字节的整数倍
	{

		//return (size+7)&~7;
		size_t _align = 1 << align;
		return (size + (_align - 1))&~(_align - 1);//size+7再按位与上7取反，刚好每次都把低三位置为0，就可以达到每次都对齐到8的整数倍处
	}
};
#ifdef _WIN32
	typedef size_t PageID;
#else
	typedef long long PageID;
#endif


struct Span//公有的，给别人访问的一般就不用设计成class了
{
	PageID _pageid = 0;//页号
	size_t _npage = 0;//页数
	//32位平台下页数为 4G / 4k,也就是1024 * 1024页，100W
	//64位平台下页数为 4G * 4G / 4k,

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _list = nullptr;//链接对象的自由链表(_list指向第一个对象)
	size_t objsize = 0;//一个对象的大小

	size_t usecount = 0;//对象的使用计数

};

//双向带头循环的span链表，比较控制。
class SpanList
{
public:
	SpanList()//构造函数
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}
	Span* End()
	{
		return _head;
	}

	void PushBack(Span* newspan)
	{
		Insert(End(), newspan);
	}

	void PushFront(Span* newspan)
	{
		Insert(Begin(), newspan);
	}

	Span* PopBack()
	{
		Span* span = _head->_prev;
		Erase(span);

		return span;
	}

	Span* PopFront()
	{
		Span* span = _head->_next;
		Erase(span);

		return span;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void Insert(Span* cur, Span* newspan)//在cur的前面插入一个newspan
	{
		//prev newspan cur
		Span* prev = cur->_prev;

		newspan->_prev = prev;
		prev->_next = newspan;
		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Erase(Span* cur)
	{
		//prev cur  next
		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	void Lock()
	{
		_mutex.lock();
	}
	void Unlock()
	{
		_mutex.unlock();
	}

	~SpanList()
	{
		Span* cur = _head->_next;
		while (cur != _head)
		{
			Span* next = cur->_next;
			delete cur;
			cur = next;
		}
		delete _head;
		_head = nullptr;
	}

	SpanList(const SpanList&) = delete;//没有拷贝的需求，但是不写，编译器会自动生成浅拷贝，所以就直接把拷贝构造设置为delete函数
	SpanList& operator=(const SpanList&) = delete;//赋值也一样

private:
	Span* _head;
	std::mutex _mutex;
};