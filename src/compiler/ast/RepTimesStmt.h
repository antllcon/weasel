#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>

class RepTimesStmt final : public Stmt
{
public:
	RepTimesStmt(std::unique_ptr<Expr> countExpr,
				 std::unique_ptr<Stmt> body,
				 const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const Expr& GetCountExpr() const;
	[[nodiscard]] const Stmt& GetBody() const;

private:
	std::unique_ptr<Expr> m_countExpr;
	std::unique_ptr<Stmt> m_body;
	SourceRange m_range;
};
