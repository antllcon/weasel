#pragma once

class ErrorExpr;
class ImplicitCastExpr;
class BinaryExpr;
class IdentifierExpr;
class NumberExpr;
class FunctionCallExpr;
class RepStmt;
class RunStmt;
class BlockStmt;
class VarDeclStmt;
class AssignStmt;
class ExprStmt;
class FunctionDeclStmt;
class ReturnStmt;
class IfStmt;

class IAstVisitor
{
public:
	virtual ~IAstVisitor() = default;

	virtual void Visit(const ErrorExpr& node) = 0;
	virtual void Visit(const ImplicitCastExpr& node) = 0;
	virtual void Visit(const BinaryExpr& node) = 0;
	virtual void Visit(const IdentifierExpr& node) = 0;
	virtual void Visit(const NumberExpr& node) = 0;
	virtual void Visit(const FunctionCallExpr& node) = 0;
	virtual void Visit(const BlockStmt& node) = 0;
	virtual void Visit(const RunStmt& node) = 0;
	virtual void Visit(const RepStmt& node) = 0;
	virtual void Visit(const VarDeclStmt& node) = 0;
	virtual void Visit(const AssignStmt& node) = 0;
	virtual void Visit(const ExprStmt& node) = 0;
	virtual void Visit(const FunctionDeclStmt& node) = 0;
	virtual void Visit(const ReturnStmt& node) = 0;
	virtual void Visit(const IfStmt& node) = 0;
};