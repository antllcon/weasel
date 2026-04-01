#pragma once
#include "src/vm/value/Value.h"
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

private:
	uint32_t m_refCount;
	std::vector<Value> m_fields;
};