#pragma once
#include "FieldDecl.h"
#include "Stmt.h"
#include <string>
#include <vector>

class StructDeclStmt final : public Stmt
{
public:
	StructDeclStmt(std::string name, std::vector<FieldDecl> fields, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::vector<FieldDecl>& GetFields() const;

private:
	std::string m_name;
	std::vector<FieldDecl> m_fields;
	SourceRange m_range;
};