#include "TextAssembler.h"
#include "src/vm/OpCode.h"
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
	{"cst", OpCode::Constant},

	{"add_i8", OpCode::AddI8},
	{"add_u8", OpCode::AddU8},
	{"add_i16", OpCode::AddI16},
	{"add_u16", OpCode::AddU16},
	{"add_i32", OpCode::AddI32},
	{"add_u32", OpCode::AddU32},
	{"add_i64", OpCode::AddI64},
	{"add_u64", OpCode::AddU64},
	{"add_f32", OpCode::AddF32},
	{"add_f64", OpCode::AddF64},

	{"sub_i8", OpCode::SubI8},
	{"sub_u8", OpCode::SubU8},
	{"sub_i16", OpCode::SubI16},
	{"sub_u16", OpCode::SubU16},
	{"sub_i32", OpCode::SubI32},
	{"sub_u32", OpCode::SubU32},
	{"sub_i64", OpCode::SubI64},
	{"sub_u64", OpCode::SubU64},
	{"sub_f32", OpCode::SubF32},
	{"sub_f64", OpCode::SubF64},

	{"mul_i8", OpCode::MulI8},
	{"mul_u8", OpCode::MulU8},
	{"mul_i16", OpCode::MulI16},
	{"mul_u16", OpCode::MulU16},
	{"mul_i32", OpCode::MulI32},
	{"mul_u32", OpCode::MulU32},
	{"mul_i64", OpCode::MulI64},
	{"mul_u64", OpCode::MulU64},
	{"mul_f32", OpCode::MulF32},
	{"mul_f64", OpCode::MulF64},

	{"div_i8", OpCode::DivI8},
	{"div_u8", OpCode::DivU8},
	{"div_i16", OpCode::DivI16},
	{"div_u16", OpCode::DivU16},
	{"div_i32", OpCode::DivI32},
	{"div_u32", OpCode::DivU32},
	{"div_i64", OpCode::DivI64},
	{"div_u64", OpCode::DivU64},
	{"div_f32", OpCode::DivF32},
	{"div_f64", OpCode::DivF64},

	{"rem_i8", OpCode::RemI8},
	{"rem_u8", OpCode::RemU8},
	{"rem_i16", OpCode::RemI16},
	{"rem_u16", OpCode::RemU16},
	{"rem_i32", OpCode::RemI32},
	{"rem_u32", OpCode::RemU32},
	{"rem_i64", OpCode::RemI64},
	{"rem_u64", OpCode::RemU64},

	{"eq_i8", OpCode::EqI8},
	{"eq_u8", OpCode::EqU8},
	{"eq_i16", OpCode::EqI16},
	{"eq_u16", OpCode::EqU16},
	{"eq_i32", OpCode::EqI32},
	{"eq_u32", OpCode::EqU32},
	{"eq_i64", OpCode::EqI64},
	{"eq_u64", OpCode::EqU64},
	{"eq_f32", OpCode::EqF32},
	{"eq_f64", OpCode::EqF64},

	{"lt_i8", OpCode::LtI8},
	{"lt_u8", OpCode::LtU8},
	{"lt_i16", OpCode::LtI16},
	{"lt_u16", OpCode::LtU16},
	{"lt_i32", OpCode::LtI32},
	{"lt_u32", OpCode::LtU32},
	{"lt_i64", OpCode::LtI64},
	{"lt_u64", OpCode::LtU64},
	{"lt_f32", OpCode::LtF32},
	{"lt_f64", OpCode::LtF64},

	{"bit_and", OpCode::BitAnd},
	{"bit_or", OpCode::BitOr},
	{"bit_xor", OpCode::BitXor},
	{"bit_not", OpCode::BitNot},
	{"shl", OpCode::Shl},
	{"shr", OpCode::Shr},

	{"ld_loc", OpCode::LoadLocal},
	{"st_loc", OpCode::StoreLocal},
	{"pop", OpCode::Pop},
	{"dup", OpCode::Dup},

	{"jmp", OpCode::Jump},
	{"jmp_f", OpCode::JumpIfFalse},
	{"jmp_t", OpCode::JumpIfTrue},

	{"alloc_st", OpCode::AllocateStruct},
	{"get_fld", OpCode::GetField},
	{"set_fld", OpCode::StoreField},

	{"alloc_arr", OpCode::AllocateArray},
	{"ld_elem", OpCode::LoadElement},
	{"st_elem", OpCode::StoreElement},

	{"retain", OpCode::Retain},
	{"release", OpCode::Release},

	{"call", OpCode::Call},
	{"ret", OpCode::Return}};

void AssertIsFileOpened(bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть файл для ассемблирования");
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
		AssertIsInvalidNumberFormat(true);
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
	return code == OpCode::LoadLocal || code == OpCode::StoreLocal || code == OpCode::Jump || code == OpCode::JumpIfFalse || code == OpCode::JumpIfTrue || code == OpCode::AllocateStruct || code == OpCode::GetField || code == OpCode::StoreField;
}

bool TakesTwo32BitArgs(OpCode code)
{
	return code == OpCode::Call;
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
						AssertIsInvalidNumberFormat(true);
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
							AssertIsInvalidNumberFormat(true);
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
						AssertIsInvalidNumberFormat(true);
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