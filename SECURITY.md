# Security Policy

## Supported Versions

Security fixes are made on the `staging` branch and shipped with the next release.
Only the latest release line receives fixes; older releases are not patched.

| Version             | Supported          |
| ------------------- | ------------------ |
| 2.3.x (latest)      | :white_check_mark: |
| `staging` (dev)     | :white_check_mark: |
| < 2.3               | :x:                |

## Reporting a Vulnerability

Please **do not** open a public issue, pull request or discussion for a security problem.

Report it privately through GitHub's
[private vulnerability reporting](https://github.com/mne-tools/mne-cpp/security/advisories/new)
(*Security and quality* tab → *Report a vulnerability*). Please include:

- the affected version or commit, platform and component (library, application, tool or plugin),
- steps or a minimal input file to reproduce the problem,
- the impact you expect (e.g. crash, memory corruption, code execution, data disclosure).

What to expect:

- We acknowledge your report within **7 days**.
- We confirm or decline the issue and share our assessment within **30 days**, and keep you
  updated at least every 30 days until it is resolved.
- If the report is accepted, we prepare a fix in a private fork, publish a GitHub Security Advisory
  (requesting a CVE where appropriate) together with the fixed release, and credit you unless you
  prefer to stay anonymous.
- If the report is declined, we explain why. You are welcome to disclose it publicly after that.

Please give us a reasonable time to release a fix before any public disclosure.

## Scope

In scope: the MNE-CPP libraries, applications, tools and plugins in this repository, and the
release binaries and installers published on the GitHub releases page.

MNE-CPP is research software and is **not** a certified medical device; reports that only concern
its clinical suitability are out of scope. Vulnerabilities in third-party dependencies (Qt, Eigen,
LSL, ONNX Runtime, …) should be reported upstream; tell us as well if MNE-CPP is affected.
