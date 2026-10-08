# Cooperative elemental puzzle platformer

The user corrected the demonstration game on October 8, 2026: build a Fireboy-and-Watergirl-inspired cooperative puzzle platformer using our reusable 2D engine. `elemental_coop` is a neutral working module name, not a finalized product title. Both characters are human-controlled. Use original or licensed art, audio and room layouts.

## Proposed first playable slice

Two players navigate a single room together. Each can walk and jump; fire and water characters have different hazard tolerances. One player holds a pressure plate to open a door for the other. Both must reach their matching exits to complete the room. Either character's death fails the shared attempt; restart resets both characters and puzzle objects.

| Mechanic | Headless starter | Work remaining |
|---|---|---|
| Two player roles | Two distinct entities, fire slot 0 / water slot 1 | Character assignment, visible art and animation |
| Walking / jump | Independent action frames; ground-only jump | Keyboard/controller adapters, solid platform collision |
| Ground | A single flat floor constrains character feet | Tilemap floors, walls, ceilings and player hitboxes |
| Elemental hazards | Fire survives lava; water survives water; opposite element and toxic hazards cause shared loss | Real hazard colliders and contact dispatch |
| Cooperative exits | Both matching contacts must be reported in the same simulation step | Exit triggers, visual feedback and next-level transition |
| Restart | Both characters reset in place without extra entities | Reset switches, doors, collectibles and network round generation |
| Pressure plates / doors | Proposed level schema only | Linked trigger logic, collision changes and synchronization |
| Multiplayer | Two local action streams; scripted demo | Live local input and real host/client connection |
| Optional AI | Generic patrol direction state machine | An agreed moving hazard/NPC adapter, if useful |

These detailed rules are starter proposals for team review, not a claim that every mechanic or rule is finalized.

## Proposed controls and UI

| Action | Fire character | Water character |
|---|---|---|
| Move left / right | A / D | Left / Right arrow |
| Jump | W | Up arrow |

Controls are a proposal; no keyboard mapping exists in the executable yet. Keep bindings configurable, isolate each player's buffer and clear held movement on focus loss. Local co-op uses both sets. In network mode each client controls its assigned slot; the host validates ownership.

Proposed navigation: title → local co-op or host/join → character assignment → room → shared failure/restart or level complete. Pause behavior in network play requires an explicit team decision. Rohan owns game UI; Gunveer's ImGui tools are separate developer UI.

## Level data boundary

See `assets/elemental_coop/levels/room.example.json`. It sketches two spawns, solid rectangles, hazard rectangles, a linked plate/door and matching exits. It is a custom proposed schema, not Tiled's exported format, and the current code does not load it. Gunveer should map a reviewed Tiled object-layer subset into a neutral engine model; Blake interprets game-specific prefab and property values.

Validation must reject unsupported schema versions, duplicate object IDs, missing/duplicate player roles, non-finite coordinates, invalid sizes and links to nonexistent doors. Define whether character positions are feet or top-left sprite coordinates before wiring draw/hitbox adapters; the current Game uses feet.

## Network scope

Henry's initial target is two processes controlling one character each in the same room, with host-authoritative movement and puzzle state proposed. Transport choice remains pending. Synchronize character positions/velocities, door/plate state, deaths, exit readiness, level identity and round generation. Reject wrong-slot inputs and stale packets after restart. Agree disconnect behavior and coordinated restart with Blake/Rohan.

The loopback queue is a test double, not multiplayer networking. Local co-op already supplies two player roles, but it does not finish Henry's networking responsibility. Prototype the real connection alongside local room development.

## First team integration

1. Render two distinct characters and one shared room.
2. Connect each local control set to its own action buffer.
3. Replace the flat floor with solid platform collisions and grounded detection.
4. Connect elemental hazards and both exits to game rules.
5. Add one linked plate/door puzzle and complete shared restart.
6. Demonstrate the same room across two connected processes.

Stretch features: multiple rooms, collectibles, moving platforms, patrol hazards and saved level progress. Leave matchmaking, complex prediction and large content sets outside the first slice.
