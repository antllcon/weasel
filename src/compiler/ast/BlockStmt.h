#pragma once
#include "Stmt.h"
#include <memory>
#include <vector>

class BlockStmt final : public Stmt
{
public:
	BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const std::vector<std::unique_ptr<Stmt>>& GetStmts() const;

private:
	std::vector<std::unique_ptr<Stmt>> m_stmts;
	SourceRange m_range;
};
