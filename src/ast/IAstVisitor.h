#pragma once

class ErrorExpr;
class ImplicitCastExpr;
class BinaryExpr;
class IdentifierExpr;
class RepStmt;
class RunStmt;
class BlockStmt;

class IAstVisitor
{
public:
	virtual ~IAstVisitor() = default;

	virtual void Visit(const ErrorExpr& node) = 0;
	virtual void Visit(const ImplicitCastExpr& node) = 0;
	virtual void Visit(const BinaryExpr& node) = 0;
	virtual void Visit(const IdentifierExpr& node) = 0;

	virtual void Visit(const BlockStmt& node) = 0;
	virtual void Visit(const RunStmt& node) = 0;
	virtual void Visit(const RepStmt& node) = 0;
};