#define _CRT_SECURE_NO_WARNINGS  1

//����Page_Cache,Central_Cache,Thread_Cache�����й��õ�ͷ�ļ���



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
const size_t num = 184;//��������������ô�����������
const size_t Npages = 129;//PageCache�����ҳ
const size_t Pageshift = 12;//2��12�η�(4096=4k=1ҳ)

//�������һ��ָ�����飬ÿ��������������ָ�롣
//ÿ��ָ��ָ�����һ������




inline static void*& NEXT_OBJ(void* obj)//��ȡ��ǰ�������һ�����󣬴�����
{
	return *((void**)obj);//32λƽ̨����4���ֽڣ�64λƽ̨����8���ֽ�
}

class FreeList
{
public:
	void PushRange(void* start, void* end, size_t n)//���������Ķ���
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

	void Push(void* obj)//����һ������
	{
		NEXT_OBJ(obj) = _list;// *((void**)obj) = _list; �Ȱ���ǿתΪvoid** ��Ȼ��
		_list = obj;
		_size++;
	}
	void* Pop()//����һ������
	{
		/*void* next = *((void**)_list);
		void* cur = _list;
		_list = next;
		return cur;*/

		void* obj = _list;
		_list = NEXT_OBJ(obj);
		_size--;

		return obj;//���ر������Ķ���
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
	void* _list = nullptr;//����������һ��ָ�룬ָ�����Ķ����
	size_t _size = 0;//��¼�������������ж��ٸ�����
	size_t _maxsize = 1;

};

class Size//���С���������±�λ��
{ 
	//���������Ƭ�˷��ʿ�����%12����
	/* [1,128]                 8byte����     freelist[0,16) 128/8=16
	[129,1024]              16byte����      freelist[16,72) (1024-129)/16=56
	[1025,8*1024]            128byte����     freelist[72,128) (8*1024-1025)/128=56
	[8*1024+1,64*1024]       1024byte����    freelist[72,184)*/

public:
	inline static size_t Index(size_t size)//����һ��size���㣬���size������������ĸ�λ��
	{
		//return _Index(size, 3);

		assert(size <= Max_Byte);
		//ÿ�����������
		static int group_array[4] = { 16, 56, 56, 56 };

		if (size <= 128)
		{
			return _Index(size, 3);//��8�ֽڶ���
		}
		else if (size <= 1024)
		{
			return _Index(size - 128, 4) + group_array[0];//��16�ֽڶ���
		}
		else if (size <= 8192)
		{
			return _Index(size - 1024, 7) + group_array[0] + group_array[1];//��128�ֽڶ���
		}
		else //(size<= 65536)
		{
			return _Index(size - 8 * 1024, 10) + group_array[0] + group_array[1] + group_array[2];//��1024�ֽڶ���
		}
	}

	//�����С�ļ���
	static inline size_t Roundup(size_t bytes)
	{
		assert(bytes <= Max_Byte);
		if (bytes <= 128)
		{
			return _Roundup(bytes, 3);//8�ֽڶ���
		}
		else if (bytes <= 1024)
		{
			return _Roundup(bytes, 4);//16�ֽڶ���
		}
		else if (bytes <= 8192)
		{
			return _Roundup(bytes, 7);//128�ֽڶ���
		}
		else // (bytes<=65536)
		{
			return _Roundup(bytes, 10);//1024�ֽڶ���
		}
	}
	//�ж�һ�δ����Ļ����ö��ٸ�����С���󣬾Ͷ���һ�㣻����󣬾�����һ��
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

	//����һ����PageCache��ȡ����ҳ
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;//һ�������ƶ����ֽ���
		npage >>= Pageshift;//����ȡ��
		if (npage == 0)
		{
			npage = 1;
		}
		return npage;
	}

	//inline static size_t Roundup(size_t size)//���뵽8�ֽڵ�������
	//{
	//	return _Roundup(size, 3);//����֮����Ҫ��װ������������Ϊ���ǵ�������ԽС���ڴ���Ƭ����Խ��
	//}
private:
	inline static size_t _Index(size_t size, size_t align)//����һ��size���㣬���size������������ĸ�λ��
	{
		//9-16(+7��)  --->16-23
		//return ((size+7)>>3)-1;
		size_t _align = 1 << align;//align = 3�Ļ� _align = 8
		return ((size + _align - 1) >> align) - 1;
	}

	//������ԽС���ڴ���ƬԽ�ᣬ�˷ѵ�Խ�٣���������Խ�࣬���Ҫ���зֶΡ�
	inline static size_t _Roundup(size_t size, size_t align)//���뵽8�ֽڵ�������
	{

		//return (size+7)&~7;
		size_t _align = 1 << align;
		return (size + (_align - 1))&~(_align - 1);//size+7�ٰ�λ����7ȡ�����պ�ÿ�ζ��ѵ���λ��Ϊ0���Ϳ��Դﵽÿ�ζ����뵽8����������
	}
};
#ifdef _WIN32
	typedef size_t PageID;
#else
	typedef long long PageID;
#endif


struct Span//���еģ������˷��ʵ�һ��Ͳ�����Ƴ�class��
{
	PageID _pageid = 0;//ҳ��
	size_t _npage = 0;//ҳ��
	//32λƽ̨��ҳ��Ϊ 4G / 4k,Ҳ����1024 * 1024ҳ��100W
	//64λƽ̨��ҳ��Ϊ 4G * 4G / 4k,

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _list = nullptr;//���Ӷ������������(_listָ���һ������)
	size_t objsize = 0;//һ������Ĵ�С

	size_t usecount = 0;//�����ʹ�ü���

};

//˫���ͷѭ����span�����ȽϿ��ơ�
class SpanList
{
public:
	SpanList()//���캯��
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

	void Insert(Span* cur, Span* newspan)//��cur��ǰ�����һ��newspan
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

	SpanList(const SpanList&) = delete;//û�п��������󣬵��ǲ�д�����������Զ�����ǳ���������Ծ�ֱ�Ӱѿ�����������Ϊdelete����
	SpanList& operator=(const SpanList&) = delete;//��ֵҲһ��

private:
	Span* _head;
	std::mutex _mutex;
};