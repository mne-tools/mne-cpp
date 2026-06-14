#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright (C) 2026, Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Generate doc/website/src/data/contributors.json from git shortlog.
# Called automatically during website CI deploy so the contributor section
# always reflects the latest commit counts.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="${REPO_ROOT}/doc/website/src/data"
OUT_FILE="${OUT_DIR}/contributors.json"

# Excluded accounts (bots, CI, AI tools)
EXCLUDE_PATTERN='dependabot\[bot\]|claude|github-actions\[bot\]|web-flow'

mkdir -p "$OUT_DIR"

# GitHub login → display name mapping (sourced from Zenodo record 10.5281/zenodo.19498106)
read -r -d '' DISPLAY_MAP_JSON << 'PYEOF' || true
{
    "LorenzE": "Lorenz Esch",
    "chdinh": "Christoph Dinh",
    "gabrielbmotta": "Gabriel Motta",
    "juangpc": "Juan Garcia-Prieto",
    "RDoerfel": "Ruben Dörfel",
    "LostSign": "Lars Debor",
    "1DIce": "Daniel Knobl",
    "ViktorKL": "Viktor Klüber",
    "MartinHenfworx": "Martin Henfling",
    "rickytjen": "Ricky Tjen",
    "JanaKiesel": "Jana Kiesel",
    "limin-sun": "Limin Sun",
    "floschl": "Florian Schlembach",
    "joewalter": "Daniel Strohmeier",
    "sheinke": "Simon Heinke",
    "Andrey1994": "Andrey Parfenov",
    "TiKunze": "Tim Kunze",
    "farndt": "Felix Arndt",
    "wayneMead": "Wayne Mead",
    "louiseichhorst": "Louis Eichhorst",
    "femigr": "Felix Griesau",
    "jvorwerk": "Johannes Vorwerk",
    "cdoshi": "Chiran Doshi",
    "jobehrens": "Johannes Behrens",
    "cpieloth": "Christof Pieloth",
    "alexrockhill": "Alex Rockhill",
    "robertdicamillo": "Robert Dicamillo",
    "SachdevaS": "Sugandha Sachdeva",
    "er06645810": "Erik Hornberger",
    "mfarisyahya": "Faris Yahya",
    "ag-fieldline": "ag-fieldline",
    "fjpolo": "Franco Polo",
    "MKlamke": "Marco Klamke",
    "betaha": "betaha",
    "Julius-L": "Julius Lerm",
    "benkay86": "Benjamin Kay",
    "jasmainak": "Mainak Jas",
    "PetrosSimidyan": "Petros Simidyan",
    "larsoner": "Eric Larson",
    "GBeret": "Gab Beret",
    "imsorryk": "Felix Schwarzmeier",
    "johaenns": "Johannes Fuhrwerk",
    "buildqa": "buildqa"
}
PYEOF

# git shortlog -sne gives: <count> <TAB> <Name> <email>
# We need GitHub login, so we map author email → login via git log.
# Simpler approach: use git shortlog with mailmap, then map known
# authors. But the most reliable way for GitHub is to extract from
# the commit trailer or use the GitHub username directly.
#
# Pragmatic approach: extract unique authors from git log with
# format=%aN, count commits, then map to GitHub login via a
# co-author mapping file or the existing known mapping.
#
# Simplest reliable approach: use GitHub API if available, otherwise
# fall back to git shortlog by name.

# Use GitHub API if GH_TOKEN or GITHUB_TOKEN is available (CI environment)
if command -v gh &>/dev/null && gh auth status &>/dev/null 2>&1; then
    echo "Using GitHub API to fetch contributors..."
    gh api "repos/mne-tools/mne-cpp/contributors?per_page=100&anon=0" \
        --jq '[.[] | select(.login != "dependabot[bot]" and .login != "claude" and .login != "github-actions[bot]" and .login != "web-flow") | {login: .login, contributions: .contributions}]' \
    | python3 -c "
import json, sys
DISPLAY_MAP = ${DISPLAY_MAP_JSON}
data = json.load(sys.stdin)
for c in data:
    c['name'] = DISPLAY_MAP.get(c['login'], c['login'])
json.dump(data, sys.stdout, indent=2)
print()
" > "$OUT_FILE"
elif [ -n "${GH_TOKEN:-${GITHUB_TOKEN:-}}" ]; then
    echo "Using GitHub API (curl) to fetch contributors..."
    TOKEN="${GH_TOKEN:-${GITHUB_TOKEN:-}}"
    curl -sf -H "Authorization: token ${TOKEN}" \
        -H "Accept: application/vnd.github+json" \
        "https://api.github.com/repos/mne-tools/mne-cpp/contributors?per_page=100&anon=0" \
    | python3 -c "
import json, sys
DISPLAY_MAP = ${DISPLAY_MAP_JSON}
data = json.load(sys.stdin)
exclude = {'dependabot[bot]', 'claude', 'github-actions[bot]', 'web-flow'}
result = [{'login': c['login'], 'contributions': c['contributions'],
           'name': DISPLAY_MAP.get(c['login'], c['login'])}
          for c in data if c.get('login') not in exclude]
json.dump(result, sys.stdout, indent=2)
print()
" > "$OUT_FILE"
else
    echo "No GitHub API access; falling back to git shortlog..."
    # Fall back to git shortlog — maps author names to known GitHub logins
    python3 -c "
import subprocess, json, re, sys

# Get shortlog counts
result = subprocess.run(
    ['git', '-C', '${REPO_ROOT}', 'shortlog', '-sne', '--no-merges', 'HEAD'],
    capture_output=True, text=True, check=True,
)

# Known name/email → GitHub login mapping
NAME_MAP = {
    'christoph dinh': 'chdinh',
    'lorenz esch': 'LorenzE',
    'gabriel motta': 'gabrielbmotta',
    'juan garcia-prieto': 'juangpc',
    'ruben doerfel': 'RDoerfel',
    'ruben dörfel': 'RDoerfel',
    'daniel knobl': '1DIce',
    'lars debor': 'LostSign',
    'viktor klüber': 'ViktorKL',
    'gab beret': 'GBeret',
    'ricky tjandra': 'rickytjen',
    'jana kiesel': 'JanaKiesel',
    'florian schlegel': 'floschl',
    'daniel strohmeier': 'joewalter',
    'joe walter': 'joewalter',
    'simon heinke': 'sheinke',
    'andrey parfenov': 'Andrey1994',
    'tim kunze': 'TiKunze',
    'felix arndt': 'farndt',
    'felix schwarzmeier': 'imsorryk',
    'louise eichhorst': 'louiseichhorst',
    'johannes fuhrwerk': 'johaenns',
    'johannes ahrens': 'johaenns',
    'johannes ehlers': 'johaenns',
    'felix gr.': 'femigr',
    'felix griesau': 'femigr',
    'johanna behrens': 'jobehrens',
    'chiran doshi': 'cdoshi',
    'christoph pieloth': 'cpieloth',
    'alex rockhill': 'alexrockhill',
    'buildqa': 'buildqa',
    'sugandha sachdeva': 'SachdevaS',
    'faris yahya': 'mfarisyahya',
    'er': 'er06645810',
    'eric larson': 'larsoner',
    'fj polo': 'fjpolo',
    'beta hakan': 'betaha',
    'moritz klamke': 'MKlamke',
    'julius liebl': 'Julius-L',
    'jas': 'jasmainak',
    'mainak jas': 'jasmainak',
    'ben kay': 'benkay86',
    'petros simidyan': 'PetrosSimidyan',
    'ag-fieldline': 'ag-fieldline',
}

EXCLUDE = {'dependabot[bot]', 'claude', 'github-actions[bot]', 'web-flow',
           'dependabot-preview[bot]', 'GitHub'}

contributors = {}
for line in result.stdout.strip().split('\n'):
    m = re.match(r'\s*(\d+)\s+(.+)', line)
    if not m:
        continue
    count = int(m.group(1))
    author = m.group(2).strip()
    # Extract name (strip email)
    name = re.sub(r'\s*<[^>]+>', '', author).strip()
    if name in EXCLUDE or any(e in name for e in EXCLUDE):
        continue
    login = NAME_MAP.get(name.lower(), name)
    contributors[login] = contributors.get(login, 0) + count

# Display name mapping (from Zenodo)
DISPLAY_MAP = ${DISPLAY_MAP_JSON}

# Sort by contributions descending
sorted_c = sorted(contributors.items(), key=lambda x: -x[1])
result = [{'login': login, 'contributions': count,
           'name': DISPLAY_MAP.get(login, login)} for login, count in sorted_c]
json.dump(result, sys.stdout, indent=2)
print()
" > "$OUT_FILE"
fi

echo "Generated $(python3 -c "import json; print(len(json.load(open('${OUT_FILE}'))))" ) contributors → ${OUT_FILE}"
