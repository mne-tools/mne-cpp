function cleanAbsPath()
{
    local  cleanAbsPathStr="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$cleanAbsPathStr"
}

SCRIPT_PATH="$(cleanAbsPath "$(dirname "$0")")"
BASE_PATH="$(cleanAbsPath "${SCRIPT_PATH}/..")"

cd $BASE_PATH

SED_OS_DEPENDANT_FORMATTING=""

if [ "$(uname)" == "Darwin" ]; then
    SED_OS_DEPENDANT_FORMATTING="''"
fi

cleanup()
{
    echo "Processing files in dir: $1"
    for f in $1/*.md; do
        echo "    Processing: $f"
        sed -i ${SED_OS_DEPENDANT_FORMATTING} '/---/,/---/d' $f 
        sed -i ${SED_OS_DEPENDANT_FORMATTING} 's/{:target="_blank" rel="noopener"}//g' $f
    done
}

cp -r doc doc2

cleanup doc2/gh-pages
cleanup doc2/gh-pages/pages/development
cleanup doc2/gh-pages/pages/documentation

pandoc \
    doc2/gh-pages/*.md \
    doc2/gh-pages/pages/development/*.md \
    doc2/gh-pages/pages/documentation/*.md \
    -f gfm \
    -o test.pdf \
    --pdf-engine=xelatex \
    -V geometry:top=.75in \
    -V geometry:left=1in \
    -V geometry:right=1in \
    -V geometry:bottom=.75in \
    --extract-media=media_dir \
    --resource-path=doc2/gh-pages/images:doc2/gh-pages/images/lib:doc2/gh-pages/images/analyze

rm -r doc2 media_dir

