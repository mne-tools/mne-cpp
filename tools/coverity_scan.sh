
function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}

ScriptPath="$(cleanAbsPath "$(dirname "$0")")"
BaseFolder="$(cleanAbsPath "$ScriptPath/..")"

COV_WORKING_PATH="${BaseFolder}/cov_scan"

COVERITY_SCAN_PROJECT_NAME="mne-tools/mne-cpp"
COVERITY_SCAN_NOTIFICATION_EMAIL="mne_cpp@googlegroups.com"
COVERITY_SCAN_BRANCH_PATTERN="main"
COVERITY_SCAN_BUILD_COMMAND_PREPEND="cmake -B build -S src -DCMAKE_BUILD_TYPE=Release"
COVERITY_SCAN_BUILD_COMMAND="cmake --build build"
COVERITY_SCAN_TOKEN=$1

PLATFORM=`uname`
TOOL_ARCHIVE=${COV_WORKING_PATH}/tmp/cov-analysis-${PLATFORM}.tgz
TOOL_URL=https://scan.coverity.com/download/${PLATFORM}
TOOL_BASE=${COV_WORKING_PATH}/tmp/coverity-scan-analysis
UPLOAD_URL="https://scan.coverity.com/builds?project=mne-tools%2Fmne-cpp"
SCAN_URL="https://scan.coverity.com"

## Download and unzip coverity overity tools.
wget -nv -O $TOOL_ARCHIVE $TOOL_URL --post-data "project=$COVERITY_SCAN_PROJECT_NAME&token=$COVERITY_SCAN_TOKEN"
mkdir -p $TOOL_BASE
pushd $TOOL_BASE
tar xzf $TOOL_ARCHIVE --warning=none
popd

TOOL_DIR=`find $TOOL_BASE -type d -name 'cov-analysis*'`
export PATH=$TOOL_DIR/bin:$PATH
echo -e "echo 3"
echo -e "\033[33;1mRunning Coverity Scan Analysis Tool...\033[0m"

COV_BUILD_OPTIONS=""
RESULTS_DIR="${COV_WORKING_PATH}cov-int"
eval "${COVERITY_SCAN_BUILD_COMMAND_PREPEND}"
COVERITY_UNSUPPORTED=1 cov-build --dir $RESULTS_DIR $COV_BUILD_OPTIONS $COVERITY_SCAN_BUILD_COMMAND
cov-import-scm --dir $RESULTS_DIR --scm git --log $RESULTS_DIR/scm_log.txt 2>&1
RESULTS_ARCHIVE=analysis-results.tgz
tar czf $RESULTS_ARCHIVE $RESULTS_DIR
SHA=`git rev-parse --short HEAD`
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

