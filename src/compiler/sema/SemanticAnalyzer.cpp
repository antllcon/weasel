#include "SemanticAnalyzer.h"

#include "src/compiler/ast/ArrayAllocExpr.h"
#include "src/compiler/ast/ArrayTypeInfo.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
#include "src/compiler/ast/DoWhileStmt.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/IndexExpr.h"
#include "src/compiler/ast/ListTypeInfo.h"
#include "src/compiler/ast/MemberAccessExpr.h"
#include "src/compiler/ast/NumExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/core/LanguageTokens.h"
#include "src/compiler/stdlib/NativeRegistry.h"
#include "src/diagnostics/CompilationException.h"

namespace
{
void AssertIsLhsValidForAssignment(bool isValid, const SourceRange& range)
{
	if (!isValid)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Левая часть присваивания должна быть переменной или элементом массива",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

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

void AssertIsBool(const std::shared_ptr<TypeInfo>& type, size_t line, size_t pos)
{
	if (!type)
		return;
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(type.get());
	if (!scalar || scalar->GetBaseType() != BaseType::Bool)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Условие должно быть типа bool, получен: " + type->GetName(),
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

void AssertIsNotVoidType(const std::shared_ptr<TypeInfo>& type, const std::string& name, size_t line, size_t pos)
{
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(type.get());
	if (scalar && scalar->IsVoid())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Переменная не может иметь тип void: " + name,
			.line = line,
			.pos = pos});
	}
}

void AssertIsFunctionExists(bool exists, const std::string& name, const SourceRange& range)
{
	if (!exists)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Вызов необъявленной функции: " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsExactTypeMatch(
	const std::shared_ptr<TypeInfo>& expected,
	const std::shared_ptr<TypeInfo>& actual,
	const SourceRange& range)
{
	if (!expected || !actual)
	{
		return;
	}

	if (expected->GetName() != actual->GetName())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Строгое несоответствие типов: ожидался " + expected->GetName() + ", получен " + actual->GetName(),
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsLiteralInBounds(
	bool isInBounds,
	const std::string& value,
	const std::string& typeName,
	const SourceRange& range)
{
	if (!isInBounds)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Значение литерала '" + value + "' выходит за границы типа " + typeName,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

std::shared_ptr<TypeInfo> ParseTypeString(const std::string& typeName)
{
	if (typeName.starts_with(LanguageTokens::KwArray) && typeName.find('[') != std::string::npos)
	{
		const size_t start = typeName.find('[') + 1;
		const size_t end = typeName.find_last_of(']');
		const std::string inner = typeName.substr(start, end - start);
		return std::make_shared<ArrayTypeInfo>(ParseTypeString(inner));
	}

	if (typeName.starts_with(LanguageTokens::KwList) && typeName.find('[') != std::string::npos)
	{
		const size_t start = typeName.find('[') + 1;
		const size_t end = typeName.find_last_of(']');
		const std::string inner = typeName.substr(start, end - start);
		return std::make_shared<ListTypeInfo>(ParseTypeString(inner));
	}

	return ScalarTypeInfo::FromString(typeName);
}

void AssertIsIntegerType(const std::shared_ptr<TypeInfo>& type, const std::string& contextMessage, const SourceRange& range)
{
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(type.get());
	if (!scalar || !scalar->IsInteger())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = contextMessage,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsIndexableType(const std::shared_ptr<TypeInfo>& type, const SourceRange& range)
{
	const bool isArray = dynamic_cast<const ArrayTypeInfo*>(type.get()) != nullptr;
	const bool isList = dynamic_cast<const ListTypeInfo*>(type.get()) != nullptr;

	if (!isArray && !isList)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Индексация применима только к массивам и спискам",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsMemberFound(bool isFound, const SourceRange& range)
{
	if (!isFound)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Поле или метод не найдены для данного типа",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

bool CheckIntegerBounds(const std::string& value, BaseType baseType)
{
	try
	{
		if (baseType == BaseType::Uint)
		{
			if (!value.empty() && value[0] == '-') return false;
			(void)std::stoull(value);
			return true;
		}
		(void)std::stoll(value);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool CheckFloatBounds(const std::string& value)
{
	try
	{
		(void)std::stod(value);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
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

bool IsPrimitiveType(const std::string& name)
{
	return name == LanguageTokens::TypeInt || name == LanguageTokens::TypeUint || name == LanguageTokens::TypeReal || name == LanguageTokens::TypeBool || name == LanguageTokens::TypeString || name == LanguageTokens::TypeVoid;
}
} // namespace

SemanticAnalyzer::SemaResult SemanticAnalyzer::Analyze(const AstNode& root, DiagnosticEngine& engine)
{
	m_engine = &engine;
	CollectFunctions(root);
	root.Accept(*this);
	return SemaResult{
		std::move(m_resolvedSymbols),
		std::move(m_varDeclSlots),
		std::move(m_resolvedIterators),
		std::move(m_functions)};
}

void SemanticAnalyzer::CollectFunctions(const AstNode& root)
{
	for (const auto& native : NativeRegistry::GetAll())
	{
		FunctionInfo info;
		info.returnType = native.returnType;
		info.params = native.params;
		m_functions[native.name] = std::move(info);
	}

	const auto* program = dynamic_cast<const ProgramNode*>(&root);
	if (!program)
	{
		return;
	}

	for (const auto& decl : program->GetDeclarations())
	{
		const auto* fn = dynamic_cast<const FunctionDeclStmt*>(decl.get());
		if (!fn)
		{
			continue;
		}

		FunctionInfo info;
		info.returnType = ParseTypeString(fn->GetReturnTypeName());

		for (const auto& param : fn->GetParams())
		{
			info.params.emplace_back(param.name, ParseTypeString(param.typeName));
		}

		m_functions[fn->GetName()] = std::move(info);
	}
}

void SemanticAnalyzer::Visit(const ArrayAllocExpr& node)
{
	node.GetSize().Accept(*this);
	AssertIsIntegerType(node.GetSize().GetResolvedType(), "Размер массива должен быть целым числом", node.GetSize().GetRange());

	auto elementType = ParseTypeString(node.GetElementTypeName());
	const_cast<ArrayAllocExpr&>(node).SetResolvedType(std::make_shared<ArrayTypeInfo>(elementType));
}

void SemanticAnalyzer::Visit(const IndexExpr& node)
{
	node.GetReceiver().Accept(*this);
	node.GetIndex().Accept(*this);

	auto receiverType = node.GetReceiver().GetResolvedType();
	AssertIsIndexableType(receiverType, node.GetRange());
	AssertIsIntegerType(node.GetIndex().GetResolvedType(), "Индекс должен быть целым числом", node.GetIndex().GetRange());

	if (const auto* arrayType = dynamic_cast<const ArrayTypeInfo*>(receiverType.get()))
	{
		const_cast<IndexExpr&>(node).SetResolvedType(arrayType->GetElementType());
	}
	else if (const auto* listType = dynamic_cast<const ListTypeInfo*>(receiverType.get()))
	{
		const_cast<IndexExpr&>(node).SetResolvedType(listType->GetElementType());
	}
}

void SemanticAnalyzer::Visit(const MemberAccessExpr& node)
{
	node.GetReceiver().Accept(*this);
	auto receiverType = node.GetReceiver().GetResolvedType();

	const bool isArray = dynamic_cast<const ArrayTypeInfo*>(receiverType.get()) != nullptr;
	const bool isList = dynamic_cast<const ListTypeInfo*>(receiverType.get()) != nullptr;

	if ((isArray || isList) && node.GetField() == "size")
	{
		const_cast<MemberAccessExpr&>(node).SetResolvedType(ScalarTypeInfo::Make(BaseType::Uint));
		return;
	}

	AssertIsMemberFound(false, node.GetRange());
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
	m_maxSlot = 0;
	m_currentReturnType = m_functions[node.GetName()].returnType;

	m_table.EnterScope();
	for (const auto& param : node.GetParams())
	{
		const uint32_t slot = m_nextSlot++;
		m_maxSlot = std::max(m_maxSlot, m_nextSlot);
		m_table.Declare(param.name, ParseTypeString(param.typeName), false, slot);
	}
	node.GetBody().Accept(*this);
	m_table.LeaveScope();

	m_functions[node.GetName()].maxSlots = m_maxSlot;
	m_currentReturnType = nullptr;
}

void SemanticAnalyzer::Visit(const BlockStmt& node)
{
	m_table.EnterScope();
	const uint32_t savedSlot = m_nextSlot;
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
	m_nextSlot = savedSlot;
	m_table.LeaveScope();
}

void SemanticAnalyzer::Visit(const VarDeclStmt& node)
{
	if (node.GetTypeName().empty())
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Явное указание типа переменной обязательно: " + node.GetName(),
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
		return;
	}

	auto type = ParseTypeString(node.GetTypeName());
	AssertIsNotVoidType(type, node.GetName(), node.GetRange().start.line, node.GetRange().start.pos);

	if (node.GetInit())
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = type;
		node.GetInit()->Accept(*this);
		m_expectedType = savedExpected;

		AssertIsExactTypeMatch(type, node.GetInit()->GetResolvedType(), node.GetRange());
	}

	const uint32_t slot = m_nextSlot++;
	m_maxSlot = std::max(m_maxSlot, m_nextSlot);
	const bool isMutable = node.GetModifier() == VarModifier::Var;

	const bool isNew = m_table.Declare(node.GetName(), type, isMutable, slot);
	AssertIsVariableNotRedeclared(isNew, node.GetName());
	m_varDeclSlots[&node] = slot;
}

void SemanticAnalyzer::Visit(const AssignStmt& node)
{
	const bool isIdentifier = dynamic_cast<const IdentifierExpr*>(&node.GetLhs()) != nullptr;
	const bool isIndexExpr = dynamic_cast<const IndexExpr*>(&node.GetLhs()) != nullptr;
	AssertIsLhsValidForAssignment(isIdentifier || isIndexExpr, node.GetRange());

	node.GetLhs().Accept(*this);

	const auto savedExpected = m_expectedType;
	m_expectedType = node.GetLhs().GetResolvedType();
	node.GetRhs().Accept(*this);
	m_expectedType = savedExpected;

	if (isIdentifier)
	{
		const auto& ident = static_cast<const IdentifierExpr&>(node.GetLhs());
		auto info = m_table.Resolve(ident.GetName());
		if (info)
		{
			AssertIsMutable(*info, ident.GetName());
		}
	}

	AssertIsExactTypeMatch(
		node.GetLhs().GetResolvedType(),
		node.GetRhs().GetResolvedType(),
		node.GetRange());
}

void SemanticAnalyzer::Visit(const IfStmt& node)
{
	node.GetCondition().Accept(*this);
	AssertIsBool(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);

	node.GetThenBlock().Accept(*this);

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
		elseBlock->Accept(*this);
	}
}

void SemanticAnalyzer::Visit(const RepStmt& node)
{
	m_table.EnterScope();
	const uint32_t savedSlot = m_nextSlot;

	std::vector<SymbolInfo> iterInfos;
	for (const auto& iterName : node.GetIterators())
	{
		const uint32_t slot = m_nextSlot++;
		m_maxSlot = std::max(m_maxSlot, m_nextSlot);
		const bool isNew = m_table.Declare(iterName, nullptr, true, slot);
		AssertIsVariableNotRedeclared(isNew, iterName);
		iterInfos.push_back(SymbolInfo{nullptr, slot, true});
	}
	m_resolvedIterators[&node] = std::move(iterInfos);

	node.GetOriginalBody().Accept(*this);

	m_nextSlot = savedSlot;
	m_table.LeaveScope();
}

void SemanticAnalyzer::Visit(const RunStmt& node)
{
	node.GetCondition().Accept(*this);
	AssertIsBool(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);
	node.GetBody().Accept(*this);
}

void SemanticAnalyzer::Visit(const DoWhileStmt& node)
{
	node.GetBody().Accept(*this);
	node.GetCondition().Accept(*this);
	AssertIsBool(
		node.GetCondition().GetResolvedType(),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);
}

void SemanticAnalyzer::Visit(const ReturnStmt& node)
{
	if (!node.GetValue())
	{
		return;
	}

	const auto savedExpected = m_expectedType;
	m_expectedType = m_currentReturnType;
	node.GetValue()->Accept(*this);
	m_expectedType = savedExpected;

	AssertIsExactTypeMatch(m_currentReturnType, node.GetValue()->GetResolvedType(), node.GetRange());
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
	m_resolvedSymbols[&node] = *result;
}

void SemanticAnalyzer::Visit(const BinaryExpr& node)
{
	node.GetLeft().Accept(*this);
	node.GetRight().Accept(*this);

	const auto leftType = node.GetLeft().GetResolvedType();
	const auto rightType = node.GetRight().GetResolvedType();

	AssertIsExactTypeMatch(leftType, rightType, node.GetRange());

	const auto resultType = IsComparisonOp(node.GetOp()) || IsLogicalOp(node.GetOp())
		? ScalarTypeInfo::Make(BaseType::Bool)
		: leftType;

	const_cast<BinaryExpr&>(node).SetResolvedType(resultType);
}

void SemanticAnalyzer::Visit(const UnaryExpr& node)
{
	node.GetOperand().Accept(*this);
	const_cast<UnaryExpr&>(node).SetResolvedType(node.GetOperand().GetResolvedType());
}

void SemanticAnalyzer::Visit(const FunctionCallExpr& node)
{
	if (node.GetName() == "print")
	{
		for (const auto& arg : node.GetArgs())
		{
			arg->Accept(*this);
		}
		const_cast<FunctionCallExpr&>(node).SetResolvedType(ScalarTypeInfo::Make(BaseType::Void));
		return;
	}

	if (IsPrimitiveType(node.GetName()) && node.GetArgs().size() == 1)
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = nullptr;
		node.GetArgs()[0]->Accept(*this);
		m_expectedType = savedExpected;
		auto targetType = ParseTypeString(node.GetName());
		const_cast<FunctionCallExpr&>(node).SetResolvedType(targetType);
		return;
	}

	for (const auto& arg : node.GetArgs())
	{
		arg->Accept(*this);
	}

	AssertIsFunctionExists(m_functions.contains(node.GetName()), node.GetName(), node.GetRange());

	const auto& fn = m_functions[node.GetName()];
	AssertIsArgCountMatch(fn.params.size(), node.GetArgs().size(), node.GetName());

	for (size_t i = 0; i < fn.params.size(); ++i)
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = fn.params[i].second;

		node.GetArgs()[i]->Accept(*this);

		m_expectedType = savedExpected;

		AssertIsExactTypeMatch(
			fn.params[i].second,
			node.GetArgs()[i]->GetResolvedType(),
			node.GetArgs()[i]->GetRange());
	}

	const_cast<FunctionCallExpr&>(node).SetResolvedType(fn.returnType);
}

void SemanticAnalyzer::Visit(const NumExpr& node)
{
	if (m_expectedType)
	{
		if (const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(m_expectedType.get()))
		{
			if (scalar->IsInteger() && !node.IsFloat())
			{
				const bool isInBounds = CheckIntegerBounds(node.GetValue(), scalar->GetBaseType());
				AssertIsLiteralInBounds(isInBounds, node.GetValue(), scalar->GetName(), node.GetRange());

				const_cast<NumExpr&>(node).SetResolvedType(m_expectedType);
				return;
			}

			if (scalar->IsFloat() && node.IsFloat())
			{
				const bool isInBounds = CheckFloatBounds(node.GetValue());
				AssertIsLiteralInBounds(isInBounds, node.GetValue(), scalar->GetName(), node.GetRange());

				const_cast<NumExpr&>(node).SetResolvedType(m_expectedType);
				return;
			}
		}
	}

	const auto base = node.IsFloat() ? BaseType::Real : BaseType::Int;
	auto type = ScalarTypeInfo::Make(base);
	const_cast<NumExpr&>(node).SetResolvedType(type);
}

void SemanticAnalyzer::Visit(const StringExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::String);
	const_cast<StringExpr&>(node).SetResolvedType(type);
}

void SemanticAnalyzer::Visit(const BoolExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::Bool);
	const_cast<BoolExpr&>(node).SetResolvedType(type);
}

void SemanticAnalyzer::Visit(const ArrayLiteralExpr& /*node*/)
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