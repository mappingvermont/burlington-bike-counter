---
name: feedback-no-commits
description: Never commit to git in this project — user manages commits manually
metadata:
  type: feedback
---

Never run git commit (or any git write commands) in this project.

**Why:** User manages all commits manually and does not want Claude committing on their behalf.

**How to apply:** Do not commit even when asked to "save" changes or at natural stopping points. Only make file edits; leave git operations to the user.
