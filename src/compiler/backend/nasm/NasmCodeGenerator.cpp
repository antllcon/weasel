#include "NasmCodeGenerator.h"

#include "src/compiler/ast/ArrayLiteralExpr.h"
#include "src/compiler/ast/DoWhileStmt.h"
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

#include <stdexcept>
#include <vector>

namespace
{
const char* ArgRegisters[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
constexpr int MaxRegArgs = 6;

uint32_t CountVarDecls(const Stmt& stmt);

uint32_t CountVarDeclsInBlock(const BlockStmt& block)
{
	uint32_t count = 0;
	for (const auto& s : block.GetStmts())
	{
		count += CountVarDecls(*s);
	}
	return count;
}

uint32_t CountVarDecls(const Stmt& stmt)
{
	if (const auto* v = dynamic_cast<const VarDeclStmt*>(&stmt))
	{
		(void)v;
		return 1;
	}
	if (const auto* b = dynamic_cast<const BlockStmt*>(&stmt))
	{
		return CountVarDeclsInBlock(*b);
	}
	if (const auto* i = dynamic_cast<const IfStmt*>(&stmt))
	{
		uint32_t count = CountVarDeclsInBlock(i->GetThenBlock());
		if (const Stmt* el = i->GetElseNode())
		{
			count += CountVarDecls(*el);
		}
		return count;
	}
	if (const auto* r = dynamic_cast<const RunStmt*>(&stmt))
	{
		return CountVarDecls(r->GetBody());
	}
	if (const auto* r = dynamic_cast<const RepStmt*>(&stmt))
	{
		return 1 + CountVarDecls(r->GetOriginalBody());
	}
	return 0;
}

uint32_t AlignTo16(uint32_t size)
{
	return (size + 15) & ~15u;
}
}

NasmCodeGenerator::NasmCodeGenerator() = default;

std::string NasmCodeGenerator::Generate(const AstNode& root)
{
	m_out << "bits 64\n";
	m_out << "default rel\n\n";
	m_out << "extern printf\n\n";
	m_out << "section .data\n";
	m_out << "    _fmt_int  db \"%lld\", 10, 0\n";
	m_out << "    _fmt_str  db \"%s\", 10, 0\n\n";
	m_out << "section .text\n\n";

	root.Accept(*this);
	return m_out.str();
}

std::string NasmCodeGenerator::MakeLabel(const std::string& prefix)
{
	return "." + prefix + "_" + std::to_string(m_labelCounter++);
}

int32_t NasmCodeGenerator::GetVarOffset(const std::string& name) const
{
	const auto it = m_varOffsets.find(name);
	if (it == m_varOffsets.end())
	{
		throw std::runtime_error("Переменная не найдена при генерации NASM: " + name);
	}
	return it->second;
}

void NasmCodeGenerator::Emit(const std::string& line)
{
	m_out << "    " << line << "\n";
}

void NasmCodeGenerator::EmitLabel(const std::string& label)
{
	m_out << label << ":\n";
}

void NasmCodeGenerator::EmitExprToRax(const Expr& expr)
{
	expr.Accept(*this);
}

void NasmCodeGenerator::EnterFunction(const FunctionDeclStmt& node)
{
	m_varOffsets.clear();
	m_nextOffset = 0;

	const uint32_t paramCount = static_cast<uint32_t>(node.GetParams().size());
	const uint32_t localCount = CountVarDeclsInBlock(node.GetBody());
	const uint32_t totalSlots = paramCount + localCount;
	const uint32_t frameSize = AlignTo16(totalSlots * 8 + 8);

	Emit("push rbp");
	Emit("mov rbp, rsp");
	Emit("sub rsp, " + std::to_string(frameSize));

	for (uint32_t i = 0; i < paramCount; ++i)
	{
		m_nextOffset -= 8;
		m_varOffsets[node.GetParams()[i].name] = m_nextOffset;

		if (i < static_cast<uint32_t>(MaxRegArgs))
		{
			Emit("mov [rbp" + std::to_string(m_nextOffset) + "], " + ArgRegisters[i]);
		}
	}
}

void NasmCodeGenerator::LeaveFunction()
{
	Emit("mov rsp, rbp");
	Emit("pop rbp");
	Emit("ret");
}

void NasmCodeGenerator::Visit(const ProgramNode& node)
{
	std::vector<std::string> funcNames;
	for (const auto& decl : node.GetDeclarations())
	{
		if (const auto* f = dynamic_cast<const FunctionDeclStmt*>(decl.get()))
		{
			funcNames.push_back(f->GetName());
		}
	}

	for (const auto& name : funcNames)
	{
		m_out << "global " << name << "\n";
	}
	m_out << "\n";

	for (const auto& decl : node.GetDeclarations())
	{
		decl->Accept(*this);
	}
}

void NasmCodeGenerator::Visit(const FunctionDeclStmt& node)
{
	m_out << node.GetName() << ":\n";
	EnterFunction(node);
	node.GetBody().Accept(*this);

	if (node.GetReturnTypeName() == "voided")
	{
		Emit("xor rax, rax");
		LeaveFunction();
	}

	m_out << "\n";
}

void NasmCodeGenerator::Visit(const BlockStmt& node)
{
	for (const auto& stmt : node.GetStmts())
	{
		stmt->Accept(*this);
	}
}

void NasmCodeGenerator::Visit(const VarDeclStmt& node)
{
	m_nextOffset -= 8;
	m_varOffsets[node.GetName()] = m_nextOffset;

	if (node.GetInit())
	{
		EmitExprToRax(*node.GetInit());
		Emit("mov [rbp" + std::to_string(m_nextOffset) + "], rax");
	}
	else
	{
		Emit("mov qword [rbp" + std::to_string(m_nextOffset) + "], 0");
	}
}

void NasmCodeGenerator::Visit(const AssignStmt& node)
{
	const auto* lhs = dynamic_cast<const IdentifierExpr*>(&node.GetLhs());
	if (!lhs)
	{
		throw std::runtime_error("NASM: левая часть присваивания должна быть идентификатором");
	}
	EmitExprToRax(node.GetRhs());
	const int32_t offset = GetVarOffset(lhs->GetName());
	Emit("mov [rbp" + std::to_string(offset) + "], rax");
}

void NasmCodeGenerator::Visit(const ReturnStmt& node)
{
	if (node.GetValue())
	{
		EmitExprToRax(*node.GetValue());
	}
	else
	{
		Emit("xor rax, rax");
	}
	LeaveFunction();
}

void NasmCodeGenerator::Visit(const ExprStmt& node)
{
	node.GetExpr().Accept(*this);
}

void NasmCodeGenerator::Visit(const IfStmt& node)
{
	const std::string labelElse = MakeLabel("else");
	const std::string labelEnd  = MakeLabel("end");

	EmitExprToRax(node.GetCondition());
	Emit("cmp rax, 0");
	Emit("je " + labelElse);

	node.GetThenBlock().Accept(*this);
	Emit("jmp " + labelEnd);

	EmitLabel(labelElse);
	if (node.GetElseNode())
	{
		node.GetElseNode()->Accept(*this);
	}

	EmitLabel(labelEnd);
}

void NasmCodeGenerator::Visit(const DoWhileStmt& /*node*/)
{
	throw std::runtime_error("Генерация NASM-кода для DoWhileStmt не реализована");
}

void NasmCodeGenerator::Visit(const RunStmt& node)
{
	const std::string labelLoop = MakeLabel("loop");
	const std::string labelEnd  = MakeLabel("end");

	EmitLabel(labelLoop);
	EmitExprToRax(node.GetCondition());
	Emit("cmp rax, 0");
	Emit("je " + labelEnd);

	node.GetBody().Accept(*this);
	Emit("jmp " + labelLoop);

	EmitLabel(labelEnd);
}

void NasmCodeGenerator::Visit(const RepStmt& node)
{
	const std::string& iterName = node.GetIterators()[0];

	m_nextOffset -= 8;
	m_varOffsets[iterName] = m_nextOffset;

	const std::string labelLoop = MakeLabel("rep");
	const std::string labelEnd  = MakeLabel("end");

	EmitExprToRax(*node.GetRanges()[0]);
	Emit("mov [rbp" + std::to_string(m_nextOffset) + "], rax");

	EmitLabel(labelLoop);
	EmitExprToRax(*node.GetRanges()[1]);
	Emit("mov rbx, [rbp" + std::to_string(m_nextOffset) + "]");
	Emit("cmp rbx, rax");
	Emit("jge " + labelEnd);

	node.GetOriginalBody().Accept(*this);

	Emit("mov rax, [rbp" + std::to_string(m_nextOffset) + "]");
	Emit("inc rax");
	Emit("mov [rbp" + std::to_string(m_nextOffset) + "], rax");
	Emit("jmp " + labelLoop);

	EmitLabel(labelEnd);
}

void NasmCodeGenerator::Visit(const BinaryExpr& node)
{
	EmitExprToRax(node.GetLeft());
	Emit("push rax");
	EmitExprToRax(node.GetRight());
	Emit("mov rbx, rax");
	Emit("pop rax");

	switch (node.GetOp())
	{
	case BinaryOpKind::Add:
		Emit("add rax, rbx");
		break;
	case BinaryOpKind::Sub:
		Emit("sub rax, rbx");
		break;
	case BinaryOpKind::Mul:
		Emit("imul rax, rbx");
		break;
	case BinaryOpKind::Div:
		Emit("cqo");
		Emit("idiv rbx");
		break;
	case BinaryOpKind::Mod:
		Emit("cqo");
		Emit("idiv rbx");
		Emit("mov rax, rdx");
		break;
	case BinaryOpKind::Eq:
		Emit("cmp rax, rbx");
		Emit("sete al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::NotEq:
		Emit("cmp rax, rbx");
		Emit("setne al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::Less:
		Emit("cmp rax, rbx");
		Emit("setl al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::Greater:
		Emit("cmp rax, rbx");
		Emit("setg al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::LessEq:
		Emit("cmp rax, rbx");
		Emit("setle al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::GreaterEq:
		Emit("cmp rax, rbx");
		Emit("setge al");
		Emit("movzx rax, al");
		break;
	case BinaryOpKind::LogicalAnd:
		Emit("and rax, rbx");
		break;
	case BinaryOpKind::LogicalOr:
		Emit("or rax, rbx");
		break;
	case BinaryOpKind::ShiftLeft:
		Emit("mov rcx, rbx");
		Emit("shl rax, cl");
		break;
	case BinaryOpKind::ShiftRight:
		Emit("mov rcx, rbx");
		Emit("sar rax, cl");
		break;
	case BinaryOpKind::BitwiseAnd:
		Emit("and rax, rbx");
		break;
	case BinaryOpKind::BitwiseOr:
		Emit("or rax, rbx");
		break;
	case BinaryOpKind::BitwiseXor:
		Emit("xor rax, rbx");
		break;
	}
}

void NasmCodeGenerator::Visit(const UnaryExpr& node)
{
	EmitExprToRax(node.GetOperand());
	switch (node.GetOp())
	{
	case UnaryOpKind::LogicalNot:
		Emit("cmp rax, 0");
		Emit("sete al");
		Emit("movzx rax, al");
		break;
	case UnaryOpKind::BitwiseNot:
		Emit("not rax");
		break;
	case UnaryOpKind::AddressOf:
		throw std::runtime_error("NASM: взятие адреса не поддерживается без типовой информации");
	case UnaryOpKind::Deref:
		Emit("mov rax, [rax]");
		break;
	}
}

void NasmCodeGenerator::Visit(const NumberExpr& node)
{
	Emit("mov rax, " + node.GetValue());
}

void NasmCodeGenerator::Visit(const BoolExpr& node)
{
	Emit(std::string("mov rax, ") + (node.GetValue() ? "1" : "0"));
}

void NasmCodeGenerator::Visit(const IdentifierExpr& node)
{
	const int32_t offset = GetVarOffset(node.GetName());
	Emit("mov rax, [rbp" + std::to_string(offset) + "]");
}

void NasmCodeGenerator::Visit(const FunctionCallExpr& node)
{
	const auto& args = node.GetArgs();

	if (node.GetName() == "print")
	{
		if (!args.empty())
		{
			EmitExprToRax(*args[0]);
			Emit("mov rsi, rax");
		}
		Emit("lea rdi, [_fmt_int]");
		Emit("xor eax, eax");
		Emit("call printf");
		return;
	}

	const size_t argCount = args.size();
	for (size_t i = 0; i < argCount && i < static_cast<size_t>(MaxRegArgs); ++i)
	{
		EmitExprToRax(*args[i]);
		Emit("push rax");
	}

	for (int i = static_cast<int>(std::min(argCount, static_cast<size_t>(MaxRegArgs))) - 1;
		 i >= 0; --i)
	{
		Emit("pop " + std::string(ArgRegisters[i]));
	}

	Emit("xor eax, eax");
	Emit("call " + node.GetName());
}

void NasmCodeGenerator::Visit(const StringExpr& /*node*/)
{
	throw std::runtime_error("NASM: строковые литералы в выражениях не поддерживаются");
}

void NasmCodeGenerator::Visit(const ArrayLiteralExpr& /*node*/)
{
	throw std::runtime_error("NASM: массивные литералы не поддерживаются");
}

void NasmCodeGenerator::Visit(const IndexExpr& /*node*/)
{
	throw std::runtime_error("NASM: индексация не поддерживается");
}

void NasmCodeGenerator::Visit(const MemberAccessExpr& /*node*/)
{
	throw std::runtime_error("NASM: доступ к полю не поддерживается");
}

void NasmCodeGenerator::Visit(const StructDeclStmt& /*node*/) {}
void NasmCodeGenerator::Visit(const UnionDeclStmt& /*node*/) {}
void NasmCodeGenerator::Visit(const EnumDeclStmt& /*node*/) {}
void NasmCodeGenerator::Visit(const ErrorExpr& /*node*/) {}
void NasmCodeGenerator::Visit(const ImplicitCastExpr& /*node*/) {}