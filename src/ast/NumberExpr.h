#pragma once
#include "Expr.h"
#include <string>

class NumberExpr final : public Expr
{
public:
	NumberExpr(std::string value, bool isFloat, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetValue() const;
	[[nodiscard]] bool IsFloat() const;

private:
	std::string m_value;
	bool m_isFloat;
	SourceRange m_range;
};