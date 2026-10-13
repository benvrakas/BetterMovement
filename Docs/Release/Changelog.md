# Changelog

Versions before 1.2.0 were released as **Uphill Slide**.

## 1.2.4

- **The water skim platform reaches further ahead.** It now covers about a second of travel in front of you (up to
  150 m) instead of a few metres, so players joining a host over the internet should drop into the water or
  rubber-band less often while skimming.
- **A mod icon**, shown in the Mods menu and on the mod page.

**Good to know**

- Everyone in a multiplayer session needs Better Movement 1.2.4.

## 1.2.3

- **Shift speeds up the parachute.** Parachute acceleration now only applies while you hold Shift and a direction.
  Release Shift and you keep the speed you have; let go of the movement keys to slow down.
- **Parachute acceleration** now defaults to 1.0 (was 0.3).
- **Water skim speed** now defaults to 25 m/s (was 50).

**Good to know**

- Everyone in a multiplayer session needs Better Movement 1.2.3.
- Updating from 1.2.2: your parachute acceleration and water skim speed stay at whatever you had saved (0.3 and
  50 unless you changed them). Set them to 1.0 and 25 for the new defaults.

## 1.2.2

- **Parachute acceleration** (new setting, default 0.3): the parachute speeds up like a glider while you hold a
  direction, turning part of your fall into forward speed. You pick up speed quickly from slow and less as you get
  faster, up to the max parachute speed. Let go of the movement keys to slow down. 0 turns it off (vanilla).

**Good to know**

- Everyone in a multiplayer session needs Better Movement 1.2.2.

## 1.2.1

- **The parachute keeps your momentum.** Open it while moving fast and you keep that speed, up to the max parachute
  speed, instead of the game braking you to its 15 m/s. Open it slowly and it glides at its normal speed again (in
  1.2.0 it always flew at the max parachute speed).
- **Max parachute speed** now defaults to 100 m/s, the same as the max air speed. Set it below 15 to slow the
  parachute down.
- **Landing on water from a big drop works.** Falling onto water at skim speed now always lands you on it, and the
  landing is softened so it does no fall damage. In 1.2.0, falls faster than 20 m/s went into the water instead.

**Good to know**

- Everyone in a multiplayer session needs Better Movement 1.2.1.
- Updating from 1.2.0: your max parachute speed stays at whatever you had saved (15 unless you changed it), which
  keeps no extra speed. Raise it to keep your momentum on the parachute.

## 1.2.0 — Better Movement

- **Uphill Slide is now Better Movement.** It does more than uphill slides now, so it has a new name (and a new mod
  ID). Remove Uphill Slide when you install it; don't run both.
- **Speed limits** (new): set the **max slide speed**, the **max air speed** you keep after jumping out of a slide
  (also the limit on slide jumps), and the **max parachute speed**. The parachute's normal and Shift speeds both
  scale with its limit, so you can glide faster (or slower) than vanilla's 15 m/s.
- The slide and air limits replace the fixed 100 m/s cap from 1.1.
- **Water skimming** (new): move at least the **water skim speed** (default 50 m/s) and you slide or run across water
  instead of going in. Slow down and you drop in and swim as usual. Set it to 0 to turn skimming off.

**Good to know**

- Everyone in a multiplayer session needs Better Movement 1.2.0.
- Settings start from the defaults, since the mod has a new ID: set your angles and speed loss again.

## 1.1.1

- **Slides always keep your momentum.** The "Keep your entry speed" setting is gone: every slide now keeps the speed
  you go into it with, for example from bhopping, then fades at your **Speed lost while sliding** rate.
- **Jumping out of a fast slide keeps your speed in the air.** The game used to brake you back down to normal running
  speed mid-air, and cut any jump out of a slide to 33 m/s. You now keep your takeoff speed until you land, and the
  jump limit is 100 m/s, the same cap as sliding.

**Good to know**

- Everyone in a multiplayer session needs 1.1.1.
- Chained slide jumps (each one adds the game's usual 30% boost) can now build up to 100 m/s.

## 1.1.0

- **Keep your entry speed** (new setting for each tier, off by default): a slide keeps the speed you go into it with,
  for example from bhopping, instead of being pulled down to the normal slide speed. That speed then fades at your
  **Speed lost while sliding** rate, so at 0% you keep all of it.
- **Speed up as you slide.** Speed lost while sliding now goes below 0%: a slide then gets faster the longer it lasts.
  -50% adds half your starting slide speed every second, -100% adds all of it, up to 100 m/s. With Keep your entry
  speed on, it builds from the speed you slid in with.

**Good to know**

- Everyone in a multiplayer session needs 1.1.0.

## 1.0.0

First release.

- **Slide up steeper slopes.** Set the steepest uphill slope you can keep sliding up, from 0° to 89°. Vanilla stops
  you at about 7.4°.
- **Speed lost while sliding.** Set how quickly a slide loses speed on flat ground and uphill: 100% is vanilla, 0%
  never slows down. Steep downhill slopes still speed you up as usual.
- **Separate settings for each tier:** no Blade Runners, Blade Runners, and Mk.2 to Mk.5 Blade Runners from the
  [Mk+ Blade Runners](https://ficsit.app/mod/bbladerunners) mod (every fall damage variant counts as its tier).
  Mk+ Blade Runners is optional; Better Movement works on its own.
- Default slide limits: 7.4° without Blade Runners (vanilla), 15° with Blade Runners, then 20°, 27°, 35° and 40° for
  Mk.2 to Mk.5.
- Settings are in **Esc → Mods → Better Movement** and in Satisfactory Mod Manager, and apply without a restart.

**Good to know**

- Everyone in a multiplayer session needs Better Movement. The host's settings (or the dedicated server's) apply to
  every player.
- Works on Windows and Linux dedicated servers.
