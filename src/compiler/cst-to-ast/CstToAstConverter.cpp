#include "CstToAstConverter.h"

#include "src/compiler/ast/ArrayAllocExpr.h"
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
#include "src/compiler/ast/NumExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/ClassicForStmt.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/StructDeclStmt.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/UnionDeclStmt.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/ast/WhenEntry.h"
#include "src/compiler/ast/WhenStmt.h"
#include "src/compiler/core/LanguageTokens.h"
#include "src/compiler/lexer/Token.h"
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
std::unique_ptr<Stmt> ConvertStmt(const CstNode& node);

BinaryOpKind ParseBinaryOp(const CstNode& opNode)
{
	const std::string& op = opNode.value;

	if (op == LanguageTokens::OpPlus) return BinaryOpKind::Add;
	if (op == LanguageTokens::OpMinus) return BinaryOpKind::Sub;
	if (op == LanguageTokens::OpMul) return BinaryOpKind::Mul;
	if (op == LanguageTokens::OpDiv) return BinaryOpKind::Div;
	if (op == LanguageTokens::OpMod) return BinaryOpKind::Mod;
	if (op == LanguageTokens::OpEq) return BinaryOpKind::Eq;
	if (op == LanguageTokens::OpNotEq) return BinaryOpKind::NotEq;
	if (op == LanguageTokens::OpLess) return BinaryOpKind::Less;
	if (op == LanguageTokens::OpGreater) return BinaryOpKind::Greater;
	if (op == LanguageTokens::OpLessEq) return BinaryOpKind::LessEq;
	if (op == LanguageTokens::OpGreaterEq) return BinaryOpKind::GreaterEq;
	if (op == LanguageTokens::KwAnd) return BinaryOpKind::LogicalAnd;
	if (op == LanguageTokens::KwOr) return BinaryOpKind::LogicalOr;

	ThrowConversionError("Неизвестный бинарный оператор: " + op, opNode);
}

UnaryOpKind ParseUnaryOp(const CstNode& opNode)
{
	const std::string& op = opNode.value;

	if (op == LanguageTokens::KwNot) return UnaryOpKind::LogicalNot;
	if (op == LanguageTokens::OpAddressOf) return UnaryOpKind::AddressOf;
	if (op == LanguageTokens::OpMul) return UnaryOpKind::Deref;
	if (op == LanguageTokens::OpMinus) return UnaryOpKind::Minus;

	ThrowConversionError("Неизвестный унарный оператор: " + op, opNode);
}

VarModifier ParseModifier(const CstNode& modNode)
{
	const std::string& val = modNode.children[0]->value;

	if (val == LanguageTokens::KwVal) return VarModifier::Val;
	if (val == LanguageTokens::KwVar) return VarModifier::Var;
	if (val == LanguageTokens::KwLet) return VarModifier::Def;

	ThrowConversionError("Неизвестный модификатор переменной: " + val, modNode);
}

std::string ParseType(const CstNode& typeNode)
{
	const auto& child = *typeNode.children[0];

	if (child.label == "BaseType")
	{
		return child.children[0]->value;
	}

	if (child.label == LanguageTokens::KwArray)
	{
		return std::string(LanguageTokens::KwArray) + std::string(LanguageTokens::SymBracketLeft) + ParseType(*typeNode.children[2]) + std::string(LanguageTokens::SymBracketRight);
	}

	return child.value;
}

std::string ProcessEscapeSequences(const std::string& input)
{
	std::string result;
	result.reserve(input.size());

	for (size_t i = 0; i < input.size(); ++i)
	{
		if (input[i] == '\\' && i + 1 < input.size())
		{
			const char next = input[++i];
			switch (next)
			{
			case 'n':
				result += '\n';
				break;
			case 't':
				result += '\t';
				break;
			case 'r':
				result += '\r';
				break;
			case '\\':
				result += '\\';
				break;
			case '"':
				result += '"';
				break;
			default:
				result += '\\';
				result += next;
				break;
			}
		}
		else
		{
			result += input[i];
		}
	}

	return result;
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
		return std::make_unique<NumExpr>(child.value, isFloat, range);
	}
	if (child.label == "str")
	{
		return std::make_unique<StringExpr>(ProcessEscapeSequences(child.value), range);
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

	if (child.label == LanguageTokens::KwArray)
	{
		std::string elementTypeName = ParseType(*node.children[2]);
		auto sizeExpr = ConvertExprByLabel(*node.children[5]);
		return std::make_unique<ArrayAllocExpr>(std::move(elementTypeName), std::move(sizeExpr), range);
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

	if (childCount >= 3 && node.children[1]->value == ".")
	{
		const CstNode& methodNode = *node.children[2];
		const std::string methodName = methodNode.children.empty()
			? methodNode.value
			: methodNode.children[0]->value;

		if (childCount == 3)
		{
			return std::make_unique<MemberAccessExpr>(std::move(receiver), methodName, range);
		}

		std::vector<std::unique_ptr<Expr>> args;
		args.push_back(std::move(receiver));

		if (childCount == 6)
		{
			CollectArgList(*node.children[4], args);
		}

		return std::make_unique<FunctionCallExpr>(methodName, std::move(args), range);
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

	const UnaryOpKind op = ParseUnaryOp(*node.children[0]);
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
		const BinaryOpKind op = ParseBinaryOp(*node.children[1]);
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

void AssertIsWhenValid(bool isValid, const CstNode& node)
{
	if (!isValid)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.message = "Некорректная структура оператора when",
			.line = node.location.line,
			.pos = node.location.pos});
	}
}

void CollectWhenConditions(const CstNode& node, std::vector<std::unique_ptr<Expr>>& conditions)
{
	if (node.children.size() == 3 && node.children[1]->value == LanguageTokens::SymComma)
	{
		CollectWhenConditions(*node.children[0], conditions);
		conditions.push_back(ConvertExpr(*node.children[2]));
	}
	else
	{
		conditions.push_back(ConvertExpr(*node.children[0]));
	}
}

void CollectWhenEntries(const CstNode& node, std::vector<WhenEntry>& entries)
{
	if (node.children.empty())
	{
		return;
	}

	if (node.children.size() == 2)
	{
		CollectWhenEntries(*node.children[0], entries);

		const CstNode& entryNode = *node.children[1];
		std::vector<std::unique_ptr<Expr>> conditions;
		CollectWhenConditions(*entryNode.children[0], conditions);

		entries.push_back(WhenEntry{
			std::move(conditions),
			ConvertStmt(*entryNode.children[2])});
	}
}

std::unique_ptr<Stmt> ConvertWhenStmt(const CstNode& node)
{
	std::unique_ptr<Expr> subject = nullptr;
	std::vector<WhenEntry> entries;
	std::unique_ptr<Stmt> elseBody = nullptr;

	size_t bracesIndex = 1;

	if (node.children[1]->value == LanguageTokens::SymParenLeft)
	{
		subject = ConvertExpr(*node.children[2]);
		bracesIndex = 4;
	}

	AssertIsWhenValid(node.children[bracesIndex]->value == LanguageTokens::SymBraceLeft, node);

	CollectWhenEntries(*node.children[bracesIndex + 1], entries);

	const size_t elseIndex = bracesIndex + 2;
	if (elseIndex < node.children.size())
	{
		const CstNode& elseNode = *node.children[elseIndex];
		if (!elseNode.children.empty())
		{
			elseBody = ConvertStmt(*elseNode.children[2]);
		}
	}

	return std::make_unique<WhenStmt>(
		std::move(subject),
		std::move(entries),
		std::move(elseBody),
		ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertVarDecl(const CstNode& node)
{
	const VarModifier modifier = ParseModifier(*node.children[0]);
	const std::string typeName = ParseType(*node.children[1]);
	const std::string name = node.children[2]->value;
	std::unique_ptr<Expr> init = nullptr;
	bool isMoveInit = false;

	if (node.children.size() == 5)
	{
		const std::string& opVal = node.children[3]->children[0]->value;
		isMoveInit = opVal == LanguageTokens::OpMove;
		init = ConvertExpr(*node.children[4]);
	}

	return std::make_unique<VarDeclStmt>(
		modifier, typeName, name, isMoveInit, std::move(init), ExtractRange(node));
}

std::unique_ptr<Stmt> ConvertAssignStmt(const CstNode& node)
{
	auto lhs = ConvertExpr(*node.children[0]);
	const std::string& opVal = node.children[1]->children[0]->value;
	const bool isMove = opVal == LanguageTokens::OpMove;
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

std::unique_ptr<Stmt> ConvertClassicForStmt(const CstNode& node)
{
	// repeat ( BaseType id = Expr ; Expr ; AssignStmt ) { StmtList }
	// [0]=repeat [1]=( [2]=BaseType [3]=id [4]== [5]=Expr [6]=; [7]=Expr [8]=; [9]=AssignStmt [10]=) [11]={ [12]=StmtList [13]=}
	const std::string initType = node.children[2]->children[0]->value;
	const std::string initName = node.children[3]->value;
	auto initExpr = ConvertExpr(*node.children[5]);
	auto condition = ConvertExpr(*node.children[7]);
	auto step = ConvertAssignStmt(*node.children[9]);
	auto body = ConvertStmtList(*node.children[12]);

	return std::make_unique<ClassicForStmt>(
		initType, initName, std::move(initExpr), std::move(condition), std::move(step), std::move(body), ExtractRange(node));
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
	if (node.children.size() == 14)
	{
		return ConvertClassicForStmt(node);
	}

	ThrowConversionError("Неподдерживаемая форма repeat с " + std::to_string(node.children.size()) + " дочерними узлами", node);
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

	if (node.children[1]->value == LanguageTokens::SymParenLeft)
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
	if (firstChild.label == "{") return ConvertStmtList(*node.children[1]);
	if (firstChild.label == "VarDecl") return ConvertVarDecl(firstChild);
	if (firstChild.label == "AssignStmt") return ConvertAssignStmt(firstChild);
	if (firstChild.label == "IfStmt") return ConvertIfStmt(firstChild);
	if (firstChild.label == "ForStmt") return ConvertForStmt(firstChild);
	if (firstChild.label == "WhileStmt") return ConvertWhileStmt(firstChild);
	if (firstChild.label == "DoWhileStmt") return ConvertDoWhileStmt(firstChild);
	if (firstChild.label == "ReturnStmt") return ConvertReturnStmt(firstChild);
	if (firstChild.label == "WhenStmt") return ConvertWhenStmt(firstChild);
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
	const std::string typeName = ParseType(*node.children[0]);
	const std::string name = node.children[1]->value;
	return Param{typeName, name};
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
	const std::string typeName = ParseType(*node.children[0]);
	const std::string name = node.children[2]->value;
	const bool hasParams = node.children[4]->label != LanguageTokens::SymParenRight;
	const size_t stmtListIdx = hasParams ? 7 : 6;

	std::vector<Param> params;
	if (hasParams)
	{
		CollectParams(*node.children[4], params);
	}

	auto body = ConvertStmtList(*node.children[stmtListIdx]);
	return std::make_unique<FunctionDeclStmt>(
		typeName, name, std::move(params), std::move(body), ExtractRange(node));
}

void CollectFieldList(const CstNode& node, std::vector<FieldDecl>& fields)
{
	if (node.children.empty())
	{
		return;
	}
	CollectFieldList(*node.children[0], fields);
	const CstNode& fieldNode = *node.children[1];
	const std::string typeName = ParseType(*fieldNode.children[0]);
	fields.push_back(FieldDecl{typeName, fieldNode.children[1]->value});
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
