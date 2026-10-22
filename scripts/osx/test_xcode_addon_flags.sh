#!/usr/bin/env bash
# Regression test: an addon's ADDON_DEFINES / ADDON_CFLAGS / sources end up in every app target of the
# generated Xcode projects (osx, macos with its two targets, ios, tvos), after $(inherited), once each,
# with no orphan objects (a [null] define or settings on missing UUIDs broke the build before).
# usage: test_xcode_addon_flags.sh <projectGenerator> <openFrameworks root>
set -euo pipefail
PG=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
OF=$(cd "$2" && pwd)
ADDON=ofxPGFlagsTest
ADDON_DIR="$OF/addons/$ADDON"
APPS="$OF/apps/pgFlagsTest"
command -v jq >/dev/null || { echo 'jq is required'; exit 1; }

cleanup() { rm -rf "$ADDON_DIR" "$APPS"; }
trap cleanup EXIT
cleanup
mkdir -p "$ADDON_DIR/src" "$APPS"
printf 'meta:\n\tADDON_NAME = %s\ncommon:\n\tADDON_DEFINES = FOO BAR=1\n\tADDON_CFLAGS = -DBAZ\n' "$ADDON" > "$ADDON_DIR/addon_config.mk"
printf '#pragma once\n#if !defined(FOO) || BAR != 1 || !defined(BAZ)\n#error ADDON_DEFINES / ADDON_CFLAGS not applied\n#endif\n' > "$ADDON_DIR/src/$ADDON.h"
printf '#include "%s.h"\nint %sValue() { return BAR; }\n' "$ADDON" "$ADDON" > "$ADDON_DIR/src/$ADDON.cpp"

failed=0
for platform in osx macos ios tvos; do
	project="$APPS/flags_$platform"
	"$PG" -o"$OF" -p"$platform" -a"$ADDON" "$project" > /dev/null
	# twice, like updating an existing project: nothing may be added again
	"$PG" -o"$OF" -p"$platform" -a"$ADDON" "$project" > /dev/null
	pbxproj="$project/flags_$platform.xcodeproj/project.pbxproj"
	errors=$(jq -r --arg src "$ADDON.cpp" '
		.objects as $o
		| ([$o | to_entries[] | select(.value.isa == null) | "orphan object \(.key)"]
		+ [$o | to_entries[] | select(.value.isa == "PBXNativeTarget" and .value.productType == "com.apple.product-type.application") | .value as $t
			| ($o[$t.buildConfigurationList].buildConfigurations[] | $o[.] as $c | $c.buildSettings as $b
				| (if ($b.GCC_PREPROCESSOR_DEFINITIONS | type) != "array" or ($b.GCC_PREPROCESSOR_DEFINITIONS | map(select(type != "string")) | length) > 0 then "\($t.name) \($c.name): GCC_PREPROCESSOR_DEFINITIONS \($b.GCC_PREPROCESSOR_DEFINITIONS | tostring)" else empty end),
				(["FOO", "BAR=1", "$(inherited)"][] as $d | select(($b.GCC_PREPROCESSOR_DEFINITIONS // []) | map(select(. == $d)) | length != 1) | "\($t.name) \($c.name): \($d) not once in GCC_PREPROCESSOR_DEFINITIONS \($b.GCC_PREPROCESSOR_DEFINITIONS | tostring)"),
				(["OTHER_CFLAGS", "OTHER_CPLUSPLUSFLAGS"][] as $k | select(($b[$k] // []) | (type != "array") or (.[0] != "$(inherited)") or (map(select(. == "-DBAZ")) | length != 1)) | "\($t.name) \($c.name): \($k) \($b[$k] | tostring)")),
			(if [$t.buildPhases[] | $o[.] | select(.isa == "PBXSourcesBuildPhase") | .files[] | $o[$o[.].fileRef].name] | index($src) then empty else "\($t.name): \($src) not in Compile Sources" end)])
		| .[]' "$pbxproj")
	if [[ -n $errors ]]; then
		echo "FAIL [$platform]"
		echo "$errors"
		failed=1
	else
		echo "ok   [$platform]"
	fi
done
exit $failed
