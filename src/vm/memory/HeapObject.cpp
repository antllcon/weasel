#include "HeapObject.h"
#include "src/vm/exception/BackendException.h"

namespace
{
void AssertIsFieldIndexValid(bool isValid)
{
	if (!isValid)
	{
		throw BackendException(
			CompilerPhase::VirtualMachine,
			"ICE_HEAP_FIELD_OOB",
			"Индекс поля структуры выходит за пределы памяти (ошибка компилятора)");
	}
}
} // namespace

HeapObject::HeapObject(uint32_t fieldCount)
	: m_refCount(1)
	, m_next(nullptr)
	, m_prev(nullptr)
{
	m_fields.resize(fieldCount, Value(0ull));
}

void HeapObject::Retain()
{
	m_refCount++;
}

bool HeapObject::Release()
{
	m_refCount--;
	return m_refCount == 0;
}

Value HeapObject::GetField(uint32_t index) const
{
	AssertIsFieldIndexValid(index < m_fields.size());
	return m_fields[index];
}

void HeapObject::SetField(uint32_t index, const Value& value)
{
	AssertIsFieldIndexValid(index < m_fields.size());
	m_fields[index] = value;
}

HeapObject* HeapObject::GetNext() const
{
	return m_next;
}

void HeapObject::SetNext(HeapObject* next)
{
	m_next = next;
}

HeapObject* HeapObject::GetPrev() const
{
	return m_prev;
}

void HeapObject::SetPrev(HeapObject* prev)
{
	m_prev = prev;
}