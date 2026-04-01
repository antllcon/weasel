#include "HeapObject.h"
#include "src/vm/exception/BackendException.h"

namespace
{
void AssertIsFieldIndexValid(bool isValid)
{
	if (!isValid)
	{
		throw BackendException(CompilerPhase::VirtualMachine, "ICE_HEAP_FIELD_OOB", "Индекс поля структуры выходит за пределы памяти (ошибка компилятора)");
	}
}
} // namespace

HeapObject::HeapObject(uint32_t fieldCount)
	: m_refCount(1)
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