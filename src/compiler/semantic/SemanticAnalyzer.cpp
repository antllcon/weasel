#include "SemanticAnalyzer.h"

#include "src/compiler/ast/ArrayAllocExpr.h"
#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/ArrayTypeInfo.h"
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
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/ast/WhenStmt.h"
#include "src/compiler/core/LanguageTokens.h"
#include "src/compiler/stdlib/NativeRegistry.h"
#include "src/diagnostics/CompilationException.h"

#include <unordered_set>

namespace
{
void AssertIsArrayTypeKnown(bool isKnown, const SourceRange& range)
{
	if (!isKnown)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Невозможно вывести тип пустого массива без контекста",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsArrayElementTypesMatch(bool isMatch, const std::string& expected, const std::string& actual, const SourceRange& range)
{
	if (!isMatch)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Типы элементов массива должны совпадать. Ожидался: " + expected + ", получен: " + actual,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsInitializedIfImmutable(bool hasInit, VarModifier modifier, const std::string& name, const SourceRange& range)
{
	if (!hasInit && modifier != VarModifier::Var)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Неизменяемая переменная или константа должна быть инициализирована: " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsCompileTimeConstant(const Expr* expr, const SourceRange& range)
{
	const bool isNum = dynamic_cast<const NumExpr*>(expr) != nullptr;
	const bool isBool = dynamic_cast<const BoolExpr*>(expr) != nullptr;
	const bool isString = dynamic_cast<const StringExpr*>(expr) != nullptr;

	if (!isNum && !isBool && !isString)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Значение строгой константы должно быть известно на этапе компиляции",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

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

void AssertIsMutable(const SymbolInfo& info, const std::string& name, const SourceRange& range)
{
	if (!info.isMutable)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Нельзя присвоить значение неизменяемой переменной: " + name,
			.line = range.start.line,
			.pos = range.start.pos});
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

	if (!isArray)
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

CodegenContext SemanticAnalyzer::Analyze(const AstNode& root, DiagnosticEngine& engine)
{
	m_engine = &engine;
	CollectTypes(root);
	CollectFunctions(root);
	root.Accept(*this);

	return CodegenContext{
		std::move(m_annotations),
		std::move(m_resolvedSymbols),
		std::move(m_varDeclSlots),
		std::move(m_resolvedIterators),
		std::move(m_functions)
	};
}

void SemanticAnalyzer::CollectTypes(const AstNode& root)
{
	const auto* program = dynamic_cast<const ProgramNode*>(&root);
	if (!program)
	{
		return;
	}

	for (const auto& decl : program->GetDeclarations())
	{
		if (const auto* enumDecl = dynamic_cast<const EnumDeclStmt*>(decl.get()))
		{
			auto typeInfo = std::make_shared<EnumTypeInfo>(enumDecl->GetName(), enumDecl->GetValues());
			m_typeResolver.RegisterEnum(enumDecl->GetName(), typeInfo);
		}
	}
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
		info.returnType = m_typeResolver.Resolve(fn->GetReturnTypeName());

		for (const auto& param : fn->GetParams())
		{
			info.params.emplace_back(param.name, m_typeResolver.Resolve(param.typeName));
		}

		m_functions[fn->GetName()] = std::move(info);
	}
}

void SemanticAnalyzer::Visit(const ArrayAllocExpr& node)
{
	node.GetSize().Accept(*this);
	AssertIsIntegerType(GetType(node.GetSize()), "Размер массива должен быть целым числом", node.GetSize().GetRange());

	auto elementType = m_typeResolver.Resolve(node.GetElementTypeName());
	SetType(node, std::make_shared<ArrayTypeInfo>(elementType));
}

void SemanticAnalyzer::Visit(const IndexExpr& node)
{
	const auto savedExpected = m_expectedType;
	m_expectedType = nullptr;

	node.GetReceiver().Accept(*this);
	node.GetIndex().Accept(*this);

	m_expectedType = savedExpected;

	auto receiverType = GetType(node.GetReceiver());
	AssertIsIndexableType(receiverType, node.GetRange());
	AssertIsIntegerType(GetType(node.GetIndex()), "Индекс должен быть целым числом", node.GetIndex().GetRange());
	if (const auto* arrayType = dynamic_cast<const ArrayTypeInfo*>(receiverType.get()))
	{
		SetType(node, arrayType->GetElementType());
	}
}

void SemanticAnalyzer::Visit(const MemberAccessExpr& node)
{
	if (const auto* id = dynamic_cast<const IdentifierExpr*>(&node.GetReceiver()))
	{
		if (m_typeResolver.IsEnum(id->GetName()))
		{
			auto enumType = m_typeResolver.GetEnum(id->GetName());
			const auto& fields = enumType->GetFields();

			auto it = std::ranges::find(fields, node.GetField());
			if (it != fields.end())
			{
				SetType(node, enumType);
				return;
			}

			m_engine->Report(DiagnosticData{
				.phase = CompilerPhase::Semantic,
				.message = "У перечисления '" + id->GetName() + "' нет поля '" + node.GetField() + "'",
				.line = node.GetRange().start.line,
				.pos = node.GetRange().start.pos});
			return;
		}
	}

	node.GetReceiver().Accept(*this);
	auto receiverType = GetType(node.GetReceiver());

	const bool isArray = dynamic_cast<const ArrayTypeInfo*>(receiverType.get()) != nullptr;
	if (isArray && node.GetField() == "size")
	{
		SetType(node, ScalarTypeInfo::Make(BaseType::Uint));
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
	m_scopeManager.ResetSlots();
	m_currentReturnType = m_functions[node.GetName()].returnType;

	m_scopeManager.EnterScope();
	for (const auto& param : node.GetParams())
	{
		const uint32_t slot = m_scopeManager.AllocateSlot();
		m_scopeManager.Declare(param.name, m_typeResolver.Resolve(param.typeName), false, slot);
	}
	node.GetBody().Accept(*this);
	m_scopeManager.LeaveScope();

	m_functions[node.GetName()].maxSlots = m_scopeManager.GetMaxSlots();
	m_currentReturnType = nullptr;
}

void SemanticAnalyzer::Visit(const BlockStmt& node)
{
	m_scopeManager.EnterScope();
	const uint32_t savedSlot = m_scopeManager.GetCurrentSlot();
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
	m_scopeManager.RestoreSlot(savedSlot);
	m_scopeManager.LeaveScope();
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

	auto type = m_typeResolver.Resolve(node.GetTypeName());
	AssertIsNotVoidType(type, node.GetName(), node.GetRange().start.line, node.GetRange().start.pos);

	const bool hasInit = node.GetInit() != nullptr;
	AssertIsInitializedIfImmutable(hasInit, node.GetModifier(), node.GetName(), node.GetRange());

	if (hasInit)
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = type;
		node.GetInit()->Accept(*this);
		m_expectedType = savedExpected;

		AssertIsExactTypeMatch(type, GetType(*node.GetInit()), node.GetRange());
	}

	const bool isConst = node.GetModifier() == VarModifier::Def;
	const bool isMutable = node.GetModifier() == VarModifier::Var;
	uint32_t slot = 0;

	if (isConst)
	{
		AssertIsCompileTimeConstant(node.GetInit(), node.GetInit()->GetRange());
	}
	else
	{
		slot = m_scopeManager.AllocateSlot();
		m_varDeclSlots[&node] = slot;
	}

	const bool isNew = m_scopeManager.Declare(node.GetName(), type, isMutable, slot, isConst, node.GetInit());
	AssertIsVariableNotRedeclared(isNew, node.GetName());
}

void SemanticAnalyzer::Visit(const AssignStmt& node)
{
	const bool isIdentifier = dynamic_cast<const IdentifierExpr*>(&node.GetLhs()) != nullptr;
	const bool isIndexExpr = dynamic_cast<const IndexExpr*>(&node.GetLhs()) != nullptr;
	AssertIsLhsValidForAssignment(isIdentifier || isIndexExpr, node.GetRange());

	node.GetLhs().Accept(*this);

	const auto savedExpected = m_expectedType;
	m_expectedType = GetType(node.GetLhs());
	node.GetRhs().Accept(*this);
	m_expectedType = savedExpected;

	if (isIdentifier)
	{
		const auto& ident = static_cast<const IdentifierExpr&>(node.GetLhs());
		auto info = m_scopeManager.Resolve(ident.GetName());
		if (info)
		{
			AssertIsMutable(*info, ident.GetName(), ident.GetRange());
		}
	}

	AssertIsExactTypeMatch(
		GetType(node.GetLhs()),
		GetType(node.GetRhs()),
		node.GetRange());
}

void SemanticAnalyzer::Visit(const IfStmt& node)
{
	node.GetCondition().Accept(*this);
	AssertIsBool(
		GetType(node.GetCondition()),
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
	m_scopeManager.EnterScope();
	const uint32_t savedSlot = m_scopeManager.GetCurrentSlot();

	std::vector<SymbolInfo> iterInfos;
	const auto& iters = node.GetIterators();
	const auto& ranges = node.GetRanges();

	for (size_t i = 0; i < iters.size(); ++i)
	{
		const size_t startIdx = i * 2;
		const size_t endIdx = i * 2 + 1;

		const auto savedExpected = m_expectedType;
		m_expectedType = nullptr;
		ranges[endIdx]->Accept(*this);

		auto iterType = GetType(*ranges[endIdx]);
		AssertIsIntegerType(iterType, "Граница цикла должна быть целочисленного типа", ranges[endIdx]->GetRange());

		m_expectedType = iterType;
		ranges[startIdx]->Accept(*this);
		m_expectedType = savedExpected;

		AssertIsExactTypeMatch(iterType, GetType(*ranges[startIdx]), ranges[startIdx]->GetRange());

		const uint32_t slot = m_scopeManager.AllocateSlot();

		const bool isNew = m_scopeManager.Declare(iters[i], iterType, false, slot, false, nullptr);
		AssertIsVariableNotRedeclared(isNew, iters[i]);

		iterInfos.push_back(SymbolInfo{iterType, slot, false, false, nullptr});
	}

	m_resolvedIterators[&node] = std::move(iterInfos);

	node.GetOriginalBody().Accept(*this);

	m_scopeManager.RestoreSlot(savedSlot);
	m_scopeManager.LeaveScope();
}

void SemanticAnalyzer::Visit(const RunStmt& node)
{
	node.GetCondition().Accept(*this);
	AssertIsBool(
		GetType(node.GetCondition()),
		node.GetCondition().GetRange().start.line,
		node.GetCondition().GetRange().start.pos);
	node.GetBody().Accept(*this);
}

void SemanticAnalyzer::Visit(const DoWhileStmt& node)
{
	node.GetBody().Accept(*this);
	node.GetCondition().Accept(*this);
	AssertIsBool(
		GetType(node.GetCondition()),
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

	AssertIsExactTypeMatch(m_currentReturnType, GetType(node), node.GetRange());
}

void SemanticAnalyzer::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
}

void SemanticAnalyzer::Visit(const IdentifierExpr& node)
{
	auto result = m_scopeManager.Resolve(node.GetName());
	if (!result)
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Использование необъявленной переменной: " + node.GetName(),
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
		return;
	}
	SetType(node, result->type);
	m_resolvedSymbols[&node] = *result;
}

void SemanticAnalyzer::Visit(const BinaryExpr& node)
{
	const auto savedExpected = m_expectedType;

	m_expectedType = nullptr;
	node.GetLeft().Accept(*this);
	auto leftType = GetType(node.GetLeft());

	node.GetRight().Accept(*this);
	auto rightType = GetType(node.GetRight());

	std::shared_ptr<TypeInfo> anchorType = nullptr;

	if (savedExpected != nullptr && savedExpected->GetName() != std::string(LanguageTokens::TypeBool))
	{
		anchorType = savedExpected;
	}
	else if (leftType != rightType)
	{
		const bool isLeftStrict = dynamic_cast<const IdentifierExpr*>(&node.GetLeft()) != nullptr || dynamic_cast<const MemberAccessExpr*>(&node.GetLeft()) != nullptr || dynamic_cast<const IndexExpr*>(&node.GetLeft()) != nullptr;

		const bool isRightStrict = dynamic_cast<const IdentifierExpr*>(&node.GetRight()) != nullptr || dynamic_cast<const MemberAccessExpr*>(&node.GetRight()) != nullptr || dynamic_cast<const IndexExpr*>(&node.GetRight()) != nullptr;

		if (isLeftStrict && !isRightStrict)
		{
			anchorType = leftType;
		}
		else if (isRightStrict && !isLeftStrict)
		{
			anchorType = rightType;
		}
	}

	if (anchorType != nullptr)
	{
		m_expectedType = anchorType;
		node.GetLeft().Accept(*this);
		leftType = GetType(node.GetLeft());

		node.GetRight().Accept(*this);
		rightType = GetType(node.GetRight());
	}

	m_expectedType = savedExpected;

	AssertIsExactTypeMatch(leftType, rightType, node.GetRange());

	if (IsComparisonOp(node.GetOp()) || IsLogicalOp(node.GetOp()))
	{
		SetType(node, ScalarTypeInfo::Make(BaseType::Bool));
	}
	else
	{
		SetType(node, leftType);
	}
}

void SemanticAnalyzer::Visit(const UnaryExpr& node)
{
	node.GetOperand().Accept(*this);
	SetType(node, GetType(node.GetOperand()));
}

void SemanticAnalyzer::Visit(const FunctionCallExpr& node)
{
	if (node.GetName() == "print" || node.GetName() == "println")
	{
		for (const auto& arg : node.GetArgs())
		{
			arg->Accept(*this);
		}

		SetType(node, ScalarTypeInfo::Make(BaseType::Void));
		return;
	}

	if (IsPrimitiveType(node.GetName()) && node.GetArgs().size() == 1)
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = nullptr;
		node.GetArgs()[0]->Accept(*this);
		m_expectedType = savedExpected;
		SetType(node, m_typeResolver.Resolve(node.GetName()));
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
			GetType(*node.GetArgs()[i]),
			node.GetArgs()[i]->GetRange());
	}

	SetType(node, fn.returnType);
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

				SetType(node, m_expectedType);
				return;
			}

			if (scalar->IsFloat() && node.IsFloat())
			{
				const bool isInBounds = CheckFloatBounds(node.GetValue());
				AssertIsLiteralInBounds(isInBounds, node.GetValue(), scalar->GetName(), node.GetRange());

				SetType(node, m_expectedType);
				return;
			}
		}
	}

	const auto base = node.IsFloat() ? BaseType::Real : BaseType::Int;
	auto type = ScalarTypeInfo::Make(base);
	SetType(node, type);
}

void SemanticAnalyzer::Visit(const StringExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::String);
	SetType(node, type);
}

void SemanticAnalyzer::Visit(const BoolExpr& node)
{
	auto type = ScalarTypeInfo::Make(BaseType::Bool);
	SetType(node, type);
}

void SemanticAnalyzer::Visit(const ArrayLiteralExpr& node)
{
	const auto& elements = node.GetElements();

	if (elements.empty())
	{
		AssertIsArrayTypeKnown(m_expectedType != nullptr, node.GetRange());
		SetType(node, m_expectedType);
		return;
	}

	elements[0]->Accept(*this);
	auto elementType = GetType(*elements[0]);

	for (size_t i = 1; i < elements.size(); ++i)
	{
		elements[i]->Accept(*this);
		auto currentType = GetType(*elements[i]);

		AssertIsArrayElementTypesMatch(
			elementType->GetName() == currentType->GetName(),
			elementType->GetName(),
			currentType->GetName(),
			elements[i]->GetRange());
	}

	auto arrayType = std::make_shared<ArrayTypeInfo>(elementType);
	SetType(node, arrayType);
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

void SemanticAnalyzer::Visit(const WhenStmt& node)
{
	std::shared_ptr<TypeInfo> subjectType = nullptr;
	std::shared_ptr<EnumTypeInfo> enumType = nullptr;

	if (node.GetSubject())
	{
		node.GetSubject()->Accept(*this);
		subjectType = GetType(*node.GetSubject());
		enumType = std::dynamic_pointer_cast<EnumTypeInfo>(subjectType);
	}

	std::unordered_set<std::string> coveredFields;

	for (const auto& entry : node.GetEntries())
	{
		for (const auto& cond : entry.conditions)
		{
			const auto savedExpected = m_expectedType;
			m_expectedType = subjectType ? subjectType : ScalarTypeInfo::Make(BaseType::Bool);

			cond->Accept(*this);

			m_expectedType = savedExpected;

			AssertIsExactTypeMatch(
				subjectType ? subjectType : ScalarTypeInfo::Make(BaseType::Bool),
				GetType(*cond),
				cond->GetRange());

			if (enumType)
			{
				if (const auto* memberAccess = dynamic_cast<const MemberAccessExpr*>(cond.get()))
				{
					coveredFields.insert(memberAccess->GetField());
				}
			}
		}

		entry.body->Accept(*this);
	}

	if (node.GetElseBody())
	{
		node.GetElseBody()->Accept(*this);
	}
	else if (enumType)
	{
		for (const auto& field : enumType->GetFields())
		{
			if (!coveredFields.contains(field))
			{
				m_engine->Report(DiagnosticData{
					.phase = CompilerPhase::Semantic,
					.message = "Пропущена ветка для поля '" + field + "' перечисления '" + enumType->GetName() + "'. Добавьте её или ветку else.",
					.line = node.GetRange().start.line,
					.pos = node.GetRange().start.pos});
			}
		}
	}
}

std::shared_ptr<TypeInfo> SemanticAnalyzer::GetType(const AstNode& node) const
{
	auto it = m_annotations.resolvedTypes.find(&node);
	return it != m_annotations.resolvedTypes.end() ? it->second : nullptr;
}

void SemanticAnalyzer::SetType(const AstNode& node, std::shared_ptr<TypeInfo> type)
{
	m_annotations.resolvedTypes[&node] = std::move(type);
}