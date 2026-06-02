#include "TextAssembler.h"
#include "src/compiler/vm/OpCode.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
enum class ParseState
{
	Initial,
	ConstSection,
	CodeSection
};

struct ChunkByte
{
	uint8_t m_value;
	uint32_t m_line;
};

const std::unordered_map<std::string, OpCode> MNEMONICS = {
	{"const", OpCode::Constant},

	{"add_int", OpCode::AddInt},
	{"add_uint", OpCode::AddUint},
	{"add_real", OpCode::AddReal},
	{"sub_int", OpCode::SubInt},
	{"sub_uint", OpCode::SubUint},
	{"sub_real", OpCode::SubReal},
	{"mul_int", OpCode::MulInt},
	{"mul_uint", OpCode::MulUint},
	{"mul_real", OpCode::MulReal},
	{"div_int", OpCode::DivInt},
	{"div_uint", OpCode::DivUint},
	{"div_real", OpCode::DivReal},
	{"rem_int", OpCode::RemInt},
	{"rem_uint", OpCode::RemUint},
	{"rem_real", OpCode::RemReal},

	{"eq_int", OpCode::EqInt},
	{"eq_uint", OpCode::EqUint},
	{"eq_real", OpCode::EqReal},
	{"lt_int", OpCode::LtInt},
	{"lt_uint", OpCode::LtUint},
	{"lt_real", OpCode::LtReal},

	{"load-local", OpCode::LoadLocal},
	{"store-local", OpCode::StoreLocal},
	{"pop", OpCode::Pop},
	{"dup", OpCode::Dup},

	{"jump", OpCode::Jump},
	{"jump_false", OpCode::JumpIfFalse},
	{"jump_true", OpCode::JumpIfTrue},

	{"allocate_struct", OpCode::AllocateStruct},
	{"load-field", OpCode::GetField},
	{"store-field", OpCode::StoreField},

	{"allocate_array", OpCode::AllocateArray},
	{"ld_elem", OpCode::LoadElement},
	{"st_elem", OpCode::StoreElement},

	{"retain", OpCode::Retain},
	{"release", OpCode::Release},

	{"alloc_cls", OpCode::AllocateClosure},
	{"call_cls", OpCode::CallClosure},

	{"call", OpCode::Call},
	{"ret", OpCode::Return},

	{"call_nat", OpCode::CallNative},

	{"load-string", OpCode::LoadString},
	{"panic", OpCode::Panic}};

void AssertIsFileOpened(bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть файл для ассемблирования");
	}
}

void AssertIsInvalidNumFormat(bool isInvalid)
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
		throw std::runtime_error("Неизвестная мнемоника инструкции в текстовом файле");
	}
}

void AssertIsUnknownSection(bool isUnknown)
{
	if (isUnknown)
	{
		throw std::runtime_error("Код объявлен до секции. Ожидается .const или .code");
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
	auto whitespace = " \t\n\r\f\v";
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

uint64_t ParseConstantAsRaw(const std::string& text)
{
	try
	{
		if (text.find('.') != std::string::npos || text.find('e') != std::string::npos || text.find('E') != std::string::npos)
		{
			const double value = std::stod(text);
			uint64_t raw = 0;
			std::memcpy(&raw, &value, sizeof(double));
			return raw;
		}

		if (!text.empty() && text[0] == '-')
		{
			const int64_t value = std::stoll(text);
			uint64_t raw = 0;
			std::memcpy(&raw, &value, sizeof(int64_t));
			return raw;
		}

		return std::stoull(text);
	}
	catch (const std::exception&)
	{
		AssertIsInvalidNumFormat(true);
		return 0;
	}
}

OpCode MapMnemonicToOpCode(const std::string& mnemonic)
{
	if (MNEMONICS.contains(mnemonic))
	{
		return MNEMONICS.at(mnemonic);
	}

	AssertIsUnknownMnemonic(true);
	return OpCode::Return;
}

bool Takes8BitArg(OpCode code)
{
	return code == OpCode::Constant;
}

bool Takes32BitArg(OpCode code)
{
	return code == OpCode::LoadLocal || code == OpCode::StoreLocal || code == OpCode::Jump || code == OpCode::JumpIfFalse || code == OpCode::JumpIfTrue || code == OpCode::AllocateStruct || code == OpCode::GetField || code == OpCode::StoreField || code == OpCode::CallClosure || code == OpCode::LoadString;
}

bool TakesTwo32BitArgs(OpCode code)
{
	return code == OpCode::Call || code == OpCode::AllocateClosure || code == OpCode::CallNative;
}

void WriteUint8(std::ofstream& stream, uint8_t value)
{
	stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteUint32(std::ofstream& stream, uint32_t value)
{
	stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteUint64(std::ofstream& stream, uint64_t value)
{
	stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteBinary(const std::filesystem::path& path, const std::vector<uint64_t>& constants, const std::vector<ChunkByte>& bytes)
{
	std::ofstream file(path, std::ios::binary);
	AssertIsFileOpened(file.is_open());

	WriteUint32(file, 0x314C5357);

	WriteUint32(file, static_cast<uint32_t>(constants.size()));
	for (const uint64_t constant : constants)
	{
		WriteUint64(file, constant);
	}

	WriteUint32(file, 0);

	WriteUint32(file, static_cast<uint32_t>(bytes.size()));
	for (const auto& [m_value, m_line] : bytes)
	{
		WriteUint8(file, m_value);
		WriteUint32(file, m_line);
	}
}
} // namespace

void TextAssembler::AssembleToBinary(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath)
{
	std::ifstream file(inputPath);
	AssertIsFileOpened(file.is_open());

	auto state = ParseState::Initial;
	std::vector<uint64_t> constants;
	std::vector<ChunkByte> bytes;
	uint32_t currentLine = 1;

	std::unordered_map<std::string, uint32_t> labels;

	struct UnresolvedJump
	{
		size_t byteOffset;
		std::string labelName;
	};
	std::vector<UnresolvedJump> unresolvedJumps;

	std::string line;
	while (std::getline(file, line))
	{
		std::string processedLine = TrimWhitespace(StripComments(line));

		if (processedLine.empty())
		{
			currentLine++;
			continue;
		}

		if (processedLine.back() == ':')
		{
			const std::string labelName = processedLine.substr(0, processedLine.size() - 1);
			labels[labelName] = static_cast<uint32_t>(bytes.size());
			currentLine++;
			continue;
		}

		if (processedLine == ".const")
		{
			state = ParseState::ConstSection;
		}
		else if (processedLine == ".code")
		{
			state = ParseState::CodeSection;
		}
		else if (state == ParseState::ConstSection)
		{
			constants.push_back(ParseConstantAsRaw(processedLine));
		}
		else if (state == ParseState::CodeSection)
		{
			const std::vector<std::string> tokens = Tokenize(processedLine);
			if (!tokens.empty())
			{
				const OpCode opCode = MapMnemonicToOpCode(tokens[0]);
				bytes.push_back({static_cast<uint8_t>(opCode), currentLine});

				if (Takes8BitArg(opCode) && tokens.size() >= 2)
				{
					try
					{
						const uint8_t arg = static_cast<uint8_t>(std::stoi(tokens[1]));
						bytes.push_back({arg, currentLine});
					}
					catch (const std::exception&)
					{
						AssertIsInvalidNumFormat(true);
					}
				}
				else if (Takes32BitArg(opCode) && tokens.size() >= 2)
				{
					if (opCode == OpCode::Jump || opCode == OpCode::JumpIfFalse || opCode == OpCode::JumpIfTrue)
					{
						try
						{
							const uint32_t arg = static_cast<uint32_t>(std::stoul(tokens[1]));
							bytes.push_back({static_cast<uint8_t>(arg & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 8 & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 16 & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 24 & 0xFF), currentLine});
						}
						catch (const std::invalid_argument&)
						{
							unresolvedJumps.push_back({bytes.size(), tokens[1]});
							bytes.push_back({0, currentLine});
							bytes.push_back({0, currentLine});
							bytes.push_back({0, currentLine});
							bytes.push_back({0, currentLine});
						}
					}
					else
					{
						try
						{
							const uint32_t arg = static_cast<uint32_t>(std::stoul(tokens[1]));
							bytes.push_back({static_cast<uint8_t>(arg & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 8 & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 16 & 0xFF), currentLine});
							bytes.push_back({static_cast<uint8_t>(arg >> 24 & 0xFF), currentLine});
						}
						catch (const std::exception&)
						{
							AssertIsInvalidNumFormat(true);
						}
					}
				}
				else if (TakesTwo32BitArgs(opCode) && tokens.size() >= 3)
				{
					try
					{
						uint32_t arg1 = 0;
						try
						{
							arg1 = static_cast<uint32_t>(std::stoul(tokens[1]));
						}
						catch (const std::invalid_argument&)
						{
							unresolvedJumps.push_back({bytes.size(), tokens[1]});
						}

						const uint32_t arg2 = static_cast<uint32_t>(std::stoul(tokens[2]));

						bytes.push_back({static_cast<uint8_t>(arg1 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg1 >> 8 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg1 >> 16 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg1 >> 24 & 0xFF), currentLine});

						bytes.push_back({static_cast<uint8_t>(arg2 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg2 >> 8 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg2 >> 16 & 0xFF), currentLine});
						bytes.push_back({static_cast<uint8_t>(arg2 >> 24 & 0xFF), currentLine});
					}
					catch (const std::exception&)
					{
						AssertIsInvalidNumFormat(true);
					}
				}
			}
		}
		else
		{
			AssertIsUnknownSection(true);
		}

		currentLine++;
	}

	for (const auto& [byteOffset, labelName] : unresolvedJumps)
	{
		if (!labels.contains(labelName))
		{
			throw std::runtime_error("Обнаружена неизвестная метка перехода");
		}

		const uint32_t targetAddress = labels[labelName];
		bytes[byteOffset].m_value = static_cast<uint8_t>(targetAddress & 0xFF);
		bytes[byteOffset + 1].m_value = static_cast<uint8_t>(targetAddress >> 8 & 0xFF);
		bytes[byteOffset + 2].m_value = static_cast<uint8_t>(targetAddress >> 16 & 0xFF);
		bytes[byteOffset + 3].m_value = static_cast<uint8_t>(targetAddress >> 24 & 0xFF);
	}

	WriteBinary(outputPath, constants, bytes);
}