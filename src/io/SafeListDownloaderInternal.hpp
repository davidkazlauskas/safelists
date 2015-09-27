#ifndef SAFELISTDOWNLOADERINTERNAL_DLOJJVMD
#define SAFELISTDOWNLOADERINTERNAL_DLOJJVMD

#include <util/DumbHash.hpp>
#include <io/Interval.hpp>

namespace SafeLists {

    SafeLists::IntervalList readIListAndHash(
        const char* path,SafeLists::DumbHash256& hash);

}

#endif /* end of include guard: SAFELISTDOWNLOADERINTERNAL_DLOJJVMD */
