/* Header for profiling */

#ifndef VAT7_PROFILER_H
#define VAT7_PROFILER_H

#include <stdbool.h>
#include <stdint.h> // for SIZE_MAX


size_t _vt7_profile_create (char *label);
void _vt7_profile__begin (size_t profile_id);
void _vt7_profile_end (void);
void _vt7_profiler_report (void);


#if PROFILING_ENABLED

#define VT7_PROFILE_BEGIN(label) \
	_Thread_local static size_t label##_profile_id = SIZE_MAX; \
	if (label##_profile_id == SIZE_MAX) \
	{ \
		label##_profile_id = _vt7_profile_create(#label); \
	} \
	do { _vt7_profile_begin(label##_profile_id); } while(0)

#define VT7_PROFILE_END() do { _vt7_profile_end(); } while(0)

#define VT7_PROFILE_REPORT() do { _vt7_profiler_report(); } while(0)


#else

#define PROFILE_BEGIN(label) do { } while(0)
#define PROFILE_END() do { } while(0)
#define PROFILER_REPORT() do { } while(0)


#endif // profiling enabled

#endif // header guard
