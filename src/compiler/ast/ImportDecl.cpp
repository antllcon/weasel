#include "ImportDecl.h"
#include "IAstVisitor.h"

ImportDecl::ImportDecl(std::string modulePath, const SourceRange& range)
	: m_modulePath(std::move(modulePath))
	, m_range(range)
{
}

void ImportDecl::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ImportDecl::GetRange() const
{
	return m_range;
}

const std::string& ImportDecl::GetModulePath() const
{
	return m_modulePath;
}
