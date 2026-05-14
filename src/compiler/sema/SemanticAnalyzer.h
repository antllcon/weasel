#pragma once
#include "SymbolTable.h"
#include "src/compiler/ast/AstNode.h"
#include <cstdint>
#include <string>
#include <unordered_map>

class SemanticAnalyzer
{
public:
	[[nodiscard]] std::unordered_map<std::string, uint32_t> Analyze(AstNode& root);

private:
	void AnalyzeNode(AstNode& node);
	void AnalyzeProgram(class ProgramNode& node);
	void AnalyzeFuncDecl(class FunctionDeclStmt& node);
	void AnalyzeBlock(class BlockStmt& node);
	void AnalyzeVarDecl(class VarDeclStmt& node);
	void AnalyzeAssign(class AssignStmt& node);
	void AnalyzeIf(class IfStmt& node);
	void AnalyzeRep(class RepStmt& node);
	void AnalyzeReturn(class ReturnStmt& node);
	void AnalyzeDoWhile(class DoWhileStmt& node);
	void AnalyzeRun(class RunStmt& node);

	SymbolTable m_table;
	uint32_t m_nextSlot = 0;
	std::unordered_map<std::string, uint32_t> m_slotMap;
};
