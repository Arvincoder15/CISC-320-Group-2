# Testing

From the repository root, configure and build the [SDL application](../README.md#build-and-run), then run:

```sh
ctest --test-dir build-ui -C Debug --output-on-failure
```

| Area | Coverage |
| --- | --- |
| Engine | ECS lifecycle, physics and contact events, rendering, input, AI, level tools, file caching, configuration, gameplay, and subsystem integration |
| Networking | ENet loopback sockets, protocol validation, simulated delay and loss, campaign synchronization, room creation and joining, and disconnect handling |
| Campaign and UI | Normal-input walkthroughs for the three levels, gate and hazard rules, enemy movement, menu navigation, settings persistence, and level inspection |
| SDL rendering | Drawing, asset handling, and rendering demo smoke test |

The [core workflow](../.github/workflows/ci.yml) and [SDL rendering workflow](../.github/workflows/rendering.yml) provide CI coverage. Socket tests use loopback. The SDL smoke tests use dummy video and audio drivers.

## Manual checks

- Launch the game on a desktop. Move both players, complete a gate puzzle, and check pause and retry.
- Toggle the music with **M** and the corner icon. Change master volume and sound effects in Settings. Apply the settings, then relaunch to check persistence.
- Start a two-player session. Confirm that each player controls their own character, both see a gate open, and leaving ends the session. To verify relay reachability, repeat with two computers on different networks using the shared endpoint.
