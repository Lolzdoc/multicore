//
//  radix_sort.cl
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-21.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se
//

#include "Config.h"


kernel
void radix_reorder(global int* keys, global int* output, const int key_count, global int* globalHistogram, const int pass) {
	
	int globalID = get_global_id(0);
	int localID = get_local_id(0);
	int groupID = get_group_id(0);
	const int globalSize = get_global_size(0);
	local int localHistogram[BUCKETS * WORK_ITEMS_REORDER];
	
#if 0
	for (uint bucket = 0; bucket < BUCKETS; ++bucket ) {
		localHistogram[bucket * WORK_ITEMS_REORDER + localID] = globalHistogram[bucket * globalSize + WORK_ITEMS_REORDER * groupID + localID];
	}
#else
	uint local_idx  = localID;
	uint global_idx = WORK_ITEMS_REORDER * groupID + localID; 
	for (uint bucket = 0; bucket < BUCKETS; ++bucket ) {
		localHistogram[local_idx] = globalHistogram[global_idx];
		local_idx  += WORK_ITEMS_REORDER;
		global_idx += globalSize;
	}
#endif

	int first = globalID * WORK_PER_THREAD;
	
#ifdef GPU
	for (int i = first; i < first + WORK_PER_THREAD; i+=16) {
		int16 vkey0 = vload16(i>>4, keys);
		for (int j = 0; j < 16; j++) {
			output[localHistogram[(((vkey0[j] >> (pass * BITS_PER_PASS)) & (int)(BUCKETS - 1))) * WORK_ITEMS_REORDER + localID]++] = vkey0[j];
		}
	}
#endif
	
#ifdef CPU
	for (int i = first; i < first + WORK_PER_THREAD; i+=1) {
		output[localHistogram[((keys[i] >> (pass * BITS_PER_PASS)) & (int)(BUCKETS - 1)) * WORK_ITEMS_REORDER + localID]++] = keys[i];
	}
#endif

	
}
	
	kernel
	void radix_add_offsets(global int* globalHistogram, global int* offsets) {
		int globalID = get_global_id(0);
		int index = globalID / (SCAN_WORK_ITEMS * 2);
		globalHistogram[globalID] += offsets[index];
	}
	
	kernel
	void radix_scan(global int* globalHistogram, local int* temp, global int* offsets) {
		int globalID = get_global_id(0);
		int localID = get_local_id(0);
		int groupID = get_group_id(0);
		
		int mult = 1;
		
		int size = get_local_size(0) * 2;
		
		local int2* temp2 = (local int2*)temp;
		temp2[localID] = vload2(globalID, globalHistogram);
		
		for (int d = size >> 1; d > 0; d >>= 1) {
			barrier(CLK_LOCAL_MEM_FENCE);
			if (localID < d) {
#if 1
				int ai = mult * (2 * localID + 1) - 1;
				int bi = mult * (2 * localID + 2) - 1;
#else
				int ai = mult * ((localID << 1) + 1) -1;
				int bi = mult * ((localID << 1) + 2) -1;
#endif	
				temp[bi] += temp[ai];
			}
			mult <<= 1;
		}
		
		if (localID == 0) {
			offsets[groupID] = temp[size - 1];
			temp[size - 1] = 0;
		}
		
		for (int d = 1; d < size; d *= 2) {
			mult >>= 1;
			barrier(CLK_LOCAL_MEM_FENCE);
			if (localID < d) {
#if 1
				int ai = mult * (2 * localID + 1) - 1;
				int bi = mult * (2 * localID + 2) - 1;
#else
				int ai = mult * ((localID << 1) + 1) -1;
				int bi = mult * ((localID << 1) + 2) -1;
#endif	
				
				int t = temp[ai];
				temp[ai] = temp[bi];
				temp[bi] += t;
			}
		}
		globalHistogram[2 * globalID] = temp[2 * localID];
		globalHistogram[2 * globalID + 1] = temp[2 * localID + 1];
	}
	
	kernel
	void radix_scan_offsets(global int* offsets, int to) {
		int prev = offsets[0];
		offsets[0] = 0;
		for (int i = 1; i < to; i++) {
			int curr = offsets[i];
			offsets[i] = prev;
			prev += curr;
		}
	}
	
	kernel
	void radix_count(global int* keys, global int* globalHistogram, const int pass) {
		const int globalID = get_global_id(0);
		const int localID = get_local_id(0);
		const int globalSize = get_global_size(0);
		
		local int localHistogram[BUCKETS * WORK_ITEMS_COUNT];
		
#if 0
		for (int i = 0; i < BUCKETS; ++i ) {
			localHistogram[i * WORK_ITEMS_COUNT + localID] = 0;
		}
#else
	uint local_idx  = localID;
	for (uint bucket = 0; bucket < BUCKETS; ++bucket ) {
		localHistogram[local_idx] = 0;
		local_idx  += WORK_ITEMS_COUNT;
	}
#endif
		
		int first = globalID * WORK_PER_THREAD;
#ifdef GPU
		for (int index = first; index < first + WORK_PER_THREAD; index+=16) {
			int16 vkey0 = vload16((index+0)>>4, keys);
			for (int j = 0; j < 16; j++) {
				localHistogram[((vkey0[j] >> (pass * BITS_PER_PASS)) & (int)(BUCKETS - 1))*WORK_ITEMS_COUNT+localID]++;
			}
		}
		
		
#endif
	
#ifdef CPU
		for (int index = first; index < first + WORK_PER_THREAD; index+=1) {
			localHistogram[((keys[index] >> (pass * BITS_PER_PASS)) & (int)(BUCKETS - 1)) * WORK_ITEMS_COUNT + localID]++;
		}
#endif
		
	for (int bucket = 0; bucket < BUCKETS; ++bucket) {
		globalHistogram[bucket * globalSize + globalID] = localHistogram[bucket * WORK_ITEMS_COUNT + localID];
	}
}
