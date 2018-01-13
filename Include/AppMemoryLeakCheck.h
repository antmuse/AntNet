#if defined(_DEBUG) || defined(DEBUG)
#if defined(APP_CHECK_MEM_LEAK)

#ifndef APP_MEMORYLEAKCHECK_H
#define APP_MEMORYLEAKCHECK_H

void* AppRealloc(void* pos, size_t size, const char* file, unsigned int line);

void* AppMalloc(size_t size, const char* file, unsigned int line);

void AppFree(void* pos);

void* operator new(size_t size);

void* operator new(size_t size, const char* file, unsigned int line);

void* operator new[](size_t size, const char* file, unsigned int line);

void operator delete(void* ptr);

void operator delete[](void* ptr);

void AppPrintMemoryCheckResult();

#endif //APP_MEMORYLEAKCHECK_H

#endif //APP_CHECK_MEM_LEAK
#endif //_DEBUG