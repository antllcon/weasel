#pragma once
#include "Expr.h"

class BoolExpr final : public Expr
{
public:
	BoolExpr(bool value, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] bool GetValue() const;

private:
	bool m_value;
	SourceRange m_range;
};