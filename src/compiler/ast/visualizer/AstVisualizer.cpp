#include "AstVisualizer.h"
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
#include "src/utils/logger/Logger.h"

namespace
{

std::string BinaryOpToString(BinaryOpKind op)
{
	switch (op)
	{
	case BinaryOpKind::Add:
		return "+";
	case BinaryOpKind::Sub:
		return "-";
	case BinaryOpKind::Mul:
		return "*";
	case BinaryOpKind::Div:
		return "/";
	case BinaryOpKind::Mod:
		return "%";
	case BinaryOpKind::Eq:
		return "==";
	case BinaryOpKind::NotEq:
		return "><";
	case BinaryOpKind::Less:
		return "<";
	case BinaryOpKind::Greater:
		return ">";
	case BinaryOpKind::LessEq:
		return "<=";
	case BinaryOpKind::GreaterEq:
		return ">=";
	case BinaryOpKind::LogicalAnd:
		return "and";
	case BinaryOpKind::LogicalOr:
		return "orr";
	case BinaryOpKind::ShiftLeft:
		return "<<";
	case BinaryOpKind::ShiftRight:
		return ">>";
	case BinaryOpKind::BitwiseAnd:
		return "bnd";
	case BinaryOpKind::BitwiseOr:
		return "bor";
	case BinaryOpKind::BitwiseXor:
		return "bxr";
	}
	return "?";
}

std::string UnaryOpToString(UnaryOpKind op)
{
	switch (op)
	{
	case UnaryOpKind::LogicalNot:
		return "not";
	case UnaryOpKind::BitwiseNot:
		return "bnt";
	case UnaryOpKind::AddressOf:
		return "&";
	case UnaryOpKind::Deref:
		return "*";
	}
	return "?";
}

std::string ModifierToString(VarModifier mod)
{
	switch (mod)
	{
	case VarModifier::Val:
		return "val";
	case VarModifier::Var:
		return "var";
	case VarModifier::Def:
		return "def";
	}
	return "?";
}

std::string FormatType(const std::string& sign, const std::string& name)
{
	return sign.empty() ? name : sign + " " + name;
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

	m_prefix += (savedIsLast ? "    " : "│   ");
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
	const std::string retType = FormatType(node.GetReturnTypeSign(), node.GetReturnTypeName());

	std::string label = "FuncDecl (" + node.GetName() + ": " + retType;
	if (!node.GetParams().empty())
	{
		label += " |";
		for (const auto& p : node.GetParams())
		{
			label += " " + FormatType(p.typeSign, p.typeName) + " " + p.name;
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
	const std::string type = FormatType(node.GetTypeSign(), node.GetTypeName());
	const std::string op = node.IsMoveInit() ? " <-" : " :=";
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
	PrintNode(std::string("Assign (") + (node.IsMove() ? "<-" : ":=") + ")");
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
	const auto& iters = node.GetIterators();
	std::string label = "For (";
	for (size_t i = 0; i < iters.size(); ++i)
	{
		if (i > 0)
		{
			label += ", ";
		}
		label += iters[i];
	}
	label += ")";
	PrintNode(label);

	const auto& ranges = node.GetRanges();
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		VisitChild(*ranges[i], false);
	}
	VisitChild(node.GetOriginalBody(), true);
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

void AstVisualizer::Visit(const NumberExpr& node)
{
	const std::string suffix = node.IsFloat() ? "f" : "";
	PrintNode("Number (" + node.GetValue() + suffix + ")");
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
		PrintLeaf("Field (" + FormatType(fields[i].typeSign, fields[i].typeName) + " " + fields[i].name + ")",
			i + 1 == fields.size());
	}
}

void AstVisualizer::Visit(const UnionDeclStmt& node)
{
	PrintNode("UnionDecl (" + node.GetName() + ")");
	const auto& fields = node.GetFields();
	for (size_t i = 0; i < fields.size(); ++i)
	{
		PrintLeaf("Field (" + FormatType(fields[i].typeSign, fields[i].typeName) + " " + fields[i].name + ")",
			i + 1 == fields.size());
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

void AstVisualizer::Visit(const ErrorExpr&)
{
	PrintNode("ErrorExpr");
}

void AstVisualizer::Visit(const ImplicitCastExpr& node)
{
	PrintNode("ImplicitCast");
	VisitChild(node.GetSource(), true);
}