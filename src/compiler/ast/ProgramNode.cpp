#include "ProgramNode.h"
#include "IAstVisitor.h"

ProgramNode::ProgramNode(std::vector<std::unique_ptr<AstNode>> declarations)
	: m_declarations(std::move(declarations))
{
}

void ProgramNode::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ProgramNode::GetRange() const
{
	return SourceRange{};
}

const std::vector<std::unique_ptr<AstNode>>& ProgramNode::GetDeclarations() const
{
	return m_declarations;
}