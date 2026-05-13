#include "CstVisualizer.h"
#include "src/utils/logger/Logger.h"
#include <sstream>
#include <vector>

namespace
{
constexpr size_t MaxChainLabelWidth = 55;

std::string EscapeString(const std::string& str)
{
	std::string result;
	result.reserve(str.size());

	for (char c : str)
	{
		if (c == '\n')
		{
			result += "\\n";
		}
		else if (c == '\r')
		{
			result += "\\r";
		}
		else if (c == '\t')
		{
			result += "\\t";
		}
		else
		{
			result += c;
		}
	}

	return result;
}

const CstNode* CollapseChain(const CstNode* node, bool collapseChains, std::vector<std::string>& chain)
{
	const CstNode* current = node;

	if (!collapseChains)
	{
		return current;
	}

	while (current->children.size() == 1 && current->value.empty())
	{
		const CstNode* child = current->children.front().get();
		if (!child)
		{
			break;
		}

		current = child;
		chain.push_back(current->label);

		if (!current->value.empty())
		{
			break;
		}
	}

	return current;
}

std::string FormatChainLabel(const std::vector<std::string>& chain)
{
	std::string full;
	for (size_t i = 0; i < chain.size(); ++i)
	{
		if (i > 0)
		{
			full += " -> ";
		}
		full += chain[i];
	}

	if (full.size() <= MaxChainLabelWidth || chain.size() <= 2)
	{
		return full;
	}

	return chain.front() + " -> ... -> " + chain.back();
}

void FormatNodeData(const CstNode* node, bool collapseChains, std::string& outLabel, const CstNode*& outEffectiveNode)
{
	std::vector<std::string> chain;
	chain.push_back(node->label);

	outEffectiveNode = CollapseChain(node, collapseChains, chain);
	outLabel = FormatChainLabel(chain);

	if (!outEffectiveNode->value.empty())
	{
		outLabel += " ('" + EscapeString(outEffectiveNode->value) + "')";
	}
}

void BuildTreeString(const CstNode* node, const std::string& prefix, bool isLast, bool collapseChains, std::ostringstream& stream)
{
	std::string label;
	const CstNode* effectiveNode = nullptr;
	FormatNodeData(node, collapseChains, label, effectiveNode);

	stream << prefix;
	stream << (isLast ? "└── " : "├── ");
	stream << label << "\n";

	const std::string nextPrefix = prefix + (isLast ? "    " : "│   ");

	for (size_t i = 0; i < effectiveNode->children.size(); ++i)
	{
		const bool isChildLast = (i + 1 == effectiveNode->children.size());
		if (effectiveNode->children[i])
		{
			BuildTreeString(effectiveNode->children[i].get(), nextPrefix, isChildLast, collapseChains, stream);
		}
	}
}
} // namespace

void CstVisualizer::Visualize(const CstNode& root, bool collapseChains)
{
	std::ostringstream stream;

	std::string label;
	const CstNode* effectiveNode = nullptr;
	FormatNodeData(&root, collapseChains, label, effectiveNode);

	stream << "[CST]\t\tКонкретное синтаксическое дерево:\n";
	stream << "\t\t" << label << "\n";

	for (size_t i = 0; i < effectiveNode->children.size(); ++i)
	{
		const bool isLast = (i + 1 == effectiveNode->children.size());
		if (effectiveNode->children[i])
		{
			BuildTreeString(effectiveNode->children[i].get(), "\t\t", isLast, collapseChains, stream);
		}
	}

	Logger::Log(stream.str());
}