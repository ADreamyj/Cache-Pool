#define _CRT_SECURE_NO_WARNINGS  1

#include "Thread_Cache.h"
#include "Public.h"
#include "Concurrent_Alloc.h"
#include "Page_Cache.h"

void SizeTest()
{
	/*cout<<Size::Index(10)<<endl;
	cout << Size::Index(16)<<endl;
	cout << Size::Index(128) << endl;
	cout << Size::Index(129) << endl;
	cout << Size::Index(1025) << endl;
	cout << Size::Index(1024+129) << endl;
	cout << Size::Index(8*1024+1) << endl;
	cout << Size::Index(8*1024 + 1024) << endl;*/

	cout << Size::Roundup(10) << endl;
	cout << Size::Roundup(1025) << endl;
	cout << Size::Roundup(1024 * 8 + 1) << endl;



	/*cout << Size::NumMovePage(16) << endl;
	cout <<Size:: NumMovePage(1024) << endl;
	cout << Size::NumMovePage(1024 * 8) << endl;
	cout << Size::NumMovePage(1024 * 64) << endl;*/
}

void Alloc(size_t n)
{
	std::vector<void*> v;
	for (size_t i = 0; i < n; i++)
	{
		void* ptr = ConcurrentAlloc(12);
		v.push_back(ptr);
		//cout << ptr << endl;
		printf("Allocobj:->%d->%p\n", i, ptr);
	}

	for (size_t i = 0; i < n; i++)
	{
		ConcurrentFree(v[i], 12);
		printf("Deallocobj:->%d->%p\n", i, v[i]);
	}
	v.clear();

	cout << endl;
	cout << endl;

	for (size_t i = 0; i < n; i++)
	{
		void* ptr = ConcurrentAlloc(12);
		v.push_back(ptr);
		printf("Allocobj:->%d->%p\n", i, ptr);
	}
	for (size_t i = 0; i < n; i++)
	{
		ConcurrentFree(v[i], 12);
		printf("Deallocobj:->%d->%p\n", i, v[i]);
	}
	v.clear();
}

void TestThreadCache()
{
	std::thread t1(Alloc, 1000);
	std::thread t2(Alloc, 1000);
	std::thread t3(Alloc, 1000);
	std::thread t4(Alloc, 1000);

	t1.join();
	t2.join();
	t3.join();
	t4.join();

}

void TestCentralCache()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 8; i++)
	{
		v.push_back(ConcurrentAlloc(10));
	}
	for (size_t i = 0; i < 8; i++)
	{
		//ConcurrentFree(v[i], 10);
		cout << v[i] << endl;
	}
}

void TestPageCache()
{
	PageCache::GetInstance()->NewSpan(2);
}

void TestConcurrentAllocFree()
{
	size_t n = 100000;
	std::vector<void*> v;
	for (size_t i = 0; i < n; i++)
	{
		void* ptr = ConcurrentAlloc(12);
		v.push_back(ptr);
		//cout << ptr << endl;
		//printf("Allocobj:->%d->%p\n", i, ptr);
	}

	for (size_t i = 0; i < n; i++)
	{
		ConcurrentFree(v[i], 12);
		//printf("Deallocobj:->%d->%p\n", i,v[i]);
	}
	v.clear();

	cout << endl;
	cout << endl;

	for (size_t i = 0; i < n; i++)
	{
		void* ptr = ConcurrentAlloc(12);
		v.push_back(ptr);
		//printf("Allocobj:->%d->%p\n", i, ptr);
	}
	for (size_t i = 0; i < n; i++)
	{
		ConcurrentFree(v[i], 12);
		//printf("Deallocobj:->%d->%p\n", i, v[i]);
	}
	v.clear();
}

//int main()
//{
//	
//	SizeTest();
//	TestThreadCache();
//	/*TestCentralCache();
//	TestPageCache();
//	TestConcurrentAllocFree();
//	void* ptr = VirtualAlloc(0, (Npages - 1)*(1 << Pageshift),
//		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//	PageID id = (PageID)ptr>>Pageshift;
//	cout << id << endl;
//	void* pageptr = (void*)(id << Pageshift);
//	cout << pageptr << endl;
//	cout << ptr << endl;*/
//
//	system("pause");
//	return 0;
//}