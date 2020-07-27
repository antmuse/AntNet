#ifndef APP_APPARRAY_H
#define APP_APPARRAY_H

#include "HConfig.h"
#include "AppHeapSort.h"
#include "TAllocator.h"
#include "AppMath.h"

namespace app {
namespace core {

//! Self reallocating template TArray (like stl vector) with additional features.
/** Some features are: Heap sorting, binary search methods, easier debugging.
*/
template <class T, typename TAlloc = TAllocator<T> >
class TArray {
public:

    //! Default constructor for empty TArray.
    TArray()
        : mData(0), mAllocated(0), mUsed(0),
        mStrategy(E_STRATEGY_DOUBLE), mFreeWhenDestroyed(true), mIsSorted(true) {
    }


    //! Constructs an TArray and allocates an initial chunk of memory.
    /** \param start_count Amount of elements to pre-allocate. */
    TArray(u32 start_count)
        : mData(0), mAllocated(0), mUsed(0),
        mStrategy(E_STRATEGY_DOUBLE), mFreeWhenDestroyed(true), mIsSorted(true) {
        reallocate(start_count);
    }


    //! Copy constructor
    TArray(const TArray<T, TAlloc>& other) : mData(0) {
        *this = other;
    }


    //! Destructor.
    /** Frees mAllocated memory, if setFreeWhenDestroyed was not set to
    false by the user before. */
    ~TArray() {
        clear();
    }


    //! Reallocates the TArray, make it bigger or smaller.
    /** \param new_size New size of TArray.
    \param canShrink Specifies whether the TArray is reallocated even if
    enough space is available. Setting this flag to false can speed up
    TArray usage, but may use more memory than required by the mData.
    */
    void reallocate(u32 new_size, bool canShrink = true) {
        if (mAllocated == new_size)
            return;
        if (!canShrink && (new_size < mAllocated))
            return;

        T* old_data = mData;

        mData = mAllocator.allocate(new_size); //new T[new_size];
        mAllocated = new_size;

        // copy old mData
        s32 end = mUsed < new_size ? mUsed : new_size;

        for (s32 i = 0; i < end; ++i) {
            // mData[i] = old_data[i];
            mAllocator.construct(&mData[i], old_data[i]);
        }

        // destruct old mData
        for (u32 j = 0; j < mUsed; ++j)
            mAllocator.destruct(&old_data[j]);

        if (mAllocated < mUsed)
            mUsed = mAllocated;

        mAllocator.deallocate(old_data); //delete [] old_data;
    }


    //! set a new allocation mStrategy
    /** if the maximum size of the TArray is unknown, you can define how big the
    allocation should happen.
    \param newStrategy New mStrategy to apply to this TArray. */
    void setAllocStrategy(EAllocStrategy newStrategy = E_STRATEGY_DOUBLE) {
        mStrategy = newStrategy;
    }


    //! Adds an element at back of TArray.
    /** If the TArray is too small to add this new element it is made bigger.
    \param element: Element to add at the back of the TArray. */
    void pushBack(const T& element) {
        insert(element, mUsed);
    }


    //! Adds an element at the front of the TArray.
    /** If the TArray is to small to add this new element, the TArray is
    made bigger. Please note that this is slow, because the whole TArray
    needs to be copied for this.
    \param element Element to add at the back of the TArray. */
    void pushFront(const T& element) {
        insert(element);
    }


    //! Insert item into TArray at specified position.
    /** Please use this only if you know what you are doing (possible
    performance loss). The preferred method of adding elements should be
    pushBack().
    \param element: Element to be inserted
    \param index: Where position to insert the new element. */
    void insert(const T& element, u32 index = 0) {
        APP_ASSERT(index <= mUsed);

        if (mUsed + 1 > mAllocated) {
            // this doesn't work if the element is in the same
            // TArray. So we'll copy the element first to be sure
            // we'll get no mData corruption
            const T e(element);

            // increase mData block
            u32 newAlloc;
            switch (mStrategy) {
            case E_STRATEGY_DOUBLE:
                newAlloc = mUsed + 1 + (mAllocated < 500 ?
                    (mAllocated < 5 ? 5 : mUsed) : mUsed >> 2);
                break;
            default:
            case E_STRATEGY_SAFE:
                newAlloc = mUsed + 1;
                break;
            }
            reallocate(newAlloc);

            // move TArray content and construct new element
            // first move end one up
            for (u32 i = mUsed; i > index; --i) {
                if (i < mUsed)
                    mAllocator.destruct(&mData[i]);
                mAllocator.construct(&mData[i], mData[i - 1]); // mData[i] = mData[i-1];
            }
            // then add new element
            if (mUsed > index)
                mAllocator.destruct(&mData[index]);
            mAllocator.construct(&mData[index], e); // mData[index] = e;
        } else {
            // element inserted not at end
            if (mUsed > index) {
                // create one new element at the end
                mAllocator.construct(&mData[mUsed], mData[mUsed - 1]);

                // move the rest of the TArray content
                for (u32 i = mUsed - 1; i > index; --i) {
                    mData[i] = mData[i - 1];
                }
                // insert the new element
                mData[index] = element;
            } else {
                // insert the new element to the end
                mAllocator.construct(&mData[index], element);
            }
        }
        // set to false as we don't know if we have the comparison operators
        mIsSorted = false;
        ++mUsed;
    }


    //! Clears the TArray and deletes all mAllocated memory.
    void clear() {
        if (mFreeWhenDestroyed) {
            for (u32 i = 0; i < mUsed; ++i) {
                mAllocator.destruct(&mData[i]);
            }
            mAllocator.deallocate(mData); // delete [] mData;
        }
        mData = 0;
        mUsed = 0;
        mAllocated = 0;
        mIsSorted = true;
    }


    //! Sets pointer to new TArray, using this as new workspace.
    /** Make sure that setFreeWhenDestroyed is mUsed properly.
    \param newPointer: Pointer to new TArray of elements.
    \param size: Size of the new TArray.
    \param _is_sorted Flag which tells whether the new TArray is already
    sorted.
    \param _free_when_destroyed Sets whether the new memory area shall be
    freed by the TArray upon destruction, or if this will be up to the user
    application. */
    void setPointer(T* newPointer, u32 size, bool _is_sorted = false, bool _free_when_destroyed = true) {
        clear();
        mData = newPointer;
        mAllocated = size;
        mUsed = size;
        mIsSorted = _is_sorted;
        mFreeWhenDestroyed = _free_when_destroyed;
    }


    //! Sets if the TArray should delete the memory it uses upon destruction.
    /** Also clear and setPointer will only delete the (original) memory
    area if this flag is set to true, which is also the default. The
    methods reallocate, setUsed, pushBack, pushFront, insert, and erase
    will still try to deallocate the original memory, which might cause
    troubles depending on the intended use of the memory area.
    \param f If true, the TArray frees the mAllocated memory in its
    destructor, otherwise not. The default is true. */
    void setFreeWhenDestroyed(bool f) {
        mFreeWhenDestroyed = f;
    }


    //! Sets the size of the TArray and allocates new elements if necessary.
    /** Please note: This is only secure when using it with simple types,
    because no default constructor will be called for the added elements.
    \param usedNow Amount of elements now used. */
    void setUsed(u32 usedNow) {
        if (mAllocated < usedNow) {
            reallocate(usedNow);
        }
        mUsed = usedNow;
    }


    //! Assignment operator
    const TArray<T, TAlloc>& operator=(const TArray<T, TAlloc>& other) {
        if (this == &other) {
            return *this;
        }
        mStrategy = other.mStrategy;

        if (mData)
            clear();

        //if (mAllocated < other.mAllocated)
        if (other.mAllocated == 0)
            mData = 0;
        else
            mData = mAllocator.allocate(other.mAllocated); // new T[other.mAllocated];

        mUsed = other.mUsed;
        mFreeWhenDestroyed = true;
        mIsSorted = other.mIsSorted;
        mAllocated = other.mAllocated;

        for (u32 i = 0; i < other.mUsed; ++i)
            mAllocator.construct(&mData[i], other.mData[i]); // mData[i] = other.mData[i];

        return *this;
    }


    //! Equality operator
    bool operator==(const TArray<T, TAlloc>& other) const {
        if (mUsed != other.mUsed)
            return false;

        for (u32 i = 0; i < other.mUsed; ++i)
            if (mData[i] != other[i])
                return false;
        return true;
    }


    //! Inequality operator
    bool operator!=(const TArray<T, TAlloc>& other) const {
        return !(*this == other);
    }


    //! Direct access operator
    T& operator[](u32 index) {
        APP_ASSERT(index < mUsed);

        return mData[index];
    }


    //! Direct const access operator
    const T& operator[](u32 index) const {
        APP_ASSERT(index < mUsed);

        return mData[index];
    }


    //! Gets last element.
    T& getLast() {
        APP_ASSERT(!mUsed); // access violation
        return mData[mUsed - 1];
    }


    //! Gets last element
    const T& getLast() const {
        APP_ASSERT(!mUsed); // access violation

        return mData[mUsed - 1];
    }


    //! Gets a pointer to the TArray.
    /** \return Pointer to the TArray. */
    T* pointer() {
        return mData;
    }


    //! Gets a const pointer to the TArray.
    /** \return Pointer to the TArray. */
    const T* constPointer() const {
        return mData;
    }


    //! Get number of occupied elements of the TArray.
    /** \return Size of elements in the TArray which are actually occupied. */
    u32 size() const {
        return mUsed;
    }


    //! Get amount of memory mAllocated.
    /** \return Amount of memory mAllocated. The amount of bytes
    mAllocated would be allocated_size() * sizeof(ElementTypeUsed); */
    u32 allocated_size() const {
        return mAllocated;
    }


    //! Check if TArray is empty.
    /** \return True if the TArray is empty false if not. */
    bool empty() const {
        return mUsed == 0;
    }


    //! Sorts the TArray using AppHeapSort.
    /** There is no additional memory waste and the algorithm performs
    O(n*log n) in worst case. */
    void sort() {
        if (!mIsSorted && mUsed > 1) {
            AppHeapSort(mData, mUsed);
        }
        mIsSorted = true;
    }


    //! Performs a binary search for an element, returns -1 if not found.
    /** The TArray will be sorted before the binary search if it is not
    already sorted. Caution is advised! Be careful not to call this on
    unsorted const arrays, or the slower method will be mUsed.
    \param element Element to search for.
    \return Position of the searched element if it was found,
    otherwise -1 is returned. */
    s32 binarySearch(const T& element) {
        sort();
        return binarySearch(element, 0, mUsed - 1);
    }


    //! Performs a binary search for an element if possible, returns -1 if not found.
    /** This method is for const arrays and so cannot call sort(), if the TArray is
    not sorted then linearSearch will be mUsed instead. Potentially very slow!
    \param element Element to search for.
    \return Position of the searched element if it was found,
    otherwise -1 is returned. */
    s32 binarySearch(const T& element) const {
        if (mIsSorted)
            return binarySearch(element, 0, mUsed - 1);
        else
            return linearSearch(element);
    }


    //! Performs a binary search for an element, returns -1 if not found.
    /** \param element: Element to search for.
    \param left First left index
    \param right Last right index.
    \return Position of the searched element if it was found, otherwise -1
    is returned. */
    s32 binarySearch(const T& element, s32 left, s32 right) const {
        if (!mUsed)
            return -1;

        s32 m;

        do {
            m = (left + right) >> 1;

            if (element < mData[m])
                right = m - 1;
            else
                left = m + 1;

        } while ((element < mData[m] || mData[m] < element) && left <= right);
        // this last line equals to:
        // " while((element != TArray[m]) && left<=right);"
        // but we only want to use the '<' operator.
        // the same in next line, it is "(element == TArray[m])"


        if (!(element < mData[m]) && !(mData[m] < element))
            return m;

        return -1;
    }


    //! Performs a binary search for an element, returns -1 if not found.
    //! it is used for searching a multiset
    /** The TArray will be sorted before the binary search if it is not
    already sorted.
    \param element	Element to search for.
    \param &last	return lastIndex of equal elements
    \return Position of the first searched element if it was found,
    otherwise -1 is returned. */
    s32 binary_search_multi(const T& element, s32 &last) {
        sort();
        s32 index = binarySearch(element, 0, mUsed - 1);
        if (index < 0)
            return index;

        // The search can be somewhere in the middle of the set
        // look linear previous and past the index
        last = index;

        while (index > 0 && !(element < mData[index - 1]) && !(mData[index - 1] < element)) {
            index -= 1;
        }
        // look linear up
        while (last < (s32)mUsed - 1 && !(element < mData[last + 1]) && !(mData[last + 1] < element)) {
            last += 1;
        }

        return index;
    }


    //! Finds an element in linear time, which is very slow.
    /** Use binarySearch for faster finding. Only works if ==operator is
    implemented.
    \param element Element to search for.
    \return Position of the searched element if it was found, otherwise -1
    is returned. */
    s32 linearSearch(const T& element) const {
        for (u32 i = 0; i < mUsed; ++i)
            if (element == mData[i])
                return (s32)i;

        return -1;
    }


    //! Finds an element in linear time, which is very slow.
    /** Use binarySearch for faster finding. Only works if ==operator is
    implemented.
    \param element: Element to search for.
    \return Position of the searched element if it was found, otherwise -1
    is returned. */
    s32 linearReverseSearch(const T& element) const {
        for (s32 i = mUsed - 1; i >= 0; --i) {
            if (mData[i] == element)
                return i;
        }
        return -1;
    }


    //! Erases an element from the TArray.
    /** May be slow, because all elements following after the erased
    element have to be copied.
    \param index: Index of element to be erased. */
    void erase(u32 index) {
        APP_ASSERT(index < mUsed); // access violation

        for (u32 i = index + 1; i < mUsed; ++i) {
            mAllocator.destruct(&mData[i - 1]);
            mAllocator.construct(&mData[i - 1], mData[i]); // mData[i-1] = mData[i];
        }
        mAllocator.destruct(&mData[mUsed - 1]);
        --mUsed;
    }

    /**
    * @brief 快速移除&用末位填补
    * @param index 移除元素所在位置
    */
    void quickErase(u32 index) {
        APP_ASSERT(index < mUsed);
        if (index + 1 < mUsed) {
            mAllocator.destruct(&mData[index]);
            mAllocator.construct(&mData[index], mData[mUsed - 1]);//mData[index] = mData[mUsed-1];
            //if (index > 0) {
            //    setSorted(false);
            //}
        }
        mAllocator.destruct(&mData[mUsed - 1]);
        --mUsed;
    }

    //! Erases some elements from the TArray.
    /** May be slow, because all elements following after the erased
    element have to be copied.
    \param index: Index of the first element to be erased.
    \param count: Amount of elements to be erased. */
    void erase(u32 index, s32 count) {
        if (index >= mUsed || count < 1) {
            return;
        }
        if (index + count > mUsed) {
            count = mUsed - index;
        }
        u32 i;
        for (i = index; i < index + count; ++i) {
            mAllocator.destruct(&mData[i]);
        }
        for (i = index + count; i < mUsed; ++i) {
            if (i - count >= index + count) {	// not already destructed before loop
                mAllocator.destruct(&mData[i - count]);
            }
            mAllocator.construct(&mData[i - count], mData[i]); // mData[i-count] = mData[i];
            if (i >= mUsed - count) {	// those which are not overwritten
                mAllocator.destruct(&mData[i]);
            }
        }
        mUsed -= count;
    }


    //! Sets if the TArray is sorted
    void setSorted(bool val) {
        mIsSorted = val;
    }


    //! Swap the content of this TArray container with the content of another TArray
    /** Afterwards this object will contain the content of the other object and the other
    object will contain the content of this object.
    \param other Swap content with this object	*/
    void swap(TArray<T, TAlloc>& other) {
        core::swap(mData, other.mData);
        core::swap(mAllocated, other.mAllocated);
        core::swap(mUsed, other.mUsed);
        core::swap(mAllocator, other.mAllocator);	// memory is still released by the same mAllocator mUsed for allocation
        EAllocStrategy helper_strategy(mStrategy);	// can't use core::swap with bitfields
        mStrategy = other.mStrategy;
        other.mStrategy = helper_strategy;
        bool helper_free_when_destroyed(mFreeWhenDestroyed);
        mFreeWhenDestroyed = other.mFreeWhenDestroyed;
        other.mFreeWhenDestroyed = helper_free_when_destroyed;
        bool helper_is_sorted(mIsSorted);
        mIsSorted = other.mIsSorted;
        other.mIsSorted = helper_is_sorted;
    }


private:
    T* mData;
    u32 mAllocated;
    u32 mUsed;
    TAlloc mAllocator;
    EAllocStrategy mStrategy : 4;
    bool mFreeWhenDestroyed : 1;
    bool mIsSorted : 1;
};


} //namespace core
} //namespace app

#endif //APP_APPARRAY_H

