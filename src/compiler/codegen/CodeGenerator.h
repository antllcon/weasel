#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/IAstVisitor.h"
#include "src/compiler/semantic/SemanticAnalyzer.h"
#include "src/compiler/vm/chunk/Chunk.h"
#include <cstdint>
#include <string>
#include <vector>

class CodeGenerator final : public IAstVisitor
{
public:
	explicit CodeGenerator(CodegenContext context);

	void EmitCast(const FunctionCallExpr& node);
	void EmitPrint(const FunctionCallExpr& node);
	void EmitPrintLn(const FunctionCallExpr& node);
	void EmitUserFunctionCall(const FunctionCallExpr& node);

	[[nodiscard]] Chunk Generate(const AstNode& root);

	void Visit(const ProgramNode& node) override;
	void Visit(const StructDeclStmt& node) override;
	void Visit(const ClassDeclStmt& node) override;
	void Visit(const UnionDeclStmt& node) override;
	void Visit(const ImportDecl& node) override;
	void Visit(const EnumDeclStmt& node) override;
	void Visit(const BinaryExpr& node) override;
	void Visit(const UnaryExpr& node) override;
	void Visit(const IdentifierExpr& node) override;
	void Visit(const NumExpr& node) override;
	void Visit(const StringExpr& node) override;
	void Visit(const BoolExpr& node) override;
	void Visit(const ArrayLiteralExpr& node) override;
	void Visit(const FunctionCallExpr& node) override;
	void Visit(const IndexExpr& node) override;
	void Visit(const MemberAccessExpr& node) override;
	void Visit(const BlockStmt& node) override;
	void Visit(const DoWhileStmt& node) override;
	void Visit(const RunStmt& node) override;
	void Visit(const BreakStmt& node) override;
	void Visit(const ContinueStmt& node) override;
	void Visit(const RepStmt& node) override;
	void Visit(const RepCollectionStmt& node) override;
	void Visit(const RepTimesStmt& node) override;
	void Visit(const VarDeclStmt& node) override;
	void Visit(const AssignStmt& node) override;
	void Visit(const ExprStmt& node) override;
	void Visit(const FunctionDeclStmt& node) override;
	void Visit(const ReturnStmt& node) override;
	void Visit(const IfStmt& node) override;
	void Visit(const ArrayAllocExpr& node) override;
	void Visit(const WhenStmt& node) override;

private:
	struct LoopContext
	{
		uint32_t localCountAtLoopEntry;
		uint32_t continueTarget;
		bool continueTargetKnown;
		uint32_t breakPopsNeeded;
		std::vector<uint32_t> breakPatches;
		std::vector<uint32_t> continuePatches;
	};

	struct UnresolvedCall
	{
		uint32_t patchOffset;
		std::string funcName;
		SourceRange range;
	};

	std::shared_ptr<TypeInfo> GetType(const AstNode& node) const;
	void EmitLogicalNot();
	void EmitConstant(Value value);
	void EmitReserveSlot();
	void EmitConstructor(const ClassDeclStmt& node, const std::string& key, const BlockStmt* body);
	void EmitImplicitThisCall(const FunctionCallExpr& node);
	void EmitPrintCore(const FunctionCallExpr& node);

	CodegenContext m_context;
	std::vector<LoopContext> m_loopStack;
	std::unordered_map<std::string, uint32_t> m_functionOffsets;
	std::vector<UnresolvedCall> m_unresolvedCalls;
	Chunk m_chunk;
	uint32_t m_currentLine = 0;
	uint32_t m_localCount = 0;
};