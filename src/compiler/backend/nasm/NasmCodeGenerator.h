#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/compiler/sema/SymbolTable.h"
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class NasmCodeGenerator final : public IAstVisitor
{
public:
	explicit NasmCodeGenerator(
		std::unordered_map<const AstNode*, SymbolInfo> symbols,
		std::unordered_map<const AstNode*, uint32_t> varDeclSlots,
		std::unordered_map<const AstNode*, std::vector<SymbolInfo>> repIterators,
		std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> functions);

	[[nodiscard]] std::string Generate(const AstNode& root);

	void Visit(const ProgramNode& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const EnumDeclStmt& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const UnaryExpr& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const NumExpr& node) override;
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
	void Visit(const ArrayAllocExpr& node) override;
	void Visit(const WhenStmt& node) override;

private:
	std::string MakeLabel(const std::string& prefix);
	int32_t SlotToOffset(uint32_t slot) const;

	void EmitExprToRax(const Expr& expr);
	void EnterFunction(const FunctionDeclStmt& node);
	void LeaveFunction();

	void Emit(const std::string& line);
	void EmitLabel(const std::string& label);

private:
	std::ostringstream m_out;
	uint32_t m_labelCounter = 0;

	std::unordered_map<const AstNode*, SymbolInfo> m_symbols;
	std::unordered_map<const AstNode*, uint32_t> m_varDeclSlots;
	std::unordered_map<const AstNode*, std::vector<SymbolInfo>> m_repIterators;
	std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> m_functions;
};