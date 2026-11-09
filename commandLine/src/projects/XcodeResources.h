#pragma once

#include "XcodeSettings.h"
#include <filesystem>
#include <functional>

namespace xcodeResources {
using nlohmann::json;
namespace fs = std::filesystem;

// Xcode groups can be virtual, relative to another group, or rooted elsewhere.
// File identity uses the resolved location, not UUIDs which a template can change.
struct graph {
	const json & objects;
	std::string mainGroup;
	std::map<std::string, std::string> parents;

	explicit graph(const json & project) : objects(project.at("objects")) {
		mainGroup = objects.at(project.at("rootObject").get<std::string>()).at("mainGroup");
		for (auto it = objects.begin(); it != objects.end(); ++it) {
			if (!it->is_object()) continue;
			auto isa = it->value("isa", "");
			if (isa != "PBXGroup" && isa != "PBXVariantGroup") continue;
			for (auto & child : it->value("children", json::array())) {
				std::string id = child;
				if (!objects.contains(id)) throw std::runtime_error("missing Xcode group child: " + id);
				parents.emplace(id, it.key());
			}
		}
	}

	std::pair<std::string, fs::path> location(const std::string & id, std::set<std::string> seen = {}) const {
		if (!seen.insert(id).second) throw std::runtime_error("cyclic Xcode resource group");
		const auto & node = objects.at(id);
		std::string isa = node.value("isa", ""), tree = node.value("sourceTree", "<group>");
		fs::path path = node.value("path", isa == "PBXGroup" ? "" : node.value("name", ""));
		std::pair<std::string, fs::path> base { "SOURCE_ROOT", "" };
		if (tree == "<group>") {
			auto parent = parents.find(id);
			if (parent != parents.end()) {
				// A variant group names a localized resource, not a directory.
				auto owner = parent->second;
				if (objects.at(owner).value("isa", "") == "PBXVariantGroup") {
					auto grandparent = parents.find(owner);
					owner = grandparent == parents.end() ? mainGroup : grandparent->second;
				}
				base = location(owner, seen);
			}
		} else base.first = tree;
		for (const std::string prefix : { "$(SRCROOT)/", "${SRCROOT}/", "$(PROJECT_DIR)/", "${PROJECT_DIR}/" }) {
			if (path.generic_string().rfind(prefix, 0) == 0) {
				path = path.generic_string().substr(prefix.size()); base = { "SOURCE_ROOT", "" }; break;
			}
		}
		if (path.is_absolute()) base = { "<absolute>", "" };
		base.second = (base.second / path).lexically_normal();
		return base;
	}

	std::string key(const std::string & id) const {
		auto path = location(id);
		return objects.at(id).value("isa", "") + ":" + path.first + ":" + path.second.generic_string();
	}

	std::string groupKey(const std::string & id, std::set<std::string> seen = {}) const {
		if (!seen.insert(id).second) throw std::runtime_error("cyclic Xcode navigator group");
		if (id == mainGroup) return "<main>";
		const auto & node = objects.at(id);
		auto parent = parents.find(id);
		return (parent == parents.end() ? "" : groupKey(parent->second, seen)) + "/"
			+ node.value("name", node.value("path", "")) + "|" + key(id);
	}
};

inline bool resource(const json & node) {
	if (node.value("isa", "") == "PBXVariantGroup") return true;
	if (node.value("isa", "") != "PBXFileReference") return false;
	const auto type = node.value("explicitFileType", node.value("lastKnownFileType", ""));
	if (type == "folder" || type == "folder.assetcatalog" || type.rfind("image.", 0) == 0) return true;
	static const std::set<std::string> extensions {
		".icns", ".icon", ".xcassets", ".png", ".jpg", ".jpeg", ".gif", ".pdf", ".svg", ".webp", ".tiff", ".tif", ".bmp", ".ico",
		".storyboard", ".xib", ".strings", ".stringsdict", ".xcstrings", ".bundle", ".plist", ".entitlements", ".xcconfig", ".json", ".txt",
		".wav", ".mp3", ".aiff", ".caf", ".mp4", ".mov", ".scn", ".sks", ".reality", ".mlmodel", ".mlpackage"
	};
	return extensions.count(fs::path(node.value("path", node.value("name", ""))).extension().string());
}

inline json snapshot(const json & project) {
	json result { { "references", json::object() }, { "targets", json::object() } };
	if (project.empty()) return result;
	graph g(project);
	std::set<std::string> active;
	std::function<std::string(const std::string &)> reference = [&](const std::string & id) {
		const auto & node = g.objects.at(id);
		const auto isa = node.value("isa", "");
		if (isa != "PBXFileReference" && isa != "PBXVariantGroup") throw std::runtime_error("unsupported Xcode resource reference: " + isa);
		const auto key = g.key(id);
		if (!active.insert(id).second) throw std::runtime_error("cyclic localized resource");
		json properties = node, children = json::array();
		properties.erase("children");
		for (auto & child : node.value("children", json::array())) children.push_back(reference(child.get<std::string>()));
		active.erase(id);
		result["references"][key] = { { "id", id }, { "properties", properties }, { "children", children } };
		return key;
	};
	for (auto it = g.objects.begin(); it != g.objects.end(); ++it) if (it->is_object() && resource(*it)) reference(it.key());
	const auto & root = g.objects.at(project.at("rootObject").get<std::string>());
	for (auto & targetId : root.value("targets", json::array())) {
		const auto & target = g.objects.at(targetId.get<std::string>());
		if (target.value("productType", "") != "com.apple.product-type.application") continue;
		json phases = json::object();
		std::map<std::string, int> ordinals;
		for (auto & phaseId : target.value("buildPhases", json::array())) {
			const auto & phase = g.objects.at(phaseId.get<std::string>());
			if (phase.value("isa", "") != "PBXResourcesBuildPhase") continue;
			std::string name = phase.value("name", "");
			std::string key = name + "#" + std::to_string(ordinals[name]++);
			json properties = phase, files = json::object();
			properties.erase("files");
			for (auto & buildId : phase.value("files", json::array())) {
				const auto & build = g.objects.at(buildId.get<std::string>());
				std::string file = reference(build.at("fileRef").get<std::string>());
				json settings = build;
				settings.erase("fileRef");
				std::string membership = file + "|" + build.value("platformFilter", "") + "|" + build.value("platformFilters", json::array()).dump();
				files[membership] = { { "id", buildId }, { "file", file }, { "build", settings } };
			}
			phases[key] = { { "id", phaseId }, { "properties", properties }, { "files", files } };
		}
		result["targets"][targetId.get<std::string>()] = { { "scope", target.value("name", "") }, { "phases", phases } };
	}
	return result;
}

inline json defaults(json snapshot) {
	// Generated file/build UUIDs are incidental, and may change on every update.
	for (auto & ref : snapshot["references"]) ref.erase("id");
	for (auto & target : snapshot["targets"]) for (auto & phase : target["phases"]) {
		phase.erase("id");
		for (auto & file : phase["files"]) file.erase("id");
	}
	return snapshot;
}

inline void validate(const json & baseline) {
	if (baseline.empty()) return;
	if (!baseline.at("references").is_object() || !baseline.at("targets").is_object()) throw std::runtime_error("invalid resource baseline");
	for (auto & ref : baseline.at("references")) {
		if (!ref.at("properties").is_object() || !ref.at("children").is_array()) throw std::runtime_error("invalid resource baseline reference");
		for (auto & child : ref.at("children")) if (!child.is_string() || !baseline.at("references").contains(child.get<std::string>())) throw std::runtime_error("invalid localized resource baseline");
	}
	for (auto & target : baseline.at("targets")) {
		if (!target.at("scope").is_string() || !target.at("phases").is_object()) throw std::runtime_error("invalid resource target baseline");
		for (auto & phase : target.at("phases")) {
			if (!phase.at("properties").is_object() || !phase.at("files").is_object()) throw std::runtime_error("invalid resource phase baseline");
			for (auto & file : phase.at("files")) if (!file.at("file").is_string() || !file.at("build").is_object()
				|| !baseline.at("references").contains(file.at("file").get<std::string>())) throw std::runtime_error("invalid resource membership baseline");
		}
	}
}

inline json matchingTarget(const json & targets, const std::string & id, const json & target) {
	if (targets.contains(id)) return targets[id];
	for (auto & entry : targets) if (entry.at("scope") == target.at("scope")) return entry;
	return { { "phases", json::object() } };
}

inline json merge(json & project, const json & previous, const json & baseline, const fs::path & projectDir,
	const fs::path & ofRoot, const std::function<std::string()> & newId) {
	const auto generated = snapshot(project);
	if (previous.empty()) return defaults(generated);
	const auto old = snapshot(previous);
	const auto baseRefs = baseline.value("references", json::object());
	graph before(previous), next(project);
	auto & objects = project["objects"];
	std::map<std::string, std::string> imported, groups;
	for (auto it = next.objects.begin(); it != next.objects.end(); ++it) if (it->is_object() && it->value("isa", "") == "PBXGroup") groups[next.groupKey(it.key())] = it.key();
	const auto unique = [&](const std::string & id) { return objects.contains(id) ? newId() : id; };
	const auto attach = [&](const std::string & parent, const std::string & id) {
		auto & children = objects[parent]["children"];
		if (children.is_null()) children = json::array();
		if (std::find(children.begin(), children.end(), id) == children.end()) children.push_back(id);
	};
	const auto managed = [&](const std::string & id) {
		auto location = before.location(id);
		if (location.first != "SOURCE_ROOT" && location.first != "<absolute>") return false;
		fs::path directory = fs::absolute(projectDir).lexically_normal();
		fs::path path = location.first == "SOURCE_ROOT" ? directory / location.second : location.second;
		for (auto folder : { fs::absolute(ofRoot) / "addons", directory / "local_addons" }) {
			auto relative = path.lexically_normal().lexically_relative(folder.lexically_normal());
			if (!relative.empty() && *relative.begin() != "..") return true;
		}
		return false;
	};
	std::function<std::string(const std::string &)> importReference, importGroup;
	importGroup = [&](const std::string & id) -> std::string {
		if (id.empty() || id == before.mainGroup) return next.mainGroup;
		const auto & node = before.objects.at(id);
		if (node.value("isa", "") == "PBXVariantGroup") return importReference(before.key(id));
		if (node.value("isa", "") != "PBXGroup") throw std::runtime_error("unsupported resource parent group");
		const auto key = before.groupKey(id);
		if (groups.count(key)) return groups[key];
		std::string parent = importGroup(before.parents.count(id) ? before.parents.at(id) : "");
		std::string result = unique(id);
		objects[result] = node; objects[result]["children"] = json::array(); groups[key] = result;
		attach(parent, result);
		return result;
	};
	importReference = [&](const std::string & key) -> std::string {
		if (imported.count(key)) return imported[key];
		const auto & ref = old.at("references").at(key);
		std::string oldId = ref.at("id"), id;
		const bool exists = generated.at("references").contains(key);
		if (!exists && (baseRefs.contains(key) || managed(oldId))) return "";
		auto parent = before.parents.find(oldId);
		if (!exists && parent != before.parents.end() && before.objects.at(parent->second).value("isa", "") == "PBXVariantGroup") {
			auto parentKey = before.key(parent->second);
			if (baseRefs.contains(parentKey) && !generated.at("references").contains(parentKey)) return "";
		}
		json children = json::array(), properties = ref.at("properties");
		if (exists) {
			id = generated["references"][key]["id"];
			children = objects[id].value("children", json::array());
			properties = xcodeSettings::merge(baseRefs.contains(key) ? baseRefs[key]["properties"] : json::object(), properties, generated["references"][key]["properties"]);
			// Keep the regenerated group's path representation for the same location.
			for (auto field : { "path", "sourceTree" }) {
				if (generated["references"][key]["properties"].contains(field)) properties[field] = generated["references"][key]["properties"][field];
				else properties.erase(field);
			}
		} else id = unique(oldId);
		imported[key] = id;
		objects[id] = properties;
		if (properties.value("isa", "") == "PBXVariantGroup") {
			objects[id]["children"] = children;
			const auto oldChildren = ref.at("children");
			if (baseRefs.contains(key) && exists) for (auto & child : baseRefs[key]["children"]) {
				if (std::find(oldChildren.begin(), oldChildren.end(), child) != oldChildren.end() || !generated["references"].contains(child.get<std::string>())) continue;
				auto childId = generated["references"][child.get<std::string>()]["id"];
				auto & list = objects[id]["children"]; list.erase(std::remove(list.begin(), list.end(), childId), list.end());
			}
			for (auto & child : oldChildren) { auto childId = importReference(child.get<std::string>()); if (!childId.empty()) attach(id, childId); }
		}
		if (!exists) attach(importGroup(before.parents.count(oldId) ? before.parents.at(oldId) : ""), id);
		return id;
	};
	for (auto it = old.at("references").begin(); it != old.at("references").end(); ++it) importReference(it.key());
	for (auto target = generated.at("targets").begin(); target != generated.at("targets").end(); ++target) {
		auto user = matchingTarget(old.at("targets"), target.key(), *target);
		auto base = matchingTarget(baseline.value("targets", json::object()), target.key(), *target);
		std::set<std::string> phases;
		for (auto it = user.at("phases").begin(); it != user.at("phases").end(); ++it) phases.insert(it.key());
		for (auto it = base.at("phases").begin(); it != base.at("phases").end(); ++it) phases.insert(it.key());
		for (auto it = target->at("phases").begin(); it != target->at("phases").end(); ++it) phases.insert(it.key());
		for (auto & key : phases) {
			json current = target->at("phases").value(key, json::object());
			json edited = user.at("phases").value(key, json::object()), prior = base.at("phases").value(key, json::object());
			std::string phaseId = current.value("id", "");
			const auto entries = edited.value("files", json::object()), oldEntries = prior.value("files", json::object()), freshEntries = current.value("files", json::object());
			if (!phaseId.empty()) {
				auto properties = xcodeSettings::merge(prior.value("properties", json::object()), edited.value("properties", current.at("properties")), current.at("properties"));
				properties["files"] = objects[phaseId]["files"]; objects[phaseId] = properties;
				for (auto it = oldEntries.begin(); it != oldEntries.end(); ++it) if (!entries.contains(it.key()) && freshEntries.contains(it.key())) {
					auto id = freshEntries[it.key()]["id"];
					auto & files = objects[phaseId]["files"]; files.erase(std::remove(files.begin(), files.end(), id), files.end());
				}
				// A newly recognized file must keep the user's existing target membership.
				for (auto entry = freshEntries.begin(); entry != freshEntries.end(); ++entry) {
					std::string file = entry->at("file");
					if (!user.contains("scope") || entries.contains(entry.key()) || baseRefs.contains(file)
						|| !old.at("references").contains(file) || managed(old["references"][file]["id"])) continue;
					auto & files = objects[phaseId]["files"]; auto id = entry->at("id");
					files.erase(std::remove(files.begin(), files.end(), id), files.end());
				}
			}
			for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
				std::string file = importReference(entry->at("file"));
				if (file.empty()) continue;
				if (freshEntries.contains(entry.key())) {
					auto build = xcodeSettings::merge(oldEntries.contains(entry.key()) ? oldEntries[entry.key()]["build"] : json::object(), entry->at("build"), freshEntries[entry.key()]["build"]);
					build["fileRef"] = file; objects[freshEntries[entry.key()]["id"].get<std::string>()] = build;
				} else if (!oldEntries.contains(entry.key())) {
					if (phaseId.empty()) {
						phaseId = unique(edited.at("id")); objects[phaseId] = edited.at("properties"); objects[phaseId]["files"] = json::array();
						objects[target.key()]["buildPhases"].push_back(phaseId);
					}
					std::string id = unique(entry->at("id")); objects[id] = entry->at("build"); objects[id]["fileRef"] = file; objects[phaseId]["files"].push_back(id);
				}
			}
		}
	}
	return defaults(generated);
}
} // namespace xcodeResources
