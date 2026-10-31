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
