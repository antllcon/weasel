#include "ScopeManager.h"
#include <algorithm>

void ScopeManager::EnterScope()
{
	m_table.EnterScope();
}

void ScopeManager::LeaveScope()
{
	m_table.LeaveScope();
}

void ScopeManager::ResetSlots()
{
	m_nextSlot = 0;
	m_maxSlot = 0;
}

uint32_t ScopeManager::AllocateSlot()
{
	const uint32_t slot = m_nextSlot++;
	m_maxSlot = std::max(m_maxSlot, m_nextSlot);
	return slot;
}

uint32_t ScopeManager::GetCurrentSlot() const
{
	return m_nextSlot;
}

void ScopeManager::RestoreSlot(uint32_t slot)
{
	m_nextSlot = slot;
}

uint32_t ScopeManager::GetMaxSlots() const
{
	return m_maxSlot;
}

bool ScopeManager::Declare(const std::string& name, std::shared_ptr<TypeInfo> type, bool isMutable, uint32_t slot, bool isConst, const Expr* constExpr, bool isConstRef)
{
	return m_table.Declare(name, std::move(type), isMutable, slot, isConst, constExpr, isConstRef);
}

std::optional<SymbolInfo> ScopeManager::Resolve(const std::string& name) const
{
	return m_table.Resolve(name);
}