#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>

class NasmCodeGenerator final : public IAstVisitor
{
public:
	NasmCodeGenerator();

	[[nodiscard]] std::string Generate(const AstNode& root);

	void Visit(const ProgramNode& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const EnumDeclStmt& node) override;
	void Visit(const ErrorExpr& node) override;
	void Visit(const ImplicitCastExpr& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const UnaryExpr& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const NumberExpr& node) override;
	void Visit(const StringExpr& node) override;
	void Visit(const BoolExpr& node) override;
	void Visit(const ArrayLiteralExpr& node) override;
	void Visit(const FunctionCallExpr& node) override;
	void Visit(const IndexExpr& node) override;
	void Visit(const MemberAccessExpr& node) override;
	void Visit(const BlockStmt& node) override;
	void Visit(const DoWhileStmt& node) override;
	void Visit(const RunStmt& node) override;
	void Visit(const RepStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const IfStmt& node) override;

private:
	std::string MakeLabel(const std::string& prefix);
	[[nodiscard]] int32_t GetVarOffset(const std::string& name) const;

	void EmitExprToRax(const class Expr& expr);
	void EnterFunction(const class FunctionDeclStmt& node);
	void LeaveFunction();

	void Emit(const std::string& line);
	void EmitLabel(const std::string& label);

	std::ostringstream m_out;
	uint32_t m_labelCounter = 0;

	std::unordered_map<std::string, int32_t> m_varOffsets;
	int32_t m_nextOffset = 0;
};