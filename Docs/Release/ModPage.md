<!--
Text for the Better Movement page on ficsit.app (Satisfactory Mod Repository).
- "Short description" field: the line under SHORT DESCRIPTION (at most 128 characters).
- "Full description" field: everything under FULL DESCRIPTION.
- The two transparency fields: the text under NETWORK ACTIVITY TRANSPARENCY and AI USAGE TRANSPARENCY.
Replace the [bracketed] placeholders before publishing.
-->

# SHORT DESCRIPTION

Slide up steeper ramps, keep your momentum, skim across water, and set your own top speeds. Formerly Uphill Slide.

# FULL DESCRIPTION

## Better Movement

Faster, freer movement, formerly **Uphill Slide**. In vanilla Satisfactory a slide dies the moment the ground tilts up
more than about **7.4°**, a slide pulls you down to its own speed no matter how fast you came in, and the parachute
tops out at 15 m/s. Better Movement lets you change all of that: slide up steeper ramps with better Blade Runners, keep
your bhop speed through slides and jumps, make slides speed up, skim across lakes, and pick your own top speeds.

![Sliding up a ramp and off the top](https://raw.githubusercontent.com/benvrakas/BetterMovement/main/Docs/Images/slide-uphill.gif)

![Mid-slide, partway up the ramp](https://raw.githubusercontent.com/benvrakas/BetterMovement/main/Docs/Images/slide-uphill.jpg)

### Features

- **Speed limits** (new in 1.2.0). Set the top speed for sliding, for the speed you keep in the air after a slide
  jump, and for the parachute.
- **Parachute momentum** (new in 1.2.1). Open the parachute while moving fast and you keep that speed, up to your max
  parachute speed, instead of the game braking you to 15 m/s. Open it slowly and it glides as normal.
- **Parachute acceleration** (new in 1.2.2). Hold Shift and a direction and the parachute speeds up like a glider,
  up to your max parachute speed. Release Shift to hold your speed, or let go of the movement keys to slow down.
- **Water skimming** (new in 1.2.0). Go fast enough (25 m/s by default) and you slide or run across water like a
  skipping stone, and can slide jump off it. Even a big drop lands on the water if you're fast enough. Slow down
  and you drop in and swim as usual.
- **Max uphill angle, per Blade Runners tier.** The steepest slope you can keep sliding up, from 0° to 89°.
- **Speed lost while sliding, per tier.** 100% is vanilla, 50% keeps your speed twice as long, 0% never slows down,
  on flat ground or uphill. Steep downhill slopes still speed you up as usual.
- **Speed up as you slide.** Set speed lost below 0% and a slide gets faster the longer it lasts: -50% adds half your
  starting slide speed every second, -100% adds all of it, up to your max slide speed.
- **Slides keep your momentum.** A slide keeps the speed you go into it with, for example from bhopping, instead of
  being pulled down to the normal slide speed. Jumping out of a slide keeps your speed in the air too, up to your max
  air speed, instead of the game braking you and cutting the jump to 33 m/s.
- **Optional [Mk+ Blade Runners](https://ficsit.app/mod/bbladerunners) support.** Better Movement works on its own with
  vanilla Blade Runners. If you also have Mk+ Blade Runners, Mk.2, Mk.3, Mk.4 and Mk.5 each get their own settings,
  and their reduced and no fall damage versions count as the same tier.
- **Multiplayer.** Everyone moves by the host's settings, so there's no rubber-banding.

### Default settings

| Speed limit | Default |
|---|---|
| Max slide speed | 100 m/s |
| Max air speed | 100 m/s (vanilla cuts slide jumps to 33) |
| Max parachute speed | 100 m/s (vanilla brakes you to 15) |
| Water skim speed | 25 m/s (0 turns it off) |
| Parachute acceleration | 1.0 (0 turns it off) |

| Wearing | Max uphill angle | Enough for |
|---|---|---|
| No Blade Runners | 7.4° (vanilla) | 1 m ramps |
| Blade Runners | 15° | 2 m ramps |
| Mk.2 Blade Runners | 20° | |
| Mk.3 Blade Runners | 27° | 4 m ramps |
| Mk.4 Blade Runners | 35° | |
| Mk.5 Blade Runners | 40° | |

Speed lost while sliding starts at 100% (vanilla) for every tier.

Ramps on an 8 m foundation: 1 m is 7.1°, 2 m is 14.0°, 4 m is 26.6°. Set a tier a little above the ramp you want
to slide up.

### Settings

Open **Esc → Mods → Better Movement**, or the mod's config in Satisfactory Mod Manager. Speed limits and the water
skim speed come first, then one section per Blade Runners tier. Changes apply straight away, no restart needed.

Blade Runners added by other mods (not Mk+ Blade Runners) use the **Blade Runners** settings.

### Coming from Uphill Slide

Better Movement is the same mod under a new name. Remove Uphill Slide when you install it (they change the same
things, so don't run both), and set your settings again: they start from the defaults.

### Multiplayer

- Everyone in the session needs Better Movement.
- The host's settings apply to every player. On a dedicated server, the server's config is used. Your own settings
  only matter in games you host.
- Works on Windows and Linux dedicated servers.

### Known limits

- A slide still needs speed. A speed loss of 0% or more only changes how fast the slide runs out; it doesn't push you
  up a slope you reached too slowly.
- At 0% or below a slide never slows down on its own, so it can keep going until you stop it or run into something.
  Running into something still slows you down; a negative speed loss only speeds you back up gradually.
- The parachute falls at about the same rate whatever your speed, so how far you glide per metre of height changes
  with how fast you're going.
- Skimming needs speed from somewhere: at the default 100% speed loss a slide soon drops below 25 m/s, so you'll only
  skim a short way. A lower speed loss, or a lower skim speed, takes you further.
- Landing on water from a fall, your fall is slowed at the last moment so it does no fall damage. Hold crouch as
  you land to keep sliding; land without it and you slow down and drop in.
- At the edge of the water you can skim up to about 5 m past it before you drop.
- Skimming sounds like sliding on the ground; there are no splash effects.
- In some parts of the ocean the drawn water sits higher than the game's real water surface, so you can skim a little
  below the waves you see. You skim on the same surface the game uses for swimming.
- Very steep settings (past about 45°) may do nothing more, because the game doesn't let you stand on slopes that
  steep.

### Bugs and ideas

Report them on [GitHub](https://github.com/benvrakas/BetterMovement/issues). The source code is there too.

# NETWORK ACTIVITY TRANSPARENCY

Better Movement doesn't connect to the internet. In multiplayer, the host sends its slide settings to players through
the game's own connection. There's no telemetry or analytics.

# AI USAGE TRANSPARENCY

[Edit to match how you made the mod.]

This mod was built with the help of an AI coding assistant (Anthropic's Claude):

- **Source code:** the mod's C++ source code was written by the AI, directed, reviewed and tested in game by the
  author.
- **Mod page text:** this description was drafted by the AI and edited by the author.
- **Developer documentation** in the source folder was written by the AI.
- **Art and audio:** the mod adds no art or audio. It uses the game's own settings menu.
- **In game:** the mod doesn't use or give access to generative AI.
