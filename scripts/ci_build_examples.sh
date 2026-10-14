#!/usr/bin/env bash
# Generates emptyExample and allAddonsExample with the freshly built PG and compiles them.
# usage: ci_build_examples.sh osx|linux64|vs
set -e

PLATFORM="$1"
PG_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
OF_ROOT="$( cd "${PG_DIR}/../.." && pwd )"
PG="${PG_DIR}/commandLine/bin/projectGenerator"
OF_ARG="${OF_ROOT}"
EXAMPLES=(emptyExample allAddonsExample)

if [[ "${PLATFORM}" == "vs" ]]; then
	PG="${PG}.exe"
	OF_ARG="$(cygpath -w "${OF_ROOT}")"
fi

# fails if a .c source isn't CompileAsC or a C++ source is (openFrameworks#8587)
check_vcxproj() {
	awk '
	function check() {
		if (isC && !asC) { print "error: .c not CompileAsC: " inc; bad = 1 }
		if (!isC && asC) { print "error: C++ source CompileAsC: " inc; bad = 1 }
	}
	/<ClCompile Include="/ {
		inc = $0; sub(/.*Include="/, "", inc); sub(/".*/, "", inc)
		isC = (inc ~ /\.c$/); asC = 0; open = ($0 !~ /\/>/)
		if (!open) check()
		next
	}
	open && /<CompileAs>CompileAsC<\/CompileAs>/ { asC = 1 }
	open && /<\/ClCompile>/ { open = 0; check() }
	END { exit bad }
	' "$1"
}

cd "${OF_ROOT}"

if [[ "${PLATFORM}" == "linux64" ]]; then
	echo "##[group]Building OF core [linux64]"
	make -j"$(nproc)" -C libs/openFrameworksCompiled/project Release
	echo "##[endgroup]"
fi

for e in "${EXAMPLES[@]}"; do
	dir="examples/templates/${e}"

	echo "##[group]Generating ${e} [${PLATFORM}]"
	# addons come from the example's addons.make
	"${PG}" -o"${OF_ARG}" -p"${PLATFORM}" "${OF_ARG}/${dir}"
	echo "##[endgroup]"

	echo "##[group]Building ${e} [${PLATFORM}]"
	case "${PLATFORM}" in
		osx)
			xcodebuild -quiet -project "${dir}/${e}.xcodeproj" -target "${e}" -configuration Release \
				ARCHS=arm64 ONLY_ACTIVE_ARCH=NO CODE_SIGN_IDENTITY=-
			;;
		linux64)
			make -j"$(nproc)" -C "${dir}" Release
			;;
		vs)
			check_vcxproj "${dir}/${e}.vcxproj"
			MSYS_NO_PATHCONV=1 msbuild.exe "${dir}/${e}.sln" /m /nologo /v:minimal \
				/p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143
			;;
		*)
			echo "unsupported platform: ${PLATFORM}"
			exit 1
			;;
	esac
	echo "##[endgroup]"
done

echo "Built ${EXAMPLES[*]} [${PLATFORM}]"
