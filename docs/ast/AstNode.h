#pragma once
#include "SourceRange.h"
#include "IAstVisitor.h"

class AstNode
{
public:
	virtual ~AstNode() = default;
	virtual void Accept(IAstVisitor& visitor) const = 0;
	[[nodiscard]] virtual SourceRange GetRange() const = 0;
};