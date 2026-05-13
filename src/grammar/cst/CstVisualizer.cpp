#include "CstVisualizer.h"
#include "src/utils/logger/Logger.h"
#include <sstream>

namespace
{
std::string FormatNodeData(const CstNode& node)
{
	std::string result = node.label;
	if (!node.value.empty())
	{
		result += " ('" + node.value + "')";
	}
	return result;
}

void BuildTreeString(const CstNode& node, const std::string& prefix, bool isLast, std::ostringstream& stream)
{
	stream << prefix;

	if (isLast)
	{
		stream << "└── ";
	}
	else
	{
		stream << "├── ";
	}

	stream << FormatNodeData(node) << std::endl;

	std::string nextPrefix = prefix;
	if (isLast)
	{
		nextPrefix += "    ";
	}
	else
	{
		nextPrefix += "│   ";
	}

	for (size_t i = 0; i < node.children.size(); ++i)
	{
		const bool isChildLast = (i + 1 == node.children.size());
		if (node.children[i])
		{
			BuildTreeString(*node.children[i], nextPrefix, isChildLast, stream);
		}
	}
}
}

void CstVisualizer::Visualize(const CstNode& root)
{
	std::ostringstream stream;
	stream << "[CST]\t\tКонкретное синтаксическое дерево:" << std::endl;
	stream << "\t\t" << FormatNodeData(root) << std::endl;

	for (size_t i = 0; i < root.children.size(); ++i)
	{
		const bool isLast = (i + 1 == root.children.size());
		if (root.children[i])
		{
			BuildTreeString(*root.children[i], "\t\t", isLast, stream);
		}
	}

	Logger::Log(stream.str());
}