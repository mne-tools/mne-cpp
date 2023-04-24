set -e

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
BaseFolder="$(cleanAbsPath "$ScriptPath/..")"

COV_WORKING_PATH="${BaseFolder}/cov_scan"

echo -e "Coverity working path: ${COV_WORKING_PATH}"

COVERITY_SCAN_PROJECT_NAME="mne-tools/mne-cpp"
COVERITY_SCAN_NOTIFICATION_EMAIL="mne_cpp@googlegroups.com"
COVERITY_SCAN_BRANCH_PATTERN="main"
COVERITY_SCAN_BUILD_COMMAND_PREPEND="cmake -B build -S src -DCMAKE_BUILD_TYPE=Release"
COVERITY_SCAN_BUILD_COMMAND="cmake --build build"
COVERITY_SCAN_TOKEN=$1

echo -e "Build commands: "
echo -e "    ${COVERITY_SCAN_BUILD_COMMAND_PREPEND}"
echo -e "    ${COVERITY_SCAN_BUILD_COMMAND}"

PLATFORM=`uname`
TOOL_ARCHIVE=${COV_WORKING_PATH}/tmp/cov-analysis-${PLATFORM}.tgz
TOOL_URL=https://scan.coverity.com/download/${PLATFORM}
TOOL_BASE=${COV_WORKING_PATH}/tmp/coverity-scan-analysis
UPLOAD_URL="https://scan.coverity.com/builds?project=mne-tools%2Fmne-cpp"
SCAN_URL="https://scan.coverity.com"

mkdir -p $TOOL_BASE



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
RESULTS_DIR="${COV_WORKING_PATH}/cov-int"

echo -e "Building..."
eval "${COVERITY_SCAN_BUILD_COMMAND_PREPEND}"
COVERITY_UNSUPPORTED=1 cov-build --dir $RESULTS_DIR $COV_BUILD_OPTIONS $COVERITY_SCAN_BUILD_COMMAND
cov-import-scm --dir $RESULTS_DIR --scm git --log $RESULTS_DIR/scm_log.txt 2>&1

RESULTS_ARCHIVE='analysis-results.tgz'

echo -e "Zipping results..."
tar czf $RESULTS_ARCHIVE $RESULTS_DIR
echo -e "Zipping done."

SHA=`git rev-parse --short HEAD`

echo -e "\033[33;1mUploading results...\033[0m"

response=$(curl \
  --silent --write-out "\n%{http_code}\n" \
  --form token=$COVERITY_SCAN_TOKEN \
  --form email=$COVERITY_SCAN_NOTIFICATION_EMAIL \
  --form file=@$RESULTS_ARCHIVE \
  --form version=$SHA \
  --form description="Github Actions build" \
  $UPLOAD_URL)
echo "$response"
status_code=$(echo "$response" | sed -n '$p')

echo -e "\u001b[32;1mDone.\u001b[0m"

