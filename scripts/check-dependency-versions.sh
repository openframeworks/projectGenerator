#!/usr/bin/env bash
# Report out of date projectGenerator dependencies: frontend npm packages (Electron etc.),
# GitHub Actions used by the workflows, CI Node versions and the vendored jQuery / Semantic UI.
# Maintains a tracking issue only with --file-issue (like apothecary's check-upstream-versions.sh).
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
MARKER='<!-- projectGenerator-dependency-versions -->'
TITLE='projectGenerator dependencies have newer releases'

curl_json() {
	local url=$1
	local args=(--fail --silent --show-error --retry 2 --retry-delay 1 --connect-timeout 10 --max-time 30
		-H 'Accept: application/json' -H 'User-Agent: projectGenerator-dependency-versions')
	if [[ $url == https://api.github.com/* && -n ${GH_TOKEN:-${GITHUB_TOKEN:-}} ]]; then
		args+=(-H "Authorization: Bearer ${GH_TOKEN:-$GITHUB_TOKEN}")
	fi
	curl "${args[@]}" "$url"
}

# numeric version from a tag / range: ^43.4.1 -> 43.4.1, v2.0.8 -> 2.0.8, curl-8_1 -> 8.1
version_of() {
	printf '%s' "$1" | sed -E 's/^[^0-9]*//; s/[_-]/./g; s/[^0-9.].*$//; s/\.+$//'
}

# 0 when $2 is newer than $1 (compares as many fields as $1 has, so v4 vs 4.2.1 compares majors)
is_newer() {
	local current latest i a b
	IFS=. read -ra current <<< "$1"
	IFS=. read -ra latest <<< "$2"
	for ((i = 0; i < ${#current[@]}; i++)); do
		a=${current[i]:-0}; b=${latest[i]:-0}
		((10#$b > 10#$a)) && return 0
		((10#$b < 10#$a)) && return 1
	done
	return 1
}

# newest stable (non prerelease, non draft) GitHub release, else the newest version-like tag
latest_github() {
	local repo=$1 tag
	tag=$(curl_json "https://api.github.com/repos/$repo/releases/latest" 2>/dev/null | jq -r '.tag_name // empty') || true
	if [[ -z $tag ]]; then
		tag=$(curl_json "https://api.github.com/repos/$repo/tags?per_page=100" |
			jq -r '[.[].name | select(test("^v?[0-9]+(\\.[0-9]+)*$"))] | first // empty')
	fi
	[[ -n $tag ]] || return 1
	printf '%s\n' "$tag"
}

# dependencies held back on a major: prints "<major> <note>" (Semantic UI 2.x uses $.isArray etc, removed in jQuery 4)
hold() {
	case $1 in
		jQuery) echo '3 held at 3.x for Semantic UI 2.x' ;;
		@types/jquery) echo '3 held at 3.x with jQuery' ;;
	esac
}

# newest X.y.z tag of a GitHub repo within one major
latest_github_major() {
	local repo=$1 major=$2 tag
	tag=$(curl_json "https://api.github.com/repos/$repo/tags?per_page=100" |
		jq -r --arg m "$major" '.[].name | select(test("^v?" + $m + "(\\.[0-9]+)+$"))' | sort -V | tail -n 1)
	[[ -n $tag ]] || return 1
	printf '%s\n' "$tag"
}

ROWS=()
ERRORS=()
UPDATES=0

# kind name current latest [note]
row() {
	local kind=$1 name=$2 current=$3 latest=$4 note=${5:-} status='ok'
	if is_newer "$(version_of "$current")" "$(version_of "$latest")"; then
		status='**update**'
		UPDATES=$((UPDATES + 1))
	fi
	ROWS+=("| $kind | $name | \`$current\` | \`$latest\` | $status | $note |")
	printf '%-8s %-32s %-14s %-14s %s\n' "$kind" "$name" "$current" "$latest" "${status//\*/}" >&2
}

failed() {
	ERRORS+=("- $1: $2")
	printf 'error    %s: %s\n' "$1" "$2" >&2
}

check_npm() {
	local package_json="$ROOT/frontend/package.json" name range uri held latest electron_latest major note
	while IFS=$'\t' read -r name range; do
		uri=$(jq -rn --arg n "$name" '$n | @uri')
		read -r held note <<< "$(hold "$name")"
		if [[ -n $held ]]; then
			latest=$(curl_json "https://registry.npmjs.org/$uri" | jq -r --arg m "$held" \
				'.versions | keys[] | select(test("^" + $m + "\\.[0-9]+\\.[0-9]+$"))' | sort -V | tail -n 1) || latest=''
		else
			latest=$(curl_json "https://registry.npmjs.org/$uri/latest" | jq -r '.version // empty') || latest=''
		fi
		if [[ -z $latest ]]; then
			failed "npm $name" 'registry lookup failed'
			continue
		fi
		# Electron supports only the latest three major releases
		if [[ $name == electron ]]; then
			electron_latest=${latest%%.*}
			major=$(version_of "$range"); major=${major%%.*}
			if ((major < electron_latest - 2)); then
				note="Electron $major is out of support (supported: $((electron_latest - 2))-$electron_latest)"
			fi
		fi
		row npm "$name" "$range" "$latest" "$note"
	done < <(jq -r '(.dependencies // {}) + (.devDependencies // {}) | to_entries[] | [.key, .value] | @tsv' "$package_json")
}

check_actions() {
	local action ref repo latest workflows
	workflows=$(find "$ROOT/.github/workflows" -name '*.yml' -o -name '*.yaml')
	# shellcheck disable=SC2086
	while read -r action; do
		repo=${action%@*}; ref=${action#*@}
		repo=$(cut -d/ -f1-2 <<< "$repo") # owner/repo/path@ref -> owner/repo
		[[ $ref =~ ^v?[0-9] ]] || continue # @latest / @main / sha pins follow upstream already
		if ! latest=$(latest_github "$repo"); then
			failed "action $repo" 'no release or tag found'
			continue
		fi
		row action "$repo" "$ref" "$latest"
	done < <(grep -hoE 'uses:[[:space:]]*[A-Za-z0-9_.-]+/[A-Za-z0-9_./-]+@[^[:space:]#]+' $workflows |
		sed -E 's/^uses:[[:space:]]*//' | sort -u)
}

check_node() {
	local lts version workflows
	if ! lts=$(curl_json 'https://nodejs.org/dist/index.json' | jq -r '[.[] | select(.lts != false)] | first | .version'); then
		failed 'node' 'nodejs.org lookup failed'
		return
	fi
	workflows=$(find "$ROOT/.github/workflows" -name '*.yml' -o -name '*.yaml')
	# shellcheck disable=SC2086
	while read -r version; do
		row node "node-version (CI)" "$version" "${lts#v}" 'latest LTS'
	done < <(grep -hoE "node-version:[[:space:]]*['\"]?[0-9][0-9.x]*" $workflows | grep -oE '[0-9][0-9.x]*$' | sed 's/\.x$//' | sort -u)
}

# vendored copy in frontend/static: version read from the file header
check_vendored() {
	local name=$1 file=$2 header_regex=$3 repo=$4 current held note latest
	current=$(head -c 2000 "$ROOT/$file" 2>/dev/null | grep -oE "$header_regex" | head -n 1 | grep -oE '[0-9]+(\.[0-9]+)+' || true)
	if [[ -z $current ]]; then
		failed "$name" "no version found in $file"
		return
	fi
	read -r held note <<< "$(hold "$name")"
	if [[ -n $held ]]; then
		latest=$(latest_github_major "$repo" "$held") || latest=''
	else
		latest=$(latest_github "$repo") || latest=''
	fi
	if [[ -z $latest ]]; then
		failed "$name" "no release or tag found for $repo"
		return
	fi
	row vendored "$name ($file)" "$current" "$latest" "$note"
}

publish() {
	local repository=$1 body=$2 number old payload
	[[ $repository =~ ^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$ ]] || { echo 'Invalid owner/repository' >&2; return 1; }
	number=$(gh issue list --repo "$repository" --state open --search "\"$MARKER\" in:body" --json number,body \
		--jq "[.[] | select(.body | contains(\"$MARKER\"))] | first | .number // empty")
	if ((UPDATES > 0 || ${#ERRORS[@]} > 0)); then
		if [[ -n $number ]]; then
			old=$(gh issue view "$number" --repo "$repository" --json body --jq .body)
			[[ $old != "$body" ]] || return 0
			gh issue edit "$number" --repo "$repository" --title "$TITLE" --body "$body" >/dev/null
		else
			gh issue create --repo "$repository" --title "$TITLE" --body "$body" >/dev/null
		fi
	elif [[ -n $number ]]; then
		gh issue close "$number" --repo "$repository" --comment 'All dependencies are up to date.' >/dev/null
	fi
}

main() {
	local file_issue=0 repository=${GITHUB_REPOSITORY:-} body
	while [[ $# -gt 0 ]]; do
		case $1 in
			--file-issue) file_issue=1; shift ;;
			--repository) repository=${2:?Missing repository}; shift 2 ;;
			--help|-h) echo 'Usage: check-dependency-versions.sh [--file-issue [--repository OWNER/REPO]]'; return ;;
			*) echo "Unknown argument: $1" >&2; return 1 ;;
		esac
	done
	command -v jq >/dev/null || { echo 'jq is required' >&2; return 1; }

	check_npm
	check_actions
	check_node
	check_vendored 'jQuery' 'frontend/static/js/jquery.min.js' 'jQuery v[0-9.]+' 'jquery/jquery'
	check_vendored 'Semantic UI' 'frontend/static/css/semantic.css' 'Semantic UI[ -]*[0-9.]+' 'Semantic-Org/Semantic-UI'

	body=$(
		printf '%s\n\n' "$MARKER"
		printf '%s\n\n' "$UPDATES dependencies have newer releases (checked $(date -u +%Y-%m-%d))."
		printf '%s\n%s\n' '| Kind | Dependency | Current | Latest | Status | Note |' '|---|---|---|---|---|---|'
		printf '%s\n' "${ROWS[@]}"
		if ((${#ERRORS[@]} > 0)); then
			printf '\n%s\n' '**Lookup errors**'
			printf '%s\n' "${ERRORS[@]}"
		fi
	)
	printf '%s\n' "$body"
	if [[ -n ${GITHUB_STEP_SUMMARY:-} ]]; then
		printf '%s\n' "$body" >> "$GITHUB_STEP_SUMMARY"
	fi
	if ((file_issue)); then
		[[ -n $repository ]] || { echo '--file-issue needs --repository or GITHUB_REPOSITORY' >&2; return 1; }
		publish "$repository" "$body"
	fi
}

main "$@"
