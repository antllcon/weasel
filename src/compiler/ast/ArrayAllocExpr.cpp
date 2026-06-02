#include "ArrayAllocExpr.h"
#include "IAstVisitor.h"

ArrayAllocExpr::ArrayAllocExpr(std::string elementTypeName, std::unique_ptr<Expr> size, const SourceRange& range)
	: m_elementTypeName(std::move(elementTypeName))
	, m_size(std::move(size))
	, m_range(range)
{
}

void ArrayAllocExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ArrayAllocExpr::GetRange() const
{
	return m_range;
}

const std::string& ArrayAllocExpr::GetElementTypeName() const
{
	return m_elementTypeName;
}

const Expr& ArrayAllocExpr::GetSize() const
{
	return *m_size;
}