#include "SemanticAnalyzer.h"

#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
#include "src/compiler/ast/DoWhileStmt.h"
#include "src/compiler/ast/EnumDeclStmt.h"
#include "src/compiler/ast/ErrorExpr.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/ImplicitCastExpr.h"
#include "src/compiler/ast/IndexExpr.h"
#include "src/compiler/ast/MemberAccessExpr.h"
#include "src/compiler/ast/NumberExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/StructDeclStmt.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/UnionDeclStmt.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/diagnostics/CompilationException.h"

namespace
{
void AssertIsVariableNotRedeclared(bool isNew, const std::string& name)
{
	if (!isNew)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Переменная уже объявлена в текущей области видимости: " + name});
	}
}
} // namespace

std::unordered_map<std::string, SymbolInfo> SemanticAnalyzer::Analyze(AstNode& root, DiagnosticEngine& engine)
{
	m_engine = &engine;
	root.Accept(*this);
	return std::move(m_resolvedSymbols);
}

void SemanticAnalyzer::Visit(const ProgramNode& node)
{
	for (const auto& decl : node.GetDeclarations())
	{
		decl->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const FunctionDeclStmt& node)
{
	m_nextSlot = 0;
	m_table.EnterScope();
	node.GetBody().Accept(*this);
	m_table.LeaveScope();
}

void SemanticAnalyzer::Visit(const BlockStmt& node)
{
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const VarDeclStmt& node)
{
	const uint32_t slot = m_nextSlot++;
	const bool isMutable = node.GetModifier() == VarModifier::Var;
	const bool isNew = m_table.Declare(node.GetName(), nullptr, isMutable, slot);
	AssertIsVariableNotRedeclared(isNew, node.GetName());
	m_resolvedSymbols[node.GetName()] = SymbolInfo{nullptr, slot, isMutable};

	if (node.GetInit())
	{
		node.GetInit()->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const AssignStmt& node)
{
	node.GetLhs().Accept(*this);
	node.GetRhs().Accept(*this);
}

void SemanticAnalyzer::Visit(const IfStmt& node)
{
	node.GetCondition().Accept(*this);

	m_table.EnterScope();
	node.GetThenBlock().Accept(*this);
	m_table.LeaveScope();

	const Stmt* elseNode = node.GetElseNode();
	if (!elseNode)
	{
		return;
	}

	if (const auto* elseIf = dynamic_cast<const IfStmt*>(elseNode))
	{
		elseIf->Accept(*this);
	}
	else if (const auto* elseBlock = dynamic_cast<const BlockStmt*>(elseNode))
	{
		m_table.EnterScope();
		elseBlock->Accept(*this);
		m_table.LeaveScope();
	}
}

void SemanticAnalyzer::Visit(const RepStmt& node)
{
	m_table.EnterScope();

	for (const auto& iterName : node.GetIterators())
	{
		const uint32_t slot = m_nextSlot++;
		const bool isNew = m_table.Declare(iterName, nullptr, true, slot);
		AssertIsVariableNotRedeclared(isNew, iterName);
		m_resolvedSymbols[iterName] = SymbolInfo{nullptr, slot, true};
	}

	node.GetOriginalBody().Accept(*this);
	m_table.LeaveScope();
}

void SemanticAnalyzer::Visit(const RunStmt& node)
{
	node.GetCondition().Accept(*this);
	m_table.EnterScope();
	node.GetBody().Accept(*this);
	m_table.LeaveScope();
}

void SemanticAnalyzer::Visit(const DoWhileStmt& node)
{
	m_table.EnterScope();
	node.GetBody().Accept(*this);
	m_table.LeaveScope();
	node.GetCondition().Accept(*this);
}

void SemanticAnalyzer::Visit(const ReturnStmt& node)
{
	if (node.GetValue())
	{
		node.GetValue()->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
}

void SemanticAnalyzer::Visit(const IdentifierExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const BinaryExpr& node)
{
	node.GetLeft().Accept(*this);
	node.GetRight().Accept(*this);
}

void SemanticAnalyzer::Visit(const UnaryExpr& node)
{
	node.GetOperand().Accept(*this);
}

void SemanticAnalyzer::Visit(const FunctionCallExpr& node)
{
	for (const auto& arg : node.GetArgs())
	{
		arg->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const NumberExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const StringExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const BoolExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const ArrayLiteralExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const IndexExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const MemberAccessExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const ImplicitCastExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const ErrorExpr& /*node*/)
{
}

void SemanticAnalyzer::Visit(const StructDeclStmt& /*node*/)
{
}

void SemanticAnalyzer::Visit(const UnionDeclStmt& /*node*/)
{
}

void SemanticAnalyzer::Visit(const EnumDeclStmt& /*node*/)
{
}