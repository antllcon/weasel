#include "CstToAstConverter.h"

#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/NumberExpr.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/VarDeclStmt.h"

#include <stdexcept>

namespace
{
void AssertIsNodeNotNull(const CstNode* node, const std::string& context)
{
	if (!node)
	{
		throw std::runtime_error("Нулевой узел CST в контексте: " + context);
	}
}

void AssertIsChildrenCount(const CstNode& node, size_t expected, const std::string& context)
{
	if (node.children.size() != expected)
	{
		throw std::runtime_error(
			"Неожиданное количество дочерних узлов в " + context
			+ ": ожидалось " + std::to_string(expected)
			+ ", получено " + std::to_string(node.children.size()));
	}
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
	throw std::runtime_error("Неизвестный бинарный оператор: " + op);
}

VarModifier ParseModifier(const CstNode& modNode)
{
	const std::string& val = modNode.children[0]->value;
	if (val == "val") return VarModifier::Val;
	if (val == "var") return VarModifier::Var;
	if (val == "def") return VarModifier::Def;
	throw std::runtime_error("Неизвестный модификатор переменной: " + val);
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

std::unique_ptr<Expr> ConvertPrimaryExpr(const CstNode& node)
{
	const auto& child = *node.children[0];

	if (child.label == "id" && node.children.size() == 3)
	{
		return std::make_unique<FunctionCallExpr>(
			child.value, std::vector<std::unique_ptr<Expr>>{}, SourceRange{});
	}
	if (child.label == "id" && node.children.size() == 4)
	{
		std::vector<std::unique_ptr<Expr>> args;
		CollectArgList(*node.children[2], args);
		return std::make_unique<FunctionCallExpr>(child.value, std::move(args), SourceRange{});
	}
	if (child.label == "id")
	{
		return std::make_unique<IdentifierExpr>(child.value, SourceRange{});
	}
	if (child.label == "num")
	{
		const bool isFloat = child.value.find('.') != std::string::npos;
		return std::make_unique<NumberExpr>(child.value, isFloat, SourceRange{});
	}
	if (child.label == "(")
	{
		return ConvertExprByLabel(*node.children[1]);
	}
	throw std::runtime_error("Неподдерживаемое первичное выражение: " + child.label);
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

std::unique_ptr<Expr> ConvertExprByLabel(const CstNode& node)
{
	if (node.label == "PrimaryExpr")
	{
		return ConvertPrimaryExpr(node);
	}

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

	throw std::runtime_error("Неподдерживаемый узел выражения: " + node.label
		+ " с " + std::to_string(node.children.size()) + " дочерними узлами");
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
		modifier, sign, typeName, name, isMoveInit, std::move(init), SourceRange{});
}

std::unique_ptr<Stmt> ConvertAssignStmt(const CstNode& node)
{
	auto lhs = ConvertExpr(*node.children[0]);
	const std::string& opVal = node.children[1]->children[0]->value;
	const bool isMove = (opVal == "<-");
	auto rhs = ConvertExpr(*node.children[2]);
	return std::make_unique<AssignStmt>(std::move(lhs), isMove, std::move(rhs), SourceRange{});
}

std::unique_ptr<Stmt> ConvertReturnStmt(const CstNode& node)
{
	std::unique_ptr<Expr> value = nullptr;
	if (node.children.size() == 2)
	{
		value = ConvertExpr(*node.children[1]);
	}
	return std::make_unique<ReturnStmt>(std::move(value), SourceRange{});
}

std::unique_ptr<Stmt> ConvertForStmt(const CstNode& node)
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
		std::move(iterators), std::move(ranges), std::move(body), SourceRange{});
}

std::unique_ptr<Stmt> ConvertWhileStmt(const CstNode& node)
{
	auto condition = ConvertExpr(*node.children[2]);
	auto body = ConvertStmtList(*node.children[5]);
	return std::make_unique<RunStmt>(std::move(condition), std::move(body), SourceRange{});
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
			std::move(condition), std::move(thenBlock), std::move(nested), SourceRange{});
	}

	return ConvertStmtList(*node.children[2]);
}

std::unique_ptr<Stmt> ConvertIfStmt(const CstNode& node)
{
	auto condition = ConvertExpr(*node.children[2]);
	auto thenBlock = ConvertStmtList(*node.children[5]);
	auto elseNode = ConvertElseOpt(*node.children[7]);
	return std::make_unique<IfStmt>(
		std::move(condition), std::move(thenBlock), std::move(elseNode), SourceRange{});
}

std::unique_ptr<Stmt> ConvertStmt(const CstNode& node)
{
	const auto& firstChild = *node.children[0];
	if (firstChild.label == "VarDecl") return ConvertVarDecl(firstChild);
	if (firstChild.label == "AssignStmt") return ConvertAssignStmt(firstChild);
	if (firstChild.label == "IfStmt") return ConvertIfStmt(firstChild);
	if (firstChild.label == "ForStmt") return ConvertForStmt(firstChild);
	if (firstChild.label == "WhileStmt") return ConvertWhileStmt(firstChild);
	if (firstChild.label == "ReturnStmt") return ConvertReturnStmt(firstChild);
	if (firstChild.label == "ExprStmt")
	{
		auto expr = ConvertExpr(*firstChild.children[0]);
		return std::make_unique<ExprStmt>(std::move(expr), SourceRange{});
	}
	throw std::runtime_error("Неизвестный тип оператора: " + firstChild.label);
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
	return std::make_unique<BlockStmt>(std::move(stmts), SourceRange{});
}

void CollectDecls(const CstNode& node, std::vector<const CstNode*>& decls)
{
	if (node.children.empty()) return;
	CollectDecls(*node.children[0], decls);
	decls.push_back(node.children[1].get());
}

std::unique_ptr<AstNode> ConvertFuncDecl(const CstNode& node)
{
	const auto [sign, typeName] = ParseType(*node.children[0]);
	const std::string name = node.children[2]->value;

	const bool hasParams = (node.children[4]->label != ")");
	const size_t stmtListIdx = hasParams ? 7 : 6;

	auto body = ConvertStmtList(*node.children[stmtListIdx]);
	return std::make_unique<FunctionDeclStmt>(sign, typeName, name, std::move(body), SourceRange{});
}
} // namespace

std::unique_ptr<AstNode> CstToAstConverter::Convert(const CstNode& root)
{
	const CstNode& programNode = *root.children[0];
	const CstNode& declListNode = *programNode.children[0];

	std::vector<const CstNode*> decls;
	CollectDecls(declListNode, decls);

	for (const auto* decl : decls)
	{
		const auto& firstChild = *decl->children[0];
		if (firstChild.label == "FuncDecl")
		{
			return ConvertFuncDecl(firstChild);
		}
	}

	throw std::runtime_error("Функция не найдена в программе");
}
