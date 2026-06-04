#pragma once
#include "Stmt.h"

class BreakStmt final : public Stmt
{
public:
	explicit BreakStmt(const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

private:
	SourceRange m_range;
};
