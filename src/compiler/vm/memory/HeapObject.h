#pragma once
#include "src/compiler/vm/value/Value.h"
#include <cstdint>
#include <vector>

class HeapObject
{
public:
	explicit HeapObject(uint32_t fieldCount);

	void Retain();
	bool Release();

	Value GetField(uint32_t index) const;
	void SetField(uint32_t index, const Value& value);

	HeapObject* GetNext() const;
	void SetNext(HeapObject* next);

	HeapObject* GetPrev() const;
	void SetPrev(HeapObject* prev);

private:
	uint32_t m_refCount;
	std::vector<Value> m_fields;
	HeapObject* m_next;
	HeapObject* m_prev;
};