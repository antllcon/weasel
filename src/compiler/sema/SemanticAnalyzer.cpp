#include "SemanticAnalyzer.h"

#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/DoWhileStmt.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/VarDeclStmt.h"

#include <stdexcept>

namespace
{
void AssertIsVariableDeclared(bool isDeclared, const std::string& name)
{
	if (!isDeclared)
	{
		throw std::runtime_error("Переменная не объявлена: " + name);
	}
}

void AssertIsVariableNotRedeclared(bool isNew, const std::string& name)
{
	if (!isNew)
	{
		throw std::runtime_error("Переменная уже объявлена в текущей области видимости: " + name);
	}
}
} // namespace

std::unordered_map<std::string, uint32_t> SemanticAnalyzer::Analyze(AstNode& root)
{
	AnalyzeNode(root);
	return m_slotMap;
}

void SemanticAnalyzer::AnalyzeNode(AstNode& node)
{
	if (auto* program = dynamic_cast<ProgramNode*>(&node))
	{
		AnalyzeProgram(*program);
		return;
	}
	if (auto* funcDecl = dynamic_cast<FunctionDeclStmt*>(&node))
	{
		AnalyzeFuncDecl(*funcDecl);
		return;
	}
	throw std::runtime_error("Неподдерживаемый тип узла AST");
}

void SemanticAnalyzer::AnalyzeProgram(ProgramNode& node)
{
	for (const auto& decl : node.GetDeclarations())
	{
		AnalyzeNode(*decl);
	}
}

void SemanticAnalyzer::AnalyzeFuncDecl(FunctionDeclStmt& node)
{
	m_table.EnterScope();
	AnalyzeBlock(const_cast<BlockStmt&>(node.GetBody()));
	m_table.LeaveScope();
}

void SemanticAnalyzer::AnalyzeBlock(BlockStmt& node)
{
	for (const auto& stmtPtr : node.GetStmts())
	{
		auto& stmt = *stmtPtr;
		if (auto* varDecl = dynamic_cast<VarDeclStmt*>(&stmt))
		{
			AnalyzeVarDecl(*varDecl);
		}
		else if (auto* assign = dynamic_cast<AssignStmt*>(&stmt))
		{
			AnalyzeAssign(*assign);
		}
		else if (auto* ifStmt = dynamic_cast<IfStmt*>(&stmt))
		{
			AnalyzeIf(*ifStmt);
		}
		else if (auto* repStmt = dynamic_cast<RepStmt*>(&stmt))
		{
			AnalyzeRep(*repStmt);
		}
		else if (auto* doWhileStmt = dynamic_cast<DoWhileStmt*>(&stmt))
		{
			AnalyzeDoWhile(*doWhileStmt);
		}
		else if (auto* runStmt = dynamic_cast<RunStmt*>(&stmt))
		{
			AnalyzeRun(*runStmt);
		}
		else if (auto* retStmt = dynamic_cast<ReturnStmt*>(&stmt))
		{
			AnalyzeReturn(*retStmt);
		}
		else if (dynamic_cast<ExprStmt*>(&stmt))
		{
		}
	}
}

void SemanticAnalyzer::AnalyzeVarDecl(VarDeclStmt& node)
{
	const uint32_t slot = m_nextSlot++;
	const bool isNew = m_table.Declare(node.GetName(), nullptr, node.GetModifier() == VarModifier::Var, slot);
	AssertIsVariableNotRedeclared(isNew, node.GetName());
	m_slotMap[node.GetName()] = slot;
}

void SemanticAnalyzer::AnalyzeAssign(AssignStmt& /*node*/)
{
}

void SemanticAnalyzer::AnalyzeIf(IfStmt& node)
{
	m_table.EnterScope();
	AnalyzeBlock(const_cast<BlockStmt&>(node.GetThenBlock()));
	m_table.LeaveScope();

	const Stmt* elseNode = node.GetElseNode();
	if (elseNode == nullptr)
	{
		return;
	}

	if (auto* elseIf = dynamic_cast<const IfStmt*>(elseNode))
	{
		AnalyzeIf(const_cast<IfStmt&>(*elseIf));
	}
	else if (auto* elseBlock = dynamic_cast<const BlockStmt*>(elseNode))
	{
		m_table.EnterScope();
		AnalyzeBlock(const_cast<BlockStmt&>(*elseBlock));
		m_table.LeaveScope();
	}
}

void SemanticAnalyzer::AnalyzeRep(RepStmt& node)
{
	m_table.EnterScope();
	const std::string& iterName = node.GetIterators()[0];
	const uint32_t slot = m_nextSlot++;
	const bool isNew = m_table.Declare(iterName, nullptr, true, slot);
	AssertIsVariableNotRedeclared(isNew, iterName);
	m_slotMap[iterName] = slot;

	AnalyzeBlock(const_cast<BlockStmt&>(dynamic_cast<const BlockStmt&>(node.GetOriginalBody())));
	m_table.LeaveScope();
}

void SemanticAnalyzer::AnalyzeDoWhile(DoWhileStmt& node)
{
	m_table.EnterScope();
	AnalyzeBlock(const_cast<BlockStmt&>(dynamic_cast<const BlockStmt&>(node.GetBody())));
	m_table.LeaveScope();
}

void SemanticAnalyzer::AnalyzeRun(RunStmt& node)
{
	m_table.EnterScope();
	AnalyzeBlock(const_cast<BlockStmt&>(dynamic_cast<const BlockStmt&>(node.GetBody())));
	m_table.LeaveScope();
}

void SemanticAnalyzer::AnalyzeReturn(ReturnStmt& /*node*/)
{
}
