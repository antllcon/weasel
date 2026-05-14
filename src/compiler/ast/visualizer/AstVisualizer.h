#pragma once
#include "src/compiler/ast/IAstVisitor.h"
#include <sstream>
#include <string>

class AstVisualizer final : public IAstVisitor
{
public:
	static void Visualize(const class AstNode& root);

	void Visit(const class ProgramNode& node) override;
	void Visit(const class StructDeclStmt& node) override;
	void Visit(const class UnionDeclStmt& node) override;
	void Visit(const class EnumDeclStmt& node) override;
	void Visit(const class ErrorExpr& node) override;
	void Visit(const class ImplicitCastExpr& node) override;
	void Visit(const class BinaryExpr& node) override;
	void Visit(const class UnaryExpr& node) override;
	void Visit(const class IdentifierExpr& node) override;
	void Visit(const class NumberExpr& node) override;
	void Visit(const class StringExpr& node) override;
	void Visit(const class BoolExpr& node) override;
	void Visit(const class ArrayLiteralExpr& node) override;
	void Visit(const class FunctionCallExpr& node) override;
	void Visit(const class IndexExpr& node) override;
	void Visit(const class MemberAccessExpr& node) override;
	void Visit(const class BlockStmt& node) override;
	void Visit(const class DoWhileStmt& node) override;
	void Visit(const class RunStmt& node) override;
	void Visit(const class RepStmt& node) override;
	void Visit(const class VarDeclStmt& node) override;
	void Visit(const class AssignStmt& node) override;
	void Visit(const class ExprStmt& node) override;
	void Visit(const class FunctionDeclStmt& node) override;
	void Visit(const class ReturnStmt& node) override;
	void Visit(const class IfStmt& node) override;

private:
	void PrintNode(const std::string& label);
	void PrintLeaf(const std::string& label, bool isLast);
	void VisitChild(const class AstNode& child, bool isLast);

	std::ostringstream m_stream;
	std::string m_prefix;
	bool m_isLast = true;
};
