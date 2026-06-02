#include "CodeGenerator.h"
#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
#include "src/compiler/ast/ExprStmt.h"
#include "src/compiler/ast/FunctionCallExpr.h"
#include "src/compiler/ast/FunctionDeclStmt.h"
#include "src/compiler/ast/IdentifierExpr.h"
#include "src/compiler/ast/IfStmt.h"
#include "src/compiler/ast/NumExpr.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/StringExpr.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/sema/SymbolTable.h"
#include "src/compiler/stdlib/NativeRegistry.h"
#include "src/compiler/vm/value/Value.h"
#include "src/diagnostics/CompilationException.h"

#include <stdexcept>
#include <unordered_map>

namespace
{
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
		if (scalar->GetBaseType() == BaseType::Uint) return Value(static_cast<uint64_t>(std::stoull(text)));
		return Value(static_cast<int64_t>(std::stoll(text)));
	}

	return isFloat ? Value(std::stod(text)) : Value(static_cast<int64_t>(std::stoll(text)));
}

OpCode GetBinaryOpCode(BinaryOpKind op, const TypeInfo* typeInfo)
{
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(typeInfo);
	if (!scalar) return OpCode::AddInt;

	const auto base = scalar->GetBaseType();

	switch (op)
	{
	case BinaryOpKind::Add:
		if (base == BaseType::Uint) return OpCode::AddUint;
		if (base == BaseType::Real) return OpCode::AddReal;
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

CodeGenerator::CodeGenerator(
	std::unordered_map<const AstNode*, SymbolInfo> symbols,
	std::unordered_map<const AstNode*, std::vector<SymbolInfo>> repIterators,
	std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> functions)
	: m_symbols(std::move(symbols))
	, m_repIterators(std::move(repIterators))
	, m_functions(std::move(functions))
{
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
		AssertIsFunctionOffsetResolved(m_functionOffsets.contains(call.funcName), call.funcName, call.range);
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
	throw std::runtime_error("Генерация кода для EnumDeclStmt не реализована");
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

	const auto opCode = GetBinaryOpCode(op, node.GetLeft().GetResolvedType().get());
	m_chunk.WriteOpCode(opCode, m_currentLine);

	if (op == BinaryOpKind::NotEq || op == BinaryOpKind::LessEq || op == BinaryOpKind::GreaterEq)
	{
		EmitLogicalNot();
	}
}

void CodeGenerator::Visit(const UnaryExpr& node)
{
	if (node.GetOp() == UnaryOpKind::Minus)
	{
		const auto type = node.GetOperand().GetResolvedType();
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
	const auto it = m_symbols.find(&node);
	AssertIsIdentifierResolved(it != m_symbols.end(), node.GetName(), node.GetRange());
	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(it->second.stackSlot, m_currentLine);
}

void CodeGenerator::Visit(const NumExpr& node)
{
	const Value val = ParseNumLiteral(node.GetValue(), node.GetResolvedType().get(), node.IsFloat());
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

void CodeGenerator::Visit(const ArrayLiteralExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для ArrayLiteralExpr не реализована");
}

void CodeGenerator::Visit(const FunctionCallExpr& node)
{
	if (IsPrimitiveType(node.GetName()))
	{
		node.GetArgs()[0]->Accept(*this);

		const std::string srcBase = ExtractBaseTypeName(node.GetArgs()[0]->GetResolvedType().get());
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
				throw CompilationException(DiagnosticData{
					.phase = CompilerPhase::Backend,
					.message = "Бэкенд: не реализовано нативное преобразование " + nativeName,
					.line = node.GetRange().start.line,
					.pos = node.GetRange().start.pos});
			}
		}
		return;
	}

	if (node.GetName() == "print")
	{
		const auto argCount = static_cast<uint32_t>(node.GetArgs().size());

		for (uint32_t i = 0; i < argCount; ++i)
		{
			node.GetArgs()[i]->Accept(*this);

			const std::string nativeName = NativeRegistry::ResolvePrintNative(node.GetArgs()[i]->GetResolvedType().get());
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

		if (const auto* native = NativeRegistry::FindByName("print_newline"))
		{
			EmitNativeCall(m_chunk, native->id, 0, m_currentLine);
		}

		return;
	}

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

void CodeGenerator::Visit(const IndexExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для IndexExpr не реализована");
}

void CodeGenerator::Visit(const MemberAccessExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для MemberAccessExpr не реализована");
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

void CodeGenerator::Visit(const DoWhileStmt& /*node*/)
{
	throw std::runtime_error("Генерация кода для DoWhileStmt не реализована");
}

void CodeGenerator::Visit(const RunStmt& node)
{
	const uint32_t checkIp = m_chunk.GetCodeSize();

	node.GetCondition().Accept(*this);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	node.GetBody().Accept(*this);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	m_chunk.PatchUint32(patchEnd, m_chunk.GetCodeSize());
}

void CodeGenerator::Visit(const RepStmt& node)
{
	const auto repIt = m_repIterators.find(&node);
	AssertIsIdentifierResolved(repIt != m_repIterators.end(), node.GetIterators()[0], node.GetRange());
	const uint32_t iterSlot = repIt->second[0].stackSlot;

	node.GetRanges()[0]->Accept(*this);

	const uint32_t checkIp = m_chunk.GetCodeSize();

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	node.GetRanges()[1]->Accept(*this);

	m_chunk.WriteOpCode(OpCode::AddInt, m_currentLine);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	node.GetOriginalBody().Accept(*this);

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);
	EmitConstant(Value(static_cast<uint32_t>(1)));
	m_chunk.WriteOpCode(OpCode::AddInt, m_currentLine);
	m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	m_chunk.PatchUint32(patchEnd, m_chunk.GetCodeSize());
	m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
}

void CodeGenerator::Visit(const VarDeclStmt& node)
{
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
	node.GetRhs().Accept(*this);

	const auto* id = dynamic_cast<const IdentifierExpr*>(&node.GetLhs());
	AssertIsLhsIdentifier(id != nullptr, node.GetRange());

	const auto it = m_symbols.find(id);
	AssertIsIdentifierResolved(it != m_symbols.end(), id->GetName(), node.GetRange());

	m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
	m_chunk.WriteUint32(it->second.stackSlot, m_currentLine);
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