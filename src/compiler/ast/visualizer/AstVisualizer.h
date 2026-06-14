#pragma once
#include "src/compiler/ast/IAstVisitor.h"
#include <sstream>
#include <string>

class AstVisualizer final : public IAstVisitor
{
public:
	static void Visualize(const class AstNode& root);

	void Visit(const ProgramNode& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const ClassDeclStmt& node) override;
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
	void Visit(const RepCollectionStmt& node) override;
	void Visit(const RepTimesStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const IfStmt& node) override;
	void Visit(const ArrayAllocExpr& node) override;
	void Visit(const WhenStmt& node) override;
	void Visit(const BreakStmt& node) override;
	void Visit(const ContinueStmt& node) override;

private:
	void PrintNode(const std::string& label);
	void PrintLeaf(const std::string& label, bool isLast);
	void VisitChild(const AstNode& child, bool isLast);

private:
	std::ostringstream m_stream;
	std::string m_prefix;
	bool m_isLast = true;
};
