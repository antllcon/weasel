#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/compiler/sema/SymbolTable.h"
#include "src/compiler/vm/chunk/Chunk.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class CodeGenerator final : public IAstVisitor
{
public:
	explicit CodeGenerator(
		std::unordered_map<const AstNode*, SymbolInfo> symbols,
		std::unordered_map<const AstNode*, std::vector<SymbolInfo>> repIterators,
		std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> functions);

	[[nodiscard]] Chunk Generate(const AstNode& root);

	void Visit(const ProgramNode& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const EnumDeclStmt& node) override;
	void Visit(const ErrorExpr& node) override;
	void Visit(const ImplicitCastExpr& node) override;
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

private:
	struct UnresolvedCall
	{
		uint32_t patchOffset;
		std::string funcName;
		SourceRange range;
	};

	void EmitLogicalNot();
	void EmitConstant(Value value);

	std::unordered_map<const AstNode*, SymbolInfo> m_symbols;
	std::unordered_map<const AstNode*, std::vector<SymbolInfo>> m_repIterators;
	std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> m_functions;
	std::unordered_map<std::string, uint32_t> m_functionOffsets;
	std::vector<UnresolvedCall> m_unresolvedCalls;
	Chunk m_chunk;
	uint32_t m_currentLine = 0;
	uint32_t m_localCount = 0;
};