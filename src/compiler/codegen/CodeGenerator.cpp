#include "CodeGenerator.h"

#include "src/compiler/ast/ArrayAllocExpr.h"
#include "src/compiler/ast/ArrayLiteralExpr.h"
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
#include "src/compiler/ast/MemberAccessExpr.h"
#include "src/compiler/ast/NumExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/BreakStmt.h"
#include "src/compiler/ast/ClassicForStmt.h"
#include "src/compiler/ast/ContinueStmt.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/ast/WhenStmt.h"
#include "src/compiler/semantic/SymbolTable.h"
#include "src/compiler/stdlib/NativeRegistry.h"
#include "src/compiler/stdlib/PrintResolver.h"
#include "src/compiler/vm/value/Value.h"
#include "src/diagnostics/CompilationException.h"

#include <stdexcept>
#include <unordered_map>

namespace
{

void AssertIsTypeResolved(bool found)
{
	if (!found)
	{
		throw std::runtime_error("Тип узла не найден в аннотациях семантического анализа");
	}
}

void AssertIsIdentifierResolved(bool found, const std::string& name, const SourceRange& range)
{
	if (!found)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = "Переменная не найдена при генерации кода: " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsLhsIdentifier(bool isId, const SourceRange& range)
{
	if (!isId)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = "Левая часть присваивания должна быть идентификатором",
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsFunctionOffsetResolved(bool found, const std::string& name, const SourceRange& range)
{
	if (!found)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = "Бэкенд: не найдена реализация функции " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

void AssertIsNativeCastResolved(bool isResolved, const std::string& name, const SourceRange& range)
{
	if (!isResolved)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = "Бэкенд: не реализовано нативное преобразование " + name,
			.line = range.start.line,
			.pos = range.start.pos});
	}
}

using BuiltinEmitter = std::function<void(CodeGenerator&, const FunctionCallExpr&)>;

const std::unordered_map<std::string, BuiltinEmitter>& GetBuiltinEmitters()
{
	static const std::unordered_map<std::string, BuiltinEmitter> emitters = {
		{"print", [](auto& gen, const auto& node) { gen.EmitPrint(node); }},
		{"println", [](auto& gen, const auto& node) { gen.EmitPrintLn(node); }},
	};
	return emitters;
}

bool IsPrimitiveType(const std::string& name)
{
	return name == "int" || name == "uint" || name == "real" || name == "bool" || name == "string" || name == "void";
}

void EmitNativeCall(Chunk& chunk, uint32_t nativeId, uint32_t argCount, uint32_t line)
{
	chunk.WriteOpCode(OpCode::CallNative, line);
	chunk.WriteUint32(nativeId, line);
	chunk.WriteUint32(argCount, line);
}

uint32_t EmitCallWithPlaceholder(Chunk& chunk, uint32_t argCount, uint32_t line)
{
	chunk.WriteOpCode(OpCode::Call, line);
	const uint32_t patchOffset = chunk.GetCodeSize();
	chunk.WriteUint32(0, line);
	chunk.WriteUint32(argCount, line);
	return patchOffset;
}

Value ParseNumLiteral(const std::string& text, const TypeInfo* typeInfo, bool isFloat)
{
	if (const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(typeInfo))
	{
		if (scalar->GetBaseType() == BaseType::Real) return Value(std::stod(text));
		if (scalar->GetBaseType() == BaseType::Uint) return Value(	static_cast<uint64_t>(std::stoull(text)));
		return Value(static_cast<int64_t>(std::stoll(text)));
	}

	return isFloat ? Value(std::stod(text)) : Value(static_cast<int64_t>(std::stoll(text)));
}

OpCode GetBinaryOpCode(BinaryOpKind op, const TypeInfo* typeInfo)
{
	if (dynamic_cast<const EnumTypeInfo*>(typeInfo))
	{
		if (op == BinaryOpKind::Eq || op == BinaryOpKind::NotEq)
		{
			return OpCode::EqUint;
		}
	}

	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(typeInfo);
	if (!scalar) return OpCode::AddInt;

	const auto base = scalar->GetBaseType();
	switch (op)
	{
	case BinaryOpKind::Add:
		if (base == BaseType::Uint) return OpCode::AddUint;
		if (base == BaseType::Real) return OpCode::AddReal;
		if (base == BaseType::String) return OpCode::AddString;
		return OpCode::AddInt;
	case BinaryOpKind::Sub:
		if (base == BaseType::Uint) return OpCode::SubUint;
		if (base == BaseType::Real) return OpCode::SubReal;
		return OpCode::SubInt;
	case BinaryOpKind::Mul:
		if (base == BaseType::Uint) return OpCode::MulUint;
		if (base == BaseType::Real) return OpCode::MulReal;
		return OpCode::MulInt;
	case BinaryOpKind::Div:
		if (base == BaseType::Uint) return OpCode::DivUint;
		if (base == BaseType::Real) return OpCode::DivReal;
		return OpCode::DivInt;
	case BinaryOpKind::Mod:
		if (base == BaseType::Uint) return OpCode::RemUint;
		if (base == BaseType::Real) return OpCode::RemReal;
		return OpCode::RemInt;
	case BinaryOpKind::Eq:
	case BinaryOpKind::NotEq:
		if (base == BaseType::Uint) return OpCode::EqUint;
		if (base == BaseType::Real) return OpCode::EqReal;
		if (base == BaseType::String) return OpCode::EqString;
		return OpCode::EqInt;
	case BinaryOpKind::Less:
	case BinaryOpKind::Greater:
	case BinaryOpKind::LessEq:
	case BinaryOpKind::GreaterEq:
		if (base == BaseType::Uint) return OpCode::LtUint;
		if (base == BaseType::Real) return OpCode::LtReal;
		return OpCode::LtInt;
	case BinaryOpKind::LogicalAnd:
	case BinaryOpKind::LogicalOr:
		throw std::runtime_error("Логические операторы должны обрабатываться через короткое замыкание");
	}
	return OpCode::AddInt;
}

std::string ExtractBaseTypeName(const TypeInfo* typeInfo)
{
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(typeInfo);
	if (!scalar) return "int";
	return scalar->GetName();
}
} // namespace

CodeGenerator::CodeGenerator(CodegenContext context)
	: m_context(std::move(context))
{
}

void CodeGenerator::EmitCast(const FunctionCallExpr& node)
{
	node.GetArgs()[0]->Accept(*this);
	const std::string srcBase = ExtractBaseTypeName(GetType(*node.GetArgs()[0]).get());
	const std::string targetBase = node.GetName();

	if (srcBase != targetBase)
	{
		const std::string nativeName = "__cast_" + srcBase + "_" + targetBase;
		if (const auto* native = NativeRegistry::FindByName(nativeName))
		{
			EmitNativeCall(m_chunk, native->id, 1, m_currentLine);
		}
		else
		{
			AssertIsNativeCastResolved(false, nativeName, node.GetRange());
		}
	}
}

void CodeGenerator::EmitPrintCore(const FunctionCallExpr& node)
{
	const auto argCount = static_cast<uint32_t>(node.GetArgs().size());
	for (uint32_t i = 0; i < argCount; ++i)
	{
		node.GetArgs()[i]->Accept(*this);

		const std::string nativeName = PrintResolver::Resolve(GetType(*node.GetArgs()[i]).get());
		if (const auto* native = NativeRegistry::FindByName(nativeName))
		{
			EmitNativeCall(m_chunk, native->id, 1, m_currentLine);
			m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
		}

		if (i + 1 < argCount)
		{
			if (const auto* native = NativeRegistry::FindByName("print_space"))
			{
				EmitNativeCall(m_chunk, native->id, 0, m_currentLine);
				m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
			}
		}
	}
}

void CodeGenerator::EmitPrint(const FunctionCallExpr& node)
{
	EmitPrintCore(node);
	EmitConstant(Value(static_cast<uint64_t>(0)));
}

void CodeGenerator::EmitPrintLn(const FunctionCallExpr& node)
{
	EmitPrintCore(node);
	if (const auto* native = NativeRegistry::FindByName("print_newline"))
	{
		EmitNativeCall(m_chunk, native->id, 0, m_currentLine);
	}
}

void CodeGenerator::EmitUserFunctionCall(const FunctionCallExpr& node)
{
	for (const auto& arg : node.GetArgs())
	{
		arg->Accept(*this);
	}

	const auto argCount = static_cast<uint32_t>(node.GetArgs().size());

	if (const auto* native = NativeRegistry::FindByName(node.GetName()))
	{
		EmitNativeCall(m_chunk, native->id, argCount, m_currentLine);
		return;
	}

	const uint32_t patchOffset = EmitCallWithPlaceholder(m_chunk, argCount, m_currentLine);
	m_unresolvedCalls.push_back({patchOffset, node.GetName(), node.GetRange()});
}

Chunk CodeGenerator::Generate(const AstNode& root)
{
	m_chunk.WriteOpCode(OpCode::Call, m_currentLine);
	const uint32_t mainPatchOffset = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);
	m_chunk.WriteUint32(0, m_currentLine);
	m_unresolvedCalls.push_back({mainPatchOffset, "main", SourceRange{}});

	m_chunk.WriteOpCode(OpCode::Return, m_currentLine);

	root.Accept(*this);

	for (const auto& call : m_unresolvedCalls)
	{
		AssertIsFunctionOffsetResolved(m_context.functions.contains(call.funcName), call.funcName, call.range);
		m_chunk.PatchUint32(call.patchOffset, m_functionOffsets[call.funcName]);
	}

	return std::move(m_chunk);
}

void CodeGenerator::Visit(const ProgramNode& node)
{
	for (const auto& decl : node.GetDeclarations())
	{
		decl->Accept(*this);
	}
}

void CodeGenerator::Visit(const StructDeclStmt& /*node*/)
{
	throw std::runtime_error("Генерация кода для StructDeclStmt не реализована");
}

void CodeGenerator::Visit(const UnionDeclStmt& /*node*/)
{
	throw std::runtime_error("Генерация кода для UnionDeclStmt не реализована");
}

void CodeGenerator::Visit(const EnumDeclStmt& /*node*/)
{
}

void CodeGenerator::Visit(const BinaryExpr& node)
{
	const auto op = node.GetOp();

	if (op == BinaryOpKind::LogicalAnd)
	{
		node.GetLeft().Accept(*this);
		m_chunk.WriteOpCode(OpCode::Dup, m_currentLine);
		m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
		const uint32_t jumpPatch = m_chunk.GetCodeSize();
		m_chunk.WriteUint32(0, m_currentLine);

		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
		node.GetRight().Accept(*this);

		m_chunk.PatchUint32(jumpPatch, m_chunk.GetCodeSize());
		return;
	}

	if (op == BinaryOpKind::LogicalOr)
	{
		node.GetLeft().Accept(*this);
		m_chunk.WriteOpCode(OpCode::Dup, m_currentLine);
		m_chunk.WriteOpCode(OpCode::JumpIfTrue, m_currentLine);
		const uint32_t jumpPatch = m_chunk.GetCodeSize();
		m_chunk.WriteUint32(0, m_currentLine);

		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
		node.GetRight().Accept(*this);

		m_chunk.PatchUint32(jumpPatch, m_chunk.GetCodeSize());
		return;
	}

	if (op == BinaryOpKind::Greater || op == BinaryOpKind::LessEq)
	{
		node.GetRight().Accept(*this);
		node.GetLeft().Accept(*this);
	}
	else
	{
		node.GetLeft().Accept(*this);
		node.GetRight().Accept(*this);
	}

	const auto opCode = GetBinaryOpCode(op, GetType(node.GetLeft()).get());
	m_chunk.WriteOpCode(opCode, m_currentLine);

	if (op == BinaryOpKind::NotEq || op == BinaryOpKind::LessEq || op == BinaryOpKind::GreaterEq)
	{
		EmitLogicalNot();
	}
}

void CodeGenerator::Visit(const UnaryExpr& node)
{
	if (node.GetOp() == UnaryOpKind::LogicalNot)
	{
		node.GetOperand().Accept(*this);
		m_chunk.WriteOpCode(OpCode::LogicalNot, m_currentLine);
		return;
	}

	if (node.GetOp() == UnaryOpKind::Minus)
	{
		const auto type = GetType(node.GetOperand());
		const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(type.get());

		if (scalar && scalar->GetBaseType() == BaseType::Real)
		{
			EmitConstant(Value(0.0));
		}
		else
		{
			EmitConstant(Value(static_cast<int64_t>(0)));
		}

		node.GetOperand().Accept(*this);

		const auto opCode = GetBinaryOpCode(BinaryOpKind::Sub, type.get());
		m_chunk.WriteOpCode(opCode, m_currentLine);
		return;
	}

	throw std::runtime_error("Генерация кода для других UnaryExpr пока не реализована");
}

void CodeGenerator::Visit(const IdentifierExpr& node)
{
	const auto it = m_context.symbols.find(&node);
	AssertIsIdentifierResolved(it != m_context.symbols.end(), node.GetName(), node.GetRange());

	if (it->second.isCompileTimeConst)
	{
		it->second.constExpr->Accept(*this);
		return;
	}

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(it->second.stackSlot, m_currentLine);
}
void CodeGenerator::Visit(const NumExpr& node)
{
	const Value val = ParseNumLiteral(node.GetValue(), GetType(node).get(), node.IsFloat());
	EmitConstant(val);
}

void CodeGenerator::Visit(const StringExpr& node)
{
	const uint8_t idx = m_chunk.AddString(node.GetValue());
	m_chunk.WriteOpCode(OpCode::LoadString, m_currentLine);
	m_chunk.WriteUint32(static_cast<uint32_t>(idx), m_currentLine);
}

void CodeGenerator::Visit(const BoolExpr& node)
{
	const uint32_t val = node.GetValue() ? 1 : 0;
	EmitConstant(Value(val));
}

void CodeGenerator::Visit(const ArrayLiteralExpr& node)
{
	const auto& elements = node.GetElements();
	const uint32_t size = static_cast<uint32_t>(elements.size());

	EmitConstant(Value(size));
	m_chunk.WriteOpCode(OpCode::AllocateArray, m_currentLine);

	for (uint32_t i = 0; i < size; ++i)
	{
		m_chunk.WriteOpCode(OpCode::Dup, m_currentLine);
		EmitConstant(Value(i));
		elements[i]->Accept(*this);
		m_chunk.WriteOpCode(OpCode::StoreElement, m_currentLine);
	}
}

void CodeGenerator::Visit(const FunctionCallExpr& node)
{
	if (IsPrimitiveType(node.GetName()))
	{
		EmitCast(node);
		return;
	}

	const auto& builtins = GetBuiltinEmitters();
	if (const auto it = builtins.find(node.GetName()); it != builtins.end())
	{
		it->second(*this, node);
		return;
	}

	EmitUserFunctionCall(node);
}

void CodeGenerator::Visit(const IndexExpr& node)
{
	node.GetReceiver().Accept(*this);
	node.GetIndex().Accept(*this);
	m_chunk.WriteOpCode(OpCode::LoadElement, m_currentLine);
}

void CodeGenerator::Visit(const MemberAccessExpr& node)
{
	if (const auto* enumType = dynamic_cast<const EnumTypeInfo*>(GetType(node).get()))
	{
		const auto& fields = enumType->GetFields();
		auto it = std::ranges::find(fields, node.GetField());
		if (it != fields.end())
		{
			const uint32_t index = static_cast<uint32_t>(std::distance(fields.begin(), it));
			EmitConstant(Value(static_cast<uint64_t>(index)));
			return;
		}
	}

	node.GetReceiver().Accept(*this);

	// todo это не должно быть натиной функцией?
	if (node.GetField() == "size")
	{
		m_chunk.WriteOpCode(OpCode::ArrayLength, m_currentLine);
		return;
	}

	throw std::runtime_error("Бэкенд: обращение к неизвестному полю");
}

void CodeGenerator::Visit(const BlockStmt& node)
{
	const uint32_t savedCount = m_localCount;
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
	const uint32_t pops = m_localCount - savedCount;
	for (uint32_t i = 0; i < pops; ++i)
	{
		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
	}
	m_localCount = savedCount;
}

void CodeGenerator::Visit(const DoWhileStmt& node)
{
	const uint32_t startIp = m_chunk.GetCodeSize();

	m_loopStack.push_back(LoopContext{m_localCount, 0, false, 0, {}, {}});

	node.GetBody().Accept(*this);

	const uint32_t conditionIp = m_chunk.GetCodeSize();

	node.GetCondition().Accept(*this);

	m_chunk.WriteOpCode(OpCode::JumpIfTrue, m_currentLine);
	m_chunk.WriteUint32(startIp, m_currentLine);

	const uint32_t breakIp = m_chunk.GetCodeSize();

	LoopContext ctx = std::move(m_loopStack.back());
	m_loopStack.pop_back();

	for (uint32_t patch : ctx.breakPatches)
		m_chunk.PatchUint32(patch, breakIp);
	for (uint32_t patch : ctx.continuePatches)
		m_chunk.PatchUint32(patch, conditionIp);
}

void CodeGenerator::Visit(const RunStmt& node)
{
	const uint32_t checkIp = m_chunk.GetCodeSize();

	node.GetCondition().Accept(*this);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	m_loopStack.push_back(LoopContext{m_localCount, checkIp, true, 0, {}, {}});

	node.GetBody().Accept(*this);

	LoopContext ctx = std::move(m_loopStack.back());
	m_loopStack.pop_back();

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	const uint32_t endIp = m_chunk.GetCodeSize();
	m_chunk.PatchUint32(patchEnd, endIp);

	for (uint32_t patch : ctx.breakPatches)
		m_chunk.PatchUint32(patch, endIp);
}

void CodeGenerator::Visit(const BreakStmt& /*node*/)
{
	auto& ctx = m_loopStack.back();
	const uint32_t bodyLocals = m_localCount - ctx.localCountAtLoopEntry;
	for (uint32_t i = 0; i < bodyLocals; ++i)
		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
	for (uint32_t i = 0; i < ctx.breakPopsNeeded; ++i)
		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	ctx.breakPatches.push_back(m_chunk.GetCodeSize());
	m_chunk.WriteUint32(0, m_currentLine);
}

void CodeGenerator::Visit(const ContinueStmt& /*node*/)
{
	auto& ctx = m_loopStack.back();
	const uint32_t bodyLocals = m_localCount - ctx.localCountAtLoopEntry;
	for (uint32_t i = 0; i < bodyLocals; ++i)
		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	if (ctx.continueTargetKnown)
	{
		m_chunk.WriteUint32(ctx.continueTarget, m_currentLine);
	}
	else
	{
		ctx.continuePatches.push_back(m_chunk.GetCodeSize());
		m_chunk.WriteUint32(0, m_currentLine);
	}
}

void CodeGenerator::Visit(const ClassicForStmt& node)
{
	const auto it = m_context.classicForInits.find(&node);
	AssertIsIdentifierResolved(it != m_context.classicForInits.end(), node.GetInitName(), node.GetRange());

	node.GetInitExpr().Accept(*this);

	const uint32_t checkIp = m_chunk.GetCodeSize();

	node.GetCondition().Accept(*this);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	m_loopStack.push_back(LoopContext{m_localCount, 0, false, 1, {}, {}});

	node.GetBody().Accept(*this);

	const uint32_t stepIp = m_chunk.GetCodeSize();
	node.GetStep().Accept(*this);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	const uint32_t endIp = m_chunk.GetCodeSize();
	m_chunk.PatchUint32(patchEnd, endIp);
	m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);

	const uint32_t afterPopIp = m_chunk.GetCodeSize();

	LoopContext ctx = std::move(m_loopStack.back());
	m_loopStack.pop_back();

	for (uint32_t patch : ctx.breakPatches)
		m_chunk.PatchUint32(patch, afterPopIp);
	for (uint32_t patch : ctx.continuePatches)
		m_chunk.PatchUint32(patch, stepIp);
}

void CodeGenerator::Visit(const RepStmt& node)
{
	const auto repIt = m_context.repIterators.find(&node);
	AssertIsIdentifierResolved(repIt != m_context.repIterators.end(), node.GetIterators()[0], node.GetRange());
	const uint32_t iterSlot = repIt->second[0].stackSlot;

	node.GetRanges()[0]->Accept(*this);

	const uint32_t checkIp = m_chunk.GetCodeSize();

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	node.GetRanges()[1]->Accept(*this);
	const auto iterType = GetType(*node.GetRanges()[0]);
	m_chunk.WriteOpCode(GetBinaryOpCode(BinaryOpKind::Less, iterType.get()), m_currentLine);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	m_loopStack.push_back(LoopContext{m_localCount, 0, false, 1, {}, {}});

	node.GetOriginalBody().Accept(*this);

	const uint32_t stepIp = m_chunk.GetCodeSize();

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);
	EmitConstant(Value(static_cast<int64_t>(1)));
	m_chunk.WriteOpCode(GetBinaryOpCode(BinaryOpKind::Add, iterType.get()), m_currentLine);
	m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	const uint32_t endIp = m_chunk.GetCodeSize();
	m_chunk.PatchUint32(patchEnd, endIp);
	m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);

	const uint32_t afterPopIp = m_chunk.GetCodeSize();

	LoopContext ctx = std::move(m_loopStack.back());
	m_loopStack.pop_back();

	for (uint32_t patch : ctx.breakPatches)
		m_chunk.PatchUint32(patch, afterPopIp);
	for (uint32_t patch : ctx.continuePatches)
		m_chunk.PatchUint32(patch, stepIp);
}

void CodeGenerator::Visit(const VarDeclStmt& node)
{
	if (node.GetModifier() == VarModifier::Def)
	{
		return;
	}

	if (node.GetInit())
	{
		node.GetInit()->Accept(*this);
	}
	else
	{
		EmitConstant(Value(static_cast<uint32_t>(0)));
	}
	++m_localCount;
}

void CodeGenerator::Visit(const AssignStmt& node)
{
	if (const auto* id = dynamic_cast<const IdentifierExpr*>(&node.GetLhs()))
	{
		node.GetRhs().Accept(*this);
		const auto it = m_context.symbols.find(id);
		AssertIsIdentifierResolved(it != m_context.symbols.end(), id->GetName(), node.GetRange());

		m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
		m_chunk.WriteUint32(it->second.stackSlot, m_currentLine);
		return;
	}

	if (const auto* indexExpr = dynamic_cast<const IndexExpr*>(&node.GetLhs()))
	{
		indexExpr->GetReceiver().Accept(*this);
		indexExpr->GetIndex().Accept(*this);
		node.GetRhs().Accept(*this);

		m_chunk.WriteOpCode(OpCode::StoreElement, m_currentLine);
		return;
	}

	AssertIsLhsIdentifier(false, node.GetRange());
}

void CodeGenerator::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
	m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
}

void CodeGenerator::Visit(const FunctionDeclStmt& node)
{
	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	const uint32_t jumpPatch = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	m_functionOffsets[node.GetName()] = m_chunk.GetCodeSize();
	node.GetBody().Accept(*this);

	EmitConstant(Value(static_cast<uint32_t>(0)));
	m_chunk.WriteOpCode(OpCode::Return, m_currentLine);

	m_chunk.PatchUint32(jumpPatch, m_chunk.GetCodeSize());
}

void CodeGenerator::Visit(const ReturnStmt& node)
{
	if (node.GetValue())
	{
		node.GetValue()->Accept(*this);
	}
	else
	{
		EmitConstant(Value(static_cast<uint32_t>(0)));
	}
	m_chunk.WriteOpCode(OpCode::Return, m_currentLine);
}

void CodeGenerator::Visit(const IfStmt& node)
{
	node.GetCondition().Accept(*this);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchElse = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	node.GetThenBlock().Accept(*this);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	m_chunk.PatchUint32(patchElse, m_chunk.GetCodeSize());

	if (node.GetElseNode())
	{
		node.GetElseNode()->Accept(*this);
	}

	m_chunk.PatchUint32(patchEnd, m_chunk.GetCodeSize());
}

void CodeGenerator::Visit(const ArrayAllocExpr& node)
{
	node.GetSize().Accept(*this);
	m_chunk.WriteOpCode(OpCode::AllocateArray, m_currentLine);
}

void CodeGenerator::Visit(const WhenStmt& node)
{
	const bool hasSubject = node.GetSubject() != nullptr;
	if (hasSubject)
	{
		node.GetSubject()->Accept(*this);
	}

	std::vector<uint32_t> endJumps;

	for (const auto& entry : node.GetEntries())
	{
		std::vector<uint32_t> nextBranchJumps;
		std::vector<uint32_t> matchJumps;

		for (size_t i = 0; i < entry.conditions.size(); ++i)
		{
			if (hasSubject)
			{
				m_chunk.WriteOpCode(OpCode::Dup, m_currentLine);
				entry.conditions[i]->Accept(*this);
				const auto opCode = GetBinaryOpCode(BinaryOpKind::Eq, GetType(node).get());
				m_chunk.WriteOpCode(opCode, m_currentLine);
			}
			else
			{
				entry.conditions[i]->Accept(*this);
			}

			if (i + 1 < entry.conditions.size())
			{
				m_chunk.WriteOpCode(OpCode::JumpIfTrue, m_currentLine);
				matchJumps.push_back(m_chunk.GetCodeSize());
				m_chunk.WriteUint32(0, m_currentLine);
			}
			else
			{
				m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
				nextBranchJumps.push_back(m_chunk.GetCodeSize());
				m_chunk.WriteUint32(0, m_currentLine);
			}
		}

		for (uint32_t jump : matchJumps)
		{
			m_chunk.PatchUint32(jump, m_chunk.GetCodeSize());
		}

		if (hasSubject)
		{
			m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
		}

		entry.body->Accept(*this);

		m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
		endJumps.push_back(m_chunk.GetCodeSize());
		m_chunk.WriteUint32(0, m_currentLine);

		for (uint32_t jump : nextBranchJumps)
		{
			m_chunk.PatchUint32(jump, m_chunk.GetCodeSize());
		}
	}

	if (node.GetElseBody())
	{
		if (hasSubject)
		{
			m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
		}
		node.GetElseBody()->Accept(*this);
	}
	else if (hasSubject)
	{
		m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
	}

	const uint32_t currentSize = m_chunk.GetCodeSize();
	for (uint32_t jump : endJumps)
	{
		m_chunk.PatchUint32(jump, currentSize);
	}
}

std::shared_ptr<TypeInfo> CodeGenerator::GetType(const AstNode& node) const
{
	auto it = m_context.annotations.resolvedTypes.find(&node);
	AssertIsTypeResolved(it != m_context.annotations.resolvedTypes.end());
	return it->second;
}

void CodeGenerator::EmitLogicalNot()
{
	EmitConstant(Value(static_cast<uint32_t>(0)));
	m_chunk.WriteOpCode(OpCode::EqInt, m_currentLine);
}

void CodeGenerator::EmitConstant(Value value)
{
	const uint8_t idx = m_chunk.AddConstant(value);
	m_chunk.WriteOpCode(OpCode::Constant, m_currentLine);
	m_chunk.WriteByte(idx, m_currentLine);
}