#pragma once
#include "Stmt.h"
#include "Expr.h"
#include <memory>
#include <vector>
#include <string>

class RepStmt final : public Stmt
{
public:
	RepStmt(std::vector<std::string> iterators,
			std::vector<std::unique_ptr<Expr>> ranges,
			std::unique_ptr<Stmt> originalBody,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	// Семантический анализатор (Sema) сгенерирует вложенные циклы RunStmt
	// и запишет их сюда. Бэкенд будет читать только этот метод.
	void SetLoweredBody(std::unique_ptr<Stmt> lowered);
	[[nodiscard]] Stmt* GetLoweredBody() const;

	[[nodiscard]] const std::vector<std::string>& GetIterators() const;
	[[nodiscard]] const std::vector<std::unique_ptr<Expr>>& GetRanges() const;
	[[nodiscard]] const Stmt& GetOriginalBody() const;

private:
	// Исходный сахар (для LSP, подсветки и ошибок)
	std::vector<std::string> m_iterators;
	std::vector<std::unique_ptr<Expr>> m_ranges;
	std::unique_ptr<Stmt> m_originalBody;
	SourceRange m_range;

	// Скрытое раскрытое дерево (для кодогенерации)
	std::unique_ptr<Stmt> m_loweredBody;
};