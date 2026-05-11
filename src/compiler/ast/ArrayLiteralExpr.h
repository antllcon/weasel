#pragma once
#include "Expr.h"
#include <memory>
#include <vector>

class ArrayLiteralExpr final : public Expr
{
public:
	ArrayLiteralExpr(std::vector<std::unique_ptr<Expr>> elements, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::vector<std::unique_ptr<Expr>>& GetElements() const;

private:
	std::vector<std::unique_ptr<Expr>> m_elements;
	SourceRange m_range;
};