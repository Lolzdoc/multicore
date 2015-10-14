//
//  RadixSort.cpp
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-21.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se
//

#include "RadixSort.h"
#include "Timer.h"

void RadixSort::init(ComputeContext* context, ComputeBuffer* keys, ComputeBuffer* values, size_t bufferSize, size_t keyCount) {
	mContext = context;
	mKeyBuffer = keys;
	mValueBuffer = values;
	int histogramSize = (int)(BUCKETS * (long)keyCount / WORK_PER_THREAD);
	mHistogramBuffer = new ComputeInputOutputBuffer(*mContext, sizeof(int) * histogramSize);
	mSumBuffer = new ComputeInputOutputBuffer(*mContext, sizeof(int) * histogramSize / (SCAN_WORK_ITEMS * 2));
	mSwapBuffer = new ComputeInputOutputBuffer(*mContext, sizeof(int) * (int)bufferSize);
	mValueSwapBuffer  = new ComputeInputOutputBuffer(*mContext, sizeof(int) * (int)bufferSize);
	
	mRadixSortProgram = new ComputeProgram(*mContext, ComputeProgram::sourceFromFile("radix_sort.cl"), "");
	mCountKernel = new ComputeKernel(*mRadixSortProgram, "radix_count");
	mScaneKernel = new ComputeKernel(*mRadixSortProgram, "radix_scan");
	mReorderKernel = new ComputeKernel(*mRadixSortProgram, "radix_reorder");
	mPasteKernel = new ComputeKernel(*mRadixSortProgram, "radix_add_offsets");
	mScanSumKernel = new ComputeKernel(*mRadixSortProgram, "radix_scan_offsets");

	mNumberOfKeys = (int)keyCount;
}

void RadixSort::sort(ComputeQueue* q, unsigned keyCount) {
	mNumberOfKeys = keyCount;
	int histogramSize = (int)(BUCKETS *((long)mNumberOfKeys / WORK_PER_THREAD));
	
	mScaneKernel->setLocalBufferArgument(1, sizeof(cl_uint) * SCAN_WORK_ITEMS); // local buffer
	
	mScanSumKernel->setArgument(0, *mSumBuffer);
	mScanSumKernel->setArgument(1, histogramSize / (SCAN_WORK_ITEMS * 2));
	
	
	mPasteKernel->setArgument(0, *mHistogramBuffer);
	mPasteKernel->setArgument(1, *mSumBuffer);
	mPasteKernel->setArgument(2, (SCAN_WORK_ITEMS * 2));
	
	mReorderKernel->setArgument(2, mNumberOfKeys);
	mReorderKernel->setArgument(3, *mHistogramBuffer);
	
	mCountKernel->setArgument(1, *mHistogramBuffer);
	Timer t;
#if VERBOSE
	std::cout << "Buckets: " << BUCKETS << std::endl;
	std::cout << "Work groups (count): " << mNumberOfKeys / WORK_PER_THREAD / WORK_ITEMS_COUNT << std::endl;
	std::cout << "Work groups (reorder): " << mNumberOfKeys / WORK_PER_THREAD / WORK_ITEMS_REORDER << std::endl;
	std::cout << "Work items (count): " << WORK_ITEMS_COUNT << std::endl;
	std::cout << "Work items (reorder): " << WORK_ITEMS_REORDER << std::endl;
	std::cout << "Histogram size: " << histogramSize << std::endl;
#endif
	size_t globalSize;
	size_t localSize;
	for (int pass = 0; pass < PASSES; pass++) {
#if VERBOSE
		t.start();
#endif
		
		mCountKernel->setArgument(0, *mKeyBuffer);
		mCountKernel->setArgument(2, pass);
		globalSize = mNumberOfKeys / WORK_PER_THREAD;
		localSize = WORK_ITEMS_COUNT;
		q->executeKernel(*mCountKernel, 1, &globalSize, &localSize);
		
#if VERBOSE
		q->waitForCompletion();
		std::cout << "Pass: " << pass << std::endl;
		std::cout << "Count: " << t.getTimeMs() << "ms\n";
		t.start();
#endif
		
		globalSize = histogramSize / 2;
		localSize = SCAN_WORK_ITEMS;
		mScaneKernel->setArgument(0, *mHistogramBuffer);
		mScaneKernel->setArgument(2, *mSumBuffer);
		q->executeKernel(*mScaneKernel, 1, &globalSize, &localSize);
#if VERBOSE
		q->waitForCompletion();
		std::cout << "Scan histogram: " << t.getTimeMs() << "ms\n";
		t.start();
#endif
		globalSize = 1;
		localSize = 1;
		q->executeKernel(*mScanSumKernel, 1, &globalSize, &localSize);
#if VERBOSE
		q->waitForCompletion();
		std::cout << "Scan offsets: " << t.getTimeMs() << "ms\n";
		t.start();
#endif
		
		globalSize = histogramSize;
		localSize = WORK_ITEMS_COUNT;
		q->executeKernel(*mPasteKernel, 1, &globalSize, &localSize);
#if VERBOSE
		q->waitForCompletion();
		std::cout << "Add offsets: " << t.getTimeMs() << "ms\n";
		t.start();
#endif
		globalSize = mNumberOfKeys / WORK_PER_THREAD;
		localSize = WORK_ITEMS_REORDER;
		mReorderKernel->setArgument(0, *mKeyBuffer);
		mReorderKernel->setArgument(1, *mSwapBuffer);
		mReorderKernel->setArgument(4, pass);
		q->executeKernel(*mReorderKernel, 1, &globalSize, &localSize);
#if VERBOSE
		q->waitForCompletion();
		std::cout << "Reorder: " << t.getTimeMs() << "ms\n";
#endif
		std::swap(mKeyBuffer, mSwapBuffer);
	}
}
