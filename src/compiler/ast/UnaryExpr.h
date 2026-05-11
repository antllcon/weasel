#pragma once
#include "Expr.h"
#include <memory>

enum class UnaryOpKind
{
	LogicalNot,
	BitwiseNot,
	AddressOf,
	Deref
};

class UnaryExpr final : public Expr
{
public:
	UnaryExpr(UnaryOpKind op, std::unique_ptr<Expr> operand, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] UnaryOpKind GetOp() const;
	[[nodiscard]] const Expr& GetOperand() const;

private:
	UnaryOpKind m_op;
	std::unique_ptr<Expr> m_operand;
	SourceRange m_range;
};