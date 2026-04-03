#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>

class RunStmt final : public Stmt
{
public:
	RunStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const Expr& GetCondition() const;
	[[nodiscard]] const Stmt& GetBody() const;

private:
	std::unique_ptr<Expr> m_condition;
	std::unique_ptr<Stmt> m_body;
	SourceRange m_range;
};