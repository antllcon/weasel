#include "FunctionDeclStmt.h"

FunctionDeclStmt::FunctionDeclStmt(
	std::string returnTypeSign,
	std::string returnTypeName,
	std::string name,
	std::unique_ptr<BlockStmt> body,
	const SourceRange& range)
	: m_returnTypeSign(std::move(returnTypeSign))
	, m_returnTypeName(std::move(returnTypeName))
	, m_name(std::move(name))
	, m_body(std::move(body))
	, m_range(range)
{
}

void FunctionDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange FunctionDeclStmt::GetRange() const
{
	return m_range;
}

const std::string& FunctionDeclStmt::GetName() const
{
	return m_name;
}

const std::string& FunctionDeclStmt::GetReturnTypeSign() const
{
	return m_returnTypeSign;
}

const std::string& FunctionDeclStmt::GetReturnTypeName() const
{
	return m_returnTypeName;
}

const BlockStmt& FunctionDeclStmt::GetBody() const
{
	return *m_body;
}
