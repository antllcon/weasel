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
#include "src/compiler/ast/TypeInfo.h"
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

void AssertIsLhsIdentifier(const Expr& lhs)
{
	if (!dynamic_cast<const IdentifierExpr*>(&lhs))
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Левая часть присваивания должна быть идентификатором"});
	}
}

void AssertIsMutable(const SymbolInfo& info, const std::string& name)
{
	if (!info.isMutable)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Нельзя присвоить значение неизменяемой переменной: " + name});
	}
}

void AssertIsTypesMatch(
	const std::shared_ptr<TypeInfo>& expected,
	const std::shared_ptr<TypeInfo>& actual,
	const std::string& context)
{
	if (!expected || !actual)
		return;
	if (expected->GetName() != actual->GetName())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Несовместимые типы в " + context
			           + ": ожидался " + expected->GetName()
			           + ", получен " + actual->GetName()});
	}
}

void AssertIsBoolen(const std::shared_ptr<TypeInfo>& type, size_t line, size_t pos)
{
	if (!type)
		return;
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(type.get());
	if (!scalar || scalar->GetBaseType() != BaseType::Boolen)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Условие должно быть типа boolen, получен: " + type->GetName(),
			.line = line,
			.pos = pos});
	}
}

void AssertIsArgCountMatch(size_t expected, size_t actual, const std::string& name)
{
	if (expected != actual)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Неверное количество аргументов при вызове функции " + name
			           + ": ожидалось " + std::to_string(expected)
			           + ", получено " + std::to_string(actual)});
	}
}

void AssertIsFunctionExists(bool exists, const std::string& name)
{
	if (!exists)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Вызов необъявленной функции: " + name});
	}
}

bool IsComparisonOp(BinaryOpKind op)
{
	return op == BinaryOpKind::Eq || op == BinaryOpKind::NotEq
	    || op == BinaryOpKind::Less || op == BinaryOpKind::Greater
	    || op == BinaryOpKind::LessEq || op == BinaryOpKind::GreaterEq;
}

bool IsLogicalOp(BinaryOpKind op)
{
	return op == BinaryOpKind::LogicalAnd || op == BinaryOpKind::LogicalOr;
}
} // namespace

SemanticAnalyzer::SemaResult SemanticAnalyzer::Analyze(AstNode& root, DiagnosticEngine& engine)
{
	m_engine = &engine;
	CollectFunctions(root);
	root.Accept(*this);
	return SemaResult{std::move(m_resolvedSymbols), std::move(m_functions)};
}

void SemanticAnalyzer::CollectFunctions(const AstNode& root)
{
	FunctionInfo printInfo;
	printInfo.returnType = ScalarTypeInfo::Make(BaseType::Voided);
	printInfo.params.emplace_back("value", ScalarTypeInfo::Make(BaseType::Number));
	m_functions["print"] = std::move(printInfo);

	const auto* program = dynamic_cast<const ProgramNode*>(&root);
	if (!program)
		return;

	for (const auto& decl : program->GetDeclarations())
	{
		const auto* fn = dynamic_cast<const FunctionDeclStmt*>(decl.get());
		if (!fn)
			continue;

		FunctionInfo info;
		info.returnType = ScalarTypeInfo::FromStrings(
			fn->GetReturnTypeSign(), fn->GetReturnTypeName());

		for (const auto& param : fn->GetParams())
		{
			auto paramType = ScalarTypeInfo::FromStrings(param.typeSign, param.typeName);
			info.params.emplace_back(param.name, std::move(paramType));
		}

		m_functions[fn->GetName()] = std::move(info);
	}
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
	m_currentReturnType = m_functions[node.GetName()].returnType;

	m_table.EnterScope();
	for (const auto& param : node.GetParams())
	{
		const uint32_t slot = m_nextSlot++;
		auto paramType = ScalarTypeInfo::FromStrings(param.typeSign, param.typeName);
		m_table.Declare(param.name, paramType, false, slot);
		m_resolvedSymbols[param.name] = SymbolInfo{std::move(paramType), slot, false};
	}
	node.GetBody().Accept(*this);
	m_table.LeaveScope();

	m_currentReturnType = nullptr;
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
	if (node.GetInit())
	{
		node.GetInit()->Accept(*this);
	}

	const uint32_t slot = m_nextSlot++;
	const bool isMutable = node.GetModifier() == VarModifier::Var;
	auto type = ScalarTypeInfo::FromStrings(node.GetTypeSign(), node.GetTypeName());

	const bool isNew = m_table.Declare(node.GetName(), type, isMutable, slot);
	AssertIsVariableNotRedeclared(isNew, node.GetName());
	m_resolvedSymbols[node.GetName()] = SymbolInfo{type, slot, isMutable};
}

void SemanticAnalyzer::Visit(const AssignStmt& node)
{
	AssertIsLhsIdentifier(node.GetLhs());

	node.GetLhs().Accept(*this);
	node.GetRhs().Accept(*this);

	const auto& ident = static_cast<const IdentifierExpr&>(node.GetLhs());
	auto info = m_table.Resolve(ident.GetName());
	if (!info)
		return;

	AssertIsMutable(*info, ident.GetName());
	AssertIsTypesMatch(
		node.GetLhs().GetResolvedType(),
		node.GetRhs().GetResolvedType(),
		"присваивании переменной " + ident.GetName());
}

void SemanticAnalyzer::Visit(const IfStmt& node)
{
	node.GetCondition().Accept(*this);
	AssertIsBoolen(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);

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
	AssertIsBoolen(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);
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
	AssertIsBoolen(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);
}

void SemanticAnalyzer::Visit(const ReturnStmt& node)
{
	if (!node.GetValue())
		return;

	node.GetValue()->Accept(*this);
	AssertIsTypesMatch(
		m_currentReturnType,
		node.GetValue()->GetResolvedType(),
		"возвращаемом значении функции");
}

void SemanticAnalyzer::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
}

void SemanticAnalyzer::Visit(const IdentifierExpr& node)
{
	auto result = m_table.Resolve(node.GetName());
	if (!result)
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Использование необъявленной переменной: " + node.GetName(),
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
		return;
	}
	const_cast<IdentifierExpr&>(node).SetResolvedType(result->type);
}

void SemanticAnalyzer::Visit(const BinaryExpr& node)
{
	node.GetLeft().Accept(*this);
	node.GetRight().Accept(*this);

	auto leftType = node.GetLeft().GetResolvedType();
	auto rightType = node.GetRight().GetResolvedType();

	if (leftType && rightType && leftType->GetName() != rightType->GetName())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Несовместимые типы операндов: "
			           + leftType->GetName() + " и " + rightType->GetName()});
	}

	std::shared_ptr<TypeInfo> resultType;
	if (IsComparisonOp(node.GetOp()) || IsLogicalOp(node.GetOp()))
	{
		resultType = ScalarTypeInfo::Make(BaseType::Boolen);
	}
	else
	{
		resultType = leftType;
	}

	const_cast<BinaryExpr&>(node).SetResolvedType(resultType);
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

	AssertIsFunctionExists(m_functions.contains(node.GetName()), node.GetName());

	const auto& fn = m_functions[node.GetName()];
	AssertIsArgCountMatch(fn.params.size(), node.GetArgs().size(), node.GetName());

	for (size_t i = 0; i < fn.params.size(); ++i)
	{
		AssertIsTypesMatch(
			fn.params[i].second,
			node.GetArgs()[i]->GetResolvedType(),
			"аргументе " + fn.params[i].first + " функции " + node.GetName());
	}

	const_cast<FunctionCallExpr&>(node).SetResolvedType(fn.returnType);
}

void SemanticAnalyzer::Visit(const NumberExpr& node)
{
	auto base = node.IsFloat() ? BaseType::Double : BaseType::Number;
	auto type = ScalarTypeInfo::Make(base);
	const_cast<NumberExpr&>(node).SetResolvedType(type);
}

void SemanticAnalyzer::Visit(const StringExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::String);
	const_cast<StringExpr&>(node).SetResolvedType(type);
}

void SemanticAnalyzer::Visit(const BoolExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::Boolen);
	const_cast<BoolExpr&>(node).SetResolvedType(type);
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