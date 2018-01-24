#include "CSpinlock.h"
#include "HAtomicOperator.h"

namespace irr {


CSpinlock::CSpinlock() :
    mValue(0) {
}


CSpinlock::~CSpinlock() {
}


void CSpinlock::lock() {
    while(AppAtomicFetchCompareSet(1, 0, &mValue)) {
        //busy waiting
    }
}


bool CSpinlock::trylock() {
    return 0 == AppAtomicFetchCompareSet(1, 0, &mValue);
}


void CSpinlock::unlock() {
    AppAtomicFetchCompareSet(0, 1, &mValue);
}


} //namespace irr