#include "PrintResolver.h"
#include "src/compiler/ast/ScalarTypeInfo.h"

namespace PrintResolver
{
std::string Resolve(const TypeInfo* typeInfo) noexcept
{
	const auto* scalar = dynamic_cast<const ScalarTypeInfo*>(typeInfo);
	if (!scalar)
	{
		return "print_signed";
	}

	if (scalar->GetBaseType() == BaseType::String) return "print_string";
	if (scalar->GetBaseType() == BaseType::Bool) return "print_bool";
	if (scalar->GetBaseType() == BaseType::Real) return "print_real";
	if (scalar->GetBaseType() == BaseType::Uint) return "print_unsigned";

	return "print_signed";
}
} // namespace PrintResolver