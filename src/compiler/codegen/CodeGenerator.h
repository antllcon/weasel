#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/compiler/vm/chunk/Chunk.h"
#include <cstdint>
#include <string>
#include <unordered_map>

class CodeGenerator final : public IAstVisitor
{
public:
	explicit CodeGenerator(std::unordered_map<std::string, uint32_t> slotMap);

	[[nodiscard]] Chunk Generate(const AstNode& root);

	void Visit(const ErrorExpr& node) override;
	void Visit(const ImplicitCastExpr& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const NumberExpr& node) override;
	void Visit(const FunctionCallExpr& node) override;
	void Visit(const BlockStmt& node) override;
	void Visit(const RunStmt& node) override;
	void Visit(const RepStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const IfStmt& node) override;

private:
	void EmitLogicalNot();
	void EmitConstantU32(uint32_t value);

	std::unordered_map<std::string, uint32_t> m_slotMap;
	Chunk m_chunk;
	uint32_t m_currentLine = 0;
};
