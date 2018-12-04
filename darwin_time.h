#pragma once

#include <mach/mach_time.h>

#include "intrinsics.h"

inline double get_time() {
	mach_timebase_info_data_t freq;
	mach_timebase_info( &freq );

	u64 ticks = mach_absolute_time();

	return double( ticks ) * ( freq.numer / 1e9 ) / double( freq.denom );
}
