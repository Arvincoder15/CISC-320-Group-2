# Team development workflow

1. Read your handoff in `docs/members/` and the shared contracts. Review unresolved decisions that affect your task.
2. Create/estimate a Jira issue before follow-up implementation. Branch as `feature/SCRUM-123-short-description` (or `fix/...`) using the real key.
3. Keep the change focused on your module. Coordinate shared interface changes with the owning member and consumers.
4. Implement meaningful C++ behavior and tests. Treat this generated starter as a starting point, not individual project completion evidence.
5. Build, run your suite, then the full suite. Add manual GUI/network evidence when applicable.
6. Open a PR using the template. Request an owning-member review and a consumer review for interface changes. Record the actual review; do not claim unperformed tests.
7. Link the PR, Jira task and Confluence diary. Repository maintainers decide merge rules and branch protection.

## Conventions

Use `PascalCase` types, `snake_case` functions/variables and `engine`/`elemental_coop` namespaces. Headers use `#pragma once`; include everything they require. Prefer RAII and values; use `unique_ptr` for exclusive dynamic ownership and `shared_ptr` only for justified shared lifetime such as asset handles. Borrowed views never imply ownership.

Engine code must not depend on game code. Avoid unapproved libraries, OS-specific assumptions and global mutable services. Main-thread-only is the baseline until explicitly revised. Use exceptions for invalid input/failed required loads in this starter, and explicit empty/false values for ordinary absence/backpressure; document each public API's behavior.

Each member writes their own unit tests and contributes integration tests with neighboring systems. Failures discovered during development get Jira bug reports with reproduction details and regression coverage. GitHub issue templates supplement, rather than replace, required Jira tracking.

The proposed GitHub Actions workflow runs Debug builds on three platforms after push. It has not itself been executed locally. Do not add GitHub CODEOWNERS until everyone supplies their actual GitHub username.
