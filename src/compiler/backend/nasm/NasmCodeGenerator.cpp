#include "NasmCodeGenerator.h"

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
#include "src/compiler/ast/RepCollectionStmt.h"
#include "src/compiler/ast/RepStmt.h"
#include "src/compiler/ast/RepTimesStmt.h"
#include "src/compiler/ast/ReturnStmt.h"
#include "src/compiler/ast/RunStmt.h"
#include "src/compiler/ast/UnaryExpr.h"
#include "src/compiler/ast/VarDeclStmt.h"

#include <stdexcept>

namespace
{
const char* ArgRegisters[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
constexpr int MaxRegArgs = 6;

uint32_t AlignTo16(uint32_t size)
{
	return size + 15 & ~15u;
}
} // namespace

NasmCodeGenerator::NasmCodeGenerator(const CodegenContext& context)
	: m_context(context)
{
}

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

int32_t NasmCodeGenerator::SlotToOffset(uint32_t slot) const
{
	return -static_cast<int32_t>(slot + 1) * 8;
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
	const uint32_t maxSlots = m_context.functions.at(node.GetName()).maxSlots;
	const uint32_t frameSize = AlignTo16(maxSlots * 8 + 8);

	Emit("push rbp");
	Emit("mov rbp, rsp");
	Emit("sub rsp, " + std::to_string(frameSize));

	const auto& params = node.GetParams();
	for (uint32_t i = 0; i < static_cast<uint32_t>(params.size()); ++i)
	{
		const int32_t offset = SlotToOffset(i);
		if (i < static_cast<uint32_t>(MaxRegArgs))
		{
			Emit("mov [rbp" + std::to_string(offset) + "], " + ArgRegisters[i]);
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

	if (node.GetReturnTypeName() == "void")
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
	const auto slotIt = m_context.varDeclSlots.find(&node);
	if (slotIt == m_context.varDeclSlots.end())
	{
		throw std::runtime_error("NASM: слот переменной не найден: " + node.GetName());
	}
	const int32_t offset = SlotToOffset(slotIt->second);

	if (node.GetInit())
	{
		EmitExprToRax(*node.GetInit());
		Emit("mov [rbp" + std::to_string(offset) + "], rax");
	}
	else
	{
		Emit("mov qword [rbp" + std::to_string(offset) + "], 0");
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

	const auto it = m_context.symbols.find(lhs);
	if (it == m_context.symbols.end())
	{
		throw std::runtime_error("NASM: переменная не найдена при присваивании: " + lhs->GetName());
	}
	const int32_t offset = SlotToOffset(it->second.stackSlot);
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
	const std::string labelEnd = MakeLabel("end");

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

void NasmCodeGenerator::Visit(const ArrayAllocExpr& /*node*/)
{
}

void NasmCodeGenerator::Visit(const WhenStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const RepCollectionStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const RepTimesStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const BreakStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const ContinueStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const DoWhileStmt& /*node*/)
{
}

void NasmCodeGenerator::Visit(const RunStmt& node)
{
	const std::string labelLoop = MakeLabel("loop");
	const std::string labelEnd = MakeLabel("end");

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
	const auto repIt = m_context.repIterators.find(&node);
	if (repIt == m_context.repIterators.end())
	{
		throw std::runtime_error("NASM: итератор цикла не найден: " + node.GetIterator());
	}
	const int32_t iterOffset = SlotToOffset(repIt->second.stackSlot);

	const std::string labelLoop = MakeLabel("rep");
	const std::string labelEnd = MakeLabel("end");

	EmitExprToRax(node.GetStartExpr());
	Emit("mov [rbp" + std::to_string(iterOffset) + "], rax");

	EmitLabel(labelLoop);
	EmitExprToRax(node.GetEndExpr());
	Emit("mov rbx, [rbp" + std::to_string(iterOffset) + "]");
	Emit("cmp rbx, rax");
	Emit("jge " + labelEnd);

	node.GetBody().Accept(*this);

	Emit("mov rax, [rbp" + std::to_string(iterOffset) + "]");
	Emit("inc rax");
	Emit("mov [rbp" + std::to_string(iterOffset) + "], rax");
	Emit("jmp " + labelLoop);

	EmitLabel(labelEnd);
}

void NasmCodeGenerator::Visit(const BinaryExpr& node)
{
	const auto op = node.GetOp();

	if (op == BinaryOpKind::LogicalAnd)
	{
		const std::string labelEnd = MakeLabel("and_end");
		EmitExprToRax(node.GetLeft());
		Emit("cmp rax, 0");
		Emit("je " + labelEnd);
		EmitExprToRax(node.GetRight());
		EmitLabel(labelEnd);
		return;
	}

	if (op == BinaryOpKind::LogicalOr)
	{
		const std::string labelEnd = MakeLabel("or_end");
		EmitExprToRax(node.GetLeft());
		Emit("cmp rax, 0");
		Emit("jne " + labelEnd);
		EmitExprToRax(node.GetRight());
		EmitLabel(labelEnd);
		return;
	}
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
		break;
	case BinaryOpKind::LogicalOr:
		break;
	}
}

// todo: тут что-то не хватает относительно
// enum class UnaryOpKind
// {
// LogicalNot,
// AddressOf,
// Deref,
// Minus
// };

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
	case UnaryOpKind::AddressOf:
		throw std::runtime_error("NASM: взятие адреса не поддерживается без типовой информации");
	case UnaryOpKind::Deref:
		Emit("mov rax, [rax]");
		break;
	default:;
	}
}

void NasmCodeGenerator::Visit(const NumExpr& node)
{
	Emit("mov rax, " + node.GetValue());
}

void NasmCodeGenerator::Visit(const BoolExpr& node)
{
	Emit(std::string("mov rax, ") + (node.GetValue() ? "1" : "0"));
}

void NasmCodeGenerator::Visit(const IdentifierExpr& node)
{
	const auto it = m_context.symbols.find(&node);
	if (it == m_context.symbols.end())
	{
		throw std::runtime_error("NASM: переменная не найдена: " + node.GetName());
	}
	const int32_t offset = SlotToOffset(it->second.stackSlot);
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
		i >= 0;
		--i)
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

void NasmCodeGenerator::Visit(const StructDeclStmt& /*node*/)
{
}
void NasmCodeGenerator::Visit(const ClassDeclStmt& /*node*/)
{
}
void NasmCodeGenerator::Visit(const UnionDeclStmt& /*node*/)
{
}
void NasmCodeGenerator::Visit(const EnumDeclStmt& /*node*/)
{
}