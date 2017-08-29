#ifndef APP_IREFERENCECOUNTED_H
#define APP_IREFERENCECOUNTED_H

#include "irrTypes.h"

namespace irr {

	class IReferenceCounted {
	public:
		/// Constructor.
		IReferenceCounted() : ReferenceCounter(1) {
		}

		/// Destructor.
		virtual ~IReferenceCounted() {
		}

		void grab() const { 
			++ReferenceCounter;
		}

		bool drop() const {
			// someone is doing bad reference counting.
			_IRR_DEBUG_BREAK_IF(ReferenceCounter <= 0)

				--ReferenceCounter;
			if (!ReferenceCounter) {
				delete this;
				return true;
			}

			return false;
		}

		/**
		* @brief Get the reference count.
		* @return Current value of the reference counter. 
		*/
		s32 getReferenceCount() const {
			return ReferenceCounter;
		}

	private:
		/// The reference counter. Mutable to do reference counting on const objects.
		mutable s32 ReferenceCounter;
	};

} // end namespace irr

#endif //APP_IREFERENCECOUNTED_H
