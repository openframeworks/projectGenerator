#!/usr/bin/env python3
"""End-to-end Xcode resource updates, including user edits made by Xcode."""
import json
import pathlib
import plistlib
import shutil
import sys
import tempfile
import uuid

from test_xcode_settings import openstep, read_json, run, write_json


def uid():
    return uuid.uuid4().hex.upper()[:24]


def targets(project):
    return {key: project["objects"][key] for key in project["objects"][project["rootObject"]]["targets"]
            if project["objects"][key].get("productType") == "com.apple.product-type.application"}


def phase(project, target, name=""):
    for key in target["buildPhases"]:
        obj = project["objects"][key]
        if obj["isa"] == "PBXResourcesBuildPhase" and obj.get("name", "") == name:
            return key
    key = uid()
    project["objects"][key] = {"isa": "PBXResourcesBuildPhase", "files": [], "buildActionMask": "2147483647",
                               "runOnlyForDeploymentPostprocessing": "0", **({"name": name} if name else {})}
    target["buildPhases"].append(key)
    return key


def add_file(project, parent, path, filetype, tree="<group>"):
    key = uid()
    project["objects"][key] = {"isa": "PBXFileReference", "path": path, "sourceTree": tree, "lastKnownFileType": filetype}
    project["objects"][parent].setdefault("children", []).append(key)
    return key


def add_build(project, phase_id, file_id, **properties):
    key = uid()
    project["objects"][key] = {"isa": "PBXBuildFile", "fileRef": file_id, **properties}
    project["objects"][phase_id]["files"].append(key)
    return key


def path_for(project, key, seen=None):
    seen = set() if seen is None else seen
    assert key not in seen, "cyclic resource group"
    seen.add(key)
    obj = project["objects"][key]
    path = obj.get("path", obj.get("name", "") if obj["isa"] != "PBXGroup" else "").replace("\\", "/")
    if obj.get("sourceTree", "<group>") == "<group>":
        parent = next((parent_id for parent_id, node in project["objects"].items() if key in node.get("children", [])), None)
        if parent:
            if project["objects"][parent]["isa"] == "PBXVariantGroup":
                parent = next((parent_id for parent_id, node in project["objects"].items() if parent in node.get("children", [])), None)
            if parent:
                path = str(pathlib.PurePosixPath(path_for(project, parent, seen)) / path)
    return path


def resources(project, target, name=""):
    result = []
    for phase_id in target["buildPhases"]:
        p = project["objects"][phase_id]
        if p["isa"] != "PBXResourcesBuildPhase" or p.get("name", "") != name:
            continue
        for build_id in p["files"]:
            build = project["objects"][build_id]
            result.append((path_for(project, build["fileRef"]), build))
    return result


def exercise(pg, root, platform, legacy=False):
    project = root / "apps" / ("resources_" + platform + ("_legacy" if legacy else ""))
    template = root / "scripts/templates" / platform / "emptyExample.xcodeproj/project.pbxproj"
    original_template = template.read_bytes()
    t = read_json(template)
    # A generated resource removed by the next template must not come back.
    template_parent = t["objects"][t["rootObject"]]["mainGroup"]
    generated = add_file(t, template_parent, "old-generated.json", "text.json", "SOURCE_ROOT")
    template_variant = uid()
    t["objects"][template_variant] = {"isa": "PBXVariantGroup", "name": "Template.strings", "sourceTree": "<group>", "children": []}
    t["objects"][template_parent]["children"].append(template_variant)
    template_english = add_file(t, template_variant, "en.lproj/Template.strings", "text.plist.strings")
    for target in targets(t).values():
        add_build(t, phase(t, target), generated)
        add_build(t, phase(t, target), template_variant)
    write_json(template, t)
    run(pg, root, platform, project)
    pbx = project / (project.name + ".xcodeproj/project.pbxproj")
    data = read_json(pbx)
    objects = data["objects"]
    parent = objects[data["rootObject"]]["mainGroup"]
    artwork, virtual = uid(), uid()
    objects[artwork] = {"isa": "PBXGroup", "path": "Artwork", "name": "Artwork", "sourceTree": "<group>", "children": [virtual]}
    objects[virtual] = {"isa": "PBXGroup", "name": "App icons", "sourceTree": "<group>", "children": []}
    objects[parent]["children"].append(artwork)
    icon = add_file(data, virtual, "macIcon.icns", "image.icns")
    catalog = add_file(data, artwork, "Assets.xcassets", "folder.assetcatalog")
    navigator = add_file(data, virtual, "preview.png", "image.png")
    folder = add_file(data, artwork, "Presets", "folder")
    add_file(data, artwork, "App.entitlements", "text.plist.entitlements")
    src_icon = add_file(data, artwork, "src/sourceIcon.icns", "image.icns", "SOURCE_ROOT")
    add_file(data, artwork, "src/navigatorOnly.icns", "image.icns", "SOURCE_ROOT")
    localized = uid()
    objects[localized] = {"isa": "PBXVariantGroup", "name": "Localizable.strings", "sourceTree": "<group>", "children": []}
    objects[artwork]["children"].append(localized)
    add_file(data, localized, "en.lproj/Localizable.strings", "text.plist.strings")
    add_file(data, localized, "fr.lproj/Localizable.strings", "text.plist.strings")
    first = next(iter(targets(data)))
    target = objects[first]
    for config_id in objects[target["buildConfigurationList"]]["buildConfigurations"]:
        objects[config_id].setdefault("buildSettings", {})["CODE_SIGN_ENTITLEMENTS"] = "Artwork/App.entitlements"
    objects[template_variant]["children"].remove(template_english)
    del objects[template_english]
    add_file(data, template_variant, "fr.lproj/Template.strings", "text.plist.strings")
    main_phase = phase(data, target)
    icon_build = add_build(data, main_phase, icon, settings={"ATTRIBUTES": ["RemoveHeadersOnCopy"]}, platformFilters=["macos"])
    add_build(data, main_phase, catalog)
    add_build(data, main_phase, src_icon)
    add_build(data, main_phase, localized)
    extra_phase = phase(data, target, "Extra resources")
    add_build(data, extra_phase, folder, platformFilters=["ios", "macos"])
    # Actual addon resource generation must refresh when its config changes.
    addon = root / "addons/ofxPGResourceTest"
    (addon / "src").mkdir(parents=True, exist_ok=True)
    (addon / "src/addon.xib").write_text('<document type="com.apple.InterfaceBuilder3.Cocoa.XIB"/>')
    (addon / "addon_config.mk").write_text("meta:\n\tADDON_NAME = ofxPGResourceTest\ncommon:\n\tADDON_SOURCES = src/addon.xib\n")
    files = {"Artwork/macIcon.icns": b"custom icon bytes", "Artwork/preview.png": b"custom preview bytes",
             "src/sourceIcon.icns": b"custom source icon", "src/navigatorOnly.icns": b"not bundled",
             "Artwork/Assets.xcassets/Contents.json": b'{"info":{"version":1,"author":"xcode"}}',
             "Artwork/Assets.xcassets/AppIcon.appiconset/Contents.json": b'{"images":[],"info":{"version":1,"author":"xcode"}}',
             "Artwork/Presets/default.json": b'{"value":42}',
             "Artwork/en.lproj/Localizable.strings": b'"title" = "My app";',
             "Artwork/fr.lproj/Localizable.strings": b'"title" = "Mon app";'}
    files["fr.lproj/Template.strings"] = b'"title" = "Mon template";'
    files["Artwork/App.entitlements"] = plistlib.dumps({"com.apple.security.app-sandbox": True,
        "com.apple.security.network.client": True, "com.apple.security.application-groups": ["group.my.custom.app"]})
    for name, contents in files.items():
        file = project / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_bytes(contents)
    # Xcode's native save format must be preserved as input, not just PG's JSON.
    pbx.write_text("// !$*UTF8*$!\n" + openstep(data) if sys.platform == "darwin" else json.dumps(data))
    if legacy:
        (project / ".ofprojectgenerator/apple-settings.json").unlink()
    # Start from the updated template, and force a UUID collision with a user's icon.
    upgraded = json.loads(original_template)
    upgraded["objects"][icon] = {"isa": "PBXFileReference", "path": "new-generated.json", "sourceTree": "SOURCE_ROOT", "lastKnownFileType": "text.json"}
    upgraded["objects"][upgraded["objects"][upgraded["rootObject"]]["mainGroup"]]["children"].append(icon)
    new_variant = uid()
    upgraded["objects"][new_variant] = {"isa": "PBXVariantGroup", "name": "Template.strings", "sourceTree": "<group>", "children": []}
    upgraded["objects"][upgraded["objects"][upgraded["rootObject"]]["mainGroup"]]["children"].append(new_variant)
    add_file(upgraded, new_variant, "en.lproj/Template.strings", "text.plist.strings")
    add_file(upgraded, new_variant, "es.lproj/Template.strings", "text.plist.strings")
    for index, target in enumerate(targets(upgraded).values()):
        p = phase(upgraded, target)
        build_id = icon_build if index == 0 else uid()
        upgraded["objects"][build_id] = {"isa": "PBXBuildFile", "fileRef": icon}
        upgraded["objects"][p]["files"].append(build_id)
        add_build(upgraded, p, new_variant)
    # Match resources by target/phase semantics even when template UUIDs change.
    changed = {key: uid() for key, obj in upgraded["objects"].items()
               if obj.get("isa") in ("PBXNativeTarget", "PBXResourcesBuildPhase", "XCBuildConfiguration")}

    def remap(value):
        if isinstance(value, dict):
            return {changed.get(key, key): remap(item) for key, item in value.items()}
        if isinstance(value, list):
            return [remap(item) for item in value]
        return changed.get(value, value) if isinstance(value, str) else value

    upgraded = remap(upgraded)
    write_json(template, upgraded)
    run(pg, root, platform, project, "-aofxPGResourceTest")
    assert "ofxPGResourceTest" in pbx.read_text() and "addon.xib" in pbx.read_text(), "fixture must include a generated addon resource"

    def check():
        updated = read_json(pbx)
        first_target = next(iter(targets(updated).values()))
        entries = resources(updated, first_target)
        assert sum(path == "Artwork/macIcon.icns" for path, _ in entries) == 1, "custom icon must remain in Copy Bundle Resources"
        icon_settings = next(build for path, build in entries if path == "Artwork/macIcon.icns")
        assert icon_settings["settings"] == {"ATTRIBUTES": ["RemoveHeadersOnCopy"]}
        assert icon_settings["platformFilters"] == ["macos"]
        assert sum(path == "Artwork/Assets.xcassets" for path, _ in entries) == 1
        assert sum(path == "src/sourceIcon.icns" for path, _ in entries) == 1
        assert not any(path == "src/navigatorOnly.icns" for path, _ in entries), "newly recognized resource types must keep navigator-only membership"
        assert sum(path == "Artwork/Localizable.strings" for path, _ in entries) == 1
        assert any(path.endswith("ofxPGResourceTest/src/addon.xib") for path, _ in entries), "addon resources must use a valid per-target resource phase"
        extra = resources(updated, first_target, "Extra resources")
        assert len(extra) == 1 and extra[0][0] == "Artwork/Presets"
        assert extra[0][1]["platformFilters"] == ["ios", "macos"]
        refs = [node for node in updated["objects"].values() if node.get("isa") == "PBXFileReference"]
        assert any(node.get("path") == "preview.png" for node in refs), "navigator-only resources must remain"
        assert any(node.get("path") == "App.entitlements" for node in refs), "custom entitlement file references must remain"
        for config_id in updated["objects"][first_target["buildConfigurationList"]]["buildConfigurations"]:
            assert updated["objects"][config_id]["buildSettings"]["CODE_SIGN_ENTITLEMENTS"] == "Artwork/App.entitlements"
        assert any(node.get("path") == "en.lproj/Localizable.strings" for node in refs)
        assert any(node.get("path") == "fr.lproj/Localizable.strings" for node in refs)
        assert any(node.get("path") == "fr.lproj/Template.strings" for node in refs)
        merged_variant = next(obj for obj in updated["objects"].values() if obj.get("isa") == "PBXVariantGroup" and obj.get("name") == "Template.strings")
        locales = {updated["objects"][child]["path"] for child in merged_variant["children"]}
        assert "fr.lproj/Template.strings" in locales and "es.lproj/Template.strings" in locales
        if not legacy:
            assert "en.lproj/Template.strings" not in locales, "deleted localizations must remain deleted when a baseline exists"
        for other in list(targets(updated).values())[1:]:
            assert not any(path.startswith("Artwork/") for path, _ in resources(updated, other)), "custom resources must not leak to other targets"
            assert not any(path in ("src/sourceIcon.icns", "src/navigatorOnly.icns") for path, _ in resources(updated, other))
        for name, contents in files.items():
            assert (project / name).read_bytes() == contents, "resource contents must never change"
        assert all("isa" in obj for obj in updated["objects"].values())
        if not legacy:
            assert not any(path == "old-generated.json" for path, _ in entries), "removed generated resources must not return"
        return updated

    check()
    # A saved baseline records generated references, never the preserved user entries.
    state = read_json(project / ".ofprojectgenerator/apple-settings.json")
    recorded = state["platforms"][platform]["resources"]
    assert not any("Artwork/" in key for key in recorded["references"])
    stable_baseline = (project / ".ofprojectgenerator/apple-settings.json").read_bytes()
    run(pg, root, platform, project, "-aofxPGResourceTest")
    assert stable_baseline == (project / ".ofprojectgenerator/apple-settings.json").read_bytes(), "resource baselines must be stable across repeated updates"
    check()
    # Removing a resource from Xcode should stick; the physical file can remain.
    edited = read_json(pbx)
    remove = next(key for key, node in edited["objects"].items() if node.get("isa") == "PBXFileReference" and path_for(edited, key) == "Artwork/macIcon.icns")
    removed_builds = {key for key, node in edited["objects"].items() if node.get("fileRef") == remove}
    for node in edited["objects"].values():
        for field in ("children", "files"):
            if field in node:
                node[field] = [key for key in node[field] if key != remove and key not in removed_builds]
    for key in removed_builds | {remove}:
        del edited["objects"][key]
    write_json(pbx, edited)
    run(pg, root, platform, project, "-a")
    final = read_json(pbx)
    assert not any(node.get("path") == "macIcon.icns" for node in final["objects"].values()), "a user-removed icon must not return"
    assert "ofxPGResourceTest" not in pbx.read_text(), "removed addon references must not be restored"
    assert (project / "Artwork/macIcon.icns").read_bytes() == files["Artwork/macIcon.icns"]
    # Invalid resource baselines stop before template copies.
    state_file = project / ".ofprojectgenerator/apple-settings.json"
    bad = read_json(state_file)
    bad["platforms"][platform]["resources"] = {"references": [], "targets": {}}
    write_json(state_file, bad)
    before = pbx.read_bytes()
    output = run(pg, root, platform, project, success=False)
    assert "original settings kept" in output and pbx.read_bytes() == before
    template.write_bytes(original_template)
    print("ok", platform, "legacy" if legacy else "baseline", "icons, catalogs, groups, localization, memberships, removals and repeat updates", flush=True)


def generated_resources(pg, root, platform):
    template = root / "scripts/templates" / platform / "emptyExample.xcodeproj/project.pbxproj"
    original = template.read_bytes()
    data = read_json(template)
    for target in targets(data).values():
        target["buildPhases"] = [key for key in target["buildPhases"] if data["objects"][key]["isa"] != "PBXResourcesBuildPhase"]
    write_json(template, data)
    project = root / "apps" / ("generated_resources_" + platform)
    run(pg, root, platform, project, "-aofxPGResourceTest")
    (project / "src/autoIcon.icns").write_bytes(b"automatically discovered icon")
    for _ in range(2):
        run(pg, root, platform, project)
        pbx = read_json(project / (project.name + ".xcodeproj/project.pbxproj"))
        assert all("isa" in node for node in pbx["objects"].values()), "resources must not create orphan Xcode objects"
        for target in targets(pbx).values():
            entries = resources(pbx, target)
            assert sum(path.endswith("ofxPGResourceTest/src/addon.xib") for path, _ in entries) == 1
            assert sum(path == "src/autoIcon.icns" for path, _ in entries) == 1
            for phase_id in target["buildPhases"]:
                phase = pbx["objects"][phase_id]
                if phase["isa"] == "PBXSourcesBuildPhase":
                    paths = [path_for(pbx, pbx["objects"][key]["fileRef"]) for key in phase["files"]]
                    assert not any(path.endswith(("addon.xib", "autoIcon.icns")) for path in paths)
    template.write_bytes(original)
    print("ok", platform, "generated resource phases, addon resources and source icons", flush=True)


def main():
    pg, source = (pathlib.Path(arg).resolve() for arg in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="pg-xcode-resources-") as directory:
        root = pathlib.Path(directory)
        for folder in ("apps", "addons", "libs/openFrameworks"):
            (root / folder).mkdir(parents=True)
        shutil.copytree(source / "scripts/templates", root / "scripts/templates")
        shutil.copytree(source / "libs/openFrameworksCompiled/project", root / "libs/openFrameworksCompiled/project")
        for platform in ("osx", "macos", "ios"):
            exercise(pg, root, platform)
            exercise(pg, root, platform, legacy=True)
            generated_resources(pg, root, platform)
        if sys.platform != "win32":
            # POSIX permits these literal characters, reproducing JSON escaping
            # failures without needing a Windows host for the regression test.
            escaped_root = pathlib.Path(directory) / 'OF "quoted" root\\resources'
            for folder in ("apps", "addons", "libs/openFrameworks"):
                (escaped_root / folder).mkdir(parents=True)
            shutil.copytree(source / "scripts/templates", escaped_root / "scripts/templates")
            shutil.copytree(source / "libs/openFrameworksCompiled/project", escaped_root / "libs/openFrameworksCompiled/project")
            shutil.copytree(root / "addons/ofxPGResourceTest", escaped_root / "addons/ofxPGResourceTest")
            generated_resources(pg, escaped_root, "osx")
            print("ok JSON template substitutions with quotes, spaces and literal backslashes", flush=True)
    print("Xcode resource preservation tests passed")


if __name__ == "__main__":
    main()
