#pragma once
#include "Stmt.h"

class ContinueStmt final : public Stmt
{
public:
	explicit ContinueStmt(const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

private:
	SourceRange m_range;
};
