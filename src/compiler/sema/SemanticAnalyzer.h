#pragma once
#include "SymbolTable.h"
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SemanticAnalyzer : public IAstVisitor
{
public:
	[[nodiscard]] std::unordered_map<std::string, SymbolInfo> Analyze(AstNode& root, DiagnosticEngine& engine);

	struct FunctionInfo
	{
		std::shared_ptr<TypeInfo> returnType;
		std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> params;
	};

private:
	void CollectFunctions(const AstNode& root);
	void Visit(const ProgramNode& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const BlockStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const IfStmt& node) override;
	void Visit(const RepStmt& node) override;
	void Visit(const RunStmt& node) override;
	void Visit(const DoWhileStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const UnaryExpr& node) override;
	void Visit(const FunctionCallExpr& node) override;
	void Visit(const NumberExpr& node) override;
	void Visit(const StringExpr& node) override;
	void Visit(const BoolExpr& node) override;
	void Visit(const ArrayLiteralExpr& node) override;
	void Visit(const IndexExpr& node) override;
	void Visit(const MemberAccessExpr& node) override;
	void Visit(const ImplicitCastExpr& node) override;
	void Visit(const ErrorExpr& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const EnumDeclStmt& node) override;

	SymbolTable m_table;
	DiagnosticEngine* m_engine = nullptr;
	uint32_t m_nextSlot = 0;
	std::unordered_map<std::string, SymbolInfo>    m_resolvedSymbols;
	std::unordered_map<std::string, FunctionInfo>  m_functions;
	std::shared_ptr<TypeInfo>                      m_currentReturnType;
};