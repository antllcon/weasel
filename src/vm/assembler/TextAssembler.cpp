#include "TextAssembler.h"
#include "src/vm/OpCode.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

namespace
{
enum class ParseState
{
    Initial,
    ConstSection,
    CodeSection
};

struct Instruction
{
    OpCode m_code;
    uint32_t m_line;
};

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

uint64_t ParseDoubleAsRaw(const std::string& text)
{
    try
    {
        const double value = std::stod(text);
        uint64_t raw = 0;
        std::memcpy(&raw, &value, sizeof(double));
        return raw;
    }
    catch (const std::exception&)
    {
        AssertIsInvalidNumberFormat(true);
        return 0;
    }
}

OpCode MapMnemonicToOpCode(const std::string& mnemonic)
{
    if (mnemonic == "cst") return OpCode::Constant;
    if (mnemonic == "ret") return OpCode::Return;
    if (mnemonic == "add_sng") return OpCode::AddSingle;
    if (mnemonic == "add_dbl") return OpCode::AddDouble;
    if (mnemonic == "add_num") return OpCode::AddSNumber;

    AssertIsUnknownMnemonic(true);
    return OpCode::Return;
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

void WriteBinary(const std::filesystem::path& path, const std::vector<uint64_t>& constants, const std::vector<Instruction>& instructions)
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

    WriteUint32(file, static_cast<uint32_t>(instructions.size()));
    for (const Instruction& inst : instructions)
    {
        WriteUint8(file, static_cast<uint8_t>(inst.m_code));
        WriteUint32(file, inst.m_line);
    }
}
}

void TextAssembler::AssembleToBinary(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath)
{
    std::ifstream file(inputPath);
    AssertIsFileOpened(file.is_open());

    auto state = ParseState::Initial;
    std::vector<uint64_t> constants;
    std::vector<Instruction> instructions;
    uint32_t currentLine = 1;

    std::string line;
    while (std::getline(file, line))
    {
        std::string processedLine = TrimWhitespace(StripComments(line));

        if (processedLine.empty())
        {
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
            constants.push_back(ParseDoubleAsRaw(processedLine));
        }
        else if (state == ParseState::CodeSection)
        {
            const std::vector<std::string> tokens = Tokenize(processedLine);
            if (!tokens.empty())
            {
                OpCode opCode = MapMnemonicToOpCode(tokens[0]);
                instructions.push_back({opCode, currentLine});

                if (opCode == OpCode::Constant && tokens.size() >= 2)
                {
                    try
                    {
                        const uint8_t arg = static_cast<uint8_t>(std::stoi(tokens[1]));
                        instructions.push_back({static_cast<OpCode>(arg), currentLine});
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

    WriteBinary(outputPath, constants, instructions);
}