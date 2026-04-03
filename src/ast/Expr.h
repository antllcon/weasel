#pragma once
#include "AstNode.h"
#include "TypeInfo.h"
#include <memory>

class Expr : public AstNode
{
public:
	void SetResolvedType(std::shared_ptr<TypeInfo> type);
	[[nodiscard]] std::shared_ptr<TypeInfo> GetResolvedType() const;

	void MarkPoisoned();
	[[nodiscard]] bool IsPoisoned() const;

private:
	std::shared_ptr<TypeInfo> m_resolvedType;
	bool m_isPoisoned = false;
};