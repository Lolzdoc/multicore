//
//  main.cpp
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-20.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se

#include <algorithm>
#include <stdio.h>
#include <iostream>
#include "Compute.h"
#include "RadixSort.h"
#include <time.h>
#include "Timer.h"

int main() {
	Timer timer;
#ifdef CPU
	std::cout << "Platform: " << PLATFORM << " CPU" << std::endl;
	ComputeCpuContext context;
#else

#ifdef _WIN32
	ComputeGpuContextWin32 context;
	std::cout << "Platform: " << PLATFORM << " GPU" << std::endl;
#else
	ComputeGpuContext context;
	std::cout << "Platform: " << PLATFORM << " GPU" << std::endl;
#endif
#endif
	ComputeQueue queue(context, PLATFORM);

	int n = ELEMENTS;

	ComputeInputOutputBuffer keys(context, sizeof(int)*n);
	ComputeInputOutputBuffer values(context, sizeof(int)*n);

	int* pKeys;
	int* ref = new int[n];

	srand ((int)time(NULL));

	RadixSort sorter;
	sorter.init(&context, &keys, &values, n, n);

	float bestTime = std::numeric_limits<float>::max();
	float averageTime = 0;

	pKeys = (int*)queue.mapBuffer(keys, n*sizeof(int));
	queue.waitForCompletion();
	for (int i = 0; i < n; i++) {
		pKeys[i] = std::rand();
		ref[i] = pKeys[i];
	}

	std::cout << "Running one radix sort and one std::sort to verify sorted order." << std::endl;

	timer.start();
	sorter.sort(&queue, n);
	queue.waitForCompletion();
	float firstTime = (float) timer.getTimeMs();

	std::sort(ref, ref + n);

	for (int i = 0; i < n; i++) {
		assert(pKeys[i] == ref[i]);
	}

	std::cout << "Radix sort correctly sorted " << n << " elements." << std::endl;

	std::cout << "Sorting random data " << TEST_RUNS << " times for average execution time.";
	for (int runs = 0; runs < TEST_RUNS; runs++) {
		std::cout << "." << std::flush;
#if VERBOSE
		std::cout << "Sort run " << runs+1 << "." << std::endl;
		std::cout << "Generating random data to sort." << std::endl;
#endif

		for (int i = 0; i < n; i++) {
			pKeys[i] = std::rand();
			ref[i] = pKeys[i];
		}

#if VERBOSE
		std::cout << "Starting radix sort." << std::endl;
#endif
		timer.start();
		sorter.sort(&queue, n);
		queue.waitForCompletion();
#if VERBOSE
		std::cout << "Finished radix sorting." << std::endl;
#endif
		float t = (float) timer.getTimeMs();
		averageTime+=t;
		if (t < bestTime)
			bestTime = t;
	}

	float bestTimeStdSort = std::numeric_limits<float>::max();
	float averageTimeStdSort = 0.0f;

	for (int runs = 0; runs < TEST_RUNS; runs++) {
		std::cout << "." << std::flush;
#if VERBOSE
		std::cout << "Sort run " << runs + 1 << "." << std::endl;
		std::cout << "Generating random data to sort." << std::endl;
#endif
		for (int i = 0; i < n; i++) {
			pKeys[i] = std::rand();
			ref[i] = pKeys[i];
		}
#if VERBOSE 
		std::cout << "Starting std::sort()." << std::endl;
#endif
		timer.start();
		std::sort(ref, ref + n);
#if VERBOSE 
		std::cout << "Finished std::sort() sorting." << std::endl;
#endif
		float t = (float)timer.getTimeMs();
		averageTimeStdSort += t;
		if (t < bestTimeStdSort)
			bestTimeStdSort = t;
	}

	std::cout << std::endl;
	std::cout << "FIRST RUN TIME: " << firstTime << "ms." << std::endl;
	std::cout << "BEST TIME: " << bestTime << "ms." << std::endl;
	std::cout << "AVERAGE TIME: " << averageTime / (float)TEST_RUNS << "ms." << std::endl;
	std::cout << "STD::SORT BEST TIME " << bestTimeStdSort << "ms." << std::endl;
	std::cout << "STD::SORT AVERAGE TIME: " << averageTimeStdSort / (float)TEST_RUNS << "ms." << std::endl;
	std::cout << "speedup: " << averageTimeStdSort / averageTime << "x" << std::endl;

	return 0;
}
