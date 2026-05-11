#include "CodeGenerator.h"

#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/AssignStmt.h"
#include "src/compiler/ast/BinaryExpr.h"
#include "src/compiler/ast/BlockStmt.h"
#include "src/compiler/ast/BoolExpr.h"
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
#include "src/compiler/vm/value/Value.h"

#include <stdexcept>
#include <unordered_map>

namespace
{
void AssertIsIdentifierResolved(bool found, const std::string& name)
{
	if (!found)
	{
		throw std::runtime_error("Переменная не найдена при генерации кода: " + name);
	}
}

void AssertIsLhsIdentifier(bool isId)
{
	if (!isId)
	{
		throw std::runtime_error("Левая часть присваивания должна быть идентификатором");
	}
}
} // namespace

CodeGenerator::CodeGenerator(std::unordered_map<std::string, uint32_t> slotMap)
	: m_slotMap(std::move(slotMap))
{
}

Chunk CodeGenerator::Generate(const AstNode& root)
{
	root.Accept(*this);
	return std::move(m_chunk);
}

void CodeGenerator::EmitLogicalNot()
{
	EmitConstantU32(0);
	m_chunk.WriteOpCode(OpCode::EqU32, m_currentLine);
}

void CodeGenerator::EmitConstantU32(uint32_t value)
{
	const uint8_t idx = m_chunk.AddConstant(Value(value));
	m_chunk.WriteOpCode(OpCode::Constant, m_currentLine);
	m_chunk.WriteByte(idx, m_currentLine);
}

void CodeGenerator::Visit(const ProgramNode& node)
{
	for (const auto& decl : node.GetDeclarations())
	{
		decl->Accept(*this);
	}
}

void CodeGenerator::Visit(const UnaryExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для UnaryExpr не реализована");
}

void CodeGenerator::Visit(const StringExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для StringExpr не реализована");
}

void CodeGenerator::Visit(const BoolExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для BoolExpr не реализована");
}

void CodeGenerator::Visit(const ArrayLiteralExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для ArrayLiteralExpr не реализована");
}

void CodeGenerator::Visit(const IndexExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для IndexExpr не реализована");
}

void CodeGenerator::Visit(const MemberAccessExpr& /*node*/)
{
	throw std::runtime_error("Генерация кода для MemberAccessExpr не реализована");
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

void CodeGenerator::Visit(const FunctionDeclStmt& node)
{
	node.GetBody().Accept(*this);
}

void CodeGenerator::Visit(const BlockStmt& node)
{
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
}

void CodeGenerator::Visit(const VarDeclStmt& node)
{
	if (node.GetInit())
	{
		node.GetInit()->Accept(*this);
	}
	else
	{
		EmitConstantU32(0);
	}
}

void CodeGenerator::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
	m_chunk.WriteOpCode(OpCode::Pop, m_currentLine);
}

void CodeGenerator::Visit(const AssignStmt& node)
{
	node.GetRhs().Accept(*this);

	const auto* id = dynamic_cast<const IdentifierExpr*>(&node.GetLhs());
	AssertIsLhsIdentifier(id != nullptr);

	const auto it = m_slotMap.find(id->GetName());
	AssertIsIdentifierResolved(it != m_slotMap.end(), id->GetName());

	m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
	m_chunk.WriteUint32(it->second, m_currentLine);
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

void CodeGenerator::Visit(const RepStmt& node)
{
	const std::string& iterName = node.GetIterators()[0];
	const auto it = m_slotMap.find(iterName);
	AssertIsIdentifierResolved(it != m_slotMap.end(), iterName);
	const uint32_t iterSlot = it->second;

	node.GetRanges()[0]->Accept(*this);

	const uint32_t checkIp = m_chunk.GetCodeSize();

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	node.GetRanges()[1]->Accept(*this);

	m_chunk.WriteOpCode(OpCode::LtU32, m_currentLine);

	m_chunk.WriteOpCode(OpCode::JumpIfFalse, m_currentLine);
	const uint32_t patchEnd = m_chunk.GetCodeSize();
	m_chunk.WriteUint32(0, m_currentLine);

	node.GetOriginalBody().Accept(*this);

	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);
	EmitConstantU32(1);
	m_chunk.WriteOpCode(OpCode::AddU32, m_currentLine);
	m_chunk.WriteOpCode(OpCode::StoreLocal, m_currentLine);
	m_chunk.WriteUint32(iterSlot, m_currentLine);

	m_chunk.WriteOpCode(OpCode::Jump, m_currentLine);
	m_chunk.WriteUint32(checkIp, m_currentLine);

	m_chunk.PatchUint32(patchEnd, m_chunk.GetCodeSize());
}

void CodeGenerator::Visit(const ReturnStmt& node)
{
	if (node.GetValue())
	{
		node.GetValue()->Accept(*this);
	}
	else
	{
		EmitConstantU32(0);
	}
	m_chunk.WriteOpCode(OpCode::Return, m_currentLine);
}

void CodeGenerator::Visit(const BinaryExpr& node)
{
	node.GetLeft().Accept(*this);
	node.GetRight().Accept(*this);

	switch (node.GetOp())
	{
	case BinaryOpKind::Add:
		m_chunk.WriteOpCode(OpCode::AddU32, m_currentLine);
		break;
	case BinaryOpKind::Sub:
		m_chunk.WriteOpCode(OpCode::SubU32, m_currentLine);
		break;
	case BinaryOpKind::Mul:
		m_chunk.WriteOpCode(OpCode::MulU32, m_currentLine);
		break;
	case BinaryOpKind::Div:
		m_chunk.WriteOpCode(OpCode::DivU32, m_currentLine);
		break;
	case BinaryOpKind::Mod:
		m_chunk.WriteOpCode(OpCode::RemU32, m_currentLine);
		break;
	case BinaryOpKind::Eq:
		m_chunk.WriteOpCode(OpCode::EqU32, m_currentLine);
		break;
	case BinaryOpKind::NotEq:
		m_chunk.WriteOpCode(OpCode::EqU32, m_currentLine);
		EmitLogicalNot();
		break;
	case BinaryOpKind::Less:
		m_chunk.WriteOpCode(OpCode::LtU32, m_currentLine);
		break;
	case BinaryOpKind::Greater:
		m_chunk.WriteOpCode(OpCode::LtU32, m_currentLine);
		EmitLogicalNot();
		break;
	case BinaryOpKind::LessEq:
		m_chunk.WriteOpCode(OpCode::LtU32, m_currentLine);
		EmitLogicalNot();
		EmitLogicalNot();
		break;
	case BinaryOpKind::GreaterEq:
		m_chunk.WriteOpCode(OpCode::LtU32, m_currentLine);
		EmitLogicalNot();
		break;
	case BinaryOpKind::LogicalAnd:
		m_chunk.WriteOpCode(OpCode::BitAnd, m_currentLine);
		break;
	case BinaryOpKind::LogicalOr:
		m_chunk.WriteOpCode(OpCode::BitOr, m_currentLine);
		break;
	}
}

void CodeGenerator::Visit(const NumberExpr& node)
{
	const uint32_t value = static_cast<uint32_t>(std::stoul(node.GetValue()));
	EmitConstantU32(value);
}

void CodeGenerator::Visit(const IdentifierExpr& node)
{
	const auto it = m_slotMap.find(node.GetName());
	AssertIsIdentifierResolved(it != m_slotMap.end(), node.GetName());
	m_chunk.WriteOpCode(OpCode::LoadLocal, m_currentLine);
	m_chunk.WriteUint32(it->second, m_currentLine);
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

void CodeGenerator::Visit(const ErrorExpr& /*node*/)
{
	throw std::runtime_error("Попытка генерации кода для узла ошибки");
}

void CodeGenerator::Visit(const ImplicitCastExpr& /*node*/)
{
	throw std::runtime_error("Неявное приведение типов не поддерживается в кодогенераторе");
}

void CodeGenerator::Visit(const FunctionCallExpr& node)
{
	for (const auto& arg : node.GetArgs())
	{
		arg->Accept(*this);
	}

	const auto argCount = static_cast<uint32_t>(node.GetArgs().size());

	static const std::unordered_map<std::string, uint32_t> nativeIds = {
		{"print", 0},
	};

	const auto it = nativeIds.find(node.GetName());
	if (it == nativeIds.end())
	{
		throw std::runtime_error("Неизвестная нативная функция: " + node.GetName());
	}

	m_chunk.WriteOpCode(OpCode::CallNative, m_currentLine);
	m_chunk.WriteUint32(it->second, m_currentLine);
	m_chunk.WriteUint32(argCount, m_currentLine);
}
