#pragma once

#include <nlohmann/json.hpp>
#include "pugixml.hpp"
#include <algorithm>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace xcodeSettings {
using nlohmann::json;

inline std::string trim(const std::string & text) {
	auto first = text.find_first_not_of(" \t\r\n");
	return first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// Three-way merge. A deletion is an edit too; arrays are kept as a whole, not concatenated.
inline json merge(const json & base, const json & user, const json & next) {
	if (user == base) return next;
	if (!user.is_object() || !next.is_object()) return user;
	json result = next;
	if (base.is_object()) {
		for (auto it = base.begin(); it != base.end(); ++it) if (!user.contains(it.key())) result.erase(it.key());
	}
	for (auto it = user.begin(); it != user.end(); ++it) {
		if (!base.is_object() || !base.contains(it.key())) {
			result[it.key()] = next.contains(it.key()) && it->is_object()
				? merge(json::object(), *it, next[it.key()]) : *it;
		} else if (*it != base[it.key()]) {
			result[it.key()] = next.contains(it.key()) ? merge(base[it.key()], *it, next[it.key()]) : *it;
		}
	}
	return result;
}

// Retain plist types, especially booleans, arrays, dates and binary data.
inline json readPlistNode(pugi::xml_node node) {
	std::string type = node.name();
	json value;
	if (type == "dict") {
		value = json::object();
		for (auto key = node.first_child(); key; key = key.next_sibling()) {
			if (key.type() != pugi::node_element) continue;
			if (std::string(key.name()) != "key") throw std::runtime_error("expected a plist dictionary key");
			auto entry = key.next_sibling();
			while (entry && entry.type() != pugi::node_element) entry = entry.next_sibling();
			if (!entry || value.contains(key.child_value())) throw std::runtime_error("missing or duplicate plist value");
			value[key.child_value()] = readPlistNode(entry);
			key = entry;
		}
	} else if (type == "array") {
		value = json::array();
		for (auto child : node.children()) if (child.type() == pugi::node_element) value.push_back(readPlistNode(child));
	} else if (type == "true" || type == "false" || type == "string" || type == "integer"
		|| type == "real" || type == "data" || type == "date") {
		value = node.child_value();
	} else throw std::runtime_error("unsupported plist value: " + type);
	return { { "type", type }, { "value", value } };
}

inline pugi::xml_node plistRoot(pugi::xml_document & doc) {
	for (auto node : doc.child("plist").children()) if (node.type() == pugi::node_element) return node;
	return {};
}

inline json readPlist(const std::string & text) {
	pugi::xml_document doc;
	if (doc.load_buffer(text.data(), text.size()) && plistRoot(doc)) return readPlistNode(plistRoot(doc));
#ifdef __APPLE__
	// Real Xcode edits use OpenStep project files; user Info.plists may be binary.
	auto data = CFDataCreate(nullptr, reinterpret_cast<const UInt8 *>(text.data()), text.size());
	auto plist = CFPropertyListCreateWithData(nullptr, data, kCFPropertyListImmutable, nullptr, nullptr);
	CFRelease(data);
	if (plist) {
		auto xml = CFPropertyListCreateData(nullptr, plist, kCFPropertyListXMLFormat_v1_0, 0, nullptr);
		CFRelease(plist);
		if (xml) {
			bool parsed = doc.load_buffer(CFDataGetBytePtr(xml), CFDataGetLength(xml));
			CFRelease(xml);
			if (parsed && plistRoot(doc)) return readPlistNode(plistRoot(doc));
		}
	}
#endif
	throw std::runtime_error("cannot read property list (OpenStep/binary formats require macOS)");
}

inline json plainPlist(const json & node) {
	const auto type = node.at("type").get<std::string>();
	const auto & value = node.at("value");
	if (type == "dict" || type == "array") {
		json result = type == "dict" ? json::object() : json::array();
		for (auto it = value.begin(); it != value.end(); ++it) {
			if (type == "dict") result[it.key()] = plainPlist(*it);
			else result.push_back(plainPlist(*it));
		}
		return result;
	}
	if (type == "true" || type == "false") return type == "true";
	return value;
}

inline json readProject(const std::string & text) {
	json result = json::parse(text, nullptr, false);
	if (result.is_discarded()) result = plainPlist(readPlist(text));
	if (!result.is_object() || !result.contains("objects") || !result["objects"].is_object()
		|| !result.contains("rootObject") || !result["rootObject"].is_string()
		|| !result["objects"].contains(result["rootObject"].get<std::string>())) {
		throw std::runtime_error("invalid Xcode project");
	}
	return result;
}

inline void writePlistNode(pugi::xml_node parent, const json & node) {
	const auto type = node.at("type").get<std::string>();
	const auto & value = node.at("value");
	auto child = parent.append_child(type.c_str());
	if (type == "dict") {
		for (auto it = value.begin(); it != value.end(); ++it) {
			child.append_child("key").text().set(it.key().c_str());
			writePlistNode(child, *it);
		}
	} else if (type == "array") {
		for (auto & entry : value) writePlistNode(child, entry);
	} else if (type != "true" && type != "false") child.text().set(value.get<std::string>().c_str());
}

inline std::string writePlist(const json & node) {
	pugi::xml_document doc;
	auto declaration = doc.append_child(pugi::node_declaration);
	declaration.append_attribute("version") = "1.0";
	declaration.append_attribute("encoding") = "UTF-8";
	doc.append_child(pugi::node_doctype).set_value("plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
	auto plist = doc.append_child("plist");
	plist.append_attribute("version") = "1.0";
	writePlistNode(plist, node);
	std::ostringstream out;
	doc.save(out, "\t");
	return out.str();
}

inline json mergePlist(const json & base, const json & user, const json & next) {
	if (user == base) return next;
	if (user.at("type") != "dict" || next.at("type") != "dict") return user;
	json values = next.at("value");
	const auto & old = base.at("value");
	const auto & current = user.at("value");
	if (base.at("type") == "dict") {
		for (auto it = old.begin(); it != old.end(); ++it) if (!current.contains(it.key())) values.erase(it.key());
	}
	for (auto it = current.begin(); it != current.end(); ++it) {
		if (!old.is_object() || !old.contains(it.key())) {
			json empty = { { "type", "dict" }, { "value", json::object() } };
			values[it.key()] = values.contains(it.key()) ? mergePlist(empty, *it, values[it.key()]) : *it;
		} else if (*it != old[it.key()]) {
			values[it.key()] = values.contains(it.key()) ? mergePlist(old[it.key()], *it, values[it.key()]) : *it;
		}
	}
	return { { "type", "dict" }, { "value", values } };
}

inline std::string settingName(const std::string & key) { return trim(key.substr(0, key.find('['))); }

inline bool listSetting(const std::string & key) {
	static const std::set<std::string> lists {
		"HEADER_SEARCH_PATHS", "LIBRARY_SEARCH_PATHS", "FRAMEWORK_SEARCH_PATHS", "OTHER_LDFLAGS",
		"OTHER_CFLAGS", "OTHER_CPLUSPLUSFLAGS", "GCC_PREPROCESSOR_DEFINITIONS", "USER_PREPROCESSOR_DEFINITIONS"
	};
	return lists.count(settingName(key));
}

inline std::vector<std::string> words(const std::string & text) {
	std::vector<std::string> result;
	std::string word;
	char quote = 0;
	bool escape = false;
	for (char c : text) {
		if (!quote && !escape && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
			if (!word.empty()) { result.push_back(word); word.clear(); }
		} else {
			word += c;
			if (!escape && (c == '\'' || c == '"')) {
				if (!quote) quote = c;
				else if (quote == c) quote = 0;
			}
		}
		escape = !escape && c == '\\';
	}
	if (!word.empty()) result.push_back(word);
	return result;
}

inline std::vector<std::vector<std::string>> flagUnits(const std::string & key, const json & value) {
	std::vector<std::string> tokens;
	if (value.is_array()) {
		for (auto & part : value) {
			if (!part.is_string()) continue;
			if (settingName(key).find("SEARCH_PATHS") != std::string::npos) tokens.push_back(part.get<std::string>());
			else {
				auto parsed = words(part.get<std::string>());
				tokens.insert(tokens.end(), parsed.begin(), parsed.end());
			}
		}
	} else if (value.is_string()) tokens = words(value.get<std::string>());
	static const std::set<std::string> withArgument {
		"-framework", "-weak_framework", "-force_load", "-I", "-F", "-L", "-l", "-D", "-U",
		"-isystem", "-iquote", "-include", "-Xclang", "-Xlinker"
	};
	std::vector<std::vector<std::string>> result;
	for (size_t i = 0; i < tokens.size(); ++i) {
		std::vector<std::string> unit { tokens[i] };
		if (withArgument.count(tokens[i]) && i + 1 < tokens.size()) unit.push_back(tokens[++i]);
		result.push_back(unit);
	}
	return result;
}

// Refresh generated flags, then carry only additions made by the user to the previous output.
// This retains a custom -framework pair without reviving flags from a removed addon.
inline json mergeListSetting(const std::string & key, const json & base, const json & user, const json & next) {
	auto previous = flagUnits(key, base.is_null() ? next : base);
	auto current = flagUnits(key, user);
	auto result = flagUnits(key, next);
	for (auto & unit : current) {
		if (std::find(previous.begin(), previous.end(), unit) == previous.end()
			&& std::find(result.begin(), result.end(), unit) == result.end()) result.push_back(unit);
	}
	if (result.empty()) return nullptr;
	if (next.is_null() && std::find(result.begin(), result.end(), std::vector<std::string>{ "$(inherited)" }) == result.end()) {
		result.insert(result.begin(), { "$(inherited)" });
	}
	if (next.is_array() || (next.is_null() && user.is_array())) {
		json array = json::array();
		for (auto & unit : result) for (auto & token : unit) array.push_back(token);
		return array;
	}
	std::string text;
	for (auto & unit : result) for (auto & token : unit) { if (!text.empty()) text += ' '; text += token; }
	return text;
}

inline bool generatedSetting(const std::string & key) {
	const auto name = settingName(key);
	static const std::set<std::string> generated {
		"OF_PATH", "OF_CORE_BUILD_COMMAND", "SDKROOT", "SUPPORTED_PLATFORMS", "VALID_ARCHS", "CLANG_CXX_LIBRARY",
		"HEADER_SEARCH_PATHS", "LIBRARY_SEARCH_PATHS", "FRAMEWORK_SEARCH_PATHS", "OTHER_LDFLAGS",
		"OTHER_CFLAGS", "OTHER_CPLUSPLUSFLAGS", "GCC_PREPROCESSOR_DEFINITIONS", "USER_PREPROCESSOR_DEFINITIONS",
		"OF_NO_FMOD", "LIB_FMOD", "ENABLE_USER_SCRIPT_SANDBOXING"
	};
	return generated.count(name) || name.rfind("OF_CORE_", 0) == 0;
}

inline std::vector<int> version(const std::string & value) {
	std::vector<int> result;
	std::istringstream parts(value);
	std::string part;
	while (std::getline(parts, part, '.')) {
		if (part.empty() || part.find_first_not_of("0123456789") != std::string::npos) return {};
		try { result.push_back(std::stoi(part)); } catch (...) { return {}; }
	}
	if (result.empty()) return {};
	result.resize(4);
	return result;
}

inline std::string literal(std::string value) {
	value = trim(value);
	if (value.empty()) return value;
	if (value.front() == '"' || value.front() == '\'') {
		return value.substr(1, value.find(value.front(), 1) - 1);
	}
	return value.substr(0, value.find_first_of(" \t/"));
}

inline int languageVersion(std::string value) {
	value = literal(value);
	if (value.rfind("gnu++", 0) == 0) value = value.substr(5);
	else if (value.rfind("c++", 0) == 0) value = value.substr(3);
	else if (value.rfind("gnu", 0) == 0) value = value.substr(3);
	else if (value.rfind("c", 0) == 0) value = value.substr(1);
	if (value == "2a") return 2020;
	if (value == "2b") return 2023;
	if (value == "2c") return 2026;
	if (value == "2y") return 2029;
	try {
		if (value.find_first_not_of("0123456789") != std::string::npos) return 0;
		int year = std::stoi(value);
		return year > 50 ? 1900 + year : 2000 + year;
	} catch (...) { return 0; }
}

inline bool minimumSetting(const std::string & key) {
	const auto name = settingName(key);
	return name == "CLANG_CXX_LANGUAGE_STANDARD" || name == "CLANG_C_LANGUAGE_STANDARD"
		|| name == "MACOSX_DEPLOYMENT_TARGET" || name == "IPHONEOS_DEPLOYMENT_TARGET"
		|| name == "TVOS_DEPLOYMENT_TARGET" || name == "XROS_DEPLOYMENT_TARGET";
}

inline json requiredSetting(const std::string & key, const json & user, const json & next) {
	if (generatedSetting(key)) return next;
	const auto name = settingName(key);
	if (minimumSetting(key) && user.is_string() && next.is_string()) {
		if (name == "CLANG_CXX_LANGUAGE_STANDARD" || name == "CLANG_C_LANGUAGE_STANDARD") {
			return languageVersion(user.get<std::string>()) >= languageVersion(next.get<std::string>()) ? user : next;
		}
		const auto current = version(literal(user.get<std::string>())), minimum = version(literal(next.get<std::string>()));
		return !current.empty() && !minimum.empty() && current >= minimum ? user : next;
	}
	return user;
}

inline json mergeBuildSettings(const json & base, const json & user, const json & next, const json & minimums) {
	auto result = merge(base, user, next);
	for (auto it = next.begin(); it != next.end(); ++it) {
		if (generatedSetting(it.key())) result[it.key()] = *it;
		else if (result.contains(it.key())) result[it.key()] = requiredSetting(it.key(), result[it.key()], *it);
	}
	for (auto it = result.begin(); it != result.end();) {
		if (generatedSetting(it.key()) && !listSetting(it.key()) && !next.contains(it.key())) it = result.erase(it);
		else {
			auto name = settingName(it.key());
			if (minimums.contains(name)) *it = requiredSetting(it.key(), *it, minimums[name]);
			++it;
		}
	}
	std::set<std::string> lists;
	for (auto it = user.begin(); it != user.end(); ++it) if (listSetting(it.key())) lists.insert(it.key());
	for (auto it = next.begin(); it != next.end(); ++it) if (listSetting(it.key())) lists.insert(it.key());
	for (auto & key : lists) {
		auto value = mergeListSetting(key, base.value(key, json()), user.value(key, json()), next.value(key, json()));
		if (value.is_null()) result.erase(key);
		else result[key] = value;
	}
	return result;
}

struct configLine { std::string key, op, value, text; };

inline std::vector<configLine> configLines(const std::string & text) {
	static const std::regex assignment(R"(^\s*([A-Za-z_][A-Za-z0-9_]*(?:\[[^\]]+\])*)\s*(\+=|\?=|:=|=)\s*(.*)$)");
	std::vector<configLine> result;
	std::istringstream in(text);
	std::string line;
	while (std::getline(in, line)) {
		configLine block;
		block.text = line + "\n";
		std::smatch match;
		if (std::regex_match(line, match, assignment)) {
			block.key = match[1]; block.op = match[2]; block.value = trim(match[3]);
			while (!trim(line).empty() && trim(line).back() == '\\' && std::getline(in, line)) {
				block.text += line + "\n"; block.value += "\n" + trim(line);
			}
		}
		result.push_back(block);
	}
	return result;
}

inline json configSettings(const std::vector<configLine> & lines) {
	json result = json::object();
	for (auto & line : lines) {
		if (line.key.empty()) continue;
		if (!result.contains(line.key)) result[line.key] = json::array();
		result[line.key].push_back({ { "op", line.op }, { "value", line.value } });
	}
	return result;
}

inline json configValue(const json & settings, const std::string & key) {
	if (!settings.contains(key)) return nullptr;
	std::string value;
	for (auto & assignment : settings[key]) {
		if (assignment["op"] != "+=" && assignment["op"] != "?=") value.clear();
		if (assignment["op"] == "?=" && !value.empty()) continue;
		if (!value.empty()) value += ' ';
		value += assignment["value"].get<std::string>();
	}
	return value;
}

inline std::string configAnchor(const configLine & line) {
	if (!line.key.empty()) return "key:" + line.key;
	auto text = trim(line.text);
	if (text.rfind("#include", 0) != 0) return "";
	return text.find("CoreOF.xcconfig") != std::string::npos ? "include:CoreOF" : "include:" + text;
}

inline std::string mergeConfig(const std::string & baseline, const std::string & current, const std::string & fresh, const json & minimums) {
	auto oldLines = configLines(baseline), userLines = configLines(current), newLines = configLines(fresh);
	auto base = configSettings(oldLines), user = configSettings(userLines), next = configSettings(newLines);
	auto merged = merge(base, user, next);
	std::set<std::string> anchors;
	for (auto & line : newLines) if (!configAnchor(line).empty()) anchors.insert(configAnchor(line));
	std::map<std::string, std::vector<configLine>> additions;
	std::vector<configLine> pending;
	std::set<std::string> seenUserAnchors;
	for (auto & line : userLines) {
		auto anchor = configAnchor(line);
		if (!anchor.empty() && anchors.count(anchor)) {
			if (seenUserAnchors.insert(anchor).second) {
				auto & before = additions[anchor];
				before.insert(before.end(), pending.begin(), pending.end());
				pending.clear();
			}
		} else if (!line.key.empty()) {
			if ((merged.contains(line.key) && !generatedSetting(line.key)) || listSetting(line.key)) pending.push_back(line);
		} else if (!trim(line.text).empty() && line.text.find("CoreOF.xcconfig") == std::string::npos) {
			bool known = false;
			for (auto & entry : newLines) if (trim(entry.text) == trim(line.text)) known = true;
			for (auto & entry : oldLines) if (trim(entry.text) == trim(line.text)) known = true;
			if (!known) pending.push_back(line);
		}
	}
	additions[""] = pending;
	std::set<std::string> written;
	std::set<std::string> extraLists;
	std::string out;
	auto addUserLines = [&](const std::string & anchor) {
		for (auto & line : additions[anchor]) {
			if (listSetting(line.key)) {
				if (!extraLists.insert(line.key).second) continue;
				auto value = mergeListSetting(line.key, configValue(base, line.key), configValue(user, line.key), nullptr);
				if (!value.is_null()) out += line.key + " = " + value.get<std::string>() + "\n";
			} else out += line.text;
		}
		additions.erase(anchor);
	};
	for (auto & line : newLines) {
		if (!configAnchor(line).empty()) addUserLines(configAnchor(line));
		if (line.key.empty()) { out += line.text; continue; }
		if (!written.insert(line.key).second) continue;
		if (listSetting(line.key)) {
			auto value = mergeListSetting(line.key, configValue(base, line.key), configValue(user, line.key), configValue(next, line.key));
			out += line.key + " = " + (value.is_null() ? "" : value.get<std::string>()) + "\n";
			continue;
		}
		bool useUser = merged.contains(line.key) && user.contains(line.key) && merged[line.key] == user[line.key];
		if (generatedSetting(line.key)) useUser = false;
		if (useUser && minimumSetting(line.key)) {
			for (auto & saved : userLines) if (saved.key == line.key) {
				if (requiredSetting(line.key, saved.value, line.value) != saved.value) useUser = false;
			}
		}
		if (useUser) {
			for (auto & saved : userLines) if (saved.key == line.key) out += saved.text;
		} else if (merged.contains(line.key) || generatedSetting(line.key) || minimumSetting(line.key)) {
			for (auto & entry : newLines) if (entry.key == line.key) out += entry.text;
		}
	}
	// Keep extra includes near the setting they preceded: moving one after a user override
	// would change its effective value even if the assignment itself was retained.
	addUserLines("");
	// A per-project override must not lower a minimum inherited from CoreOF.xcconfig.
	std::string checked;
	for (auto & line : configLines(out)) {
		auto name = settingName(line.key);
		if (minimums.contains(name) && requiredSetting(line.key, line.value, minimums[name]) != line.value) {
			checked += line.key + " = " + minimums[name].get<std::string>() + "\n";
		} else checked += line.text;
	}
	return checked;
}

inline json configurations(const json & project) {
	json result = json::object();
	const auto & objects = project.at("objects");
	for (auto owner = objects.begin(); owner != objects.end(); ++owner) {
		if (!owner->is_object() || !owner->contains("buildConfigurationList")) continue;
		auto list = objects.find(owner->at("buildConfigurationList").get<std::string>());
		if (list == objects.end() || !list->contains("buildConfigurations")) continue;
		std::string scope = owner->value("isa", "") + "/" + owner->value("name", "");
		for (auto & id : list->at("buildConfigurations")) {
			auto config = objects.find(id.get<std::string>());
			if (config == objects.end()) continue;
			result[id.get<std::string>()] = { { "scope", scope + "/" + config->value("name", "") },
				{ "settings", config->value("buildSettings", json::object()) } };
		}
	}
	return result;
}

inline json matchingConfiguration(const json & configs, const std::string & id, const json & config) {
	if (configs.contains(id)) return configs[id].at("settings");
	for (auto & candidate : configs) if (candidate.at("scope") == config.at("scope")) return candidate.at("settings");
	return nullptr;
}
} // namespace xcodeSettings
