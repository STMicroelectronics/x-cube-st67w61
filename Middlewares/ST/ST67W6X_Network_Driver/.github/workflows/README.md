# PR code cleanup workflow

This repository now includes a starter workflow:

- `.github/workflows/pr-code-cleanup.yml`

It runs on pull requests (non-draft) with this sequence:

1. `codespell` on changed text/code files.
2. `MCUAStyle` on changed `*.c` and `*.h` files.

## Internal MCUAStyle tool used

- Source repository: `ssh://gerrit.st.com:29418/stm32cube/internal/mcuastyle.git`
- In CI, the tool is cloned to `/tmp/mcuastyle`
- Executable: `/tmp/mcuastyle/mcuAStyle`
- Options: `/tmp/mcuastyle/formattingOption.txt`

## Required secret

Add this repository secret in GitHub:

- `GERRIT_SSH_PRIVATE_KEY`: SSH private key with read access to the Gerrit repository.

## How failures look

- Spell check fails if `codespell` detects typos.
- AStyle check fails if MCUAStyle output contains:
  - `Severity-`
  - `>>>>> Formatted`

## Typical local fix command

From repository root on Git Bash / Linux:

```bash
/tmp/mcuastyle/mcuAStyle -I --options=/tmp/mcuastyle/formattingOption.txt <path/to/file.c>
```

Then commit the formatted files and push again.

## First tuning points

- Update the `codespell` ignore list in the workflow for project-specific terms.
- Extend/limit file globs in the two `changed-files` steps.
