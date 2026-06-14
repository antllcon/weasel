#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>
#include <string>

class RepCollectionStmt final : public Stmt
{
public:
	RepCollectionStmt(std::string valueIterator,
					  std::string indexIterator,
					  std::unique_ptr<Expr> collectionExpr,
					  std::unique_ptr<Stmt> body,
					  const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] const std::string& GetValueIterator() const;
	[[nodiscard]] const std::string& GetIndexIterator() const;
	[[nodiscard]] bool HasIndexIterator() const;
	[[nodiscard]] const Expr& GetCollectionExpr() const;
	[[nodiscard]] const Stmt& GetBody() const;

private:
	std::string m_valueIterator;
	std::string m_indexIterator;
	std::unique_ptr<Expr> m_collectionExpr;
	std::unique_ptr<Stmt> m_body;
	SourceRange m_range;
};
