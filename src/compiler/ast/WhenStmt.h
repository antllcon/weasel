#pragma once
#include "Expr.h"
#include "Stmt.h"
#include "WhenEntry.h"
#include <memory>
#include <vector>

class WhenStmt final : public Stmt
{
public:
	WhenStmt(
		std::unique_ptr<Expr> subject,
		std::vector<WhenEntry> entries,
		std::unique_ptr<Stmt> elseBody,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const Expr* GetSubject() const;
	[[nodiscard]] const std::vector<WhenEntry>& GetEntries() const;
	[[nodiscard]] const Stmt* GetElseBody() const;

private:
	std::unique_ptr<Expr> m_subject;
	std::vector<WhenEntry> m_entries;
	std::unique_ptr<Stmt> m_elseBody;
	SourceRange m_range;
};