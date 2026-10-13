# Better Movement

Faster, freer movement for Satisfactory, formerly **Uphill Slide**. Slide up steeper slopes, keep your momentum through
slides and jumps, make slides speed up, skim across water, and set your top speeds for sliding, the air and the parachute.

## Settings

Open **Mods → Better Movement** (in game or in Satisfactory Mod Manager).

**Speed limits** (for every player, in m/s):

- **Max slide speed** (default 100): the fastest a slide can go, including speed kept from bhopping and speed gained
  from a negative speed loss.
- **Max air speed** (default 100): the fastest speed you keep in the air after jumping out of a slide. Also the limit
  on jumps out of a slide, which vanilla sets to 33.
- **Max parachute speed** (default 100): the fastest speed you keep when you open the parachute while moving fast.
  Opened slowly, the parachute glides at its normal speed (10, or 15 holding Shift).
- **Parachute acceleration** (default 1.0, 0 turns it off): while you hold Shift and a direction, the parachute
  speeds up like a glider, gaining this share of gravity along your glide path, up to the max parachute speed.
  Without Shift it holds its speed.
- **Water skim speed** (default 25, 0 turns it off): moving at least this fast, you slide or run across water instead
  of going in. Slow down and you drop in.

**Each Blade Runners tier** has:

- **Max uphill angle (degrees)**: the steepest slope you can keep sliding up (0 to 89).
- **Speed lost while sliding (%)**: how quickly a slide slows down, on flat ground and uphill. 100 is
  vanilla, 50 lasts twice as long, 0 never slows down. Steep downhill slopes still speed you up as usual.
  Below 0 the slide speeds up instead: -50 adds half your starting slide speed every second, up to the max slide speed.

Slides always keep your momentum: a slide starts from the speed you go into it with (for example from bhopping), and a
jump out of a slide keeps that speed in the air until you land.

| Tier | Default angle |
|---|---|
| No Blade Runners | 7.4° (vanilla; enough for 1 m ramps) |
| Blade Runners | 15° (2 m ramps) |
| Mk.2 Blade Runners | 20° |
| Mk.3 Blade Runners | 27° (4 m ramps) |
| Mk.4 Blade Runners | 35° |
| Mk.5 Blade Runners | 40° |

Ramps on an 8 m foundation: 1 m is 7.1°, 2 m is 14.0°, 4 m is 26.6°.

## Mk+ Blade Runners (optional)

Better Movement doesn't need any other mod. With [Mk+ Blade Runners](https://ficsit.app/mod/bbladerunners) installed,
each Mk.2 to Mk.5 tier uses its own settings, and the reduced and no fall damage versions count as their tier. Without
it, those settings do nothing. Blade Runners from other mods use the Blade Runners settings.

## Multiplayer

Everyone needs the mod. The host's settings (or the dedicated server's) apply to every player.
