# Architecture

## How vanilla sliding decides "too steep"

Found by disassembling `FactoryGameSteam-FactoryGame-Win64-Shipping.dll` (game CL 502094). The starter
project's `.cpp` files are stubs, so none of this is visible in source.

`UFGCharacterMovementComponent::CanSlide()` and `CanStartSlide()` both do:

```
angle = acos( dot( Velocity.GetSafeNormal(), GravityDirection ) )   // radians; flat = pi/2, uphill > pi/2
if angle > mMaxSlideAngle: no slide
```

- `mMaxSlideAngle` (offset `0x15BC`) is 1.65 rad in the C++ constructor, but the player Blueprint overrides it to
  **1.70 rad**: pi/2 + 0.129 rad, about **7.40 degrees uphill** (read in-game from the log). Downhill angles are always
  under pi/2, so they always pass.
- `CanSlide` also ends the slide once the horizontal speed of `mBaseVelocity` drops to `MaxWalkSpeedCrouched`.
  `CanStartSlide` needs more than 1.1 x `MaxWalkSpeed`.

`TickSlide(delta)` does:

```
mSlideTime = max(0, mSlideTime + mSlopeCurve->GetFloatValue(angle) * delta)   // same angle as above, radians
```

and `GetMaxSpeed()`, while sliding, uses `mSlideCurve->GetFloatValue(mSlideTime)` times the sprint speed
(including the Blade Runners' `GetAdjustedMaxSpeed`). So `mSlopeCurve` sets how fast a slide runs out at each
slope. The slide timer runs backwards on steep downhill slopes, which is why they keep a slide going.

The vanilla curve is `/Game/FactoryGame/Character/Player/SlopeCurveMultiplier` with keys (angle rad, rate):
`(0.934, -0.606) (1.344, 0.089) (1.488, 0.857) (1.570, 1.000)`. Nothing is keyed above flat, so every uphill angle
holds the flat rate of 1.0. Uphill slides slow down exactly as fast as flat ones; they only end sooner because of the
angle limit.

While sliding, `CalcVelocity` sets the speed to `GetMaxSpeed()` along the current direction, and `GetMaxSpeed` eases
the current speed toward `mSlideCurve(mSlideTime) x sprint speed`. Nothing else in the slide code depends on slope.

## What the mod changes

Per player, on the server and on that player's own game:

- `mMaxSlideAngle` = pi/2 + the tier's "steepest uphill slide" in radians.
- `mSlopeCurve` = the vanilla curve, or a per-factor copy (`GetScaledSlopeCurve`) that samples the vanilla curve
  every half degree from 0 to pi as linear keys. Positive rates (the slide timer advancing, so the slide losing speed)
  are multiplied by the tier's "speed lost while sliding" factor; negative rates (steep downhill, the timer running
  backwards so the slide regains speed) are kept, so downhill behaves as vanilla. At 0% the timer never advances and
  the slide keeps its speed until the player stops it.

Both fields are private `UPROPERTY`s, reached through the `Accessor` access transformers in
`Config/AccessTransformers.ini`.

## Slide momentum

While sliding, `GetMaxSpeed` returns `lerp(current speed, slide target, a)` with `a = min(<character float at
0x85C>, 1)`, and the slide target is `mSlideCurve(mSlideTime) x sprint speed`. `CalcVelocity` first runs the engine's
walking velocity (which brakes anything over `GetMaxSpeed` back down), then, once `mSlideTime > 0`, sets the speed to
exactly `GetMaxSpeed()`. So a player who lands a bhop faster than the slide target is pulled down to it, even with the
slide timer frozen at 0%.

`FBMMomentum` hooks `GetMaxSpeed` (SML detours the function body, so `CalcVelocity`'s direct call is caught
too). For every tracked player it records the horizontal speed and the slide curve's value on the first call of each
slide, then returns

```
max(vanilla, min(current speed, entry speed x slideCurve(now) / slideCurve(at entry)))
```

The entry speed fades by the same curve as the normal slide speed (so "speed lost" still applies), the player is
never sped up past their current speed, and a slide slower than vanilla's is left alone. `mSlideTime` isn't a
`UPROPERTY`, so the class is a `Friend` of `UFGCharacterMovementComponent` (access transformer). The hook runs on
the server and on each player's own game, like the other settings. (1.1.0 briefly had this behind a per-tier toggle;
it is now always on.)

### In the air

The game doesn't override falling physics. The engine's `PhysFalling` runs `CalcVelocity` on the horizontal velocity,
which brakes anything over `GetMaxSpeed()` (with `FallingLateralFriction` / `BrakingDecelerationFalling`), so a fast
slide jump used to lose speed in the air. When a player goes from sliding to falling, the hook records their
horizontal takeoff speed and, until they land, returns `max(vanilla, min(horizontal speed, takeoff speed))`: no
braking toward the normal limit, and no speed gained in the air. Landing clears it (or starts a new slide, which keeps
its entry speed).

### Boost jump limit

`AFGCharacterPlayer::OnJumped`: if the jump comes within `mBoostJumpTimeWindow` (0x1600) of the last slide
(`mLastSlideTime`, 0x15FC), the horizontal velocity is multiplied by `mBoostJumpVelocityMultiplier` (0x15F4,
default 1.3), and if the resulting speed is over `mBoostJumpVelocityLimit` (0x15F8, default 3333 cm/s) it is clamped
to it. `SetSettings` sets that limit per player to the **max air speed** setting and `Reset` restores it, so fast
slides aren't cut to 33 m/s when jumping. Chained boost jumps can therefore climb to the max air speed.

## Negative speed loss (speed up)

The slide timer can't go below 0 (`TickSlide` clamps it), and the slide curve is highest at 0, so a negative slope
curve alone would behave like 0%. Instead a negative percentage freezes the timer like 0% (the scaled slope curve
uses `max(factor, 0)`) and `FBMMomentum` adds the speed-up:

- A `TickSlide` hook (after) accumulates the slide's seconds from the movement's own `delta`, so server and client
  count the same time, and raises the velocity toward `base x (1 + boost x seconds)`, capped at the **max slide
  speed** setting. Each tick adds at most one tick's worth (`base x boost x delta`), so a wall still stops the slide
  instead of being pushed through. `base` is the vanilla slide speed at the start of the slide, or the entry speed if
  that is higher. `boost` is `-percent / 100`.
- The `GetMaxSpeed` hook returns at least `min(current speed, boosted speed)`, so the engine doesn't brake the
  gained speed back down.

## Why polling instead of hooks

`ABMMovementSubsystem` ticks every 0.1 s and sets the values only when they differ. The alternative, hooking
`AFGJumpingStilts::Equip/UnEquip`, misses respawns, joins, config edits and equipment that a Blueprint subclass
equips differently. Polling handles all of those, and the work is a few comparisons per player.

## Multiplayer

Character movement is simulated on the owning client and re-run on the server. If the two disagree on
`mMaxSlideAngle`, the server corrects the client and the player rubber-bands. So:

- Only the server reads the configuration (`RefreshSettingsFromConfig`, every second) and replicates
  `TierSettings`. Clients never use their own config values.
- Each machine applies those settings to the characters it simulates: the server to all of them, and a client
  only to its own (`HasAuthority() || IsLocallyControlled()`).
- Until the settings arrive, a client leaves slides vanilla.

## Tiers

`GetTier` looks for an `AFGJumpingStilts` among the character's active equipment:

- none: `EBMTier::None`
- class path under `/bbladerunners/` whose name starts with `Mk2`..`Mk5`: that tier. Mk+ Blade Runners
  (Blueprint-only mod, `bbladerunners` 1.2.0) has `Mk2_stilts`, `Mk2Reduced_stilts`, `Mk2NoDamage_stilts`,
  `Mk3_stilts`, `Mk3NoDamage_stilts`, `Mk4_stilts` and `Mk5_stilts` under `/bbladerunners/items/...`. The fall damage variants share their tier.
- anything else, including vanilla `Equip_JumpingStilts` and other mods' Blade Runners: `EBMTier::Mk1`.

Mk+ Blade Runners is an optional plugin dependency (`"Optional": true` in the `.uplugin`). Nothing links against it;
the tier comes from the class name, so the mod works the same with or without it.

## Speed limits

`FBMSpeedLimits` (m/s, replicated from the host like the tier settings) caps:

- **Slides**: `AdjustMaxSpeed` returns at most the max slide speed while sliding, and the negative-speed-loss boost
  stops there. Set below vanilla's slide speed (about 10 m/s at the start of a slide), it slows vanilla slides too.
- **The air after a slide**: the kept takeoff speed is capped at the max air speed, which is also written to
  `mBoostJumpVelocityLimit`.
- **The parachute**: while parachuting (`CustomMovementMode` 6), the game's `GetMaxSpeed` returns the parachute
  settings' `mShiftStrafeMaxSpeed` (`+0x40`, 1500 cm/s) when sprinting and `mStrafeMaxSpeed` (`+0x3C`, 1000 cm/s)
  otherwise, from `mParachuteSettings` (`0x1010` on the movement component; `PhysParachute` runs `CalcVelocity`, which
  brakes anything over that). Like the air after a slide, the hook records the horizontal speed when the parachute
  opens and returns `min(max(vanilla, min(current speed, entry speed)), max parachute speed)`: a fast opening keeps
  its speed (no braking, no gain), a slow one glides as vanilla, and a limit below vanilla slows the parachute.
  (1.2.0 scaled vanilla's speeds by the limit instead, so every glide ran at the
  limit.)

## Parachute acceleration

`UFGCharacterMovementComponent::PhysParachute` (RVA `0x6c7a80`), per iteration:

1. Zeroes `Velocity.Z`, runs `CalcVelocity(dt, PhysicsVolume->FluidFriction, false, mBrakingDeceleration)` on the
   horizontal velocity, and puts `Velocity.Z` back.
2. If the flag at `0xff8` is set and the horizontal speed is over 1e-4, subtracts
   `direction x speed x mBrakingExponent x dt` (the "slow down" input).
3. Adds `GetGravityZ() x dt` to `Velocity.Z`.
4. Caps the fall speed (the component along gravity) at
   `min(|TerminalVelocity| x mTerminalVelocityMultiplier + horizontal speed x mLateralToTerminalConversionRatio,
   |TerminalVelocity|)`. The settings are `0x34` and `0x38` in `UFGParachuteSettings` (the header defaults are 0.33).

In game the fall speed barely changes with horizontal speed (glide slopes vary with speed), so the shipped parachute
asset probably sets `mLateralToTerminalConversionRatio` near 0; the asset values haven't been read. Nothing in the
parachute speeds you up. `FBMMomentum::TickGlide` runs from an `UpdateCharacterStateBeforeMovement` hook (once per move, on the owning
client, in its replays and in the server's re-run, like the water skim hook). While parachuting with movement input
and sprinting (`GetIsSprinting()`, `mIsSprinting` at `0x1018`: the flag vanilla's `GetMaxSpeed` uses for the Shift
strafe speed, sent to the server in the saved move's compressed flags), it adds `k x |g| x sin(glide angle) x dt` to the horizontal speed along the current horizontal velocity, or along the
input for a near standstill. `sin(glide angle) = fall speed / sqrt(horizontal^2 + fall^2)` is gravity's share along
the flight path, as for a real glider, and `k` is the **parachute acceleration** setting: the fraction kept after
drag. The gain stops at the max parachute speed. The new speed is recorded as the parachute's kept speed, so the
`GetMaxSpeed` hook doesn't let `CalcVelocity` brake it back down, so releasing Shift holds the speed. Without input,
nothing is added and the vanilla braking slows the parachute. (Step 2's flag at `0xff8` is probably `mIsPressingJump`,
from the header's field order: vanilla's "slow down" is likely holding Jump.)

With a roughly constant fall speed `w`, `sin = w / sqrt(v^2 + w^2)` goes from 1 near a standstill toward `w / v`,
so the gain is `k x g` from slow (9.8 m/s² at the default `k = 1`) and falls off about as `1 / v` at speed.

## Water skimming

Vanilla water is an `AFGWaterVolume` (an `APhysicsVolume` with `bWaterVolume`, brush profile `OverlapAllDynamic`). It
has no blocking collision; `UCharacterMovementComponent::PhysicsVolumeChanged` switches a character to swimming when
their capsule's centre enters it. So a skimming player needs something solid at the surface, and standing on it keeps
the capsule's centre out of the water.

`FBMWaterSkim` hooks `UCharacterMovementComponent::UpdateCharacterStateBeforeMovement` (public, not overridden by
the game), which `PerformMovement` calls once per move, after `MaybeUpdateBasedMovement` and before
`StartNewPhysics`. It runs for every move: the owning client's, its replays after a correction, and the server's
re-run of each `ServerMove`. So both sides place the same floor for the same move, from the same state.

- A player skims while walking or falling with a horizontal speed of at least the host's **water skim speed**
  (`FBMSpeedLimits`, replicated like the other limits).
- Fall damage comes from `AFGCharacterBase::CalculateFallDamage(zSpeed)` on landing. So in the move where a player
  falling faster than 6 m/s would reach the tile under them, their `Velocity.Z` is set to -6 m/s (about a normal
  jump's landing speed), and the landing does no damage. In 1.2.0, falls over 20 m/s skipped the tiles and went
  into the water, which played as "sometimes a fall onto water doesn't slide".
- The world is split into 10 m grid cells. Each cell within 10 m of the player's path for the next
  `max(1 s, move delta)` (along the horizontal velocity, at most 150 m) whose centre has a water surface between 20 m
  below and 0.5 m above the player's feet gets a tile: a `UBoxComponent`, 10.2 m square and 1 m thick, with its top
  5 cm above the surface. Tiles off that path are destroyed, and every tile goes as soon as the player stops
  qualifying. The player then falls in and swims. Until 1.2.4 the tiles only covered `speed x move delta + 10 m`
  around the player; the look-ahead is for remote clients, whose moves the server runs late and sometimes from a
  corrected position, so its tiles have to reach where the client actually is.
- The surface height comes from a vertical `LineTraceComponent` against each water volume's brush whose bounds contain
  the cell centre, falling back to the top of the bounds if the trace misses but the volume encloses that point.
  Heights are cached per cell, and the cache is cleared whenever the number of physics volumes changes, since water
  volumes stream in and out with World Partition.
- Tiles are **Static**, created in place and never moved. A movable tile would be a *dynamic* base: the engine would
  drag the player by its movement, and the client would send `ServerMove` positions relative to a component the
  server can't resolve (tiles aren't replicated). A static base means absolute positions and no based movement.
- Tiles block only the owner's capsule object type (query only, `WorldStatic`), and every other player's capsule gets
  `IgnoreComponentWhenMoving` for them, so on the server one player's tiles don't hold up another swimming player.
  They're owned by the subsystem actor, which exists on the server and every client.
- Skimming at 100% speed loss is brief, because the slide loses speed as on flat ground.
- `AFGWaterVolume::IsOverlapInVolume` counts any overlap as inside only for `AFGVehicle`s; players use the engine's
  test (capsule centre inside the brush). The post-process `EncompassesPoint` override also tests the brush. So the
  brush top is the gameplay surface. In parts of the ocean the rendered water sits above it, and skimmers (like
  swimmers) are then below the visible surface. Tiles deliberately stay on the brush: visible water meshes differ
  between ocean, lakes and rivers, and may have no collision.

## Name

Released as **Uphill Slide** (`UphillSlide`, 1.0.0 to 1.1.1) and renamed to **Better Movement** (`BetterMovement`)
in 1.2.0, when it grew beyond uphill slides. `MovementPlus` was already taken on ficsit.app. The rename changed the
plugin, module, class prefix (`US` to `BM`), log category and config ID, so the old config file isn't read.
