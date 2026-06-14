#pragma once
#include "AstAnnotations.h"
#include "ScopeManager.h"
#include "TypeResolver.h"
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/BreakStmt.h"
#include "src/compiler/ast/ContinueStmt.h"
#include "src/compiler/ast/RepCollectionStmt.h"
#include "src/compiler/ast/RepTimesStmt.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/compiler/codegen/CodegenContext.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SemanticAnalyzer : public IAstVisitor
{
public:
	[[nodiscard]] CodegenContext Analyze(const AstNode& root, DiagnosticEngine& engine);
private:
	void CollectTypes(const AstNode& root);
	void CollectFunctions(const AstNode& root);

	[[nodiscard]] std::shared_ptr<TypeInfo> GetType(const AstNode& node) const;
	void SetType(const AstNode& node, std::shared_ptr<TypeInfo> type);

	void Visit(const ProgramNode& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const BlockStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const IfStmt& node) override;
	void Visit(const BreakStmt& node) override;
	void Visit(const ContinueStmt& node) override;
	void Visit(const RepStmt& node) override;
	void Visit(const RepCollectionStmt& node) override;
	void Visit(const RepTimesStmt& node) override;
	void Visit(const RunStmt& node) override;
	void Visit(const DoWhileStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const UnaryExpr& node) override;
	void Visit(const FunctionCallExpr& node) override;
	void Visit(const NumExpr& node) override;
	void Visit(const StringExpr& node) override;
	void Visit(const BoolExpr& node) override;
	void Visit(const ArrayLiteralExpr& node) override;
	void Visit(const ArrayAllocExpr& node) override;
	void Visit(const IndexExpr& node) override;
	void Visit(const MemberAccessExpr& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const EnumDeclStmt& node) override;
	void Visit(const WhenStmt& node) override;

	TypeResolver m_typeResolver;
	ScopeManager m_scopeManager;
	DiagnosticEngine* m_engine = nullptr;

	AstAnnotations m_annotations;
	std::unordered_map<const AstNode*, SymbolInfo> m_resolvedSymbols;
	std::unordered_map<const AstNode*, uint32_t> m_varDeclSlots;
	std::unordered_map<const AstNode*, SymbolInfo> m_resolvedIterators;
	std::unordered_map<const AstNode*, CollectionLoopInfo> m_collectionLoopInfos;
	std::unordered_map<const AstNode*, TimesLoopInfo> m_repTimesInfos;
	int m_loopDepth = 0;
	std::unordered_map<std::string, FunctionInfo> m_functions;
	std::unordered_map<std::string, std::vector<std::string>> m_overloadsByName;
	std::string m_entryPointKey;

	std::shared_ptr<TypeInfo> m_currentReturnType;
	std::shared_ptr<TypeInfo> m_expectedType;
};