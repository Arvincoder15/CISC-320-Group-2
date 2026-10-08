# Shared AI development context

Copy this document and links to the live contracts into Confluence. Keep it synchronized through reviewed PRs.

## Facts to give an assistant

We are CISC 320 Group 2, building a reusable C++ 2D multiplayer engine and a separate Flappy-inspired demo. Read README, docs/TEAM.md, docs/ARCHITECTURE.md, docs/DECISIONS.md, the owning member handoff and existing code before editing. Do not assume pending decisions are final.

Starter baseline: C++20, CMake >=3.20, no external runtime dependencies, CTest with a temporary minimal runner. The notes proposed C++26 and intended SDL3/JSON/ImGui/Tiled; neither full C++26 compatibility nor those integrations are established here. Rendering and networking implementations are test doubles. There is no real multiplayer or graphical application yet.

## Reusable prompt

> I own [subsystem] and am working on Jira [key]. Implement [bounded behavior] using the repository's existing contracts. First explain the design and consumers. Keep engine code game-independent, make resource ownership explicit, add meaningful tests, and document any public interface changes. Identify proposals or missing dependencies instead of pretending they are complete. Do not change another owner's contract or introduce a dependency without flagging it for review. Report exact test results and remaining limitations.

## Review responsibilities

The student owning the change must understand and verify generated code, check compatibility and licensing, and follow course policy for AI use/attribution. Never fabricate Jira activity, time spent, meeting attendance, test outcomes or individual contributions. Keep secrets and credentials out of prompts/repository files. This shared bootstrap is generated assistance; meaningful individual implementation remains each owner's work.
