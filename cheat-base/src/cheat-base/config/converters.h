#pragma once

#include <nlohmann/json.hpp>
#include <cheat-base/Hotkey.h>
#include <imgui.h>
#include "fields/Enum.h"

namespace config::converters
{

	template<typename T>
	inline nlohmann::json ToJson(const T& value)
	{
		return nlohmann::json(value);
	}

	template<typename T>
	inline void FromJson(T& value, const nlohmann::json& jObject)
	{
		value = jObject.get<T>();
	}

	// Here is storing all simple converters json<->class

	// ImColor
	template<>
	inline nlohmann::json ToJson(const ImColor& value)
	{
		return nlohmann::json((ImU32)value);
	}

	template<>
	inline void FromJson(ImColor& value, const nlohmann::json& jObject)
	{
		value = { (ImU32)jObject };
	}

	// Hotkey
	template<>
	inline nlohmann::json ToJson(const Hotkey& value)
	{
		auto keys = value.GetKeys();
		if (keys.empty())
			return {};

		if (keys.size() == 1)
			return keys[0];

		return nlohmann::json(value.GetKeys());
	}

	template<>
	inline void FromJson(Hotkey& value, const nlohmann::json& jObject)
	{
		if (jObject.is_null() || jObject.empty())
			return;

		if (jObject.is_number())
		{
			value = { jObject.get<short>() };
			return;
		}

		value = { jObject.get<std::vector<short>>() };
	}

	// Enum
	template<typename T>
	inline nlohmann::json ToJson(const Enum<T>& enumValue)
	{
		return {
			{ "name", magic_enum::enum_name(enumValue.value()) },
			{ "value", enumValue.raw() }
		};
	}

	template<typename T>
	inline void FromJson(Enum<T>& value, const nlohmann::json& jObject)
	{
		value = jObject["value"].get<uint32_t>();
	}
}