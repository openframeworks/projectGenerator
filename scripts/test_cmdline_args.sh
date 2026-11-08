#!/usr/bin/env bash

# projectGenerator command line argument tests: attached / separate / = values, options before and
# after the project path, empty and bad values, and the project never ending up somewhere else
# (a value taken as the path once made projects at the OF root or named after an option value)
#
# usage: test_cmdline_args.sh <projectGenerator> <openFrameworks root> [platform]
#   platform defaults to the host's: osx, linux64 or vs

PG="$1"
OF="$2"
PLATFORM="$3"
if [ -z "$PG" ] || [ -z "$OF" ] || [ ! -f "$PG" ] || [ ! -d "$OF/scripts/templates" ]; then
	echo "usage: $0 <projectGenerator> <openFrameworks root> [platform]"
	exit 2
fi
PG="$(cd "$(dirname "$PG")" && pwd)/$(basename "$PG")"
OF="$(cd "$OF" && pwd)"
if [ -z "$PLATFORM" ]; then
	case "$(uname -s)" in
		Darwin) PLATFORM=osx ;;
		Linux) PLATFORM=linux64 ;;
		*) PLATFORM=vs ;;
	esac
fi
case "$PLATFORM" in
	osx|ios|tvos|macos) PROJECT_FILE="myApp.xcodeproj" ;;
	vs) PROJECT_FILE="myApp.vcxproj" ;;
	*) PROJECT_FILE="Makefile" ;;
esac
XCODE=0
[ "$PROJECT_FILE" = "myApp.xcodeproj" ] && XCODE=1
# vs has no config.make template, so defines / version only go to the vcxproj there
HAS_CONFIG_TEMPLATE=0
[ -f "$OF/scripts/templates/$PLATFORM/config.make" ] && HAS_CONFIG_TEMPLATE=1

# everything runs from apps/ with relative paths (no MSYS path conversion on Windows)
RUN_DIR="$OF/apps"
TEST_DIR="$RUN_DIR/myApps/pgArgTests"
APP="myApps/pgArgTests/myApp"
APP_DIR="$TEST_DIR/myApp"

FAILS=0
CASES=0
CASE=""
OUT=""
CODE=0

# apps/myApps may not exist in a fresh checkout; made before the snapshot, removed at the end
CREATED_MYAPPS=0
[ -d "$RUN_DIR/myApps" ] || { mkdir -p "$RUN_DIR/myApps"; CREATED_MYAPPS=1; }

listing() {
	for d in "$OF" "$RUN_DIR" "$RUN_DIR/myApps"; do
		ls -A "$d" 2>/dev/null | grep -vx pgArgTests | sed "s|^|$d/|"
	done
}
BEFORE="$(listing)"

reset() {
	rm -rf "$TEST_DIR"
	mkdir -p "$APP_DIR/src"
	if [ -d "$OF/examples/templates/emptyExample/src" ]; then
		cp "$OF/examples/templates/emptyExample/src/"* "$APP_DIR/src/"
	else
		printf '#include "ofMain.h"\nint main() { return 0; }\n' > "$APP_DIR/src/main.cpp"
	fi
}

pg() {
	OUT="$(cd "$RUN_DIR" && "$PG" "$@" 2>&1)"
	CODE=$?
}

fail() {
	echo "  FAIL [$CASE]: $1"
	FAILS=$((FAILS + 1))
}

start() {
	CASE="$1"
	CASES=$((CASES + 1))
	reset
}

config() {
	tr -d '\r' < "$APP_DIR/config.make" 2>/dev/null | grep -E "^$1[[:space:]]*=" | tail -1 | sed -E "s/^$1[[:space:]]*=[[:space:]]*//"
}

expect_exit() {
	[ "$CODE" = "$1" ] || fail "exit $CODE, expected $1"
}

expect_updated() {
	[ -e "$APP_DIR/$PROJECT_FILE" ] || fail "$APP/$PROJECT_FILE not written"
}

expect_not_updated() {
	[ ! -e "$APP_DIR/$PROJECT_FILE" ] || fail "$APP/$PROJECT_FILE written"
}

expect_config() {
	local got
	got="$(config "$1")"
	[ "$got" = "$2" ] || fail "config.make $1 = '$got', expected '$2'"
}

expect_no_config() {
	local got
	got="$(config "$1")"
	[ -z "$got" ] || fail "config.make $1 = '$got', expected none"
}

expect_output() {
	echo "$OUT" | grep -qE "$1" || fail "output has no '$1'"
}

expect_file() {
	[ -e "$1" ] || fail "${1#$OF/} missing"
}

expect_in_file() {
	tr -d '\r' < "$1" 2>/dev/null | grep -qF -- "$2" || fail "${1#$OF/} has no '$2'"
}

# Xcode / Visual Studio list the sources; make projects read them from config.make
project_sources_file() {
	if [ "$XCODE" = 1 ]; then echo "$APP_DIR/myApp.xcodeproj/project.pbxproj"; elif [ "$PLATFORM" = vs ]; then echo "$APP_DIR/myApp.vcxproj"; fi
}

expect_src_in_project() {
	local f
	f="$(project_sources_file)"
	[ -z "$f" ] || expect_in_file "$f" "$1"
}

expect_src_not_in_project() {
	local f
	f="$(project_sources_file)"
	[ -z "$f" ] || ! tr -d '\r' < "$f" | grep -qF -- "$1" || fail "${f#$OF/} still has $1"
}

expect_pbxproj() {
	[ "$XCODE" = 1 ] || return 0
	grep -q "$1" "$APP_DIR/myApp.xcodeproj/project.pbxproj" 2>/dev/null || fail "project.pbxproj has no $1"
}

expect_no_pbxproj() {
	[ "$XCODE" = 1 ] || return 0
	! grep -q "$1" "$APP_DIR/myApp.xcodeproj/project.pbxproj" 2>/dev/null || fail "project.pbxproj has $1"
}

# nothing new at the OF root, in apps/ or apps/myApps/ (a project named after an option value);
# only reported (each once), never deleted
expect_no_stray() {
	local now
	now="$(listing)"
	if [ "$now" != "$BEFORE" ]; then
		fail "stray files: $(diff <(echo "$BEFORE") <(echo "$now") | grep '^>' | sed 's/^> //' | tr '\n' ' ')"
		BEFORE="$now"
	fi
}

echo "projectGenerator argument tests: $PG"
echo "openFrameworks: $OF  platform: $PLATFORM"
"$PG" --version 2>/dev/null | grep -i projectGenerator

# ---------------------------------------------------------------- the project path

start "attached values: -o.. -p$PLATFORM path"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "= values: --ofPath=.. --platforms=$PLATFORM path"
pg --ofPath=.. --platforms="$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "separate values: -o .. -p $PLATFORM path"
pg -o .. -p "$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "separate long values: --ofPath .. --platforms $PLATFORM path"
pg --ofPath .. --platforms "$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "separate values, absolute OF path"
pg -o "$OF" -p "$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "-o .. -p $PLATFORM -f Security path (was: project at the OF root)"
pg -o .. -p "$PLATFORM" -f Security "$APP"
expect_exit 0; expect_updated; expect_config PROJECT_FRAMEWORKS "Security"; expect_no_stray

start "--frameworks Security --platforms $PLATFORM path (was: project named Security)"
pg -o.. --frameworks Security --platforms "$PLATFORM" "$APP"
expect_exit 0; expect_updated; expect_config PROJECT_FRAMEWORKS "Security"; expect_no_stray

start "--platforms $PLATFORM --frameworks Security path (was: project named $PLATFORM)"
pg -o.. --platforms "$PLATFORM" --frameworks Security "$APP"
expect_exit 0; expect_updated; expect_config PROJECT_FRAMEWORKS "Security"; expect_no_stray

start "every value option separated: -a -D -P -s"
mkdir -p "$TEST_DIR/shared" && printf 'int shared() { return 1; }\n' > "$TEST_DIR/shared/shared.cpp"
pg -o .. -p "$PLATFORM" -a ofxGui -D A=1 -P "echo done" -s myApps/pgArgTests/shared "$APP"
expect_exit 0; expect_updated; expect_no_stray
grep -q ofxGui "$APP_DIR/addons.make" 2>/dev/null || fail "addons.make has no ofxGui"

start "empty values before the path, as OF's CI scripts: -a\"\" -p\"$PLATFORM\" -t\"\" path (was: path taken as the template)"
pg -o"$OF" -v -a"" -p"$PLATFORM" -t"" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "bare -t right before the path, as the GUI with no template (was: path taken as the template)"
pg -o.. -a -p"$PLATFORM" -t "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "empty separate values: -a \"\" -t \"\" path"
pg -o.. -p"$PLATFORM" -a "" -t "" "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "project path only (host platform, OF path from -o)"
pg -o.. "$APP"
expect_exit 0; expect_no_stray

start "missing project path"
pg -o.. -p"$PLATFORM"
expect_exit 64; expect_output "Missing project path"; expect_no_stray

start "bare value option at the end: -p $PLATFORM (was: project named $PLATFORM)"
pg -o.. -p "$PLATFORM"
expect_exit 64; expect_output "Missing project path"; expect_no_stray

start "two project paths"
pg -o.. -p"$PLATFORM" "$APP" myApps/pgArgTests/other
expect_exit 64; expect_output "More than one project path"; expect_not_updated; expect_no_stray

start "option after the project path"
pg -o.. "$APP" -p"$PLATFORM"
expect_exit 64; expect_output "More than one project path"; expect_not_updated; expect_no_stray

start "the OF folder as the project"
pg -o.. -p"$PLATFORM" ..
expect_exit 64; expect_output "openFrameworks folder"; expect_no_stray

start "unknown platform (was: crash)"
pg -o.. -pbogus "$APP"
expect_exit 64; expect_output "Unknown platform"; expect_not_updated; expect_no_stray

start "unknown option"
pg -o.. --bogus -p"$PLATFORM" "$APP"
expect_exit 64; expect_output "Unknown option"; expect_not_updated; expect_no_stray

start "misspelled option: --platform $PLATFORM (was: printed the host platform, exit 0)"
pg -o.. --platform "$PLATFORM" "$APP"
expect_exit 64; expect_not_updated; expect_no_stray

start "--getplatform / -i print the host platform"
pg --getplatform
expect_exit 0; expect_output "ofHostPlatform"

start "OF path that doesn't exist: an error, or the OF folder found above the projectGenerator"
pg -o../nope -p"$PLATFORM" "$APP"
[ "$CODE" = 0 ] && expect_updated; expect_no_stray

start "dry run writes nothing"
pg -o.. -p"$PLATFORM" -d "$APP"
expect_exit 0; expect_not_updated; expect_no_stray

start "recursive update of a folder"
mkdir -p "$TEST_DIR/other/src" && cp "$APP_DIR/src/"* "$TEST_DIR/other/src/"
pg -o.. -p"$PLATFORM" -r myApps/pgArgTests
expect_exit 0; expect_updated; expect_no_stray
[ -e "$TEST_DIR/other/${PROJECT_FILE/myApp/other}" ] || fail "other/${PROJECT_FILE/myApp/other} not written"

# ---------------------------------------------------------------- how the GUI and openFrameworks' scripts call it

# frontend/index.js generateFunction: -v -o -a -p -s -t [-D -P -k -W --app-*] [-f --system-libs] path
start "GUI generate, no addons / template / source: -a<space> -s -t path"
pg -v -o"$OF" "-a " -p"$PLATFORM" -s -t "$APP"
expect_exit 0; expect_updated; expect_no_stray

start "GUI generate with addons, defines, post-build, version"
pg -v -o"$OF" -a"ofxGui,ofxOsc" -p"$PLATFORM" -s -t -D"A=1,B" -P"echo gui-postbuild" --app-version=2.0 --app-build=7 "$APP"
expect_exit 0; expect_updated; expect_no_stray
expect_in_file "$APP_DIR/addons.make" "ofxOsc"

# frontend/index.js updateFunction (update multiple): -r -v -o -p [-t] folder
start "GUI update multiple: -r -v -o -p folder"
mkdir -p "$TEST_DIR/other/src" && cp "$APP_DIR/src/"* "$TEST_DIR/other/src/"
pg -r -v -o"$OF" -p"$PLATFORM" myApps/pgArgTests
expect_exit 0; expect_updated; expect_no_stray

# scripts/dev/create_package.sh, scripts/osx/snapshotAllExamples.sh
start "openFrameworks create_package: --recursive -p -o folder"
mkdir -p "$TEST_DIR/other/src" && cp "$APP_DIR/src/"* "$TEST_DIR/other/src/"
pg --recursive -p"$PLATFORM" -o"$OF" "$TEST_DIR"
expect_exit 0; expect_updated; expect_no_stray

# scripts/ci/cmake/build.sh
if [ -f "$OF/scripts/templates/cmake/CMakeLists.txt" ]; then
	start "openFrameworks cmake build.sh: -o -t\"cmake\" -p path"
	pg -o"$OF" -t"cmake" -p"$PLATFORM" "$APP_DIR"
	expect_exit 0; expect_file "$APP_DIR/CMakeLists.txt"; expect_no_stray
fi

# ---------------------------------------------------------------- project paths

start "spaces in the project name"
mkdir -p "$TEST_DIR/my App/src" && cp "$APP_DIR/src/"* "$TEST_DIR/my App/src/"
pg -o.. -p"$PLATFORM" "myApps/pgArgTests/my App"
expect_exit 0; expect_file "$TEST_DIR/my App/${PROJECT_FILE/myApp/my App}"; expect_no_stray

start "trailing slash and ./ in the project path"
pg -o.. -p"$PLATFORM" "./$APP/"
expect_exit 0; expect_updated; expect_no_stray

start "project path relative to apps/myApps (a name only)"
mkdir -p "$TEST_DIR/../pgArgTestsByName/src" && cp "$APP_DIR/src/"* "$TEST_DIR/../pgArgTestsByName/src/"
pg -o"$OF" -p"$PLATFORM" pgArgTestsByName
[ -e "$RUN_DIR/myApps/pgArgTestsByName/${PROJECT_FILE/myApp/pgArgTestsByName}" ] || [ -e "$RUN_DIR/pgArgTestsByName/${PROJECT_FILE/myApp/pgArgTestsByName}" ] || fail "pgArgTestsByName not generated"
rm -rf "$RUN_DIR/myApps/pgArgTestsByName" "$RUN_DIR/pgArgTestsByName"
expect_no_stray

# ---------------------------------------------------------------- addons

start "missing addon is kept in addons.make"
pg -o.. -p"$PLATFORM" -a"ofxGui,ofxDoesNotExist" "$APP"
expect_exit 0; expect_updated; expect_output "addon not found: ofxDoesNotExist"
expect_in_file "$APP_DIR/addons.make" "ofxDoesNotExist"

start "addons with spaces and duplicates"
pg -o.. -p"$PLATFORM" -a"ofxGui, ofxOsc,ofxGui" "$APP"
expect_exit 0
[ "$(tr -d '\r' < "$APP_DIR/addons.make" | grep -cx ofxGui)" = 1 ] || fail "addons.make should list ofxGui once"
expect_in_file "$APP_DIR/addons.make" "ofxOsc"

start "addons.make comments and #url of a missing addon are kept"
printf 'ofxGui\nofxMissingOne#https://github.com/example/ofxMissingOne\n' > "$APP_DIR/addons.make"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_in_file "$APP_DIR/addons.make" "ofxMissingOne#https://github.com/example/ofxMissingOne"

start "no addons and no addons.make: none written"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; [ ! -e "$APP_DIR/addons.make" ] || fail "empty addons.make written"

# ---------------------------------------------------------------- external sources (-s)

mkshared() {
	mkdir -p "$TEST_DIR/shared" && printf 'int sharedFn() { return 1; }\n' > "$TEST_DIR/shared/sharedSource.cpp"
}

start "-s relative to the current folder (was: silently nothing)"
mkshared
pg -o.. -p"$PLATFORM" -s myApps/pgArgTests/shared "$APP"
expect_exit 0; expect_config PROJECT_EXTERNAL_SOURCE_PATHS "../shared"
expect_src_in_project "sharedSource.cpp"

start "-s relative to the project"
mkshared
pg -o.. -p"$PLATFORM" -s ../shared "$APP"
expect_exit 0; expect_config PROJECT_EXTERNAL_SOURCE_PATHS "../shared"

start "-s absolute, written relative to the project"
mkshared
pg -o.. -p"$PLATFORM" -s "$TEST_DIR/shared" "$APP"
expect_exit 0; expect_config PROJECT_EXTERNAL_SOURCE_PATHS "../shared"

start "external sources kept on update without -s, empty -s removes them (was: dropped / stale)"
mkshared
pg -o.. -p"$PLATFORM" -s ../shared "$APP"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_config PROJECT_EXTERNAL_SOURCE_PATHS "../shared"
expect_src_in_project "sharedSource.cpp"
pg -o.. -p"$PLATFORM" -s -t "$APP"
expect_exit 0; expect_no_config PROJECT_EXTERNAL_SOURCE_PATHS
expect_src_not_in_project "sharedSource.cpp"

# ---------------------------------------------------------------- other options

start "-t vscode adds VS Code to the platform (#476)"
pg -o.. -p"$PLATFORM" -tvscode "$APP"
expect_exit 0; expect_updated; expect_file "$APP_DIR/.vscode"

start "-P post-build script lands in the project"
pg -o.. -p"$PLATFORM" -P "echo pg-postbuild-test" "$APP"
expect_exit 0
if [ "$XCODE" = 1 ]; then expect_pbxproj "pg-postbuild-test"; elif [ "$PLATFORM" = vs ]; then expect_in_file "$APP_DIR/myApp.vcxproj" "pg-postbuild-test"; fi

start "-k bad MSYS2 environment warns"
pg -o.. -pmsys2 -k bogus "$APP"
expect_exit 0; expect_output "unknown msys2 environment bogus"

start "-w / -i / --getofpath"
pg -w
expect_exit 0; expect_output "openFrameworks projectGenerator"
pg -i
expect_exit 0; expect_output "ofHostPlatform"
pg -o.. --getofpath
expect_exit 0; expect_output '"ofPath"'

start "-l lists templates"
pg -o.. -p"$PLATFORM" -l "$APP"
expect_exit 0

# ---------------------------------------------------------------- system frameworks / libraries

start "frameworks: commas"
pg -o.. -p"$PLATFORM" -fSecurity,CoreBluetooth "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security CoreBluetooth"
expect_pbxproj "Security.framework"; expect_pbxproj "CoreBluetooth.framework"

start "frameworks: comma and space"
pg -o.. -p"$PLATFORM" -f"Security, CoreBluetooth" "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security CoreBluetooth"

start "frameworks: spaces only"
pg -o.. -p"$PLATFORM" -f"Security CoreBluetooth" "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security CoreBluetooth"
expect_no_pbxproj "Security CoreBluetooth"

start "frameworks: .framework suffix and duplicates"
pg -o.. -p"$PLATFORM" -fSecurity.framework,Security,CoreBluetooth "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security CoreBluetooth"

start "system libs: libX.tbd, -lX, libX, X"
pg -o.. -p"$PLATFORM" --system-libs=libsqlite3.tbd,-lz,libiconv,c++ "$APP"
expect_exit 0; expect_config PROJECT_SYSTEM_LIBS "sqlite3 z iconv c++"
expect_pbxproj "usr/lib/libsqlite3.tbd"; expect_pbxproj "usr/lib/libz.tbd"

start "libraries given to -f go to PROJECT_SYSTEM_LIBS"
pg -o.. -p"$PLATFORM" -fSecurity,libsqlite3 "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security"; expect_config PROJECT_SYSTEM_LIBS "sqlite3"

start "update without -f / --system-libs keeps them, empty removes them"
pg -o.. -p"$PLATFORM" -fSecurity --system-libs=sqlite3 "$APP"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security"; expect_config PROJECT_SYSTEM_LIBS "sqlite3"
pg -o.. -p"$PLATFORM" --system-libs= "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security"; expect_no_config PROJECT_SYSTEM_LIBS
pg -o.. -p"$PLATFORM" -f "$APP"
expect_exit 0; expect_updated; expect_no_config PROJECT_FRAMEWORKS; expect_no_stray
expect_no_pbxproj "Security.framework"

start "config.make PROJECT_FRAMEWORKS += lines are read"
printf 'PROJECT_FRAMEWORKS = Security\nPROJECT_FRAMEWORKS += CoreBluetooth\n' > "$APP_DIR/config.make"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_config PROJECT_FRAMEWORKS "Security CoreBluetooth"; expect_pbxproj "CoreBluetooth.framework"

# ---------------------------------------------------------------- bundle id

start "bundle id: none by default"
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_no_config APP_BUNDLE_ID; expect_no_pbxproj '"PRODUCT_BUNDLE_IDENTIFIER"'

start "bundle id: --bundle-id com.example.my-app, kept on update"
pg -o.. -p"$PLATFORM" --bundle-id com.example.my-app "$APP"
expect_exit 0; expect_config APP_BUNDLE_ID "com.example.my-app"; expect_pbxproj '"PRODUCT_BUNDLE_IDENTIFIER": "com.example.my-app"'
pg -o.. -p"$PLATFORM" "$APP"
expect_exit 0; expect_config APP_BUNDLE_ID "com.example.my-app"; expect_pbxproj '"PRODUCT_BUNDLE_IDENTIFIER": "com.example.my-app"'

start "bundle id: bad value warns and keeps the project's"
pg -o.. -p"$PLATFORM" --bundle-id=com.example.good "$APP"
pg -o.. -p"$PLATFORM" --bundle-id=nodots "$APP"
expect_exit 0; expect_output "bundle id nodots should be"; expect_config APP_BUNDLE_ID "com.example.good"

if [ "$XCODE" = 1 ]; then
	start "bundle id: a custom Project.xcconfig PRODUCT_BUNDLE_IDENTIFIER is kept (was: reset to cc.openFrameworks.<name>)"
	pg -o.. -p"$PLATFORM" "$APP"
	sed -i.bak 's/^PRODUCT_BUNDLE_IDENTIFIER = .*/PRODUCT_BUNDLE_IDENTIFIER = com.example.fromxcconfig/' "$APP_DIR/Project.xcconfig"
	pg -o.. -p"$PLATFORM" "$APP"
	expect_exit 0; expect_config APP_BUNDLE_ID "com.example.fromxcconfig"; expect_pbxproj '"PRODUCT_BUNDLE_IDENTIFIER": "com.example.fromxcconfig"'
	grep -q '^PRODUCT_BUNDLE_IDENTIFIER = com.example.fromxcconfig' "$APP_DIR/Project.xcconfig" || fail "Project.xcconfig was reset"
fi

# ---------------------------------------------------------------- Visual Studio version

if [ "$PLATFORM" = vs ]; then
	expect_toolset() {
		expect_in_file "$APP_DIR/myApp.vcxproj" "<PlatformToolset>$1<"
	}

	start "vs version: no -t is 2022"
	pg -o.. -pvs "$APP"
	expect_exit 0; expect_toolset v143

	start "vs version: -t2022, and no stray project or template warning"
	pg -o.. -pvs -t2022 "$APP"
	expect_exit 0; expect_toolset v143; expect_no_stray
	echo "$OUT" | grep -q "Cannot find 2022" && fail "2022 treated as a template"

	for pair in 2026:v145 2019:v142; do
		VS_VERSION="${pair%%:*}"
		VS_TOOLSET="${pair##*:}"
		[ -d "$OF/scripts/templates/vs$VS_VERSION" ] || continue

		start "vs version: -pvs -t$VS_VERSION"
		pg -o.. -pvs -t"$VS_VERSION" "$APP"
		expect_exit 0; expect_toolset "$VS_TOOLSET"; expect_no_stray

		start "vs version: an update without -t keeps $VS_VERSION, -t2022 changes it"
		pg -o.. -pvs -t"$VS_VERSION" "$APP"
		pg -o.. -pvs "$APP"
		expect_exit 0; expect_toolset "$VS_TOOLSET"
		pg -o.. -pvs -t2022 "$APP"
		expect_exit 0; expect_toolset v143

		start "vs version: the old -pvs$VS_VERSION is the vs platform with that version"
		pg -o.. -pvs"$VS_VERSION" "$APP"
		expect_exit 0; expect_toolset "$VS_TOOLSET"

		start "vs version: goes with another template, -t$VS_VERSION,gitignore"
		pg -o.. -pvs -t"$VS_VERSION",gitignore "$APP"
		expect_exit 0; expect_toolset "$VS_TOOLSET"

		# addon libs live in lib/vs for every version (lost when vs20xx was its own platform)
		if [ -d "$OF/addons/ofxSvg/libs/libxml2/lib/vs" ]; then
			start "vs version: -t$VS_VERSION keeps the addon's lib/vs libs"
			pg -o.. -pvs -t"$VS_VERSION" -aofxSvg "$APP"
			expect_exit 0; expect_in_file "$APP_DIR/myApp.vcxproj" 'libxml2\lib\vs'
		fi
	done
fi

# ---------------------------------------------------------------- defines / version / emscripten

if [ "$HAS_CONFIG_TEMPLATE" = 1 ]; then
	start "defines: -D A=1,B"
	pg -o.. -p"$PLATFORM" -D A=1,B "$APP"
	expect_exit 0; expect_config PROJECT_DEFINES "A=1 B"

	start "app version and build"
	pg -o.. -p"$PLATFORM" --app-version 1.2.3 --app-build 42 "$APP"
	expect_exit 0; expect_config APP_VERSION "1.2.3"; expect_config APP_BUILD "42"

	start "bad app version / build keep the project's"
	pg -o.. -p"$PLATFORM" --app-version=1.2.3 --app-build=42 "$APP"
	pg -o.. -p"$PLATFORM" --app-version=1.x --app-build=0 "$APP"
	expect_exit 0; expect_output "should be digits and dots"; expect_output "should be a positive integer"
	expect_config APP_VERSION "1.2.3"; expect_config APP_BUILD "42"

	start "emscripten wasm: 64, then a bad value keeps it"
	pg -o.. -p"$PLATFORM" -W 64 "$APP"
	expect_config PROJECT_EMSCRIPTEN_WASM "64"
	pg -o.. -p"$PLATFORM" -W48 "$APP"
	expect_exit 0; expect_output "unknown emscripten wasm"; expect_config PROJECT_EMSCRIPTEN_WASM "64"
fi

rm -rf "$TEST_DIR"
expect_no_stray
[ "$CREATED_MYAPPS" = 1 ] && rmdir "$RUN_DIR/myApps" 2>/dev/null

echo "$CASES cases, $FAILS failures"
[ "$FAILS" = 0 ] || exit 1
echo "projectGenerator argument tests passed"
