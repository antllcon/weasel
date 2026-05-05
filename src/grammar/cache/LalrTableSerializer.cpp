#include "LalrTableSerializer.h"
#include <cstring>
#include <stdexcept>

namespace
{
void WriteUint64(std::vector<uint8_t>& buf, uint64_t value)
{
	const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
	buf.insert(buf.end(), bytes, bytes + sizeof(value));
}

void WriteUint8(std::vector<uint8_t>& buf, uint8_t value)
{
	buf.push_back(value);
}

void WriteString(std::vector<uint8_t>& buf, const std::string& str)
{
	WriteUint64(buf, str.size());
	const auto* bytes = reinterpret_cast<const uint8_t*>(str.data());
	buf.insert(buf.end(), bytes, bytes + str.size());
}

uint64_t ReadUint64(const uint8_t* data, size_t& offset, size_t size)
{
	if (offset + sizeof(uint64_t) > size)
	{
		throw std::runtime_error("Повреждён кэш-файл: неожиданный конец данных");
	}
	uint64_t value = 0;
	std::memcpy(&value, data + offset, sizeof(value));
	offset += sizeof(uint64_t);
	return value;
}

uint8_t ReadUint8(const uint8_t* data, size_t& offset, size_t size)
{
	if (offset + sizeof(uint8_t) > size)
	{
		throw std::runtime_error("Повреждён кэш-файл: неожиданный конец данных");
	}
	return data[offset++];
}

std::string ReadString(const uint8_t* data, size_t& offset, size_t size)
{
	const auto len = ReadUint64(data, offset, size);
	if (offset + len > size)
	{
		throw std::runtime_error("Повреждён кэш-файл: строка обрезана");
	}
	std::string str(reinterpret_cast<const char*>(data + offset), len);
	offset += len;
	return str;
}

void SerializeRules(std::vector<uint8_t>& buf, const raw::Rules& rules)
{
	WriteUint64(buf, rules.size());
	for (const auto& rule : rules)
	{
		WriteString(buf, rule.name);
		WriteUint64(buf, rule.alternatives.size());
		for (const auto& alt : rule.alternatives)
		{
			WriteUint64(buf, alt.size());
			for (const auto& sym : alt)
			{
				WriteString(buf, sym);
			}
		}
	}
}

void SerializeActionTable(std::vector<uint8_t>& buf,
	const std::vector<std::unordered_map<std::string, LalrAction>>& actionTable)
{
	WriteUint64(buf, actionTable.size());
	for (const auto& stateMap : actionTable)
	{
		WriteUint64(buf, stateMap.size());
		for (const auto& [key, action] : stateMap)
		{
			WriteString(buf, key);
			WriteUint8(buf, static_cast<uint8_t>(action.type));
			WriteUint64(buf, action.ruleIndex);
			WriteUint64(buf, action.altIndex);
		}
	}
}

void SerializeGotoTable(std::vector<uint8_t>& buf,
	const std::vector<std::unordered_map<std::string, size_t>>& gotoTable)
{
	WriteUint64(buf, gotoTable.size());
	for (const auto& stateMap : gotoTable)
	{
		WriteUint64(buf, stateMap.size());
		for (const auto& [key, target] : stateMap)
		{
			WriteString(buf, key);
			WriteUint64(buf, static_cast<uint64_t>(target));
		}
	}
}

raw::Rules DeserializeRules(const uint8_t* data, size_t& offset, size_t size)
{
	const auto ruleCount = ReadUint64(data, offset, size);
	raw::Rules rules;
	rules.reserve(ruleCount);

	for (uint64_t i = 0; i < ruleCount; ++i)
	{
		raw::Rule rule;
		rule.name = ReadString(data, offset, size);
		const auto altCount = ReadUint64(data, offset, size);
		rule.alternatives.reserve(altCount);

		for (uint64_t j = 0; j < altCount; ++j)
		{
			const auto symCount = ReadUint64(data, offset, size);
			raw::Alternative alt;
			alt.reserve(symCount);
			for (uint64_t k = 0; k < symCount; ++k)
			{
				alt.push_back(ReadString(data, offset, size));
			}
			rule.alternatives.push_back(std::move(alt));
		}
		rules.push_back(std::move(rule));
	}
	return rules;
}

std::vector<std::unordered_map<std::string, LalrAction>> DeserializeActionTable(
	const uint8_t* data, size_t& offset, size_t size)
{
	const auto stateCount = ReadUint64(data, offset, size);
	std::vector<std::unordered_map<std::string, LalrAction>> actionTable;
	actionTable.resize(stateCount);

	for (uint64_t i = 0; i < stateCount; ++i)
	{
		const auto entryCount = ReadUint64(data, offset, size);
		for (uint64_t j = 0; j < entryCount; ++j)
		{
			auto key = ReadString(data, offset, size);
			const auto typeRaw = ReadUint8(data, offset, size);
			const auto ruleIndex = ReadUint64(data, offset, size);
			const auto altIndex = ReadUint64(data, offset, size);

			LalrAction action{
				.type = static_cast<LalrActionType>(typeRaw),
				.ruleIndex = static_cast<size_t>(ruleIndex),
				.altIndex = static_cast<size_t>(altIndex)};
			actionTable[i].emplace(std::move(key), action);
		}
	}
	return actionTable;
}

std::vector<std::unordered_map<std::string, size_t>> DeserializeGotoTable(
	const uint8_t* data, size_t& offset, size_t size)
{
	const auto stateCount = ReadUint64(data, offset, size);
	std::vector<std::unordered_map<std::string, size_t>> gotoTable;
	gotoTable.resize(stateCount);

	for (uint64_t i = 0; i < stateCount; ++i)
	{
		const auto entryCount = ReadUint64(data, offset, size);
		for (uint64_t j = 0; j < entryCount; ++j)
		{
			auto key = ReadString(data, offset, size);
			const auto target = ReadUint64(data, offset, size);
			gotoTable[i].emplace(std::move(key), static_cast<size_t>(target));
		}
	}
	return gotoTable;
}
} // namespace

namespace LalrTableSerializer
{
std::vector<uint8_t> Serialize(const LalrTable& table)
{
	std::vector<uint8_t> buf;
	SerializeRules(buf, table.augmentedRules);
	SerializeActionTable(buf, table.actionTable);
	SerializeGotoTable(buf, table.gotoTable);
	return buf;
}

LalrTable Deserialize(const std::vector<uint8_t>& data)
{
	size_t offset = 0;
	const auto size = data.size();
	const auto* ptr = data.data();

	LalrTable table;
	table.augmentedRules = DeserializeRules(ptr, offset, size);
	table.actionTable = DeserializeActionTable(ptr, offset, size);
	table.gotoTable = DeserializeGotoTable(ptr, offset, size);
	return table;
}
} // namespace LalrTableSerializer