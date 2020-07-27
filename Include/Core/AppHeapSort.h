#ifndef APP_HEAPSORT_H
#define APP_HEAPSORT_H

#include "HConfig.h"

namespace app {
namespace core {

//! Sinks an element into the heap.
template<class T>
inline void AppHeapSink(T* array, s32 element, s32 max) {
    while((element << 1) < max) { // there is a left child
        s32 j = (element << 1);

        if(j + 1 < max && array[j] < array[j + 1])
            j = j + 1; // take right child

        if(array[element] < array[j]) {
            T t = array[j]; // swap elements
            array[j] = array[element];
            array[element] = t;
            element = j;
        } else
            return;
    }
}


//! Sorts an array with size 'size' using AppHeapSort.
template<class T>
inline void AppHeapSort(T* array_, s32 size) {
    // for AppHeapSink we pretent this is not c++, where
    // arrays start with index 0. So we decrease the array pointer,
    // the maximum always +2 and the element always +1

    T* virtualArray = array_ - 1;
    s32 virtualSize = size + 2;
    s32 i;

    // build heap

    for(i = ((size - 1) / 2); i >= 0; --i)
        AppHeapSink(virtualArray, i + 1, virtualSize - 1);

    // sort array, leave out the last element (0)
    for(i = size - 1; i > 0; --i) {
        T t = array_[0];
        array_[0] = array_[i];
        array_[i] = t;
        AppHeapSink(virtualArray, 1, i + 1);
    }
}

} // end namespace core
} // end namespace app

#endif //APP_HEAPSORT_H

