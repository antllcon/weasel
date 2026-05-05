#pragma once
#include "Expr.h"
#include "Stmt.h"
#include <memory>
#include <string>

enum class VarModifier
{
	Val,
	Var,
	Def
};

class VarDeclStmt final : public Stmt
{
public:
	VarDeclStmt(
		VarModifier modifier,
		std::string typeSign,
		std::string typeName,
		std::string name,
		bool isMoveInit,
		std::unique_ptr<Expr> init,
		const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	[[nodiscard]] SourceRange GetRange() const override;

	[[nodiscard]] VarModifier GetModifier() const;
	[[nodiscard]] const std::string& GetTypeSign() const;
	[[nodiscard]] const std::string& GetTypeName() const;
	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] bool IsMoveInit() const;
	[[nodiscard]] const Expr* GetInit() const;

private:
	VarModifier m_modifier;
	std::string m_typeSign;
	std::string m_typeName;
	std::string m_name;
	bool m_isMoveInit;
	std::unique_ptr<Expr> m_init;
	SourceRange m_range;
};
