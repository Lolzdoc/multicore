//
//  Timer.h
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-21.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se
//

#pragma once

#ifdef __WIN32
#include <windows.h>
#else
#include <chrono>
#endif

class Timer {
#ifdef __WIN32
public:
	inline void start() {
		QueryPerformanceCounter(&m_start);
	};

	inline double getTimeMs() const {
		LARGE_INTEGER now;
		LARGE_INTEGER freq;
		
		QueryPerformanceCounter(&now);
		QueryPerformanceFrequency(&freq);
	
		return ((double)1000)*(now.QuadPart - m_start.QuadPart) / static_cast<double>(freq.QuadPart);
   };

private:
	LARGE_INTEGER m_start;
#else
public:
	inline void start() {
		startTime = std::chrono::high_resolution_clock::now();
	};
	
	inline double getTimeMs() const {
		auto diff = std::chrono::high_resolution_clock::now() - startTime;
		return std::chrono::duration <double, std::milli> (diff).count();
	};
	
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
#endif
};
