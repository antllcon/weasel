#include "SymbolTable.h"
#include <stdexcept>

namespace
{
void AssertIsScopeNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Попытка покинуть несуществующую область видимости");
	}
}
} // namespace

void SymbolTable::EnterScope()
{
	m_scopes.emplace_back();
}

void SymbolTable::LeaveScope()
{
	AssertIsScopeNotEmpty(m_scopes.empty());
	m_scopes.pop_back();
}

bool SymbolTable::Declare(const std::string& name, std::shared_ptr<TypeInfo> type, bool isMutable, uint32_t slot)
{
	if (m_scopes.empty())
	{
		return false;
	}
	auto& current = m_scopes.back();
	if (current.contains(name))
	{
		return false;
	}
	current[name] = SymbolInfo{std::move(type), slot, isMutable};
	return true;
}

std::optional<SymbolInfo> SymbolTable::Resolve(const std::string& name) const
{
	for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
	{
		const auto found = it->find(name);
		if (found != it->end())
		{
			return found->second;
		}
	}
	return std::nullopt;
}
