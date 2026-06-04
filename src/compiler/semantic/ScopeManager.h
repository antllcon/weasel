#pragma once
#include "SymbolTable.h"
#include "src/compiler/ast/Expr.h"
#include "src/compiler/ast/TypeInfo.h"
#include <memory>
#include <optional>
#include <string>

class ScopeManager
{
public:
	void EnterScope();
	void LeaveScope();

	void ResetSlots();
	[[nodiscard]] uint32_t AllocateSlot();
	[[nodiscard]] uint32_t GetCurrentSlot() const;
	void RestoreSlot(uint32_t slot);
	[[nodiscard]] uint32_t GetMaxSlots() const;

	bool Declare(const std::string& name, std::shared_ptr<TypeInfo> type, bool isMutable, uint32_t slot, bool isConst = false, const Expr* constExpr = nullptr);
	[[nodiscard]] std::optional<SymbolInfo> Resolve(const std::string& name) const;

private:
	SymbolTable m_table;
	uint32_t m_nextSlot = 0;
	uint32_t m_maxSlot = 0;
};