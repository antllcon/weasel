#pragma once
#include "Stmt.h"
#include <string>

class ImportDecl final : public Stmt
{
public:
	ImportDecl(std::string modulePath, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetModulePath() const;

private:
	std::string m_modulePath;
	SourceRange m_range;
};
