# Repository instructions

## Attribution

Do not add assistant attribution to commits, pull requests, issues or comments.

## Release versions

Whenever the GUI version in `frontend/package.json` increases, also increase the
command-line version in `commandLine/src/defines.h`, including GUI-only,
packaging and dependency releases. Both versions must advance relative to the
PR's base branch; their numbers do not need to match. A CLI-only release does not
require a GUI version bump.

Keep both root version fields in `frontend/npm-shrinkwrap.json` equal to the GUI
version, and state both GUI and CLI versions in the PR description. Run
`python3 scripts/check_release_versions.py <base-ref>` before pushing a version
change. See the release version policy in `README.md`.
