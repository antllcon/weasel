#pragma once
#include <array>
#include <string_view>

namespace LanguageTokens
{
inline constexpr std::string_view TypeInt = "int";
inline constexpr std::string_view TypeUint = "uint";
inline constexpr std::string_view TypeReal = "real";
inline constexpr std::string_view TypeBool = "bool";
inline constexpr std::string_view TypeString = "string";
inline constexpr std::string_view TypeVoid = "void";

inline constexpr std::string_view KwAlias = "alias";
inline constexpr std::string_view KwAnd = "and";
inline constexpr std::string_view KwArray = "array";
inline constexpr std::string_view KwElse = "else";
inline constexpr std::string_view KwEnum = "enum";
inline constexpr std::string_view KwFalse = "false";
inline constexpr std::string_view KwImport = "import";
inline constexpr std::string_view KwIn = "in";
inline constexpr std::string_view KwLet = "let";
inline constexpr std::string_view KwNot = "not";
inline constexpr std::string_view KwOr = "or";
inline constexpr std::string_view KwRep = "rep";
inline constexpr std::string_view KwReturn = "return";
inline constexpr std::string_view KwRun = "run";
inline constexpr std::string_view KwStruct = "class";
inline constexpr std::string_view KwTrue = "true";
inline constexpr std::string_view KwUnion = "union";
inline constexpr std::string_view KwVal = "val";
inline constexpr std::string_view KwVar = "var";
inline constexpr std::string_view KwWhen = "when";

inline constexpr std::string_view OpMove = "<-";
inline constexpr std::string_view OpNotEq = "><";
inline constexpr std::string_view OpAssign = "=";
inline constexpr std::string_view OpEq = "==";
inline constexpr std::string_view OpLessEq = "<=";
inline constexpr std::string_view OpGreaterEq = ">=";
inline constexpr std::string_view OpRange = "..";
inline constexpr std::string_view OpPlus = "+";
inline constexpr std::string_view OpMinus = "-";
inline constexpr std::string_view OpMul = "*";
inline constexpr std::string_view OpDiv = "/";
inline constexpr std::string_view OpMod = "%";
inline constexpr std::string_view OpLess = "<";
inline constexpr std::string_view OpGreater = ">";
inline constexpr std::string_view OpAddressOf = "&";

inline constexpr std::string_view SymParenLeft = "(";
inline constexpr std::string_view SymParenRight = ")";
inline constexpr std::string_view SymBracketLeft = "[";
inline constexpr std::string_view SymBracketRight = "]";
inline constexpr std::string_view SymBraceLeft = "{";
inline constexpr std::string_view SymBraceRight = "}";
inline constexpr std::string_view SymColon = ":";
inline constexpr std::string_view SymComma = ",";
inline constexpr std::string_view SymDot = ".";

inline constexpr std::array<std::string_view, 26> AllKeywords = {
	KwAlias, KwAnd, KwArray, TypeBool, KwStruct /* "class" */, KwElse, KwEnum, KwFalse, KwImport, KwIn, TypeInt, KwLet, KwNot, KwOr, TypeReal, KwRep, KwReturn, KwRun, TypeString, KwTrue, TypeUint, KwUnion, KwVal, KwVar, TypeVoid, KwWhen};
} // namespace LanguageTokens