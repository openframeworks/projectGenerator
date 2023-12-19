#!/usr/bin/env bash
set -e

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Function to calculate SHA for security
calculate_hash() {
    local file=$1
    if [[ -f "$file" ]]; then
        if command -v sha256sum &>/dev/null; then
            sha256sum "$file" | awk '{print $1}'
        elif command -v sha1sum &>/dev/null; then
            sha1sum "$file" | awk '{print $1}'
        elif command -v sha512sum &>/dev/null; then
            sha512sum "$file" | awk '{print $1}'
        elif command -v md5sum &>/dev/null; then
            md5sum "$file" | awk '{print $1}'
        elif command -v md5 &>/dev/null; then
            md5 -q "$file"
        else
            echo "No suitable hash function found."
        fi
    else
        echo "N/A"
    fi
}

# Get current date and time in ISO 8601 format
BUILD_TIME=$(date -u +"%Y-%m-%d T%H:%M:%SZ")

# Check if git is available and repository exists
if command -v git &>/dev/null && git rev-parse --git-dir > /dev/null 2>&1; then
    # Get the current Git commit hash
    GIT_HASH=$(git rev-parse HEAD)

    # Get the current Git branch
    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
else
    GIT_HASH="N/A"
    GIT_BRANCH="N/A"
fi

if [ -z "${BINARY_SEC+x}" ]; then
    BINARY_SEC=${1:-}
fi


# If OUTPUT_LOCATION is not set, and if the second argument is not provided,
# set OUTPUT_LOCATION to the directory of BINARY_SEC
if [ -z "${OUTPUT_LOCATION+x}" ]; then
    if [ -z "$2" ]; then
        if [ -n "$BINARY_SEC" ]; then
            OUTPUT_LOCATION=$(dirname "$BINARY_SEC")
        else
            OUTPUT_LOCATION=.
        fi
    else
        OUTPUT_LOCATION=$2
    fi
    echo "OUTPUT_LOCATION: $OUTPUT_LOCATION"
fi

# Calculate SHA hash for the provided binary, if available
BINARY_SHA=$(calculate_hash "$BINARY_SEC")

OUTPUT_FILE="${OUTPUT_LOCATION:-.}/version.json"

# Create or overwrite the version.json file
cat <<EOF > "$OUTPUT_FILE"
{
  "buildTime": "$BUILD_TIME",
  "gitHash": "$GIT_HASH",
  "gitBranch": "$GIT_BRANCH",
  "binarySha": "$BINARY_SHA"
}
EOF

# Display the contents of the version.json file
cat "$OUTPUT_FILE"
