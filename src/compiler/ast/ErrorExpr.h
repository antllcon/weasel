#pragma once
#include "Expr.h"

class ErrorExpr final : public Expr
{
public:
	explicit ErrorExpr(const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

private:
	SourceRange m_range;
};