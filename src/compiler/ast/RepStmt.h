#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>
#include <string>

class RepStmt final : public Stmt
{
public:
	RepStmt(std::string iterator,
			std::unique_ptr<Expr> startExpr,
			std::unique_ptr<Expr> endExpr,
			std::unique_ptr<Expr> stepExpr,
			bool isDown,
			std::unique_ptr<Stmt> body,
			const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetIterator() const;
	[[nodiscard]] const Expr& GetStartExpr() const;
	[[nodiscard]] const Expr& GetEndExpr() const;
	[[nodiscard]] const Expr* GetStepExpr() const;
	[[nodiscard]] bool IsDown() const;
	[[nodiscard]] const Stmt& GetBody() const;

private:
	std::string m_iterator;
	std::unique_ptr<Expr> m_startExpr;
	std::unique_ptr<Expr> m_endExpr;
	std::unique_ptr<Expr> m_stepExpr;
	bool m_isDown;
	std::unique_ptr<Stmt> m_body;
	SourceRange m_range;
};
