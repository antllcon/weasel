#pragma once
#include "BlockStmt.h"
#include "Expr.h"
#include "Stmt.h"
#include <memory>

class IfStmt final : public Stmt
{
public:
	IfStmt(
		std::unique_ptr<Expr> condition,
		std::unique_ptr<BlockStmt> thenBlock,
		std::unique_ptr<Stmt> elseNode,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const Expr& GetCondition() const;
	[[nodiscard]] const BlockStmt& GetThenBlock() const;
	[[nodiscard]] const Stmt* GetElseNode() const;

private:
	std::unique_ptr<Expr> m_condition;
	std::unique_ptr<BlockStmt> m_thenBlock;
	std::unique_ptr<Stmt> m_elseNode;
	SourceRange m_range;
};
