#pragma once
#include "Expr.h"
#include <memory>
#include <string>
#include <vector>

class FunctionCallExpr final : public Expr
{
public:
	FunctionCallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::vector<std::unique_ptr<Expr>>& GetArgs() const;

private:
	std::string m_name;
	std::vector<std::unique_ptr<Expr>> m_args;
	SourceRange m_range;
};
