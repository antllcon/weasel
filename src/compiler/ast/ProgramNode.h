#pragma once
#include "AstNode.h"
#include <memory>
#include <vector>

class ProgramNode final : public AstNode
{
public:
	explicit ProgramNode(std::vector<std::unique_ptr<AstNode>> declarations);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::vector<std::unique_ptr<AstNode>>& GetDeclarations() const;

private:
	std::vector<std::unique_ptr<AstNode>> m_declarations;
};