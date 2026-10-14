/* Profiling code lives here */

#include "engine_libc/eng_stdbool.h"
#include "engine_libc/eng_stdint.h"
#include "engine_libc/eng_stdio.h"

#include "engine_tools/eng_logging.h"
#include "engine_tools/eng_assert.h"
#include "engine_tools/eng_profiling.h"
#include "engine_tools/eng_timing.h"

// TODO documentation for profiling
// TODO Improve reporting and add instance subregion data


struct ProfileRegionInstance
{
	uint64_t begin_time;
	size_t parent_region_id;
};


struct Profile
{
	char *label;
	uint64_t run_count;
	uint64_t max_time;
	uint64_t max_on_count;
	uint64_t min_time;
	uint64_t min_on_count;
	uint64_t total_time;  // in seconds
	struct ProfileRegionInstance current_instance;
};


#define MAX_PROFILES 256
static struct Profile profiles[MAX_PROFILES];
static _Atomic size_t next_profile_index = 0;

_Thread_local static size_t active_profile = SIZE_MAX;


size_t _vt7_profiler_create (char *label)
{
	size_t profile_index;
	__atomic_load(&next_profile_index, &profile_index, __ATOMIC_ACQUIRE);
	if (profile_index < MAX_PROFILES - 1)
	{
		struct Profile new_profile =
		{
			label,
			0u,  // run_count
			0u,  // max_time
			0u,  // max_on_count
			UINT64_MAX, // min_time
			0u,  // min_on_count
			0u,  // total_time
			{
				0u,  // begin time
				SIZE_MAX  // parent region id
			}
		};
		size_t profile_id = next_profile_index;
		next_profile_index++;
		profiles[profile_id] = new_profile;
		INFO_LOG("%s profile created with id %zu", label, profile_id);
		return profile_id;
	}
	else
	{
		WARNING_LOG("Too many profiles to create %s profile.", label);
		return SIZE_MAX;
	}
}


void _vt7_profiler_begin (size_t profile_id)
{
	ENG_ASSERT(profile_id < next_profile_index);
	ENG_ASSERT(next_profile_index < MAX_PROFILES -1);
	profiles[profile_id].current_instance.parent_region_id =
		active_profile;
	active_profile = profile_id;
	ENG_ASSERT(active_profile != SIZE_MAX);
	profiles[profile_id].current_instance.begin_time =
		ENG_timing_precision_count();
}


void _vt7_profiler_end (void)
{
	ENG_ASSERT(active_profile != SIZE_MAX);
	uint64_t now = ENG_timing_precision_count();
	uint64_t begin =
		profiles[active_profile].current_instance.begin_time;
	uint64_t elapsed = now - begin;
	profiles[active_profile].run_count++;
	profiles[active_profile].total_time += elapsed;
	if (elapsed < profiles[active_profile].min_time)
	{
		profiles[active_profile].min_time = elapsed;
		profiles[active_profile].min_on_count =
			profiles[active_profile].run_count;
	}
	if (elapsed > profiles[active_profile].max_time)
	{
		profiles[active_profile].max_time = elapsed;
		profiles[active_profile].max_on_count =
			profiles[active_profile].run_count;
	}
	size_t ap_id = active_profile;
	active_profile =
		profiles[ap_id].current_instance.parent_region_id;
}


void _vt7_profile_report (void)
{
	static bool is_new_report = true;
	uint64_t freq = ENG_timing_precision_frequency();
	char *open_mode = is_new_report ? "w" : "a";
	FILE *f = fopen("log/profiling.log", open_mode);
		if (f == NULL)
		{
			WARNING_LOG("Failed to open file for profiler report");
			return;
		}
		fprintf(f, "PROFILING REPORT\n\n");
		for (size_t i = 0; i < next_profile_index; i++)
		{
			fprintf(f, "%s\n", profiles[i].label);
			fprintf(f, " run count: %lu\n", profiles[i].run_count);
			double total_t = (double)profiles[i].total_time
				/ (double)freq;
			fprintf(f, " total time: %f seconds\n", total_t);
			double avg_t = (1000.0 * total_t)
				/ (double)profiles[i].run_count;
			fprintf(f, " average time: %f milliseconds\n", avg_t);
			double max_t = (1000.0 * (double)profiles[i].max_time)
				/ (double)freq;
			fprintf(f, " max time: %f milliseconds\n", max_t);
			fprintf(f, " max time on count %lu\n",
				profiles[i].max_on_count);
			double min_t = (1000.0 * (double)profiles[i].min_time)
				/ (double)freq;
			fprintf(f, " min time: %f milliseconds\n", min_t);
			fprintf(f, " min time on count %lu\n",
				profiles[i].min_on_count);
			fprintf(f, "\n");
		}
		fprintf(f, "\n");
	fclose(f);
	is_new_report = false;
}
