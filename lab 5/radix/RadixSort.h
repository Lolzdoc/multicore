//
//  RadixSort.h
//  pgRadixSort
//
//  Created by Per Ganestam on 2014-11-21.
//  Copyright (c) 2014 Per Ganestam. All rights reserved.
//	per.ganestam@cs.lth.se
//

#pragma once

#include "Config.h"


#include "Compute.h"
#include <iostream>

class RadixSort {
public:
	void init(ComputeContext* context, ComputeBuffer* keys, ComputeBuffer* values, size_t bufferSize, size_t keyCount);
	void sort(ComputeQueue* q, unsigned keyCount);

public:
	ComputeContext* mContext;
	ComputeBuffer* mKeyBuffer;
	ComputeBuffer* mValueBuffer;
	ComputeBuffer* mValueSwapBuffer;
	ComputeBuffer* mSwapBuffer;
	ComputeBuffer* mHistogramBuffer;
	ComputeBuffer* mSumBuffer;
	ComputeProgram* mRadixSortProgram;
	ComputeKernel* mCountKernel;
	ComputeKernel* mScaneKernel;
	ComputeKernel* mReorderKernel;
	ComputeKernel* mPasteKernel;
	ComputeKernel* mScanSumKernel;

	unsigned mNumberOfKeys;
};