#ifndef BASE_PROFILE_H
#define BASE_PROFILE_H

#if PROFILE_BUILD
#include <Tracy/tracy/TracyC.h>

#define prof_function_begin()             TracyCZoneNC(prof_function, __func__, 0x37B24D, true)
#define prof_function_end()               TracyCZoneEnd(prof_function)
#define prof_zone_begin(identifier, name) TracyCZoneNC(identifier, name, 0x7048E8, true)
#define prof_zone_end(identifier)         TracyCZoneEnd(identifier)
#define prof_frame_done()                 TracyCFrameMark
#else
#define prof_function_begin()
#define prof_function_end()
#define prof_zone_begin(identifier, name)
#define prof_zone_end(identifier)
#define prof_frame_done()
#endif

#endif // BASE_PROFILE_H
