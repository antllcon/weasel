#pragma once
#include "Stmt.h"
#include <string>
#include <vector>

class EnumDeclStmt final : public Stmt
{
public:
	EnumDeclStmt(std::string name, std::vector<std::string> values, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::vector<std::string>& GetValues() const;

private:
	std::string m_name;
	std::vector<std::string> m_values;
	SourceRange m_range;
};