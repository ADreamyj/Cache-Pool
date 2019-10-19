#define _CRT_SECURE_NO_WARNINGS  1

#pragma once

#include"Public.h"

//Thread_Cache解决了高并发下，多线程去申请空间，需要用到一个锁
//这个时候，给每个线程分配一个Thread_Cache，这样就没有锁的竞争

#pragma once

class ThreadCache
{
public:
	
	// 申请对象
	void* Allocate(size_t size);

	// 释放对象
	void Deallocate(void* ptr, size_t size);

	// 从中心缓存获取对象    
	void* Fetch_From_Central_Cache(size_t index, size_t size);
	
	// 释放对象时，链表过长时，回收内存回到中心堆    
	void ListTooLong(FreeList* list, size_t size); 

private:
	FreeList _FreeList[num];//num是大小num越小，自由链表越多
};
//TLS
_declspec (thread) static ThreadCache* tlslist = nullptr;//保证各个线程之间互不干扰
//把它定义为静态全局变量的，是为了只在当前文件可见

//平时创建的变量属于所有的线程，它可以被任意修改，都会出现问题。
//有3个thread_Cache，如何找到Thread_Cache，保存Thread_Cache的指针，
//全局变量只能指向一个，其他的就没法调用，
//我们把所有的线程链接起来，通过一个全局的变量指向它，就可以找到我们想要的指针。
//但是这样的话，就必须要进行加锁


//我们可以这样定义，给每一个线程都定义一个全局变量，这样它们就不会互相干扰，也不用加锁。
//这就要使用到TLS，分为动态和静态。
//我们通常使用的时静态，静态就够我们进行使用了。

//_declspec (thread)不加这个的时候，就只有一个全局的TLS指针，指向很多的线程
//加上这个_declspec之后，就等于每个线程都会有一个TLS指针

//thread这个库文件时C++11之后提出来的，它可以同时兼容windows和Linux。


//在使用这个的时候，每个线程都有自己的TLS,当每个线程来申请内存时调用Allocate接口，
//第一次来就new上一个，第二次来就不用管了，调用Allocate,每个Thread_Cache中又有一个自由链表
//这个自由链表本质上是一个指针数组，但是我们把每个指针定义成了一个对象，
//所有也可以称其为对象数组，每个对象有两个成员，每一个对象是一个_freeList，
//_freeList后面有一个指针，指向了后面的对象块，size记录有多少个对象，
//Alloc调用freeList-》pop,这个操作不用返回，因为之后要经常的操作。
//每个freelist中有一个指针，以及size,

//pop就是将该对象返回出去，指向下一个对象。
//push就是把该对象头插进来就ok,

//每个thread_Cache里面有一个自由链表，每个自由链表的每个位置不是用一个指针去组织的
//在这里我们放入了一个对象，也就是说，我们现在取一个List,插入一个List都是通过这个
//对象来完成的。