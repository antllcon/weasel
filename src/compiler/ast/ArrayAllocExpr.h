#pragma once
#include "Expr.h"
#include <memory>
#include <string>

class ArrayAllocExpr final : public Expr
{
public:
	ArrayAllocExpr(std::string elementTypeName, std::unique_ptr<Expr> size, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	SourceRange GetRange() const override;

	const std::string& GetElementTypeName() const;
	const Expr& GetSize() const;

private:
	std::string m_elementTypeName;
	std::unique_ptr<Expr> m_size;
	SourceRange m_range;
};