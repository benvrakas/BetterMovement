<!-- Paste into the Changelog field when uploading 1.3.1 to ficsit.app (published there: 1.3.0). -->

- Skimming holds up on laggy or busy servers. With high ping or a slow server, players often fell into the water
  mid-skim. Near water, the server now takes your position as your game reports it, and while your updates are late it
  waits for them instead of moving you itself.
- If you still drop in, the game log now says whether the server corrected your position (look for
  `LogBetterMovement`). Please include it in a report.
- Everyone in a multiplayer session needs 1.3.1. Near water the server doesn't check movement for cheating; setting the
  water skim speed to 0 turns this off.
