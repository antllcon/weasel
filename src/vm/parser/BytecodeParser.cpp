#include "BytecodeParser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
const std::string EXPECTED_EXTENSION = ".wesbc";

enum class ParseState
{
	Initial,
	ConstSection,
	CodeSection
};

void AssertIsFileExtensionValid(const std::filesystem::path& path)
{
	if (path.extension() != EXPECTED_EXTENSION)
	{
		throw std::runtime_error("Неверное расширение файла. Ожидается .wesbc");
	}
}

void AssertIsFileOpened(bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть файл для чтения");
	}
}

void AssertIsUnknownSection(bool isUnknown)
{
	if (isUnknown)
	{
		throw std::runtime_error("Код объявлен до секции. Ожидается .const или .code");
	}
}

void AssertIsInvalidNumberFormat(bool isInvalid)
{
	if (isInvalid)
	{
		throw std::runtime_error("Ошибка парсинга числа: неверный формат");
	}
}

void AssertIsUnknownMnemonic(bool isUnknown)
{
	if (isUnknown)
	{
		throw std::runtime_error("Неизвестная мнемоника инструкции");
	}
}

void AssertIsMissingArgument(bool isMissing)
{
	if (isMissing)
	{
		throw std::runtime_error("Инструкции требуется аргумент, но он отсутствует");
	}
}

std::string StripComments(const std::string& line)
{
	const size_t hashPos = line.find('#');
	if (hashPos != std::string::npos)
	{
		return line.substr(0, hashPos);
	}
	return line;
}

std::string TrimWhitespace(const std::string& line)
{
	const char* whitespace = " \t\n\r\f\v";
	const size_t start = line.find_first_not_of(whitespace);
	if (start == std::string::npos)
	{
		return "";
	}

	const size_t end = line.find_last_not_of(whitespace);
	return line.substr(start, end - start + 1);
}

std::vector<std::string> Tokenize(const std::string& line)
{
	std::vector<std::string> tokens;
	std::istringstream stream(line);
	std::string token;

	while (stream >> token)
	{
		tokens.push_back(token);
	}

	return tokens;
}

void ParseConstant(const std::string& line, Chunk& chunk)
{
	try
	{
		const double value = std::stod(line);
		chunk.AddConstant(value);
	}
	catch (const std::exception&)
	{
		AssertIsInvalidNumberFormat(true);
	}
}

void ParseInstruction(const std::vector<std::string>& tokens, Chunk& chunk)
{
	if (tokens.empty())
	{
		return;
	}

	const std::string& mnemonic = tokens[0];

	if (mnemonic == "cst")
	{
		AssertIsMissingArgument(tokens.size() < 2);
		try
		{
			const int arg = std::stoi(tokens[1]);
			chunk.WriteOpCode(OpCode::Constant);
			chunk.WriteByte(static_cast<uint8_t>(arg));
		}
		catch (const std::exception&)
		{
			AssertIsInvalidNumberFormat(true);
		}
	}
	else if (mnemonic == "add")
	{
		chunk.WriteOpCode(OpCode::Add);
	}
	else if (mnemonic == "sub")
	{
		chunk.WriteOpCode(OpCode::Subtract);
	}
	else if (mnemonic == "mul")
	{
		chunk.WriteOpCode(OpCode::Multiply);
	}
	else if (mnemonic == "div")
	{
		chunk.WriteOpCode(OpCode::Divide);
	}
	else if (mnemonic == "neg")
	{
		chunk.WriteOpCode(OpCode::Negate);
	}
	else if (mnemonic == "ret")
	{
		chunk.WriteOpCode(OpCode::Return);
	}
	else
	{
		AssertIsUnknownMnemonic(true);
	}
}

std::ifstream OpenTextFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	AssertIsFileOpened(file.is_open());
	return file;
}

void ParseFileContent(std::ifstream& file, Chunk& chunk)
{
	ParseState state = ParseState::Initial;
	std::string line;

	while (std::getline(file, line))
	{
		line = StripComments(line);
		line = TrimWhitespace(line);

		if (line.empty())
		{
			continue;
		}

		if (line == ".const")
		{
			state = ParseState::ConstSection;
			continue;
		}

		if (line == ".code")
		{
			state = ParseState::CodeSection;
			continue;
		}

		if (state == ParseState::ConstSection)
		{
			ParseConstant(line, chunk);
		}
		else if (state == ParseState::CodeSection)
		{
			const std::vector<std::string> tokens = Tokenize(line);
			ParseInstruction(tokens, chunk);
		}
		else
		{
			AssertIsUnknownSection(true);
		}
	}
}
} // namespace

Chunk BytecodeParser::ParseFile(const std::filesystem::path& filePath)
{
	AssertIsFileExtensionValid(filePath);
	auto file = OpenTextFile(filePath);
	Chunk chunk;
	ParseFileContent(file, chunk);
	return chunk;
}