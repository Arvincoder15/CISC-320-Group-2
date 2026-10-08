# Test strategy

Ten CTest suites exist: architecture, rendering, physics, networking, ai, tools, interaction, infrastructure, gameplay and integration. The starter runner uses exceptions and returns nonzero on failure, including Release builds. Each developer owns their suite; Arvin maintains shared setup.

The unit checks exercise ID deletion/non-reuse, copied render commands, motion/overlap boundaries, bounded loopback ordering, patrol-state transitions, level validation, independent held axes and one-shot actions, cache reuse/lifetime/failure, and two-player movement, grounded jumping, elemental hazards, shared loss/restart and joint exits. The integration check follows two independent inputs → gameplay → physics → world → two draw commands → two loopback snapshots. These checks do not establish SDL rendering, real socket behavior or full game correctness.

Next tests by integration stage:

1. SDL window creation, bad texture load and correct resource teardown (platform-dependent integration test).
2. Valid/invalid JSON configuration, supported Tiled subset, safe failed level reload.
3. Elemental hazards, joint exit completion, linked pressure plates/doors and repeated shared restart without stale objects.
4. Two processes connecting, state agreement, malformed packet rejection, ordering and disconnect handling.
5. Manual menus, input focus, audio levels, tool input capture and cross-platform assets.

Record compiler/OS, commit, commands and outcomes with each test report. Log reproducible defects in Jira and attach regression cases. Use CI for headless checks; add explicit environment-specific jobs when graphical/network integrations arrive. See `VALIDATION.md` for checks actually run on the starter.
