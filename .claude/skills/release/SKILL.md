---
name: release
description: Cut a new dwarf2cpp release end-to-end — set the version, curate and stamp CHANGELOG, commit, tag, push, then watch the Build workflow publish to PyPI and create the GitHub release. Use when the user asks to release/ship a new version (e.g. "release 0.2.0", "cut a release", "ship v0.1.1").
---

# Release dwarf2cpp

Drive the release from a local checkout. Execute the steps in order; do not
skip. Stop only at points flagged **STOP**.

## Pipeline (what happens after the tag is pushed)

`build.yml` is the only workflow. It builds wheels on every push/PR and, on a
`v*` **tag** push, additionally builds the sdist, publishes to PyPI (via OIDC),
and creates the GitHub release with notes extracted from `CHANGELOG.md`.

```
git push --follow-tags origin main
        |
        v
   build.yml (on tag)
     build_wheels  (ubuntu / windows / macos, cp312-abi3)
        |
        +-- make_sdist
        +-- publish   (needs build_wheels + make_sdist: PyPI + GitHub release)
```

The tag push must be authored by the user (not `GITHUB_TOKEN`) or `build.yml`
won't trigger — that's why this skill pushes the tag from the local checkout,
never from a workflow.

The version lives in `pyproject.toml` (`version = "X.Y.Z"`). The tag is
`vX.Y.Z` and must equal it (the wheels carry the pyproject version; a mismatch
publishes the wrong version under the tag).

---

## Step 0 — determine the target version

```shell
grep '^version' pyproject.toml              # current version
git tag --sort=-version:refname | head -n5  # recent tags
git rev-parse --abbrev-ref HEAD             # must be main
```

Compute the next version: default = previous tag's patch + 1; a minor/major
bump only if the user said so. If `pyproject.toml`'s `version` is not already
the target, edit it with `Edit`. If a bump is needed and the user hasn't
confirmed the target, **STOP** to confirm.

---

## Step 1 — pre-flight checks (all must pass)

```shell
git status --porcelain                                # must be empty
git rev-list --left-right --count @{u}...HEAD         # must be "0 0"
gh run list --branch main --workflow=Build --limit 3  # latest must be success
```

1. **Working tree clean.**
2. **In sync with origin** (left-right `0  0`).
3. **Build green on HEAD** — *every* wheel matrix job (ubuntu / windows /
   macos). One red job means the `publish` job fails after the tag is pushed,
   leaving an orphan tag and a half-published release.

If any fails, surface the exact failure and **STOP**. For a failed job:

```shell
gh api repos/EndstoneMC/dwarf2cpp/actions/jobs/<job_id>/logs 2>&1 | tail -100
```

---

## Step 2 — curate CHANGELOG `[Unreleased]`

```shell
git log --format='%h %s' v<prev>..HEAD
```

Read `CHANGELOG.md` (top ~30 lines). For each commit decide if it's
user-visible:

- **Include**: API/CLI changes, behavioral changes, bug fixes, dependency or
  toolchain changes users notice, supported-input changes.
- **Skip**: internal refactors, CI/build-only changes, tests, docs/skill-only.

Follow [Keep a Changelog](https://keepachangelog.com/en/1.1.0/): group under
`### Added` / `### Changed` / `### Deprecated` / `### Removed` / `### Fixed` /
`### Security` (omit empty groups), past tense, one bullet per change. Edit in
anything missing. **STOP** and show the final `[Unreleased]` body for approval
before stamping — the one human checkpoint.

---

## Step 3 — stamp `CHANGELOG.md`

```shell
date -u +%Y-%m-%d                          # DATE
git tag --sort=-version:refname | head -n1  # PREV_TAG
```

With `Edit`, turn

```
## [Unreleased]

### Added
- ...
```

into

```
## [Unreleased]

## [X.Y.Z] - YYYY-MM-DD

### Added
- ...
```

Move every `[Unreleased]` bullet into the new `[X.Y.Z]` block and leave
`[Unreleased]` empty for the next cycle. The `## [X.Y.Z]` header must be
**unique and non-empty** — `build.yml`'s `publish` job greps for it and fails
the release if it's missing.

---

## Step 4 — commit, tag, push

```shell
git add pyproject.toml CHANGELOG.md
git commit -m "Release X.Y.Z"
git tag -a vX.Y.Z -m "Release X.Y.Z"
git push --follow-tags origin main
```

`--follow-tags` pushes the commit and the annotated tag together. If the push
is rejected (someone pushed first):

```shell
git pull --rebase origin main
git push --follow-tags origin main
```

---

## Step 5 — watch the Build run

```shell
gh run list --workflow=Build --limit 3   # a new run against vX.Y.Z
gh run watch <run_id>                     # blocks until done (use run_in_background for long waits)
```

On a tag push: `build_wheels` (3 wheels — `manylinux` / `win_amd64` /
`macosx`, all `cp312-abi3`) → `make_sdist` → `publish` (PyPI + GitHub release).
Expect ~30–60 min, dominated by the LLVM-from-source build; much faster with a
warm Conan cache.

---

## Step 6 — verify

```shell
gh release view vX.Y.Z          # release exists, has notes + wheels + sdist
pip index versions dwarf2cpp    # PyPI lists X.Y.Z
```

Report the verified artifacts back to the user.

---

## Recovery

- **`publish` failed — missing CHANGELOG section.** Fix the `## [X.Y.Z]`
  header in a follow-up commit, then rerun the `publish` job from the Actions
  UI (no re-tag).
- **One wheel matrix job failed.** Rerun just that job; the others' artifacts
  are kept and `publish` picks them up on retry.
- **Wheels built but PyPI publish failed.** Rerun `publish`; wheels are already
  in artifacts.
- **CHANGELOG typo, tag already pushed.** Fix in a follow-up commit, then
  `gh release edit vX.Y.Z --notes-file <new-body>`.
- **Wrong commit tagged.** Delete the tag locally and on origin
  (`git push --delete origin vX.Y.Z`), yank from PyPI, delete the GH release,
  restart from Step 3.

## Pitfalls

- **`GITHUB_TOKEN` pushes don't cascade**, so the tag must be pushed from the
  local checkout (this skill) — never create the tag inside a workflow.
- **`[Unreleased]` must remain after stamping** — copy its contents into the
  new `[X.Y.Z]` stanza, don't rename it.
- **Tag must match `pyproject.toml` version** — the wheels carry the pyproject
  version; `vX.Y.Z` and `version = "X.Y.Z"` must agree.
