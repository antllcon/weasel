#pragma once
#include "src/ast/Expr.h"
#include <memory>

enum class CastKind
{
	IntegralPromotion,
	FloatToDouble,
	LValueToRValue
};

class ImplicitCastExpr final : public Expr
{
public:
	ImplicitCastExpr(std::unique_ptr<Expr> source, CastKind kind);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const Expr& GetSource() const;
	[[nodiscard]] CastKind GetCastKind() const;

private:
	std::unique_ptr<Expr> m_source;
	CastKind m_kind;
};