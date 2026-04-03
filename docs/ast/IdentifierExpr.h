#pragma once
#include "Expr.h"
#include <string>

class IdentifierExpr final : public Expr
{
public:
	IdentifierExpr(std::string name, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const std::string& GetName() const;

private:
	std::string m_name;
	SourceRange m_range;
};