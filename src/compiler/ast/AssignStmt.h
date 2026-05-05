#pragma once
#include "Expr.h"
#include "Stmt.h"
#include <memory>

class AssignStmt final : public Stmt
{
public:
	AssignStmt(
		std::unique_ptr<Expr> lhs,
		bool isMove,
		std::unique_ptr<Expr> rhs,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const Expr& GetLhs() const;
	[[nodiscard]] const Expr& GetRhs() const;
	[[nodiscard]] bool IsMove() const;

private:
	std::unique_ptr<Expr> m_lhs;
	bool m_isMove;
	std::unique_ptr<Expr> m_rhs;
	SourceRange m_range;
};
