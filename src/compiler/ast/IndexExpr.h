#pragma once
#include "Expr.h"
#include <memory>

class IndexExpr final : public Expr
{
public:
	IndexExpr(std::unique_ptr<Expr> receiver, std::unique_ptr<Expr> index, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const Expr& GetReceiver() const;
	[[nodiscard]] const Expr& GetIndex() const;

private:
	std::unique_ptr<Expr> m_receiver;
	std::unique_ptr<Expr> m_index;
	SourceRange m_range;
};