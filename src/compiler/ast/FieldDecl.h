#pragma once
#include <string>

struct FieldDecl
{
	std::string typeName;
	std::string name;
	bool isPublic = true;
};