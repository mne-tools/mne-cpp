#!/usr/bin/env bash

set -euo pipefail

function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}


echo -e "\033[33;1mSetting Parameters...\033[0m"

ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BaseFolder="$(cleanAbsPath "$ScriptPath/../..")"

COV_WORKING_PATH="${BaseFolder}/cov_scan"
COVERITY_SCAN_BUILD_PATH="${COV_WORKING_PATH}/build"
RESULTS_DIR="${COV_WORKING_PATH}/cov-int"
# Limit the scan to MNE-CPP's own libraries. Including the GUI applications
# (mne_scan / mne_analyze* / mne_inspect / mne_browse / mne_dipole_fit) and
# their generated UI/MOC translation units roughly tripled the cov-int
# tarball and pushed it past the Coverity Scan upload limit (HTTP 413).
# Library coverage is the primary security-relevant surface anyway.
COVERITY_SCAN_WHITELIST=(
    "${BaseFolder}/src/libraries"
)

echo -e "Coverity working path: ${COV_WORKING_PATH}"

COVERITY_SCAN_PROJECT_NAME="mne-tools/mne-cpp"
COVERITY_SCAN_NOTIFICATION_EMAIL="mne_cpp@googlegroups.com"
COVERITY_SCAN_BRANCH_PATTERN="staging"

# Prefer Ninja when available; falls back to the platform default otherwise.
CMAKE_GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR_ARGS=(-G Ninja)
fi

declare -a COVERITY_SCAN_CONFIGURE_COMMAND=(
    cmake
    "${CMAKE_GENERATOR_ARGS[@]}"
    -B "${COVERITY_SCAN_BUILD_PATH}"
    -S "${BaseFolder}/src"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DBUILD_APPLICATIONS=OFF
    -DBUILD_TOOLS=OFF
    -DBUILD_EXAMPLES=OFF
    -DBUILD_TESTS=OFF
)
declare -a COVERITY_SCAN_BUILD_COMMAND=(
    cmake
    --build "${COVERITY_SCAN_BUILD_PATH}"
)
COVERITY_SCAN_TOKEN=$1

echo -e "Build commands: "
printf '    %q ' "${COVERITY_SCAN_CONFIGURE_COMMAND[@]}"
printf '\n'
printf '    %q ' "${COVERITY_SCAN_BUILD_COMMAND[@]}"
printf '\n'
echo -e "Whitelisted source roots:"
for whitelistPath in "${COVERITY_SCAN_WHITELIST[@]}"; do
    echo -e "    ${whitelistPath}"
done

PLATFORM=`uname`
TOOL_ARCHIVE=${COV_WORKING_PATH}/tmp/cov-analysis-${PLATFORM}.tgz
TOOL_URL=https://scan.coverity.com/download/${PLATFORM}
TOOL_BASE=${COV_WORKING_PATH}/tmp/coverity-scan-analysis
UPLOAD_URL="https://scan.coverity.com/builds?project=mne-tools%2Fmne-cpp"
SCAN_URL="https://scan.coverity.com"

mkdir -p $TOOL_BASE
mkdir -p "$COV_WORKING_PATH"



echo -e "\033[33;1mDownloading coverity tools...\033[0m"

echo -e "Downloading to ${TOOL_ARCHIVE}..."

## Download and unzip coverity overity tools.
wget -nv -O $TOOL_ARCHIVE $TOOL_URL --post-data "project=$COVERITY_SCAN_PROJECT_NAME&token=$COVERITY_SCAN_TOKEN"
echo -e "Download done."

pushd $TOOL_BASE
echo -e "Unzipping..."
tar xzf $TOOL_ARCHIVE --warning=none
echo -e "Unzip done."
popd

TOOL_DIR=`find $TOOL_BASE -type d -name 'cov-analysis*'`
export PATH=$TOOL_DIR/bin:$PATH

echo -e "Found tool directory: ${TOOL_DIR}"

echo -e "\033[33;1mRunning Coverity Scan Analysis Tool...\033[0m"

COV_BUILD_OPTIONS=""

echo -e "Building..."
"${COVERITY_SCAN_CONFIGURE_COMMAND[@]}"
echo -e "Validating whitelist against compile_commands.json..."
python3 - "${COVERITY_SCAN_BUILD_PATH}/compile_commands.json" "${COVERITY_SCAN_BUILD_PATH}" "${COVERITY_SCAN_WHITELIST[@]}" <<'PY'
import json
import os
import sys

compile_commands_path, build_root, *allowed_roots = sys.argv[1:]
build_root = os.path.realpath(build_root)
allowed_roots = [os.path.realpath(path) for path in allowed_roots]

with open(compile_commands_path, "r", encoding="utf-8") as handle:
    compile_commands = json.load(handle)

unexpected_sources = []
for entry in compile_commands:
    source_path = os.path.realpath(entry["file"])
    if source_path == build_root or source_path.startswith(build_root + os.sep):
        continue
    if any(source_path == root or source_path.startswith(root + os.sep) for root in allowed_roots):
        continue
    unexpected_sources.append(source_path)

if unexpected_sources:
    print("ERROR: Coverity whitelist rejected source files outside libraries/applications.")
    for source_path in sorted(set(unexpected_sources)):
        print(source_path)
    sys.exit(1)

print("Whitelist validation passed.")
PY
COVERITY_UNSUPPORTED=1 cov-build --dir "$RESULTS_DIR" $COV_BUILD_OPTIONS "${COVERITY_SCAN_BUILD_COMMAND[@]}"
cov-import-scm --dir "$RESULTS_DIR" --scm git --log "$RESULTS_DIR/scm_log.txt" 2>&1

RESULTS_ARCHIVE='analysis-results.tgz'

echo -e "Zipping results..."
tar czf "$RESULTS_ARCHIVE" -C "$COV_WORKING_PATH" "$(basename "$RESULTS_DIR")"
echo -e "Zipping done."

ARCHIVE_SIZE_BYTES=$(stat -c%s "$RESULTS_ARCHIVE" 2>/dev/null || stat -f%z "$RESULTS_ARCHIVE")
ARCHIVE_SIZE_MB=$(( ARCHIVE_SIZE_BYTES / 1024 / 1024 ))
echo -e "Archive size: ${ARCHIVE_SIZE_MB} MiB (${ARCHIVE_SIZE_BYTES} bytes)"

# Coverity Scan rejects uploads above ~500 MiB with a Cloudflare 413.
# Surface this before we waste minutes on a curl that will silently fail.
if (( ARCHIVE_SIZE_MB > 500 )); then
    echo -e "\033[31;1mERROR: cov-int archive exceeds the 500 MiB Coverity Scan upload limit.\033[0m" >&2
    echo -e "Reduce the scan footprint (build fewer targets) and retry." >&2
    exit 2
fi

SHA=`git rev-parse --short HEAD`

echo -e "\033[33;1mUploading results...\033[0m"

response=$(curl \
  --silent --write-out "\n%{http_code}\n" \
  --form token=$COVERITY_SCAN_TOKEN \
  --form email=$COVERITY_SCAN_NOTIFICATION_EMAIL \
  --form file=@$RESULTS_ARCHIVE \
  --form version=$SHA \
  --form description="GitHub Actions build (${COVERITY_SCAN_BRANCH_PATTERN}, whitelist: libraries)" \
  $UPLOAD_URL)
echo "$response"
status_code=$(echo "$response" | sed -n '$p')

if [[ ! "$status_code" =~ ^2[0-9][0-9]$ ]]; then
    echo -e "\033[31;1mERROR: Coverity Scan upload failed with HTTP ${status_code}.\033[0m" >&2
    exit 3
fi

echo -e "\u001b[32;1mDone.\u001b[0m"
