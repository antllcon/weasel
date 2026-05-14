#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>

class DoWhileStmt final : public Stmt
{
public:
	DoWhileStmt(std::unique_ptr<Stmt> body, std::unique_ptr<Expr> condition, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const Stmt& GetBody() const;
	[[nodiscard]] const Expr& GetCondition() const;

private:
	std::unique_ptr<Stmt> m_body;
	std::unique_ptr<Expr> m_condition;
	SourceRange m_range;
};
