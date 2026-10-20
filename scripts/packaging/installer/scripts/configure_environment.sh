#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Add the MNE-CPP command-line tools to PATH in the user's shell start-up files,
# or remove them again. Installed into <install dir>/scripts by the installer.
#
# Usage: configure_environment.sh install|uninstall

set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BEGIN="# >>> MNE-CPP >>>"
END="# <<< MNE-CPP <<<"
FISH_FILE="${HOME}/.config/fish/conf.d/mne-cpp.fish"

remove_block()
{
    [ -f "$1" ] || return 0
    awk -v b="${BEGIN}" -v e="${END}" '$0 == b {skip = 1} !skip {print} $0 == e {skip = 0}' "$1" > "$1.mne-cpp.tmp"
    cat "$1.mne-cpp.tmp" > "$1"
    rm -f "$1.mne-cpp.tmp"
}

startup_file()
{
    case "$(basename "${SHELL:-sh}")" in
        zsh) echo "${HOME}/.zshrc" ;;
        bash) if [ "$(uname)" = Darwin ]; then echo "${HOME}/.bash_profile"; else echo "${HOME}/.bashrc"; fi ;;
        *) echo "${HOME}/.profile" ;;
    esac
}

uninstall()
{
    for f in "${HOME}/.zshrc" "${HOME}/.bash_profile" "${HOME}/.bashrc" "${HOME}/.profile"; do
        remove_block "$f"
    done
    rm -f "${FISH_FILE}"
}

case "${1:-}" in
    install)
        uninstall
        file="$(startup_file)"
        {
            echo ""
            echo "${BEGIN}"
            echo "export MNE_CPP_ROOT=\"${ROOT}\""
            echo 'export PATH="$MNE_CPP_ROOT/bin:$PATH"'
            echo 'if [ -d "$MNE_CPP_ROOT/data/MNE-sample-data" ]; then export MNE_DATASETS_SAMPLE_PATH="$MNE_CPP_ROOT/data"; fi'
            echo "${END}"
        } >> "${file}"
        if [ "$(basename "${SHELL:-sh}")" = fish ]; then
            mkdir -p "$(dirname "${FISH_FILE}")"
            printf 'set -gx MNE_CPP_ROOT "%s"\nfish_add_path -g "$MNE_CPP_ROOT/bin"\n' "${ROOT}" > "${FISH_FILE}"
        fi
        echo "MNE-CPP: added ${ROOT}/bin to PATH in ${file}"
        ;;
    uninstall)
        uninstall
        echo "MNE-CPP: removed the PATH entries"
        ;;
    *)
        echo "Usage: $0 install|uninstall" >&2
        exit 2
        ;;
esac
