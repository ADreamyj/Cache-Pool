#define _CRT_SECURE_NO_WARNINGS  1

#pragma once

#include"Public.h"

//central_Cache：中心缓存是所有线程所共享的，它可以周期性的从
//Thread_Cache中回收对象，避免了一个线程占用太多内存。
//而其他线程的内存吃紧，达到内存分配在多个线程中均衡调度的问题
//central_Cache是存在竞争的，所以从这里取内存对象是需要加锁的，

//CentralCache要设计为单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance()//获取一份实例对象
	{
		return &_inst;
	}
	//从page cache获取一个span
	Span* GetOneSpan(SpanList& spanlist, size_t byte_size);

	//从中心缓存获取一定数量的对象返回给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t size);



	CentralCache(const CentralCache&) = delete;//防止拷贝
	CentralCache& operator=(const CentralCache&) = delete;//防赋值

private:
	SpanList _spanlist[num];

	CentralCache(){}//拷贝构造私有化

	//懒汉模式在第一次调用时才创建，多个线程第一次调用时，需要加锁，一加锁效率就会降低，所以不建议使用懒汉模式
	static CentralCache _inst;//这里写的是饿汉模式，本身就是线程安全的，它在main函数之前就创建，而线程是在main函数之后才创建的，这样的话，就不用加锁了，也没有双重检查
};

//Central_Cache在这里使用单例模式是来进行构建，在使用的时候需要加锁。
//每个thread_Cache在一个线程里面，每个线程都要来Central_Cache来申请空间。

// 1.central_cache本质是由一个哈希映射的span对象自由链表构成 
// 2.每个映射大小的empty span挂在一个链表中，nonempty span挂在一个链表中 
// 3.为了保证全局只有唯一的central_cache，这个类被设计成了单例模式。 

//申请内存：
//1. 当thread_cache中没有内存时，就会批量向central_ache申请一些内存对象，
//central_cache也有一个哈 希映射的freelist，freelist中挂着span，从span中取出对
//象给thread_cache，这个过程是需要加锁的。 
//2. central_cache中没有非空的span时，则将空的span链在一起，向page_cache申请一个
//span对象， span对象中是一些以页为单位的内存，切成需要的内存大小，并链接起来，
//挂到span中。 
//3. central_cache的span中有一个use_count，分配一个对象给thread_cache，就++use_count
//
//释放内存：
//1. 当thread_cache过长或者线程销毁，则会将内存释放回central_cache中的，释
//放回来时--use_count。 当use_count减到0时则表示所有对象都回到了span，则将span
//释放回page_cache，page_cache中 会对前后相邻的空闲页进行合并。

//申请空间，先向系统申请申请一块较大的空间，128页，这128页给一个span，要一页的span
//就把span分成1页span和128页span，保证span都是连续的，