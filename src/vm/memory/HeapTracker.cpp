#include "HeapTracker.h"

HeapTracker::HeapTracker()
	: m_firstObject(nullptr)
{
}

HeapTracker::~HeapTracker()
{
	Clear();
}

void HeapTracker::Track(HeapObject* object)
{
	object->SetNext(m_firstObject);
	object->SetPrev(nullptr);

	if (m_firstObject != nullptr)
	{
		m_firstObject->SetPrev(object);
	}

	m_firstObject = object;
}

void HeapTracker::Untrack(HeapObject* object)
{
	if (object->GetPrev() != nullptr)
	{
		object->GetPrev()->SetNext(object->GetNext());
	}
	else
	{
		m_firstObject = object->GetNext();
	}

	if (object->GetNext() != nullptr)
	{
		object->GetNext()->SetPrev(object->GetPrev());
	}
}

void HeapTracker::Clear()
{
	while (m_firstObject != nullptr)
	{
		HeapObject* next = m_firstObject->GetNext();
		delete m_firstObject;
		m_firstObject = next;
	}
}