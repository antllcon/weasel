#include "VarDeclStmt.h"

VarDeclStmt::VarDeclStmt(
	VarModifier modifier,
	std::string typeSign,
	std::string typeName,
	std::string name,
	bool isMoveInit,
	std::unique_ptr<Expr> init,
	const SourceRange& range)
	: m_modifier(modifier)
	, m_typeSign(std::move(typeSign))
	, m_typeName(std::move(typeName))
	, m_name(std::move(name))
	, m_isMoveInit(isMoveInit)
	, m_init(std::move(init))
	, m_range(range)
{
}

void VarDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange VarDeclStmt::GetRange() const
{
	return m_range;
}

VarModifier VarDeclStmt::GetModifier() const
{
	return m_modifier;
}

const std::string& VarDeclStmt::GetTypeSign() const
{
	return m_typeSign;
}

const std::string& VarDeclStmt::GetTypeName() const
{
	return m_typeName;
}

const std::string& VarDeclStmt::GetName() const
{
	return m_name;
}

bool VarDeclStmt::IsMoveInit() const
{
	return m_isMoveInit;
}

const Expr* VarDeclStmt::GetInit() const
{
	return m_init.get();
}
