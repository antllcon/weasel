#include "AstVisualizer.h"

#include "src/compiler/ast/ArrayAllocExpr.h"
#include "src/compiler/ast/BreakStmt.h"
#include "src/compiler/ast/ContinueStmt.h"
#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
#include "src/compiler/ast/DoWhileStmt.h"
#include "src/compiler/ast/EnumDeclStmt.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FieldDecl.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/IndexExpr.h"
#include "src/compiler/ast/MemberAccessExpr.h"
#include "src/compiler/ast/NumExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/RepCollectionStmt.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/RepTimesStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/StructDeclStmt.h"
#include "src/compiler/ast/ClassDeclStmt.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/UnionDeclStmt.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/ast/WhenStmt.h"
#include "src/compiler/core/LanguageTokens.h"
#include "src/utils/logger/Logger.h"

namespace
{
std::string BinaryOpToString(BinaryOpKind op)
{
	switch (op)
	{
	case BinaryOpKind::Add:
		return std::string(LanguageTokens::OpPlus);
	case BinaryOpKind::Sub:
		return std::string(LanguageTokens::OpMinus);
	case BinaryOpKind::Mul:
		return std::string(LanguageTokens::OpMul);
	case BinaryOpKind::Div:
		return std::string(LanguageTokens::OpDiv);
	case BinaryOpKind::Mod:
		return std::string(LanguageTokens::OpMod);
	case BinaryOpKind::Eq:
		return std::string(LanguageTokens::OpEq);
	case BinaryOpKind::NotEq:
		return std::string(LanguageTokens::OpNotEq);
	case BinaryOpKind::Less:
		return std::string(LanguageTokens::OpLess);
	case BinaryOpKind::Greater:
		return std::string(LanguageTokens::OpGreater);
	case BinaryOpKind::LessEq:
		return std::string(LanguageTokens::OpLessEq);
	case BinaryOpKind::GreaterEq:
		return std::string(LanguageTokens::OpGreaterEq);
	case BinaryOpKind::LogicalAnd:
		return std::string(LanguageTokens::KwAnd);
	case BinaryOpKind::LogicalOr:
		return std::string(LanguageTokens::KwOr);
	}
	return "?";
}

std::string UnaryOpToString(UnaryOpKind op)
{
	switch (op)
	{
	case UnaryOpKind::LogicalNot:
		return std::string(LanguageTokens::KwNot);
	case UnaryOpKind::AddressOf:
		return std::string(LanguageTokens::OpAddressOf);
	case UnaryOpKind::Deref:
		return std::string(LanguageTokens::OpMul);
	case UnaryOpKind::Minus:
		return std::string(LanguageTokens::OpMinus);
	}
	return "?";
}

std::string ModifierToString(VarModifier mod)
{
	switch (mod)
	{
	case VarModifier::Val:
		return std::string(LanguageTokens::KwVal);
	case VarModifier::Var:
		return std::string(LanguageTokens::KwVar);
	case VarModifier::Def:
		return std::string(LanguageTokens::KwLet);
	}
	return "?";
}
} // namespace

void AstVisualizer::Visualize(const AstNode& root)
{
	AstVisualizer vis;
	vis.m_prefix = "\t\t";
	vis.m_isLast = true;
	vis.m_stream << "[ast]\t\tАбстрактное синтаксическое дерево:" << std::endl;
	root.Accept(vis);
	Logger::Log(vis.m_stream.str());
}

void AstVisualizer::PrintNode(const std::string& label)
{
	m_stream << m_prefix;
	m_stream << (m_isLast ? "└── " : "├── ");
	m_stream << label << std::endl;
}

void AstVisualizer::PrintLeaf(const std::string& label, bool isLast)
{
	const std::string childConnector = m_isLast ? "    " : "│   ";
	m_stream << m_prefix << childConnector;
	m_stream << (isLast ? "└── " : "├── ");
	m_stream << label << std::endl;
}

void AstVisualizer::VisitChild(const AstNode& child, bool isLast)
{
	const std::string savedPrefix = m_prefix;
	const bool savedIsLast = m_isLast;

	m_prefix += savedIsLast ? "    " : "│   ";
	m_isLast = isLast;
	child.Accept(*this);

	m_prefix = savedPrefix;
	m_isLast = savedIsLast;
}

void AstVisualizer::Visit(const ProgramNode& node)
{
	m_stream << m_prefix << "Program" << std::endl;

	const auto& decls = node.GetDeclarations();
	for (size_t i = 0; i < decls.size(); ++i)
	{
		VisitChild(*decls[i], i + 1 == decls.size());
	}
}

void AstVisualizer::Visit(const FunctionDeclStmt& node)
{
	std::string label = "FuncDecl (" + node.GetName() + ": " + node.GetReturnTypeName();
	if (!node.GetParams().empty())
	{
		label += " |";
		for (const auto& p : node.GetParams())
		{
			label += " " + p.typeName + " " + p.name;
		}
	}
	label += ")";
	PrintNode(label);
	VisitChild(node.GetBody(), true);
}

void AstVisualizer::Visit(const BlockStmt& node)
{
	PrintNode("Block");
	const auto& stmts = node.GetStmts();
	for (size_t i = 0; i < stmts.size(); ++i)
	{
		VisitChild(*stmts[i], i + 1 == stmts.size());
	}
}

void AstVisualizer::Visit(const VarDeclStmt& node)
{
	const std::string type = node.GetTypeName();
	const std::string op = node.IsMoveInit() ? " " + std::string(LanguageTokens::OpMove) : " " + std::string(LanguageTokens::OpAssign);
	std::string label = "VarDecl (" + ModifierToString(node.GetModifier()) + " " + type + " " + node.GetName();
	label += node.GetInit() ? op + ")" : ")";
	PrintNode(label);
	if (node.GetInit())
	{
		VisitChild(*node.GetInit(), true);
	}
}

void AstVisualizer::Visit(const AssignStmt& node)
{
	PrintNode(std::string("Assign (") + (node.IsMove() ? std::string(LanguageTokens::OpMove) : std::string(LanguageTokens::OpAssign)) + ")");
	VisitChild(node.GetLhs(), false);
	VisitChild(node.GetRhs(), true);
}

void AstVisualizer::Visit(const IfStmt& node)
{
	PrintNode("If");
	const bool hasElse = node.GetElseNode() != nullptr;
	VisitChild(node.GetCondition(), false);
	VisitChild(node.GetThenBlock(), !hasElse);

	if (hasElse)
	{
		VisitChild(*node.GetElseNode(), true);
	}
}

void AstVisualizer::Visit(const ArrayAllocExpr& node)
{
	PrintNode("ArrayAlloc (" + node.GetElementTypeName() + ")");
	VisitChild(node.GetSize(), true);
}

void AstVisualizer::Visit(const WhenStmt& node)
{
	PrintNode("When");
	if (node.GetSubject())
	{
		VisitChild(*node.GetSubject(), false);
	}

	const auto& entries = node.GetEntries();
	for (size_t i = 0; i < entries.size(); ++i)
	{
		PrintNode("Branch");
		for (const auto& cond : entries[i].conditions)
		{
			VisitChild(*cond, false);
		}
		VisitChild(*entries[i].body, i + 1 == entries.size() && !node.GetElseBody());
	}

	if (node.GetElseBody())
	{
		PrintNode("Else");
		VisitChild(*node.GetElseBody(), true);
	}
}

void AstVisualizer::Visit(const BreakStmt& /*node*/)
{
	PrintLeaf("break", true);
}

void AstVisualizer::Visit(const ContinueStmt& /*node*/)
{
	PrintLeaf("continue", true);
}

void AstVisualizer::Visit(const RunStmt& node)
{
	PrintNode("While");
	VisitChild(node.GetCondition(), false);
	VisitChild(node.GetBody(), true);
}

void AstVisualizer::Visit(const DoWhileStmt& node)
{
	PrintNode("DoWhile");
	VisitChild(node.GetBody(), false);
	VisitChild(node.GetCondition(), true);
}

void AstVisualizer::Visit(const RepStmt& node)
{
	const std::string label = node.IsDown()
		? "RepDown (" + node.GetIterator() + ")"
		: "RepRange (" + node.GetIterator() + ")";
	PrintNode(label);
	VisitChild(node.GetStartExpr(), false);
	VisitChild(node.GetEndExpr(), node.GetStepExpr() == nullptr);
	if (node.GetStepExpr())
		VisitChild(*node.GetStepExpr(), false);
	VisitChild(node.GetBody(), true);
}

void AstVisualizer::Visit(const RepCollectionStmt& node)
{
	const std::string label = node.HasIndexIterator()
		? "RepCollection (" + node.GetIndexIterator() + ", " + node.GetValueIterator() + ")"
		: "RepCollection (" + node.GetValueIterator() + ")";
	PrintNode(label);
	VisitChild(node.GetCollectionExpr(), false);
	VisitChild(node.GetBody(), true);
}

void AstVisualizer::Visit(const RepTimesStmt& node)
{
	PrintNode("RepTimes");
	VisitChild(node.GetCountExpr(), false);
	VisitChild(node.GetBody(), true);
}

void AstVisualizer::Visit(const ReturnStmt& node)
{
	PrintNode("Return");
	if (node.GetValue())
	{
		VisitChild(*node.GetValue(), true);
	}
}

void AstVisualizer::Visit(const ExprStmt& node)
{
	PrintNode("ExprStmt");
	VisitChild(node.GetExpr(), true);
}

void AstVisualizer::Visit(const BinaryExpr& node)
{
	PrintNode("Binary (" + BinaryOpToString(node.GetOp()) + ")");
	VisitChild(node.GetLeft(), false);
	VisitChild(node.GetRight(), true);
}

void AstVisualizer::Visit(const UnaryExpr& node)
{
	PrintNode("Unary (" + UnaryOpToString(node.GetOp()) + ")");
	VisitChild(node.GetOperand(), true);
}

void AstVisualizer::Visit(const IdentifierExpr& node)
{
	PrintNode("Identifier (" + node.GetName() + ")");
}

void AstVisualizer::Visit(const NumExpr& node)
{
	const std::string suffix = node.IsFloat() ? "f" : "";
	PrintNode("Num (" + node.GetValue() + suffix + ")");
}

void AstVisualizer::Visit(const StringExpr& node)
{
	PrintNode("String (" + node.GetValue() + ")");
}

void AstVisualizer::Visit(const BoolExpr& node)
{
	PrintNode(std::string("Bool (") + (node.GetValue() ? "true" : "false") + ")");
}

void AstVisualizer::Visit(const FunctionCallExpr& node)
{
	PrintNode("Call (" + node.GetName() + ")");
	const auto& args = node.GetArgs();
	for (size_t i = 0; i < args.size(); ++i)
	{
		VisitChild(*args[i], i + 1 == args.size());
	}
}

void AstVisualizer::Visit(const ArrayLiteralExpr& node)
{
	PrintNode("ArrayLiteral");
	const auto& elems = node.GetElements();
	for (size_t i = 0; i < elems.size(); ++i)
	{
		VisitChild(*elems[i], i + 1 == elems.size());
	}
}

void AstVisualizer::Visit(const IndexExpr& node)
{
	PrintNode("Index");
	VisitChild(node.GetReceiver(), false);
	VisitChild(node.GetIndex(), true);
}

void AstVisualizer::Visit(const MemberAccessExpr& node)
{
	PrintNode("Member (." + node.GetField() + ")");
	VisitChild(node.GetReceiver(), true);
}

void AstVisualizer::Visit(const StructDeclStmt& node)
{
	PrintNode("StructDecl (" + node.GetName() + ")");
	const auto& fields = node.GetFields();
	for (size_t i = 0; i < fields.size(); ++i)
	{
		PrintLeaf("Field (" + fields[i].typeName + " " + fields[i].name + ")", i + 1 == fields.size());
	}
}

void AstVisualizer::Visit(const ClassDeclStmt& node)
{
	PrintNode("ClassDecl (" + node.GetName() + ")");
	const auto& fields = node.GetFields();
	for (size_t i = 0; i < fields.size(); ++i)
	{
		PrintLeaf("Field (" + fields[i].typeName + " " + fields[i].name + ")", i + 1 == fields.size());
	}
}

void AstVisualizer::Visit(const UnionDeclStmt& node)
{
	PrintNode("UnionDecl (" + node.GetName() + ")");
	const auto& fields = node.GetFields();
	for (size_t i = 0; i < fields.size(); ++i)
	{
		PrintLeaf("Field (" + fields[i].typeName + " " + fields[i].name + ")", i + 1 == fields.size());
	}
}

void AstVisualizer::Visit(const EnumDeclStmt& node)
{
	PrintNode("EnumDecl (" + node.GetName() + ")");
	const auto& values = node.GetValues();
	for (size_t i = 0; i < values.size(); ++i)
	{
		PrintLeaf("Value (" + values[i] + ")", i + 1 == values.size());
	}
}