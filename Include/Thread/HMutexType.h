#ifndef APP_HMUTEXTYPE_H
#define APP_HMUTEXTYPE_H

namespace irr {

///Mutex type defines.
enum EMutexType {
    ///Recursive mutex, that is, the same mutex can be 
    /// locked multiple times by the same thread (but, of course,
    /// not by other threads).
    EMT_RECURSIVE,

    ///Not recursive mutex, but more faster than recursive mutex.
    EMT_NO_RECURSIVE
};

} //namespace irr

#endif //APP_HMUTEXTYPE_H
