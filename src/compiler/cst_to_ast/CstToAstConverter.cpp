#include "CstToAstConverter.h"

#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
#include "src/compiler/ast/DoWhileStmt.h"
#include "src/compiler/ast/EnumDeclStmt.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
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

SourceLocation FindFirstLeaf(const CstNode& node)
{
	if (node.children.empty())
	{
		return node.location;
	}
	return FindFirstLeaf(*node.children.front());
}

SourceLocation FindLastLeaf(const CstNode& node)
{
	if (node.children.empty())
	{
		return node.location;
	}
	return FindLastLeaf(*node.children.back());
}

SourceRange ExtractRange(const CstNode& node)
{
	return SourceRange{FindFirstLeaf(node), FindLastLeaf(node)};
}

[[noreturn]] void ThrowConversionError(const std::string& message, const CstNode& node)
{
	const auto loc = FindFirstLeaf(node);
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Parser,
		.message = message,
		.actual = node.label,
		.line = loc.line,
		.pos = loc.pos});
}

std::unique_ptr<Expr> ConvertExprByLabel(const CstNode& node);
std::unique_ptr<BlockStmt> ConvertStmtList(const CstNode& node);
std::unique_ptr<Stmt> ConvertElseOpt(const CstNode& node);

BinaryOpKind ParseBinaryOp(const std::string& op)
{
	if (op == "+") return BinaryOpKind::Add;
	if (op == "-") return BinaryOpKind::Sub;
	if (op == "*") return BinaryOpKind::Mul;
	if (op == "/") return BinaryOpKind::Div;
	if (op == "%") return BinaryOpKind::Mod;
	if (op == "==") return BinaryOpKind::Eq;
	if (op == "><") return BinaryOpKind::NotEq;
	if (op == "<") return BinaryOpKind::Less;
	if (op == ">") return BinaryOpKind::Greater;
	if (op == "<=") return BinaryOpKind::LessEq;
	if (op == ">=") return BinaryOpKind::GreaterEq;
	if (op == "and") return BinaryOpKind::LogicalAnd;
	if (op == "orr") return BinaryOpKind::LogicalOr;
	if (op == "<<") return BinaryOpKind::ShiftLeft;
	if (op == ">>") return BinaryOpKind::ShiftRight;
	if (op == "bnd") return BinaryOpKind::BitwiseAnd;
	if (op == "bor") return BinaryOpKind::BitwiseOr;
	if (op == "bxr") return BinaryOpKind::BitwiseXor;
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Parser,
		.message = "Неизвестный бинарный оператор",
		.actual = op});
}

UnaryOpKind ParseUnaryOp(const std::string& op)
{
	if (op == "not") return UnaryOpKind::LogicalNot;
	if (op == "bnt") return UnaryOpKind::BitwiseNot;
	if (op == "&") return UnaryOpKind::AddressOf;
	if (op == "*") return UnaryOpKind::Deref;
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Parser,
		.message = "Неизвестный унарный оператор",
		.actual = op});
}

VarModifier ParseModifier(const CstNode& modNode)
{
	const std::string& val = modNode.children[0]->value;
	if (val == "val") return VarModifier::Val;
	if (val == "var") return VarModifier::Var;
	if (val == "def") return VarModifier::Def;
	ThrowConversionError("Неизвестный модификатор переменной: " + val, modNode);
}

std::pair<std::string, std::string> ParseType(const CstNode& typeNode)
{
	if (typeNode.children.size() == 2 && typeNode.children[0]->label == "Sign")
	{
		const std::string sign = typeNode.children[0]->children[0]->value;
		const std::string base = typeNode.children[1]->children[0]->value;
		return {sign, base};
	}
	if (typeNode.children.size() == 1)
	{
		const auto& child = *typeNode.children[0];
		if (child.label == "BaseType")
		{
			return {"", child.children[0]->value};
		}
		return {"", child.value};
	}
	return {"", "voided"};
}

void CollectArgList(const CstNode& node, std::vector<std::unique_ptr<Expr>>& args);
std::unique_ptr<Expr> ConvertPostfixExpr(const CstNode& node);
std::unique_ptr<Expr> ConvertUnaryExpr(const CstNode& node);

std::unique_ptr<Expr> ConvertPrimaryExpr(const CstNode& node)
{
	const auto& child = *node.children[0];
	const auto range = ExtractRange(node);

	if (child.label == "id" && node.children.size() == 3)
	{
		return std::make_unique<FunctionCallExpr>(
			child.value, std::vector<std::unique_ptr<Expr>>{}, range);
	}
	if (child.label == "id" && node.children.size() == 4)
	{
		std::vector<std::unique_ptr<Expr>> args;
		CollectArgList(*node.children[2], args);
		return std::make_unique<FunctionCallExpr>(child.value, std::move(args), range);
	}
	if (child.label == "id")
	{
		return std::make_unique<IdentifierExpr>(child.value, range);
	}
	if (child.label == "num")
	{
		const bool isFloat = child.value.find('.') != std::string::npos;
		return std::make_unique<NumberExpr>(child.value, isFloat, range);
	}
	if (child.label == "str")
	{
		return std::make_unique<StringExpr>(child.value, range);
	}
	if (child.label == "true")
	{
		return std::make_unique<BoolExpr>(true, range);
	}
	if (child.label == "false")
	{
		return std::make_unique<BoolExpr>(false, range);
	}
	if (child.label == "(")
	{
		return ConvertExprByLabel(*node.children[1]);
	}
	if (child.label == "[")
	{
		std::vector<std::unique_ptr<Expr>> elements;
		CollectArgList(*node.children[1], elements);
		return std::make_unique<ArrayLiteralExpr>(std::move(elements), range);
	}
	ThrowConversionError("Неподдерживаемое первичное выражение", node);
}

void CollectArgList(const CstNode& node, std::vector<std::unique_ptr<Expr>>& args)
{
	if (node.children.size() == 3)
	{
		CollectArgList(*node.children[0], args);
		args.push_back(ConvertExprByLabel(*node.children[2]));
	}
	else
	{
		args.push_back(ConvertExprByLabel(*node.children[0]));
	}
}

std::unique_ptr<Expr> ConvertPostfixExpr(const CstNode& node)
{
	if (node.children.size() == 1)
	{
		return ConvertExprByLabel(*node.children[0]);
	}

	auto receiver = ConvertPostfixExpr(*node.children[0]);
	const size_t childCount = node.children.size();
	const auto range = ExtractRange(node);

	if (childCount == 4 && node.children[1]->value == "[")
	{
		auto index = ConvertExprByLabel(*node.children[2]);
		return std::make_unique<IndexExpr>(std::move(receiver), std::move(index), range);
	}

	if (childCount == 3 && node.children[1]->value == ".")
	{
		const std::string field = node.children[2]->value;
		return std::make_unique<MemberAccessExpr>(std::move(receiver), field, range);
	}

	if (childCount == 5 && node.children[1]->value == ".")
	{
		const std::string method = node.children[2]->value;
		std::vector<std::unique_ptr<Expr>> args;
		args.push_back(std::move(receiver));
		return std::make_unique<FunctionCallExpr>(method, std::move(args), range);
	}

	if (childCount == 6 && node.children[1]->value == ".")
	{
		const std::string method = node.children[2]->value;
		std::vector<std::unique_ptr<Expr>> args;
		args.push_back(std::move(receiver));
		CollectArgList(*node.children[4], args);
		return std::make_unique<FunctionCallExpr>(method, std::move(args), range);
	}

	ThrowConversionError(
		"Неподдерживаемый постфиксный узел с " + std::to_string(childCount) + " дочерними узлами",
		node);
}

std::unique_ptr<Expr> ConvertUnaryExpr(const CstNode& node)
{
	if (node.children.size() == 1)
	{
		return ConvertExprByLabel(*node.children[0]);
	}

	const UnaryOpKind op = ParseUnaryOp(node.children[0]->value);
	auto operand = ConvertUnaryExpr(*node.children[1]);
	return std::make_unique<UnaryExpr>(op, std::move(operand), ExtractRange(node));
}

std::unique_ptr<Expr> ConvertExprByLabel(const CstNode& node)
{
	if (node.label == "PrimaryExpr") return ConvertPrimaryExpr(node);
	if (node.label == "PostfixExpr") return ConvertPostfixExpr(node);
	if (node.label == "UnaryExpr") return ConvertUnaryExpr(node);

	if (node.children.size() == 1)
	{
		return ConvertExprByLabel(*node.children[0]);
	}

	if (node.children.size() == 3)
	{
		auto left = ConvertExprByLabel(*node.children[0]);
		auto right = ConvertExprByLabel(*node.children[2]);
		const BinaryOpKind op = ParseBinaryOp(node.children[1]->value);
		return std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
	}

	ThrowConversionError(
		"Неподдерживаемый узел выражения с " + std::to_string(node.children.size()) + " дочерними узлами",
		node);
}

std::unique_ptr<Expr> ConvertExpr(const CstNode& node)
{
	return ConvertExprByLabel(node);
}

std::unique_ptr<Stmt> ConvertVarDecl(const CstNode& node)
{
	const VarModifier modifier = ParseModifier(*node.children[0]);
	const auto [sign, typeName] = ParseType(*node.children[1]);
	const std::string name = node.children[2]->value;

	std::unique_ptr<Expr> init = nullptr;
	bool isMoveInit = false;

	if (node.children.size() == 5)
	{
		const std::string& opVal = node.children[3]->children[0]->value;
		isMoveInit = (opVal == "<-");
		init = ConvertExpr(*node.children[4]);
	}

	return std::make_unique<VarDeclStmt>(
		modifier, sign, typeName, name, isMoveInit, std::move(init), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertAssignStmt(const CstNode& node)
{
	auto lhs = ConvertExpr(*node.children[0]);
	const std::string& opVal = node.children[1]->children[0]->value;
	const bool isMove = (opVal == "<-");
	auto rhs = ConvertExpr(*node.children[2]);
	return std::make_unique<AssignStmt>(std::move(lhs), isMove, std::move(rhs), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertReturnStmt(const CstNode& node)
{
	std::unique_ptr<Expr> value = nullptr;
	if (node.children.size() == 2)
	{
		value = ConvertExpr(*node.children[1]);
	}
	return std::make_unique<ReturnStmt>(std::move(value), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertForStmt1D(const CstNode& node)
{
	const std::string iterName = node.children[2]->value;
	auto startExpr = ConvertExpr(*node.children[4]);
	auto endExpr = ConvertExpr(*node.children[6]);
	auto body = ConvertStmtList(*node.children[9]);

	std::vector<std::string> iterators = {iterName};
	std::vector<std::unique_ptr<Expr>> ranges;
	ranges.push_back(std::move(startExpr));
	ranges.push_back(std::move(endExpr));

	return std::make_unique<RepStmt>(
		std::move(iterators), std::move(ranges), std::move(body), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertForStmt2D(const CstNode& node)
{
	const std::string iter1 = node.children[2]->value;
	const std::string iter2 = node.children[4]->value;
	auto start1 = ConvertExpr(*node.children[6]);
	auto end1 = ConvertExpr(*node.children[8]);
	auto start2 = ConvertExpr(*node.children[10]);
	auto end2 = ConvertExpr(*node.children[12]);
	auto body = ConvertStmtList(*node.children[15]);

	std::vector<std::string> iterators = {iter1, iter2};
	std::vector<std::unique_ptr<Expr>> ranges;
	ranges.push_back(std::move(start1));
	ranges.push_back(std::move(end1));
	ranges.push_back(std::move(start2));
	ranges.push_back(std::move(end2));

	return std::make_unique<RepStmt>(
		std::move(iterators), std::move(ranges), std::move(body), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertForStmt(const CstNode& node)
{
	if (node.children.size() == 11)
	{
		return ConvertForStmt1D(node);
	}
	if (node.children.size() == 17)
	{
		return ConvertForStmt2D(node);
	}
	ThrowConversionError(
		"Неподдерживаемая форма rep с " + std::to_string(node.children.size()) + " дочерними узлами",
		node);
}

std::unique_ptr<Stmt> ConvertWhileStmt(const CstNode& node)
{
	auto condition = ConvertExpr(*node.children[2]);
	auto body = ConvertStmtList(*node.children[5]);
	return std::make_unique<RunStmt>(std::move(condition), std::move(body), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertDoWhileStmt(const CstNode& node)
{
	auto body = ConvertStmtList(*node.children[1]);
	auto condition = ConvertExpr(*node.children[5]);
	return std::make_unique<DoWhileStmt>(std::move(body), std::move(condition), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertElseOpt(const CstNode& node)
{
	if (node.children.empty())
	{
		return nullptr;
	}

	if (node.children[1]->value == "(")
	{
		auto condition = ConvertExpr(*node.children[2]);
		auto thenBlock = ConvertStmtList(*node.children[5]);
		auto nested = ConvertElseOpt(*node.children[7]);
		return std::make_unique<IfStmt>(
			std::move(condition), std::move(thenBlock), std::move(nested), ExtractRange(node));
	}

	return ConvertStmtList(*node.children[2]);
}

std::unique_ptr<Stmt> ConvertIfStmt(const CstNode& node)
{
	auto condition = ConvertExpr(*node.children[2]);
	auto thenBlock = ConvertStmtList(*node.children[5]);
	auto elseNode = ConvertElseOpt(*node.children[7]);
	return std::make_unique<IfStmt>(
		std::move(condition), std::move(thenBlock), std::move(elseNode), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertStmt(const CstNode& node)
{
	const auto& firstChild = *node.children[0];
	if (firstChild.label == "VarDecl") return ConvertVarDecl(firstChild);
	if (firstChild.label == "AssignStmt") return ConvertAssignStmt(firstChild);
	if (firstChild.label == "IfStmt") return ConvertIfStmt(firstChild);
	if (firstChild.label == "ForStmt") return ConvertForStmt(firstChild);
	if (firstChild.label == "WhileStmt") return ConvertWhileStmt(firstChild);
	if (firstChild.label == "DoWhileStmt") return ConvertDoWhileStmt(firstChild);
	if (firstChild.label == "ReturnStmt") return ConvertReturnStmt(firstChild);
	if (firstChild.label == "ExprStmt")
	{
		auto expr = ConvertExpr(*firstChild.children[0]);
		return std::make_unique<ExprStmt>(std::move(expr), ExtractRange(firstChild));
	}
	ThrowConversionError("Неизвестный тип оператора", firstChild);
}

void CollectStmts(const CstNode& node, std::vector<std::unique_ptr<Stmt>>& stmts)
{
	if (node.children.empty()) return;
	CollectStmts(*node.children[0], stmts);
	stmts.push_back(ConvertStmt(*node.children[1]));
}

std::unique_ptr<BlockStmt> ConvertStmtList(const CstNode& node)
{
	std::vector<std::unique_ptr<Stmt>> stmts;
	CollectStmts(node, stmts);
	return std::make_unique<BlockStmt>(std::move(stmts), ExtractRange(node));
}

void CollectDecls(const CstNode& node, std::vector<const CstNode*>& decls)
{
	if (node.children.empty()) return;
	CollectDecls(*node.children[0], decls);
	decls.push_back(node.children[1].get());
}

Param ConvertParam(const CstNode& node)
{
	const auto [sign, typeName] = ParseType(*node.children[0]);
	const std::string name = node.children[1]->value;
	return Param{sign, typeName, name};
}

void CollectParams(const CstNode& node, std::vector<Param>& params)
{
	if (node.children.size() == 3)
	{
		CollectParams(*node.children[0], params);
		params.push_back(ConvertParam(*node.children[2]));
	}
	else
	{
		params.push_back(ConvertParam(*node.children[0]));
	}
}

std::unique_ptr<AstNode> ConvertFuncDecl(const CstNode& node)
{
	const auto [sign, typeName] = ParseType(*node.children[0]);
	const std::string name = node.children[2]->value;

	const bool hasParams = (node.children[4]->label != ")");
	const size_t stmtListIdx = hasParams ? 7 : 6;

	std::vector<Param> params;
	if (hasParams)
	{
		CollectParams(*node.children[4], params);
	}

	auto body = ConvertStmtList(*node.children[stmtListIdx]);
	return std::make_unique<FunctionDeclStmt>(
		sign, typeName, name, std::move(params), std::move(body), ExtractRange(node));
}

void CollectFieldList(const CstNode& node, std::vector<FieldDecl>& fields)
{
	if (node.children.empty())
	{
		return;
	}
	CollectFieldList(*node.children[0], fields);
	const CstNode& fieldNode = *node.children[1];
	const auto [sign, typeName] = ParseType(*fieldNode.children[0]);
	fields.push_back(FieldDecl{sign, typeName, fieldNode.children[1]->value});
}

void CollectEnumIdList(const CstNode& node, std::vector<std::string>& values)
{
	if (node.children.size() == 3)
	{
		CollectEnumIdList(*node.children[0], values);
		values.push_back(node.children[2]->value);
	}
	else
	{
		values.push_back(node.children[0]->value);
	}
}

std::unique_ptr<AstNode> ConvertStructDecl(const CstNode& node)
{
	const std::string name = node.children[1]->value;
	std::vector<FieldDecl> fields;
	CollectFieldList(*node.children[3], fields);
	return std::make_unique<StructDeclStmt>(name, std::move(fields), ExtractRange(node));
}

std::unique_ptr<AstNode> ConvertUnionDecl(const CstNode& node)
{
	const std::string name = node.children[1]->value;
	std::vector<FieldDecl> fields;
	CollectFieldList(*node.children[3], fields);
	return std::make_unique<UnionDeclStmt>(name, std::move(fields), ExtractRange(node));
}

std::unique_ptr<AstNode> ConvertEnumDecl(const CstNode& node)
{
	const std::string name = node.children[1]->value;
	std::vector<std::string> values;
	CollectEnumIdList(*node.children[3], values);
	return std::make_unique<EnumDeclStmt>(name, std::move(values), ExtractRange(node));
}

std::unique_ptr<AstNode> ConvertDecl(const CstNode& node)
{
	const auto& firstChild = *node.children[0];
	if (firstChild.label == "FuncDecl") return ConvertFuncDecl(firstChild);
	if (firstChild.label == "StructDecl") return ConvertStructDecl(firstChild);
	if (firstChild.label == "UnionDecl") return ConvertUnionDecl(firstChild);
	if (firstChild.label == "EnumDecl") return ConvertEnumDecl(firstChild);
	if (firstChild.label == "VarDecl") return ConvertVarDecl(firstChild);
	ThrowConversionError("Неподдерживаемый тип декларации", firstChild);
}
} // namespace

std::unique_ptr<AstNode> CstToAstConverter::Convert(const CstNode& root)
{
	const CstNode& programNode = *root.children[0];
	const CstNode& declListNode = *programNode.children[0];

	std::vector<const CstNode*> declNodes;
	CollectDecls(declListNode, declNodes);

	std::vector<std::unique_ptr<AstNode>> declarations;
	declarations.reserve(declNodes.size());

	for (const auto* decl : declNodes)
	{
		declarations.push_back(ConvertDecl(*decl));
	}

	return std::make_unique<ProgramNode>(std::move(declarations));
}
