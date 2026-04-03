#pragma once
#include "Expr.h"

enum class BinaryOpKind
{
	Add, Sub, Mul, Div, Mod,
	Eq, NotEq, Less, Greater, LessEq, GreaterEq,
	LogicalAnd, LogicalOr
};

class BinaryExpr final : public Expr
{
public:
	BinaryExpr(BinaryOpKind op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] BinaryOpKind GetOp() const;
	[[nodiscard]] const Expr& GetLeft() const;
	[[nodiscard]] const Expr& GetRight() const;

private:
	BinaryOpKind m_op;
	std::unique_ptr<Expr> m_left;
	std::unique_ptr<Expr> m_right;
};