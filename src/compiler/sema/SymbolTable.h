#pragma once
#include "src/compiler/ast/TypeInfo.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct SymbolInfo
{
	std::shared_ptr<TypeInfo> type;
	uint32_t stackSlot;
	bool isMutable;
};

class SymbolTable
{
public:
	void EnterScope();
	void LeaveScope();

	bool Declare(const std::string& name, std::shared_ptr<TypeInfo> type, bool isMutable, uint32_t slot);
	[[nodiscard]] std::optional<SymbolInfo> Resolve(const std::string& name) const;

private:
	std::vector<std::unordered_map<std::string, SymbolInfo>> m_scopes;
};
