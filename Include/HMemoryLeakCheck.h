#if defined(_DEBUG) || defined(DEBUG)

#ifndef APP_HMEMORYLEAKCHECK_H
#define APP_HMEMORYLEAKCHECK_H

//define APP_CHECK_MEM_LEAK in you IDE
//#define APP_CHECK_MEM_LEAK

#if defined(APP_CHECK_MEM_LEAK)
#include "AppMemoryLeakCheck.h"
#define new new(__FILE__, __LINE__)
#define malloc(_SIZE_) AppMalloc(_SIZE_, __FILE__, __LINE__)
#define free(_POS_)    AppFree(_POS_)
#define realloc(_POS_, _SIZE_) AppRealloc(_POS_, _SIZE_, __FILE__, __LINE__)
#endif  //APP_CHECK_MEM_LEAK

#endif //APP_HMEMORYLEAKCHECK_H
#endif //_DEBUG
