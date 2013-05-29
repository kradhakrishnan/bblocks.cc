#ifndef _CORE_ASSERT_H_
#define _CORE_ASSERT_H_

#include <errno.h>
#include <iostream>
#include <string>
#include <string.h>
#include <errno.h>

#define __completion_handler__ /* async notification */
#define __interrupt__ /* synchronous callback */
#define __async_operation__ /* async operation function */

// pointer meta information

#define __own__ /* own the ptr henceforth */

#define STD_ERROR std::cerr
#define STD_INFO std::cout
#define ENDL std::endl

#define DEADEND {\
    STD_ERROR << "Unexpected code path reached. "\
              << __FILE__ << " : " << __LINE__\
              << ENDL;\
    abort();\
}

// depricate NOTREACHED
#define NOTREACHED {\
    STD_ERROR << "Unexpected code path reached. "\
              << __FILE__ << " : " << __LINE__\
              << ENDL;\
    abort();\
}

#define NOTIMPLEMENTED {\
    STD_ERROR << "Not implemented. "\
          << __FILE__ << " : " << __LINE__\
          << ENDL;\
    abort();\
}

#ifdef DEBUG_BUILD
#define ASSERT(x) {\
    if (!bool(x)) {\
        STD_ERROR << "ASSERT: " << #x << " . "\
                  << __FILE__ << " : " << __LINE__\
                  << " system-error: " << strerror(errno) \
                  << ENDL;\
        abort();\
    }\
}
#else
#define ASSERT(x)
#endif

#define INVARIANT(x) {\
    if (! bool(x)) {\
        STD_ERROR << "Invariant condition violated. The system is halting"\
                  << " to prevent corruption. INVARIANT: " << #x << " "\
                  << __FILE__ << ":" << __LINE__ \
                  << " syserror: " << strerror(errno) << ENDL;\
        abort();\
    }\
}

#endif
