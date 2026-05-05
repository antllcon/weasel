#pragma once
#include "Expr.h"
#include "Stmt.h"
#include <memory>

class ReturnStmt final : public Stmt
{
public:
	explicit ReturnStmt(std::unique_ptr<Expr> value, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const Expr* GetValue() const;

private:
	std::unique_ptr<Expr> m_value;
	SourceRange m_range;
};
