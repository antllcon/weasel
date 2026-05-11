#pragma once
#include "Expr.h"
#include <memory>
#include <string>

class MemberAccessExpr final : public Expr
{
public:
	MemberAccessExpr(std::unique_ptr<Expr> receiver, std::string field, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const Expr& GetReceiver() const;
	[[nodiscard]] const std::string& GetField() const;

private:
	std::unique_ptr<Expr> m_receiver;
	std::string m_field;
	SourceRange m_range;
};