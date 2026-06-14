#pragma once
#include "ClassField.h"
#include "ConstructorDecl.h"
#include "FunctionDeclStmt.h"
#include "Stmt.h"
#include <memory>
#include <string>
#include <vector>

struct ClassMethod
{
	std::unique_ptr<FunctionDeclStmt> decl;
	bool isPublic = false;
};

class ClassDeclStmt final : public Stmt
{
public:
	ClassDeclStmt(std::string name,
				  std::vector<ClassField> fields,
				  std::vector<ConstructorDecl> constructors,
				  std::vector<ClassMethod> methods,
				  const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::vector<ClassField>& GetFields() const;
	[[nodiscard]] const std::vector<ConstructorDecl>& GetConstructors() const;
	[[nodiscard]] const std::vector<ClassMethod>& GetMethods() const;

private:
	std::string m_name;
	std::vector<ClassField> m_fields;
	std::vector<ConstructorDecl> m_constructors;
	std::vector<ClassMethod> m_methods;
	SourceRange m_range;
};
