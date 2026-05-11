#pragma once
#include "Expr.h"
#include <string>

class StringExpr final : public Expr
{
public:
	StringExpr(std::string value, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetValue() const;

private:
	std::string m_value;
	SourceRange m_range;
};