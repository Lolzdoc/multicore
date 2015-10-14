//
//  Config.h
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-21.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se

#ifndef pgRadixSort_Config_h
#define pgRadixSort_Config_h

#define VERBOSE		1

#define INTEL

#define CPU
#undef GPU

#if !(defined(GPU) ^ defined(CPU))
#error "Pick one."
#endif

#define ELEMENTS (1 << 24) // Number of values to sort


#ifdef INTEL
#define PLATFORM ("Intel")

#ifdef GPU
// GPU Settings
#define WORK_ITEMS_COUNT	(128)
#define WORK_ITEMS_REORDER	(4)
#define BITS_TO_SORT		(32)
#define BITS_PER_PASS		(4)
#define WORK_PER_THREAD		(128)
#define SCAN_WORK_ITEMS		(256)

#else
// CPU Settings

#if 0

/* Using SIMD-vectorization actually slows down the program when compiled
 * by Apple's compiler!
 *
 * If you enable this, you need to enable simd-vectorization in Compute.h
 * by not using the following in that file:
 *	 "-cl-mad-enable -cl-auto-vectorize-disable";
 */
	
#define WORK_ITEMS_COUNT	(4)
#else
#define WORK_ITEMS_COUNT	(1)
#endif

#define WORK_ITEMS_REORDER	(1)
#define BITS_TO_SORT		(32)
#define BITS_PER_PASS		(16)
#define SCAN_WORK_ITEMS		(1)
#define WORK_DIVISION		(8)
#define WORK_PER_THREAD		(ELEMENTS / (WORK_DIVISION))
#endif
#endif

#define BUCKETS			(1 << BITS_PER_PASS)
#define PASSES			(BITS_TO_SORT / BITS_PER_PASS)

#define TEST_RUNS 5 // Runs to sort for average execution time

#endif
