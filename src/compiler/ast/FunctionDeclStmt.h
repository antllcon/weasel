#pragma once
#include "BlockStmt.h"
#include "Stmt.h"
#include <memory>
#include <string>

class FunctionDeclStmt final : public Stmt
{
public:
	FunctionDeclStmt(
		std::string returnTypeSign,
		std::string returnTypeName,
		std::string name,
		std::unique_ptr<BlockStmt> body,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;
	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::string& GetReturnTypeSign() const;
	[[nodiscard]] const std::string& GetReturnTypeName() const;
	[[nodiscard]] const BlockStmt& GetBody() const;

private:
	std::string m_returnTypeSign;
	std::string m_returnTypeName;
	std::string m_name;
	std::unique_ptr<BlockStmt> m_body;
	SourceRange m_range;
};
