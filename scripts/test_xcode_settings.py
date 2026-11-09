#!/usr/bin/env python3
"""Generate, customize, upgrade and update Apple projects in an isolated OF fixture.

Usage: test_xcode_settings.py <projectGenerator> <openFrameworks root>
"""
import datetime
import json
import os
import pathlib
import plistlib
import shutil
import subprocess
import sys
import tempfile
import uuid


def read_json(path):
    return json.loads(path.read_text())


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def read_plist(path):
    return plistlib.loads(path.read_bytes())


def write_plist(path, value, binary=False):
    path.write_bytes(plistlib.dumps(value, fmt=plistlib.FMT_BINARY if binary else plistlib.FMT_XML))


def configs(project):
    return {key: obj for key, obj in project["objects"].items() if obj.get("isa") == "XCBuildConfiguration"}


def linked_sdk(project, target_id):
    objects = project["objects"]
    files = {}
    for phase_id in objects[target_id]["buildPhases"]:
        phase = objects[phase_id]
        if phase.get("isa") != "PBXFrameworksBuildPhase":
            continue
        for build_id in phase.get("files", []):
            build = objects[build_id]
            ref = objects.get(build.get("fileRef"), {})
            if ref.get("sourceTree") == "SDKROOT":
                files[ref.get("path", ref.get("name", ""))] = build.get("settings", {})
    return files


def openstep(value):
    if isinstance(value, dict):
        return "{\n" + "\n".join(json.dumps(k) + " = " + openstep(v) + ";" for k, v in value.items()) + "\n}"
    if isinstance(value, list):
        return "(" + ",".join(openstep(v) for v in value) + ")"
    return json.dumps(str(value))


def run(pg, root, platform, project, *extra, success=True):
    result = subprocess.run([str(pg), "-o" + str(root), "-p" + platform, *extra, str(project)],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                            env={**os.environ, "TZ": "UTC0"})
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout)
    return result.stdout


def exercise(pg, root, platform):
    print("testing [" + platform + "]", flush=True)
    template = root / "scripts/templates" / platform
    plist_name = "openFrameworks-Info.plist" if platform in ("osx", "macos") else "ofxiOS-Info.plist"
    plist_template = template / plist_name
    template_project = template / "emptyExample.xcodeproj/project.pbxproj"

    # Known defaults make this a real template upgrade test, rather than just another update.
    defaults = read_plist(plist_template)
    defaults["PGTemplateDefault"] = "before"
    defaults["PGNested"] = {"unchanged": "before", "custom": "before"}
    write_plist(plist_template, defaults)
    config_template = template / "Project.xcconfig"
    config_template.write_text(config_template.read_text() + "\nPG_TEMPLATE_DEFAULT = before\n")
    pbx_defaults = read_json(template_project)
    for obj in configs(pbx_defaults).values():
        obj.setdefault("buildSettings", {})["PG_TEMPLATE_DEFAULT"] = "before"
    write_json(template_project, pbx_defaults)

    project = root / "apps" / ("settings_" + platform)
    run(pg, root, platform, project)
    pbx_path = project / (project.name + ".xcodeproj/project.pbxproj")
    plist_path = project / plist_name
    config_path = project / "Project.xcconfig"
    state = project / ".ofprojectgenerator/apple-settings.json"
    assert state.is_file(), "new projects must record a baseline"
    workspace = project / (project.name + ".xcodeproj/project.xcworkspace/contents.xcworkspacedata")
    workspace.parent.mkdir(parents=True, exist_ok=True)
    workspace.write_text('<Workspace version="1.0"><FileRef location="group:CustomPackage.xcodeproj"/></Workspace>\n')
    template_workspace = template / "emptyExample.xcodeproj/project.xcworkspace/contents.xcworkspacedata"
    template_workspace_before = template_workspace.read_bytes() if template_workspace.exists() else None

    settings = read_plist(plist_path)
    settings["NSHighResolutionCapable"] = True
    settings["NSCameraUsageDescription"] = "Camera access for my installation"
    settings["PGNested"]["custom"] = "mine"
    settings["PGCustomTypes"] = {"data": b"\x00\xff\x01", "date": datetime.datetime(2025, 1, 2),
                                 "number": 42, "real": 1.25, "array": [True, "mine", 7]}
    settings.pop("CFBundleSignature", None)
    if platform == "ios":
        settings["UISupportedInterfaceOrientations"] = ["UIInterfaceOrientationLandscapeLeft"]
    write_plist(plist_path, settings)

    xcconfig = config_path.read_text().replace("HIGH_RESOLUTION_CAPABLE = NO", "HIGH_RESOLUTION_CAPABLE = YES")
    if platform in ("osx", "macos"):
        if "HIGH_RESOLUTION_CAPABLE = YES" not in xcconfig:
            xcconfig += "\nHIGH_RESOLUTION_CAPABLE = YES\n"
        xcconfig = xcconfig.replace("HIGH_RESOLUTION_CAPABLE = YES",
                                    '#include? "Retina.xcconfig"\nHIGH_RESOLUTION_CAPABLE = YES')
        (project / "Retina.xcconfig").write_text("HIGH_RESOLUTION_CAPABLE = NO\n")
    xcconfig += '\n// My conditional setting\nMY_SETTING[sdk=iphoneos*][config=Debug] = one \\\n    two\n#include? "Custom.xcconfig"\n'
    config_path.write_text(xcconfig)
    with config_path.open("a") as out:
        out.write("OTHER_CFLAGS += -DMY_XCCONFIG_FLAG\nOTHER_LDFLAGS += -framework MySettingsFramework\n")
        out.write("OTHER_LDFLAGS += -lsqlite3 -framework CoreBluetooth\n")
    (project / "App.xcconfig").write_text("MY_APP_SETTING = stays\n")
    (project / "Custom.xcconfig").write_text("MY_CUSTOM_SETTING = stays\n")
    with (project / "config.make").open("a") as out:
        out.write("\nPROJECT_DEFINES = PG_FROM_CONFIG_MAKE=1\n")

    existing = read_json(pbx_path)
    for key, obj in configs(existing).items():
        build = obj.setdefault("buildSettings", {})
        build["DEVELOPMENT_TEAM"] = "TEAM_" + obj["name"]
        build["INFOPLIST_KEY_NSHighResolutionCapable"] = "YES"
        build["PG_USER_SETTING"] = obj["name"]
        build["ARCHS"] = "arm64"
        build["OTHER_CFLAGS"] = ["$(inherited)", "-DPG_USER_FLAG=1"]
        build["OTHER_LDFLAGS"] = ["$(inherited)", "-framework", "MyCustomFramework"]
        build["CLANG_CXX_LANGUAGE_STANDARD"] = "c++26" if obj["name"] == "Debug" else "c++11"
        build["MACOSX_DEPLOYMENT_TARGET"] = "14.0" if obj["name"] == "Debug" else "10.9"
        build["IPHONEOS_DEPLOYMENT_TARGET"] = "17.0" if obj["name"] == "Debug" else "8.0"
    native_targets = [key for key, obj in existing["objects"].items() if obj.get("isa") == "PBXNativeTarget"]
    sdk_target = native_targets[0]
    sdk_phase = next(key for key in existing["objects"][sdk_target]["buildPhases"] if existing["objects"][key]["isa"] == "PBXFrameworksBuildPhase")
    for config_id in existing["objects"][existing["objects"][sdk_target]["buildConfigurationList"]]["buildConfigurations"]:
        existing["objects"][config_id]["buildSettings"]["OTHER_LDFLAGS"] += ["-lsqlite3", "-framework", "CoreBluetooth"]
    for name in ("System/Library/Frameworks/CoreBluetooth.framework", "usr/lib/libsqlite3.tbd"):
        file_id = uuid.uuid5(uuid.NAMESPACE_URL, name + "/file").hex.upper()[:24]
        build_id = uuid.uuid5(uuid.NAMESPACE_URL, name + "/build").hex.upper()[:24]
        existing["objects"][file_id] = {"isa": "PBXFileReference", "path": name, "sourceTree": "SDKROOT"}
        existing["objects"][build_id] = {"isa": "PBXBuildFile", "fileRef": file_id, "settings": {"ATTRIBUTES": ["Weak"]}}
        existing["objects"][build_id]["platformFilters"] = ["ios" if platform == "ios" else "macos"]
        existing["objects"][sdk_phase]["files"].append(build_id)
    root_object = existing["objects"][existing["rootObject"]]
    attributes = root_object.setdefault("attributes", {}).setdefault("TargetAttributes", {})
    for key in native_targets:
        attributes.setdefault(key, {}).update({"DevelopmentTeam": "MY_TEAM", "ProvisioningStyle": "Manual",
                                              "SystemCapabilities": {"com.apple.Push": {"enabled": "1"}}})
    write_json(pbx_path, existing)
    if (project / "of.entitlements").exists():
        entitlements = read_plist(project / "of.entitlements")
        entitlements["com.apple.security.application-groups"] = ["group.my.installation"]
        write_plist(project / "of.entitlements", entitlements)

    defaults["PGTemplateDefault"] = "after"
    defaults["PGNewDefault"] = True
    defaults["PGNested"] = {"unchanged": "after", "custom": "new default", "added": True}
    defaults["NSCameraUsageDescription"] = "New template camera text"
    write_plist(plist_template, defaults)
    config_template.write_text(config_template.read_text().replace("PG_TEMPLATE_DEFAULT = before", "PG_TEMPLATE_DEFAULT = after"))
    for obj in configs(pbx_defaults).values():
        obj["buildSettings"]["PG_TEMPLATE_DEFAULT"] = "after"
    changed_ids = {key: uuid.uuid5(uuid.NAMESPACE_URL, key + "/PGSettings").hex.upper()[:24]
                   for key in configs(pbx_defaults)}

    def replace_ids(value):
        if isinstance(value, dict):
            return {changed_ids.get(key, key): replace_ids(item) for key, item in value.items()}
        if isinstance(value, list):
            return [replace_ids(item) for item in value]
        return changed_ids.get(value, value) if isinstance(value, str) else value

    pbx_defaults = replace_ids(pbx_defaults)
    write_json(template_project, pbx_defaults)

    # A regenerated addon must be refreshed as well as the template, not restored from the old project.
    addon_config = root / "addons/ofxPGSettingsTest/addon_config.mk"
    addon_config.parent.mkdir(parents=True, exist_ok=True)
    addon_config.write_text("meta:\n\tADDON_NAME = ofxPGSettingsTest\ncommon:\n\tADDON_DEFINES = PG_ADDON_BEFORE=1\n")
    (project / "src/extra.cpp").write_text("int pgExtraSource() { return 1; }\n")
    run(pg, root, platform, project, "-aofxPGSettingsTest")

    updated = read_plist(plist_path)
    assert "CustomPackage.xcodeproj" in workspace.read_text()
    assert template_workspace_before == (template_workspace.read_bytes() if template_workspace.exists() else None), "updates must never copy workspace settings back into OF templates"
    assert updated["NSHighResolutionCapable"] is True
    assert updated["NSCameraUsageDescription"] == settings["NSCameraUsageDescription"]
    assert updated["PGCustomTypes"] == settings["PGCustomTypes"]
    assert "CFBundleSignature" not in updated, "a user deletion must stay deleted"
    assert updated["PGTemplateDefault"] == "after" and updated["PGNewDefault"] is True
    assert updated["PGNested"] == {"unchanged": "after", "custom": "mine", "added": True}
    if platform == "ios":
        assert updated["UISupportedInterfaceOrientations"] == settings["UISupportedInterfaceOrientations"]
    xcconfig = config_path.read_text()
    if platform in ("osx", "macos"):
        assert "HIGH_RESOLUTION_CAPABLE = YES" in xcconfig
        assert xcconfig.index('#include? "Retina.xcconfig"') < xcconfig.index("HIGH_RESOLUTION_CAPABLE = YES")
    assert "PG_TEMPLATE_DEFAULT = after" in xcconfig
    assert "-DMY_XCCONFIG_FLAG" in xcconfig and "-framework MySettingsFramework" in xcconfig
    assert "$(OF_CORE_LIBS)" in xcconfig, "required OF linker settings must remain"
    assert xcconfig.count('MY_SETTING[sdk=iphoneos*][config=Debug] = one \\\n    two') == 1
    assert xcconfig.count('#include? "Custom.xcconfig"') == 1
    assert (project / "App.xcconfig").read_text() == "MY_APP_SETTING = stays\n"

    updated_project = read_json(pbx_path)
    for key, obj in configs(updated_project).items():
        build = obj["buildSettings"]
        assert build["DEVELOPMENT_TEAM"] == "TEAM_" + obj["name"]
        assert build["PG_USER_SETTING"] == obj["name"]
        assert build["PG_TEMPLATE_DEFAULT"] == "after"
        assert build["ARCHS"] == "arm64"
        assert "-DPG_USER_FLAG=1" in build["OTHER_CFLAGS"]
        assert "MyCustomFramework" in str(build["OTHER_LDFLAGS"])
        assert build["CLANG_CXX_LANGUAGE_STANDARD"] == ("c++26" if obj["name"] == "Debug" else "c++23")
        if platform in ("osx", "macos"):
            assert float(build["MACOSX_DEPLOYMENT_TARGET"]) >= 12.0
        if platform == "ios":
            assert float(build["IPHONEOS_DEPLOYMENT_TARGET"]) >= 15.0
    for key in native_targets:
        saved = updated_project["objects"][updated_project["rootObject"]]["attributes"]["TargetAttributes"][key]
        assert saved["DevelopmentTeam"] == "MY_TEAM" and saved["ProvisioningStyle"] == "Manual"
        assert saved["SystemCapabilities"]["com.apple.Push"]["enabled"] == "1"
        for config in updated_project["objects"][updated_project["objects"][key]["buildConfigurationList"]]["buildConfigurations"]:
            assert "PG_FROM_CONFIG_MAKE=1" in updated_project["objects"][config]["buildSettings"]["GCC_PREPROCESSOR_DEFINITIONS"]
    assert all("isa" in obj for obj in updated_project["objects"].values()), "settings must not create orphan configuration objects"
    assert "extra.cpp" in pbx_path.read_text()
    assert linked_sdk(updated_project, sdk_target)["usr/lib/libsqlite3.tbd"] == {"ATTRIBUTES": ["Weak"]}
    assert "System/Library/Frameworks/CoreBluetooth.framework" in linked_sdk(updated_project, sdk_target)
    for phase_id in updated_project["objects"][sdk_target]["buildPhases"]:
        for build_id in updated_project["objects"][phase_id].get("files", []):
            build = updated_project["objects"][build_id]
            ref = updated_project["objects"].get(build.get("fileRef"), {})
            if ref.get("path") == "usr/lib/libsqlite3.tbd":
                assert build.get("platformFilters") == ["ios" if platform == "ios" else "macos"]
    for target_id in native_targets[1:]:
        assert "usr/lib/libsqlite3.tbd" not in linked_sdk(updated_project, target_id), "per-target libraries must not leak to other targets"
    if (project / "of.entitlements").exists():
        assert read_plist(project / "of.entitlements")["com.apple.security.application-groups"] == ["group.my.installation"]

    addon_config.write_text(addon_config.read_text().replace("PG_ADDON_BEFORE", "PG_ADDON_AFTER"))
    run(pg, root, platform, project)
    assert "PG_ADDON_AFTER=1" in pbx_path.read_text() and "PG_ADDON_BEFORE=1" not in pbx_path.read_text()
    before = {path: path.read_bytes() for path in (plist_path, config_path, state)}
    before_configs = configs(read_json(pbx_path))
    run(pg, root, platform, project)
    assert before == {path: path.read_bytes() for path in before}, "repeated settings updates must be stable"
    # Source/file UUIDs are randomly regenerated by the existing generator.
    assert before_configs == configs(read_json(pbx_path)), "repeated build settings must be stable"

    if sys.platform == "darwin":
        pbx_path.write_text("// !$*UTF8*$!\n" + openstep(read_json(pbx_path)))
        write_plist(plist_path, read_plist(plist_path), binary=True)
        run(pg, root, platform, project)
        assert read_plist(plist_path)["PGCustomTypes"] == settings["PGCustomTypes"]
        assert configs(read_json(pbx_path))

    # No baseline (older PG): preserve existing user values, then record the first baseline.
    state.unlink()
    run(pg, root, platform, project)
    assert read_plist(plist_path)["NSHighResolutionCapable"] is True
    assert "PG_USER_SETTING" in pbx_path.read_text() and state.exists()

    run(pg, root, platform, project, "--defines")
    assert "PG_FROM_CONFIG_MAKE=1" in pbx_path.read_text(), "a missing defines argument must not clear saved values"
    run(pg, root, platform, project, "--defines=GUI_EDITED=1")
    assert "GUI_EDITED=1" in pbx_path.read_text() and "PG_FROM_CONFIG_MAKE=1" not in pbx_path.read_text()
    assert "-DPG_USER_FLAG=1" not in pbx_path.read_text()
    assert "PG_ADDON_AFTER=1" in pbx_path.read_text(), "editing project defines must keep addon defines"
    run(pg, root, platform, project, "--defines=", "--frameworks=", "--system-libs=")
    assert "GUI_EDITED=1" not in pbx_path.read_text()
    assert "PG_ADDON_AFTER=1" in pbx_path.read_text()
    assert "usr/lib/libsqlite3.tbd" not in linked_sdk(read_json(pbx_path), sdk_target)
    assert "System/Library/Frameworks/CoreBluetooth.framework" not in linked_sdk(read_json(pbx_path), sdk_target)
    assert "-lsqlite3" not in pbx_path.read_text() and "CoreBluetooth" not in pbx_path.read_text()
    assert "-lsqlite3" not in config_path.read_text() and "CoreBluetooth" not in config_path.read_text()
    assert "MyCustomFramework" in pbx_path.read_text(), "SDK edits must keep unrelated third-party flags"

    run(pg, root, platform, project, "--app-version=3.2.1", "--app-build=42", "--bundle-id=com.example.test")
    for obj in configs(read_json(pbx_path)).values():
        # Version options apply to native target configurations, as before.
        if obj["buildSettings"].get("PRODUCT_BUNDLE_IDENTIFIER") == "com.example.test":
            assert obj["buildSettings"]["MARKETING_VERSION"] == "3.2.1"
            assert obj["buildSettings"]["CURRENT_PROJECT_VERSION"] == "42"

    plist_path.write_text("this is not a property list")
    before = {path: path.read_bytes() for path in (pbx_path, plist_path, config_path, state)}
    output = run(pg, root, platform, project, success=False)
    assert "original settings kept" in output
    assert before == {path: path.read_bytes() for path in before}, "unreadable settings must not be overwritten"
    print("ok [" + platform + "] custom settings, template upgrade, required minimums, addon regeneration, repeated updates, legacy projects and invalid input")


def main():
    pg, source = (pathlib.Path(arg).resolve() for arg in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="pg-apple-settings-") as folder:
        root = pathlib.Path(folder)
        for name in ("apps", "addons", "libs/openFrameworks"):
            (root / name).mkdir(parents=True, exist_ok=True)
        shutil.copytree(source / "scripts/templates", root / "scripts/templates")
        shutil.copytree(source / "libs/openFrameworksCompiled/project", root / "libs/openFrameworksCompiled/project")
        for platform in ("osx", "macos", "ios"):
            exercise(pg, root, platform)
    print("Apple settings preservation tests passed")


if __name__ == "__main__":
    main()
