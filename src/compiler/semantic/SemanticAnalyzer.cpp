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
#include "src/compiler/ast/BreakStmt.h"
#include "src/compiler/ast/ContinueStmt.h"
#include "src/compiler/ast/RepCollectionStmt.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/RepTimesStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/ast/StructDeclStmt.h"
#include "src/compiler/ast/StructTypeInfo.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/UnionDeclStmt.h"
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

void AssertIsNotConstRef(const SymbolInfo& info, const std::string& name, const SourceRange& range)
{
	if (info.isConstRef)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Нельзя изменить содержимое параметра, объявленного как val: " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

bool IsMutableParam(const std::optional<VarModifier>& modifier)
{
	return modifier == VarModifier::Var;
}

bool IsConstRefParam(const std::optional<VarModifier>& modifier)
{
	return modifier == VarModifier::Val;
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

void AssertIsArgCountInRange(size_t actual, size_t required, size_t total, const std::string& name)
{
	if (actual < required || actual > total)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Неверное количество аргументов при вызове функции " + name
				+ ": ожидалось от " + std::to_string(required)
				+ " до " + std::to_string(total)
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

bool DefinitelyReturns(const Stmt& stmt)
{
	if (const auto* ret = dynamic_cast<const ReturnStmt*>(&stmt))
	{
		return ret->GetValue() != nullptr;
	}

	if (const auto* block = dynamic_cast<const BlockStmt*>(&stmt))
	{
		for (const auto& inner : block->GetStmts())
		{
			if (DefinitelyReturns(*inner))
			{
				return true;
			}
		}
		return false;
	}

	if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt))
	{
		const Stmt* elseNode = ifStmt->GetElseNode();
		return elseNode != nullptr
			&& DefinitelyReturns(ifStmt->GetThenBlock())
			&& DefinitelyReturns(*elseNode);
	}

	if (const auto* whenStmt = dynamic_cast<const WhenStmt*>(&stmt))
	{
		const Stmt* elseBody = whenStmt->GetElseBody();
		if (elseBody == nullptr || !DefinitelyReturns(*elseBody))
		{
			return false;
		}
		for (const auto& entry : whenStmt->GetEntries())
		{
			if (!DefinitelyReturns(*entry.body))
			{
				return false;
			}
		}
		return true;
	}

	return false;
}

std::string MangleFunction(const std::string& name, const std::vector<ParamInfo>& params)
{
	std::string key = name + "(";
	for (size_t i = 0; i < params.size(); ++i)
	{
		if (i > 0)
		{
			key += ",";
		}
		key += params[i].type ? params[i].type->GetName() : "?";
	}
	key += ")";
	return key;
}

size_t RequiredParamCount(const std::vector<ParamInfo>& params)
{
	size_t required = 0;
	for (const auto& param : params)
	{
		if (param.defaultValue == nullptr)
		{
			++required;
		}
	}
	return required;
}

[[noreturn]] void ThrowNoMatchingOverload(const std::string& name, const SourceRange& range)
{
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Semantic,
		.message = "Нет подходящей перегрузки функции '" + name + "' для переданных аргументов",
		.line = range.start.line,
		.pos = range.start.pos});
}

[[noreturn]] void ThrowAmbiguousCall(const std::string& name, const SourceRange& range)
{
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Semantic,
		.message = "Неоднозначный вызов перегруженной функции '" + name + "'",
		.line = range.start.line,
		.pos = range.start.pos});
}

bool IsFlexibleLiteralExpr(const Expr& expr)
{
	if (dynamic_cast<const NumExpr*>(&expr) != nullptr)
	{
		return true;
	}
	if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expr))
	{
		if (IsComparisonOp(binary->GetOp()) || IsLogicalOp(binary->GetOp()))
		{
			return false;
		}
		return IsFlexibleLiteralExpr(binary->GetLeft()) && IsFlexibleLiteralExpr(binary->GetRight());
	}
	if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
	{
		return IsFlexibleLiteralExpr(unary->GetOperand());
	}
	return false;
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
		std::move(m_collectionLoopInfos),
		std::move(m_repTimesInfos),
		std::move(m_functions),
		std::move(m_entryPointKey)
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
		else if (const auto* structDecl = dynamic_cast<const StructDeclStmt*>(decl.get()))
		{
			m_typeResolver.RegisterStruct(structDecl->GetName(), std::make_shared<StructTypeInfo>(structDecl->GetName(), structDecl->GetFields()));
		}
		else if (const auto* unionDecl = dynamic_cast<const UnionDeclStmt*>(decl.get()))
		{
			m_typeResolver.RegisterStruct(unionDecl->GetName(), std::make_shared<StructTypeInfo>(unionDecl->GetName(), unionDecl->GetFields()));
		}
	}
}

void SemanticAnalyzer::CollectFunctions(const AstNode& root)
{
	for (const auto& native : NativeRegistry::GetAll())
	{
		FunctionInfo info;
		info.returnType = native.returnType;
		for (const auto& [name, type] : native.params)
		{
			info.params.push_back(ParamInfo{name, type, std::nullopt, nullptr});
		}
		const std::string key = MangleFunction(native.name, info.params);
		m_overloadsByName[native.name].push_back(key);
		m_functions[key] = std::move(info);
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
			info.params.push_back(ParamInfo{param.name, m_typeResolver.Resolve(param.typeName), param.modifier, param.defaultValue.get()});
		}

		const std::string key = MangleFunction(fn->GetName(), info.params);
		m_annotations.functionDeclKeys[fn] = key;

		if (m_functions.contains(key))
		{
			m_engine->Report(DiagnosticData{
				.phase = CompilerPhase::Semantic,
				.message = "Повторное объявление функции с той же сигнатурой: " + key,
				.line = fn->GetRange().start.line,
				.pos = fn->GetRange().start.pos});
			continue;
		}

		m_overloadsByName[fn->GetName()].push_back(key);
		m_functions[key] = std::move(info);

		if (fn->GetName() == "main")
		{
			m_entryPointKey = key;
		}
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
	const std::string& key = m_annotations.functionDeclKeys[&node];
	m_currentReturnType = m_functions[key].returnType;

	bool seenDefault = false;
	for (const auto& param : node.GetParams())
	{
		if (param.defaultValue != nullptr)
		{
			seenDefault = true;
			const auto paramType = m_typeResolver.Resolve(param.typeName);
			const auto savedExpected = m_expectedType;
			m_expectedType = paramType;
			param.defaultValue->Accept(*this);
			m_expectedType = savedExpected;
			AssertIsExactTypeMatch(paramType, GetType(*param.defaultValue), param.defaultValue->GetRange());
		}
		else if (seenDefault)
		{
			m_engine->Report(DiagnosticData{
				.phase = CompilerPhase::Semantic,
				.message = "Параметр '" + param.name + "' без значения по умолчанию не может следовать за параметром со значением по умолчанию",
				.line = node.GetRange().start.line,
				.pos = node.GetRange().start.pos});
		}
	}

	m_scopeManager.EnterScope();
	for (const auto& param : node.GetParams())
	{
		const uint32_t slot = m_scopeManager.AllocateSlot();
		const bool isMutable = IsMutableParam(param.modifier);
		const bool isConstRef = IsConstRefParam(param.modifier);
		m_scopeManager.Declare(param.name, m_typeResolver.Resolve(param.typeName), isMutable, slot, false, nullptr, isConstRef);
	}
	node.GetBody().Accept(*this);
	m_scopeManager.LeaveScope();

	if (m_currentReturnType->GetName() != std::string(LanguageTokens::TypeVoid)
		&& !DefinitelyReturns(node.GetBody()))
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Функция '" + node.GetName() + "' должна возвращать значение на всех путях выполнения",
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
	}

	m_functions[key].maxSlots = m_scopeManager.GetMaxSlots();
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

	if (isIndexExpr)
	{
		const auto& indexExpr = static_cast<const IndexExpr&>(node.GetLhs());
		if (const auto* id = dynamic_cast<const IdentifierExpr*>(&indexExpr.GetReceiver()))
		{
			const auto info = m_scopeManager.Resolve(id->GetName());
			if (info)
			{
				AssertIsNotConstRef(*info, id->GetName(), id->GetRange());
			}
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

void SemanticAnalyzer::Visit(const BreakStmt& node)
{
	if (m_loopDepth == 0)
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Оператор break используется вне цикла",
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
	}
}

void SemanticAnalyzer::Visit(const ContinueStmt& node)
{
	if (m_loopDepth == 0)
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Оператор continue используется вне цикла",
			.line = node.GetRange().start.line,
			.pos = node.GetRange().start.pos});
	}
}

void SemanticAnalyzer::Visit(const RepStmt& node)
{
	m_scopeManager.EnterScope();
	const uint32_t savedSlot = m_scopeManager.GetCurrentSlot();

	const auto savedExpected = m_expectedType;
	m_expectedType = nullptr;
	node.GetEndExpr().Accept(*this);
	auto iterType = GetType(node.GetEndExpr());
	AssertIsIntegerType(iterType, "Граница цикла должна быть целочисленного типа", node.GetEndExpr().GetRange());

	m_expectedType = iterType;
	node.GetStartExpr().Accept(*this);
	m_expectedType = savedExpected;
	AssertIsExactTypeMatch(iterType, GetType(node.GetStartExpr()), node.GetStartExpr().GetRange());

	if (node.GetStepExpr())
	{
		m_expectedType = iterType;
		node.GetStepExpr()->Accept(*this);
		m_expectedType = savedExpected;
		AssertIsExactTypeMatch(iterType, GetType(*node.GetStepExpr()), node.GetStepExpr()->GetRange());
	}

	const uint32_t slot = m_scopeManager.AllocateSlot();
	const bool isNew = m_scopeManager.Declare(node.GetIterator(), iterType, false, slot, false, nullptr);
	AssertIsVariableNotRedeclared(isNew, node.GetIterator());

	m_resolvedIterators[&node] = SymbolInfo{iterType, slot, false, false, nullptr};

	++m_loopDepth;
	node.GetBody().Accept(*this);
	--m_loopDepth;

	m_scopeManager.RestoreSlot(savedSlot);
	m_scopeManager.LeaveScope();
}

void SemanticAnalyzer::Visit(const RepCollectionStmt& node)
{
	m_scopeManager.EnterScope();
	const uint32_t savedSlot = m_scopeManager.GetCurrentSlot();

	const auto savedExpected = m_expectedType;
	m_expectedType = nullptr;
	node.GetCollectionExpr().Accept(*this);
	m_expectedType = savedExpected;

	const auto collType = GetType(node.GetCollectionExpr());
	const auto* arrayType = dynamic_cast<const ArrayTypeInfo*>(collType.get());
	if (!arrayType)
	{
		m_engine->Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Выражение в repeat...in должно быть массивом",
			.line = node.GetCollectionExpr().GetRange().start.line,
			.pos = node.GetCollectionExpr().GetRange().start.pos});
		m_scopeManager.RestoreSlot(savedSlot);
		m_scopeManager.LeaveScope();
		return;
	}

	auto elementType = arrayType->GetElementType();
	auto intType = m_typeResolver.Resolve(std::string(LanguageTokens::TypeInt));

	const uint32_t arraySlot = m_scopeManager.AllocateSlot();
	const uint32_t counterSlot = m_scopeManager.AllocateSlot();
	if (node.HasIndexIterator())
	{
		const bool isNew = m_scopeManager.Declare(node.GetIndexIterator(), intType, false, counterSlot, false, nullptr);
		AssertIsVariableNotRedeclared(isNew, node.GetIndexIterator());
	}

	const uint32_t valueSlot = m_scopeManager.AllocateSlot();
	const bool isNew = m_scopeManager.Declare(node.GetValueIterator(), elementType, false, valueSlot, false, nullptr);
	AssertIsVariableNotRedeclared(isNew, node.GetValueIterator());

	m_collectionLoopInfos[&node] = CollectionLoopInfo{
		SymbolInfo{elementType, valueSlot, false, false, nullptr},
		arraySlot,
		counterSlot
	};

	++m_loopDepth;
	node.GetBody().Accept(*this);
	--m_loopDepth;

	m_scopeManager.RestoreSlot(savedSlot);
	m_scopeManager.LeaveScope();
}

void SemanticAnalyzer::Visit(const RepTimesStmt& node)
{
	m_scopeManager.EnterScope();
	const uint32_t savedSlot = m_scopeManager.GetCurrentSlot();

	const auto savedExpected = m_expectedType;
	m_expectedType = nullptr;
	node.GetCountExpr().Accept(*this);
	m_expectedType = savedExpected;

	const auto countType = GetType(node.GetCountExpr());
	AssertIsIntegerType(countType, "Количество итераций должно быть целым числом", node.GetCountExpr().GetRange());

	const uint32_t limitSlot = m_scopeManager.AllocateSlot();
	const uint32_t counterSlot = m_scopeManager.AllocateSlot();
	m_repTimesInfos[&node] = TimesLoopInfo{limitSlot, counterSlot};

	++m_loopDepth;
	node.GetBody().Accept(*this);
	--m_loopDepth;

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
	++m_loopDepth;
	node.GetBody().Accept(*this);
	--m_loopDepth;
}

void SemanticAnalyzer::Visit(const DoWhileStmt& node)
{
	++m_loopDepth;
	node.GetBody().Accept(*this);
	--m_loopDepth;
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
	const bool resultIsBool = IsComparisonOp(node.GetOp()) || IsLogicalOp(node.GetOp());

	std::shared_ptr<TypeInfo> propagated = nullptr;
	if (!resultIsBool
		&& savedExpected != nullptr
		&& savedExpected->GetName() != std::string(LanguageTokens::TypeBool))
	{
		propagated = savedExpected;
	}

	m_expectedType = propagated;
	node.GetLeft().Accept(*this);
	auto leftType = GetType(node.GetLeft());

	node.GetRight().Accept(*this);
	auto rightType = GetType(node.GetRight());

	if (propagated == nullptr && leftType != rightType)
	{
		const bool isLeftFlexible = IsFlexibleLiteralExpr(node.GetLeft());
		const bool isRightFlexible = IsFlexibleLiteralExpr(node.GetRight());

		std::shared_ptr<TypeInfo> anchorType = nullptr;
		if (isLeftFlexible && !isRightFlexible)
		{
			anchorType = rightType;
		}
		else if (isRightFlexible && !isLeftFlexible)
		{
			anchorType = leftType;
		}

		if (anchorType != nullptr)
		{
			m_expectedType = anchorType;
			node.GetLeft().Accept(*this);
			leftType = GetType(node.GetLeft());

			node.GetRight().Accept(*this);
			rightType = GetType(node.GetRight());
		}
	}

	m_expectedType = savedExpected;

	AssertIsExactTypeMatch(leftType, rightType, node.GetRange());

	if (resultIsBool)
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

	const auto overloadIt = m_overloadsByName.find(node.GetName());
	AssertIsFunctionExists(overloadIt != m_overloadsByName.end(), node.GetName(), node.GetRange());
	const auto& candidateKeys = overloadIt->second;

	if (candidateKeys.size() == 1)
	{
		const std::string& key = candidateKeys.front();
		const auto& fn = m_functions[key];
		AssertIsArgCountInRange(node.GetArgs().size(), RequiredParamCount(fn.params), fn.params.size(), node.GetName());

		for (size_t i = 0; i < node.GetArgs().size(); ++i)
		{
			const auto savedExpected = m_expectedType;
			m_expectedType = fn.params[i].type;

			node.GetArgs()[i]->Accept(*this);

			m_expectedType = savedExpected;

			AssertIsExactTypeMatch(
				fn.params[i].type,
				GetType(*node.GetArgs()[i]),
				node.GetArgs()[i]->GetRange());
		}

		m_annotations.resolvedCallTargets[&node] = key;
		SetType(node, fn.returnType);
		return;
	}

	std::vector<std::shared_ptr<TypeInfo>> argTypes;
	for (const auto& arg : node.GetArgs())
	{
		const auto savedExpected = m_expectedType;
		m_expectedType = nullptr;
		arg->Accept(*this);
		m_expectedType = savedExpected;
		argTypes.push_back(GetType(*arg));
	}

	std::vector<std::string> matches;
	for (const auto& key : candidateKeys)
	{
		const auto& fn = m_functions[key];
		if (argTypes.size() < RequiredParamCount(fn.params) || argTypes.size() > fn.params.size())
		{
			continue;
		}
		bool isMatch = true;
		for (size_t i = 0; i < argTypes.size(); ++i)
		{
			if (!fn.params[i].type || !argTypes[i]
				|| fn.params[i].type->GetName() != argTypes[i]->GetName())
			{
				isMatch = false;
				break;
			}
		}
		if (isMatch)
		{
			matches.push_back(key);
		}
	}

	if (matches.empty())
	{
		ThrowNoMatchingOverload(node.GetName(), node.GetRange());
	}
	if (matches.size() > 1)
	{
		ThrowAmbiguousCall(node.GetName(), node.GetRange());
	}

	m_annotations.resolvedCallTargets[&node] = matches.front();
	SetType(node, m_functions[matches.front()].returnType);
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