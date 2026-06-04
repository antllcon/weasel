#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>
#include <string>

class ClassicForStmt final : public Stmt
{
public:
	ClassicForStmt(
		std::string initType,
		std::string initName,
		std::unique_ptr<Expr> initExpr,
		std::unique_ptr<Expr> condition,
		std::unique_ptr<Stmt> step,
		std::unique_ptr<Stmt> body,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetInitType() const;
	[[nodiscard]] const std::string& GetInitName() const;
	[[nodiscard]] const Expr& GetInitExpr() const;
	[[nodiscard]] const Expr& GetCondition() const;
	[[nodiscard]] const Stmt& GetStep() const;
	[[nodiscard]] const Stmt& GetBody() const;

private:
	std::string m_initType;
	std::string m_initName;
	std::unique_ptr<Expr> m_initExpr;
	std::unique_ptr<Expr> m_condition;
	std::unique_ptr<Stmt> m_step;
	std::unique_ptr<Stmt> m_body;
	SourceRange m_range;
};
