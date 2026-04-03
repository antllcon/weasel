#pragma once
#include "src/vm/memory/HeapObject.h"

class HeapTracker
{
public:
	HeapTracker();
	~HeapTracker();

	HeapTracker(const HeapTracker&) = delete;
	HeapTracker& operator=(const HeapTracker&) = delete;

	void Track(HeapObject* object);
	void Untrack(HeapObject* object);
	void Clear();

private:
	HeapObject* m_firstObject;
};