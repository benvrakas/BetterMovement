# Debugging

Everything logs to **`LogBetterMovement`** in `%LOCALAPPDATA%\FactoryGame\Saved\Logs\FactoryGame.log`:

```powershell
Select-String LogBetterMovement "$env:LOCALAPPDATA\FactoryGame\Saved\Logs\FactoryGame.log"
```

## Expected sequence

```
Better Movement module starting
Config: 22 of 22 settings/sections use SML's editor widgets
Hook installed: UFGCharacterMovementComponent::GetMaxSpeed
Hook installed: UFGCharacterMovementComponent::TickSlide
Slide subsystem started (server)                       (or "client" when joining someone)
Speed limits: slide X m/s, air Y m/s, parachute Z m/s      (server only, and again after each config change)
Settings: <tier>: uphill up to X degrees, speed loss Y%      (x6; server only, and again after each config change)
Tracking <player> (server): vanilla max slide angle 1.7000 rad (7.40 degrees uphill), slope curve /Game/...
Vanilla slope curve keys (slope angle rad, slide time rate): (t, v) ...
Blade Runners class /Game/FactoryGame/Equipment/JumpingStilts/Equip_JumpingStilts.Equip_JumpingStilts_C uses the Blade Runners settings
<player> (server) now uses Blade Runners: uphill up to 15.00 degrees (max slide angle 1.8326 rad), speed loss 100%
<player>: boost jump limit 3333 -> 10000 cm/s (vanilla 3333)      (follows Max air speed)
```

## Symptom → first thing to check

| Symptom | Look for |
|---|---|
| No `LogBetterMovement` lines | Module didn't load; check SML's mod list in the log |
| Settings never logged | `Config: settings for ... missing` or no `Settings:` lines: the config didn't register |
| Client: `Waiting for the host's settings` and nothing after | Host doesn't have the mod, or the subsystem didn't replicate |
| Vanilla angle isn't 1.70 rad | The game changed the default; the mod still works, but the "vanilla is about 7.4" text is wrong |
| Wrong tier for Mk+ Blade Runners | The `Blade Runners class ... uses ...` line shows the class path; the naming rule is in `GetTier` |
| Still stops at ~7.4 degrees | Is `now uses ...` logged for that player on **both** server and own client? |
| Rubber-banding on slopes (multiplayer) | Server and client applied different values; compare their `now uses` lines |
| Bhop speed still lost when sliding | With `Log LogBetterMovement Verbose`, each slide logs `Slide started at N cm/s (vanilla slide speed M, boost B%/s)` |
| Speed lost in the air after a slide jump | Verbose `Left the ground from a slide at N cm/s`; the `boost jump limit` line shows the jump limit (the max air speed) |
| Parachute speed unchanged | Is `Max parachute speed` set in the Speed limits section? The hook scales the game's strafe speeds; nothing is logged per deploy |
| Negative speed loss doesn't speed up | Was `Hook installed: ...TickSlide` logged? The Verbose `Slide started` line shows the boost rate it used |
| Slide dies fast | Lower "Speed lost while sliding"; the `Built slope curve` line shows the resulting rate at a few slopes |
