# LEAFWIND — Game Design Document

*A physics-driven jungle platformer about a very small monkey and a very long way home.*

This document is the single source of truth for the implementation. Everything in it is meant
to be followed literally; where a number is given, use that number as the starting value
(tune only if playtesting shows a clear problem, and record the change here).

The ten level maps live in `data/levels/level01.txt` … `level10.txt` (format in §4.1). They were
built with a generator + validator and every map passes the checks described in §8; do not
hand-edit them without re-running an equivalent validation.

Existing assets that MUST be used: `data/apa_*.tga` (64×64 monkey frames: stand L/R,
walk1/walk2 L/R, jump L/R). Everything else (terrain, vines, water, UI, particles, audio) is
generated procedurally at runtime. `groundtile.tga`, `rope.tga`, `level.txt`,
`smb3_mario_sheet.png` are no longer used and may be deleted.

---

## 1. Title & Story

### 1.1 Title

**LEAFWIND**
Tagline on the title screen: *"A little monkey. A big wind. One leaf."*

### 1.2 Cast

* **Pip** — the player. The smallest monkey in the Home Tree and the only one who has never
  swung on a vine ("Heights are for birds"). Curious, stubborn, easily delighted. Uses the existing
  reddish-brown sprites.
* **Nana Fig** — Pip's grandmother. Old, slow, warm, sly. Never seen moving; appears only in story
  cards and as the voice on signposts/the leaf. Raised Pip on her big green leaf.
* **The Big Wind** — the antagonist that turns out not to be one. Never personified visually beyond
  leaf streams and gust audio.

### 1.3 The story (for the team; the player only ever sees the cards below)

Pip sleeps every night on Nana Fig's big green leaf, the softest thing in the jungle. One morning
the Big Wind takes it. Nana Fig says she is too old to chase it — but Pip is not. Pip, who is
terrified of heights, follows the leaf out of the Home Tree for the first time in his life: through
the Vine Grove (learns to let go), across the Gully's sagging bridges, down the river on logs,
through the Mushroom Hollow, over Boulder Ridge, up the Windy Cliffs, the long way round through
the Sighing Swamp, and up through the Old Mill that monkeys built long ago to reach the mountain.

Along the way Pip finds signs that someone made this journey before: monkey-shaped scratches in the
cliff, a carved lily-pad sign reading FIG WAS HERE, and finally a faded painting in the mill of a
young monkey, a golden leaf and a storm. The twist lands on the summit of the Storm Tree: the leaf
is warm, glowing gold, and it sings in Nana Fig's voice — *she let it go on purpose*. She had made
the same journey when she was young, and she knew the only way to teach Pip to climb was to give
him something worth climbing for.

Pip swings home — not the long way round, straight down the cliffs, laughing. Nana Fig is waiting
with two cups of fig tea. That night Pip sleeps on the leaf again, which smells of home and a little
of mountain. (The final image is the concept art: the golden monkey face asleep on the green leaf.)

### 1.4 Story cards — exact text

Cards are shown full-screen between levels (see §7 for flow and §5.9 for the look). Each card is
at most three lines; a line never exceeds ~56 characters so it fits at 800×600 in the 22 px card
font. In the table below ` / ` separates lines; in the level files each line is its own
`card_in:` / `card_out:` header line, in order. Advance with Space/Enter/click; Esc skips to the level.

**Prologue** (after "New Game", before Level 1's own card):

> Pip is the smallest monkey in the Home Tree,
> and the only one who has never swung on a vine.
> "Heights," says Pip, "are for birds."

| # | Level | Intro card | Outro card |
|---|-------|-----------|------------|
| 1 | Home Tree | Every night Pip sleeps on Nana Fig's big green leaf. / It is the softest thing in the jungle, / and it smells of home. | One morning the Big Wind comes. It takes the leaf. / "Pip," says Nana Fig, "I am too old to chase it. / But you are not." |
| 2 | Vine Grove | The leaf flew toward the Vine Grove. / Nana Fig said: "A vine is just a hand held out. / Take it." | Pip let go and did not fall. / Pip let go and FLEW. / "Oh," said Pip. "Oh, oh, oh." |
| 3 | The Gully | The old bridges over the Gully sway like sleeping snakes. / Nobody has crossed them since Nana Fig was young. / Thorns grow where nobody walks. | On the far side, a green leaf caught on a thorn. / Not Nana's. Pip kept it anyway. / Just in case. |
| 4 | Riverbend | Monkeys do not swim. Everyone knows that. / Pip looked at the river. / The river looked back. | Turns out monkeys swim badly, / but they float wonderfully on logs. / Pip was a little proud, and very wet. |
| 5 | Mushroom Hollow | In the Hollow the mushrooms grow as tall as trees, / and the beetles are rude. / Pip has never liked beetles. | A hummingbird told Pip: "The Wind carried a leaf up to / the Ridge. It was singing." / Leaves don't sing. Do they? |
| 6 | Boulder Ridge | Nana Fig once said: "Heavy things are only heavy / until you find the balance." / Pip had always thought she meant the boulders. | From the Ridge, Pip saw the Storm Tree on the mountain. / At its very top something glowed, / like a small green sun. |
| 7 | Windy Cliffs | The Big Wind lives on the Cliffs. / It pushes, it pulls, it laughs. / Pip decided to laugh back. | Halfway up, Pip found scratches in the rock. Old ones. / Monkey-shaped. / Somebody had climbed here before. |
| 8 | Sighing Swamp | The Sighing Swamp is the long way round. / The short way is straight up the Cliffs, / and Pip is not THAT brave. Yet. | Deep in the swamp, on a lily pad, an old carved sign: / FIG WAS HERE. / Pip sat down very suddenly. |
| 9 | The Old Mill | Long ago, monkeys built a mill of ropes and wheels / to reach the mountain. Then they stopped going. / Nobody remembers why. | At the top of the mill, a faded painting: a young monkey, / a golden leaf, a storm. / Nana Fig had made this journey too. |
| 10 | The Storm Tree | The Storm Tree. The Big Wind roaring. / The leaf shining at the very top. / Pip did not look down. Pip looked UP. | Pip held the leaf. It was warm, and it was singing, / softly, in Nana Fig's voice: / "I let it go, little one. I knew you would follow." |

**Epilogue** (three cards after Level 10's outro, then credits):

> Pip swung home. Not the long way round.
> Straight down the Cliffs, laughing.
> Nana Fig was waiting, with two cups of fig tea.

> That night Pip slept on the big green leaf,
> which smelled of home,
> and a little of mountain.

> THE END
> (Bananas: NNN / 262 · Golden figs: N / 10)

**Bonus card** (shown after the epilogue only if all 10 golden figs were collected):

> Pip brought Nana Fig ten golden figs.
> She ate every one,
> and then she asked for more.

### 1.5 Signposts

Each level has 3–5 signposts (`S` tiles). When Pip is within 1.5 tiles a speech-bubble shows the
text (§5.9). Texts are stored in the level files (`sign:` lines, bound to `S` cells in reading
order: top row to bottom row, left to right). They are Nana Fig's voice (tutorial hints, dry humour).
The exact texts are in the level files; they are also listed per level in §4.3.

---

## 2. Core Feel Spec

### 2.1 Units and scale (REDEFINED — replaces the old 0.2 m tiles)

| Thing | Value |
|---|---|
| Tile size | **1 tile = 0.5 m** (Box2D metres). All Box2D bodies use metres. |
| Screen scale | **40 px per tile** (80 px per metre) at the 800×600 design resolution → 20×15 tiles visible. The existing letterbox view code in `main.cpp` stays. |
| Axes | Keep the existing Box2D convention: **+Y is down**, gravity is +Y. Map row 0 is the top of the level. Tile (x, y) covers metres `[0.5x, 0.5x+0.5] × [0.5y, 0.5y+0.5]`. |
| Fixed timestep | **1/60 s**, `Step(dt, 8, 3)` (velocity/position iterations — the old 500/50 is unnecessary). Accumulate real time and step at most 4 times per frame; render after stepping. Frame limit 60 / vsync. |
| Monkey sprite | 64×64 frames drawn at **native size (64 px = 1.6 tiles)**, bottom-centre of the frame aligned to the collider's bottom-centre. Never scale by fractional factors (keeps pixels crisp). |
| Player collider | Width **0.40 m (0.8 tile)**, height **0.60 m (1.2 tiles)**. Capsule: a 0.40×0.20 m box in the middle plus a circle r=0.20 m at the bottom and one at the top (three fixtures on one body, all friction 0, restitution 0). `SetFixedRotation(true)`. Mass forced to **1.0 kg** via `SetMassData`. Any corridor 2 tiles tall is passable. |
| Foot sensor | A sensor box 0.30×0.10 m centred 0.02 m below the collider bottom. `grounded` = sensor overlaps ≥1 non-sensor fixture of terrain/platform/crate/log/etc. (excluding the rope). |

### 2.2 Movement numbers

Horizontal motion is velocity-driven like the current code (compute target vx each step and
`SetLinearVelocity`), but with acceleration limits so it feels weighty:

| Parameter | Value |
|---|---|
| Run speed (max) | **4.5 m/s** (9 tiles/s; crosses the screen in 2.2 s) |
| Ground acceleration | 60 m/s² (0 → max in 0.075 s) |
| Ground deceleration (no input) | 80 m/s² |
| Turn-around braking (input opposite to velocity) | 100 m/s²; play the "brake" pose (stand frame) while |v|>1 m/s and braking |
| Air acceleration | 30 m/s² (70 % control feel) |
| Air deceleration (no input) | 10 m/s² (momentum is mostly kept — important for rope releases) |
| Gravity (rising, `vy<0`) | **25 m/s²** (world gravity) |
| Gravity (falling, `vy>0`) | **33.75 m/s²** (= 1.35×; apply the extra 8.75 m/s² as `ApplyForceToCenter` while falling) |
| Max fall speed | 14 m/s |
| Jump impulse | set **vy = −9.0 m/s** |
| → Apex height | 1.62 m = **3.24 tiles** after 0.36 s |
| → Flat jump length at full run | 6.0 tiles centre-to-centre (≈6.8 tiles edge-to-edge); **design maximum gap = 5 tiles** |
| Variable height | On jump release while rising and ≥0.06 s after take-off: `vy *= 0.4`. Tapped jump ≈ **1.35 tiles** |
| Coyote time | **0.10 s** after leaving ground (not after jumping) |
| Jump buffer | **0.12 s** — a press stored while airborne fires on landing |
| Head-bump | On ceiling contact while rising: `vy = 0.5 m/s` (no sticking). Corner correction: if only the outer 0.12 m of the top circle hits a tile corner, shift the body sideways by up to 0.12 m instead of stopping. |
| Landing | `vy` before impact > 8 m/s → "hard land" squash (sprite scaled 1.15×0.85 for 0.08 s) + dust burst + land SFX; otherwise soft land SFX + small dust |
| Slopes (`/` `\`) | Project the target velocity onto the ground tangent (from the foot-contact normal) so the player runs up/down 45° slopes at full speed and does not slide when idle (set vx=vy=0 relative to ground when no input and grounded on a slope). |
| Walls | No wall-jump, no wall-slide. Friction 0 on the player, so Pip never sticks to walls mid-jump. |

**Jump reach table (used for level design; see §8):** with a running start Pip can land on a
ledge that is `dy` tiles higher at most `dx` tiles away: dy=0 → dx=5; dy=1 → 4; dy=2 → 3; dy=3 → 2.
Falling: any drop, landing up to 6 tiles away. Never require dy=4 without a mushroom/rope/updraft.

### 2.3 State machine

`Ground`, `Air` (covers jump and fall), `Rope`, `Swim`, `Dead`, `Exiting`. Each tick:

1. Read input → intent (dir −1/0/1, jumpPressed edge, jumpHeld, up, down).
2. Sensors: grounded, in-water fraction, wind accumulators, rope-touch candidate.
3. State transitions, then velocity computation, then `SetLinearVelocity` / forces.
4. Animation selection.

Animation from the 7 frames: **stand** (idle, brake), **walk1/walk2** alternate every 0.12 s while
|vx|>0.3 on ground (rate scales with |vx|), **jump** frame in `Air` and on rope; on the rope the
jump frame is rotated with the rope segment angle. Swim: alternate walk1/walk2 slowly (0.35 s)
with the sprite bobbing ±2 px. Facing follows the last non-zero input direction (not velocity), so
Pip faces the direction he is being pushed against in wind.

### 2.4 Ropes / vines (core mechanic, refines the existing `Rope.cpp`)

* A vine is a chain of `2 × L` segments for a vine `L` tiles long (segment = 0.25 m long, 0.04 m
  wide, dynamic, density so each segment weighs 0.03 kg, linear damping 0.6, angular damping 0.5).
  Revolute joints between consecutive segments with limits ±0.6 rad; the top segment is jointed to a
  static anchor. Additionally a `b2DistanceJoint` with `minLength=0, maxLength = totalLength*1.02`
  from the anchor to the last segment stops stretching. Segments have a collision filter so they
  collide with nothing (they are sensors) — the old `PreSolve` disable trick is replaced by
  `isSensor = true` on segment fixtures.
* **Grab**: in `Air` (not `Ground`), when the collider overlaps a segment and `ropeCooldown == 0`,
  attach: `b2RevoluteJoint` between the segment and the player body at the player's centre (as now);
  the player body keeps its velocity so the grab swings the vine. Set state `Rope`, play grab SFX.
  `ropeCooldown` = 0.25 s is set on every release, and the vine just released is ignored until the
  player is not overlapping it (prevents instant re-grab).
* **Swing**: holding Left/Right applies `ApplyForceToCenter(±10 N)` to the player body (pumping).
  There is no artificial speed cap; the arc limits it (~6–7 m/s at the bottom of a long swing).
* **Climb**: Up/Down moves the attachment one segment per **0.12 s** (≈2 m/s, 4 tiles/s).
  Implement by destroying and recreating the joint on the adjacent segment (anchor at the segment
  centre). Cannot climb above segment 1 or below the last segment.
* **Release**: Space (jump) or Down+Space. Destroy joint; the player keeps the segment velocity
  (`vx *= 1.15`), and if Space: `vy = min(vy, −7.5)` (2.25-tile hop, on top of whatever upward
  velocity the swing already had). Releasing at the top of a forward swing is the way to fly far.
  Pressing Down at the bottom of a vine drops off without the hop.
* Ropes are drawn as smooth curves through the segment centres (§5.5).

### 2.5 Death, checkpoints, respawn

* Death sources: touching thorns, touching a beetle's side, being under a falling log/boulder at
  speed (§3), falling below the map bottom (`y > mapHeight*0.5 + 1 m`). No lives, no fall damage.
* On death: freeze input, sprite spins + shrinks over 0.5 s with a puff of leaves, hurt SFX, screen
  desaturates; after 0.8 s fade to black 0.2 s; respawn at the last activated checkpoint (or start)
  with **all dynamic objects reset to their level-file positions** (crates, boulders, logs, lifts,
  see-saws, crumbles, beetles — beetles killed stay dead only until the next death/restart).
  Collected bananas and the golden fig are kept. Death counter +1.
* `R` at any time = death without the animation (instant restart from checkpoint). This is also
  the documented escape from any physics softlock (e.g. a lift that left without you).
* Checkpoint activation: overlap sensor 1×2 tiles; lantern lights, chime; only moves forward (a
  checkpoint further left can still be activated — "last touched" wins).

### 2.6 Camera

Target = player centre + look-ahead of 2 tiles in the facing direction (eased over 0.4 s) and
−1 tile vertically. Horizontal follow: exponential lerp with rate 8/s. Vertical: dead zone ±1.5
tiles, then lerp 6/s; snap when the player is grounded on a new height (lerp 12/s). Clamp so the
view never shows outside the map (levels have no visible border tiles requirement — the
generator uses `#` walls at both edges anyway). Screen shake: hard landing 2 px 0.1 s, boulder
impact 4 px 0.25 s, gust start 1 px for its duration.

---

## 3. Mechanics

Each mechanic below is a tile or object in the level format (§4.1). For each: behaviour, Box2D
hint, drawing (details of colours in §5). "Reset" means the object returns to its level-file state
on death/restart.

| Introduced | Mechanic | Tile/obj |
|---|---|---|
| L1 | run/jump, bananas, golden fig, checkpoints, exit, signposts, one-way branches | `b G C E S =` |
| L2 | vines (grab, swing, climb, release) | `R |` |
| L3 | rope bridges, thorns | `-` `^` |
| L4 | water (swim), currents, floating logs | `~ { }` `L` |
| L5 | bouncy mushrooms, beetles | `M` `e` |
| L6 | pushable crates, see-saws | `X` `seesaw` |
| L7 | wind (steady), updrafts, crumbling ledges | `< >` `A` `%` |
| L8 | moving platforms (lily pads), falling logs | `mover` `F` |
| L9 | pulleys/counterweights, boulders, slopes | `pulley` `O` `/ \` |
| L10 | gust wind (timed) + everything combined | `wind: gust` |

### 3.1 Terrain `#`, slopes `/ \`, one-way branches `=`

* `#` solid. Merge horizontal runs into one static box fixture per run (as `World.cpp` does now),
  friction 0.6. Better ghost-collision behaviour: additionally merge vertically identical runs into
  rectangles (simple greedy rectangle decomposition) — optional.
* `/` and `\` are static right-triangles filling the tile: `/` has vertices (bottom-left,
  bottom-right, top-right) — walking right goes up; `\` mirrors it. Boulders roll down them.
* `=` one-way: static box the full tile wide, 0.12 m thick at the top of the tile, flagged
  `oneWay`. In `PreSolve`, disable the contact when the *player's* bottom is below the platform top
  (`playerBottom > platformTop + 0.02`) or when the player is in `dropThrough` (Down held while
  grounded on a one-way for ≥0.15 s → set `dropThrough` for 0.25 s). For all other bodies `=` is
  solid from every side.
* Drawing: §5.3.

### 3.2 Bananas `b`, Golden fig `G`

Sensor circles r=0.18 m. Banana: +1, sparkle burst, pickup SFX pitch rises with a combo (resets
after 1 s). Golden fig: one per level, rare/hidden; big sparkle + jingle; the HUD fig icon turns
gold. Bananas bob ±3 px (sin, period 1.2 s, phase by x). Drawn procedurally: banana = a yellow
crescent (thick quadratic curve with dark tips); fig = a purple teardrop with a golden glow in the
`G` variant. Total bananas across the game = 262 (28, 42, 55, 15, 23, 21, 17, 18, 17, 26).

### 3.3 Checkpoint `C`, exit `E`, start `P`, signpost `S`

* `C`: sensor 1×2 tiles (the cell and the one above). A carved totem with an unlit lantern; lit
  (warm glow + fireflies) when active. Chime on first activation only.
* `E`: sensor 1×2 tiles. A hollow tree entrance 2 tiles wide × 2.5 tall behind the cell (dark oval
  with a warm inner glow). On overlap the player walks in (0.4 s, auto-walk to the centre, sprite
  fades) → level complete. In the `storm` biome the exit is instead **the golden leaf**: a big
  glowing leaf (concept art shape) hovering 0.5 tile above the cell with rays; Pip jumps into it.
* `P`: spawn. Also the respawn point if no checkpoint touched.
* `S`: wooden signpost; within 1.5 tiles shows a bubble (§5.9).

### 3.4 Vines `R` + `|`

See §2.4. In the map, `R` is the anchor cell; the vine extends down through the consecutive `|`
cells; length in tiles = 1 + number of `|`. The anchor is drawn as a knot on a branch stub.

### 3.5 Rope bridges `-`

A maximal horizontal run of `-` bounded by `#` on both ends. Planks: one dynamic body per tile
(box 0.46×0.10 m, density → 0.15 kg each, linear damping 1.0, angular damping 2.0), chained with
`b2RevoluteJoint`s at plank edges; end planks jointed to the ground anchors. Add a
`b2DistanceJoint` anchor-to-anchor `maxLength = span*1.06` to cap total sag. Target: a 10-plank
bridge sags ≈1.0 tile under Pip at the middle, a 16-plank bridge ≈1.6 tiles; the level maps leave
2–3 tiles of clearance above thorns. Bridges bounce back when Pip jumps. Drawn as planks with two
rope lines through the plank ends (§5.5).

### 3.6 Thorns `^`

Static sensor box 0.36×0.36 m centred in the tile (deliberately smaller than the tile, generous).
Kills on overlap. Drawn as a cluster of 5–7 dark green/brown spikes with red tips growing out of the
adjacent solid tile (default: pointing up; if the solid neighbour is above, point down; etc.).

### 3.7 Water `~` and currents `{ }`

* Water tiles are a region (no fixtures). For the player, compute `submerged` = fraction of the
  collider height inside water tiles (sample at 3 heights).
* `submerged ≥ 0.5` → state `Swim`: gravity force scaled to 0.25; vertical: Up → `vy=-2.5`,
  Down → `vy=+2.5`, nothing → spring toward a rest depth where the collider centre is 0.25 m below
  the surface: `vy = clamp((restY - y)*6, -2, 2)`. Horizontal max 2.2 m/s, accel 20 m/s².
  Space when the collider centre is within 0.4 m below the surface → `vy = -6` (1.44-tile hop; enough
  for a 1-tile bank). Entering water at `vy > 4` → splash particles + splash SFX; exiting → drip.
* Current tiles `{` (push left) / `}` (push right): add a force of ±3 N (3 m/s²) to the player while
  in them, and ±1.2 N per submerged tile-area to logs and other dynamic bodies (logs drift slowly;
  the player on a log is *not* in water, so only the log drifts).
* Buoyancy for dynamic bodies (logs, crates, boulders): per body, compute the submerged fraction
  of its AABB; apply `F_up = mass * g * frac / floatFrac` at the centre of the submerged part,
  where `floatFrac` is the equilibrium submersion (log 0.35, crate 0.6, boulder 1.0 = sinks). Add
  linear damping 2.0 and angular damping 3.0 while submerged.
* Drawing: §5.6. Water is never lethal; river bottoms are the map bottom (falling out of the map
  is impossible while floating).

### 3.8 Floating logs `L`

A maximal horizontal run of `L` (3 in all shipped levels) = one dynamic box `0.5n × 0.5` m, mass
0.8 kg, friction 0.9, rotation enabled (angular damping 3 so it tilts slightly under Pip and rights
itself). Spawned with its top at the tile top; floats via §3.7. Drawn as a rounded brown log with
end rings and moss on top.

### 3.9 Bouncy mushrooms `M`

Static 1-tile body: a box 0.5×0.25 m at the top (the cap) plus a 0.2×0.25 m stem. When the foot
sensor lands on the cap with `vy > 1`: if Jump is held **or pressed within the next 0.08 s** →
`vy = -13` (6.76 tiles), else `vy = -10` (4.0 tiles). Horizontal velocity kept. Cap squashes
(scale 1.3×0.6 → back over 0.25 s), boing SFX pitched by bounce strength, spore particles.
Design rule: from a mushroom Pip reaches ledges up to **6 tiles** higher within 6 tiles sideways.

### 3.10 Beetles `e`

Dynamic body 0.4×0.25 m, fixed rotation, mass 0.3, friction 0. Walks at 1.5 m/s; turns when a
short ray ahead (0.3 m) hits terrain or when the ray ahead-and-down (0.3 m ahead, 0.4 m down) hits
nothing (ledge). Contact from the player: if the player's foot sensor overlaps the beetle's top
sensor (a thin sensor on its back) while `vy > 0` → beetle dies (flips upside-down, falls through
everything, fades), player `vy = -6`, squish SFX. Any other contact → player death. Drawn as a
glossy dark-blue oval shell with a lighter highlight, 2 antennae, 4 tiny legs animated by a 6 Hz
sine. Beetles never leave their platform, never fall, ignore the player (no chasing).

### 3.11 Crates `X`

Dynamic box 0.48×0.48 m (slightly smaller than a tile so they slide through 1-tile gaps), mass
**1.5 kg**, friction 0.5, rotation enabled but angular damping 5 (they mostly slide, may tip off
edges). Pip pushes them by walking into them (velocity-driven body pushes hard; cap the crate's
speed to 3 m/s when in contact with the player). Standing on a crate = +1 tile. Crates float
(§3.7), fall into pits (lost until reset), are not hurt by thorns. Drawn as a woven wooden crate
with a darker X of straps.

### 3.12 See-saws (`obj N: seesaw w=7 limit=25`)

Plank: dynamic box `0.5w × 0.15 m`, mass 1.5 kg, friction 0.8, angular damping 1.0, centred on the
marker cell's centre; `b2RevoluteJoint` to the static ground body at the pivot with
`enableLimit`, `±limit` degrees. The pivot post is **drawn only** (a Y-shaped stump under the
plank), it has no fixture, so Pip can walk under a raised end. With `limit=25` on a 7-tile plank the
ends drop 1.48 tiles (tilting bridge: the near end meets ground level when Pip steps on, the far
end drops as he passes the pivot). With `limit=40` the ends move ±2.25 tiles (level 6 puzzle: a
crate on one end raises the other end high enough to reach a branch 2 tiles above level-plank
height, but not from a level plank).

### 3.13 Wind `< > A` and gusts

* Wind tiles form regions. While the player's centre is inside `<`/`>`: horizontal acceleration
  **6 m/s² in the air, 2 m/s² on the ground** (added to the velocity each step). Flat jump reach
  becomes ≈ 8 tiles with the wind and ≈ 3 tiles against it; levels respect this (with-wind gaps ≤ 8
  tiles, against-wind gaps ≤ 2).
* Updraft `A`: while inside, apply an upward acceleration of 45 m/s² whenever `vy > -4`, so Pip
  rises at 4 m/s (8 tiles/s) and can steer sideways at air-control rates. Leaving the top of the
  column he coasts ~0.6 tile higher. Gravity is otherwise normal (so leaving sideways = falling).
* Crates/logs/boulders get 30 % of the player's wind acceleration (as a force).
* `wind: steady` (default): always on. `wind: gust ON OFF`: all wind tiles cycle *on* for ON seconds
  and *off* for OFF seconds, starting on; 0.6 s before *on*, the leaf particles start streaming and
  a rising whoosh plays so the player can prepare (level 10 uses `gust 2.5 3.5`).
* Drawing: wind is invisible except for particles: leaf/dust streaks flowing in the wind direction
  at 6–10 tiles/s (density 0.4 per tile² per second), and slow spiralling leaves in updrafts.

### 3.14 Crumbling ledges `%`

Static tile. When the player's foot sensor touches it: shake (±2 px, 20 Hz) for **0.35 s**, then the
tile becomes a dynamic body (density 1, no collision with the player after 0.15 s), falls, fades out
over 0.6 s, and respawns (fade in) 3 s after falling if nothing overlaps its cell. Rumble SFX on
shake, crack SFX on fall. Drawn as a grey mossy stone slab with visible cracks (distinct from `#`).

### 3.15 Movers (`obj N: mover w=3 dx=8 dy=0 t=3`)

Kinematic body `0.5w × 0.3 m` with its top at the top of the marker cell (the marker is the
leftmost tile). Moves from the marker position to (marker + dx, dy) tiles and back; each one-way
trip takes `t` seconds with a smoothstep ease and a 0.4 s pause at each end. Player carried: while
the foot sensor touches a mover, add the mover's velocity to the player's set velocity (and to
crates/logs on it via friction 1.0). Drawn as a lily pad (swamp/river) or a woven leaf raft
(elsewhere) with a small ripple/shadow.

### 3.16 Falling logs `F`

A maximal horizontal run of `F` = one hanging log: static box `0.5n × 0.5 m` hanging from two vine
lines (drawn). Trigger when (a) the player's AABB is inside the columns of the log, from the log's
bottom down 6 tiles, or (b) the player stands on it. **0.3 s** after the trigger (creak SFX, log
wobbles) it becomes dynamic: mass 4 kg, friction 0.8, `SetFixedRotation(true)` (so it lands flat
and bridges pits reliably). While its `vy > 3 m/s`, contact with the player from above = death.
Once at rest it is an ordinary 1-tile-high platform; it does not respawn until reset.

### 3.17 Pulleys (`obj N: pulley w=3 partner=M top=8 mass=1.0 ratio=1 damping=1`)

Two platforms (dynamic boxes `0.5w × 0.4 m`, `mass` kg, fixed rotation, friction 1.0) linked by a
`b2PulleyJoint`: ground anchors at (`markerX + w/2`, `top` row centre) for each platform,
`ratio` as given on the *first* of the pair (the partner must name it back; declare ratio on one side
only — the joint is created once per pair). `length1 + ratio*length2 = const`. Each platform also
gets a vertical `b2PrismaticJoint` to the ground so it cannot swing. `damping` = linear damping.
Player carried by friction (also add the platform's vy to the player's set velocity). Drawn with
rope lines from each platform's centre up to its wheel at `top`, a rope across the beam between
the wheels, and iron-banded wooden platforms. Masses matter — see level 9 notes (§4.3).

### 3.18 Boulders `O`

Dynamic circle r=0.475 m (1.9 tiles) centred on the tile centre, mass **4 kg**, friction 0.6,
restitution 0.1, angular damping 0.2. Kills the player on contact if the boulder's speed toward the
player exceeds 3 m/s, otherwise it is just a heavy pushable (Pip pushes it at ≤1 m/s on flat
ground). Removed when it leaves the map. Drawn as a grey-brown rock with 3–4 darker facets and a
moss patch that rotates with the body (so rolling reads).

---

## 4. Levels

### 4.1 Level file format (`data/levels/levelNN.txt`)

Plain text, UTF-8, `\n` line endings. A header of `key: value` lines, then `map:`, then exactly
`height` rows of exactly `width` characters, then `end`. Lines starting with `#` *before* `map:` are
comments (inside the map `#` is terrain). Unknown keys are an error.

```
name: <display name>
biome: <canopy|grove|gully|river|hollow|ridge|cliffs|swamp|mill|storm>
size: <width> <height>
wind: steady | gust <on seconds> <off seconds>
obj <digit>: <type> key=value ...          (0..9, each digit used exactly once in the map)
sign: <text>                              (one per S cell, bound in reading order)
card_in: <line>                           (up to 3 lines; shown before the level)
card_out: <line>                          (up to 3 lines; shown after the level)
map:
<rows>
end
```

Object types: `mover w= dx= dy= t=`, `seesaw w= limit=`, `pulley w= partner= top= mass= [ratio=] [damping=]`.
Defaults: `limit=25`, `ratio=1`, `damping=1`, `mass=1.0`.

**Tile legend** (one character per tile; the *only* characters allowed in the map):

| Char | Meaning | Notes |
|---|---|---|
| `.` | air | |
| `#` | solid earth/rock | |
| `/` `\` | 45° slopes | `/` rises to the right |
| `=` | one-way branch | passable from below, drop-through with Down |
| `-` | rope-bridge plank | run must be bounded by `#` on both sides |
| `^` | thorns | lethal |
| `~` | still water | |
| `{` `}` | water flowing left / right | |
| `<` `>` | wind blowing left / right | air tile |
| `A` | updraft | air tile |
| `R` | vine anchor | vine continues down through `|` cells |
| `|` | vine segment | must have `R` or `|` directly above |
| `M` | bouncy mushroom | |
| `%` | crumbling ledge | |
| `X` | crate | spawns here, falls to rest |
| `O` | boulder | 1.9-tile circle centred here; 8 neighbours must be non-solid |
| `L` | floating log | horizontal run = one log |
| `F` | hanging (falling) log | horizontal run = one log |
| `e` | beetle | spawns on the tile below |
| `P` | player start | exactly one |
| `E` | exit | exactly one |
| `C` | checkpoint | at least one |
| `b` | banana | |
| `G` | golden fig | exactly one |
| `S` | signpost | count must equal the number of `sign:` lines |
| `0`–`9` | object marker | leftmost/pivot tile of the object defined by `obj N:` |

Semantics: the map's left/right edges are solid invisible walls in addition to whatever tiles are
there; the top is open; falling below the bottom row is death. `P`, `E`, `C`, `S`, `e` always sit on
a supporting tile. Wind/water tiles under a banana/collectible are simply absent for that cell
(cosmetic holes; the designer accounted for it).

### 4.2 Design rules used by all maps

* Corridors ≥ 2 tiles tall; the player is 0.8×1.2 tiles.
* Jump table §2.2: gap ≤ 5 flat; +1 → ≤4; +2 → ≤3; +3 → ≤2. Never +4 without help.
* Mushroom: +6 within 6 sideways. Vine: grab within 5 tiles if the vine bottom is ≤2 tiles above
  the feet; release lands up to 8 tiles away. Updraft: exits ~0.6 tile above the column top.
  Tailwind gaps ≤ 8, headwind gaps ≤ 2 (flat).
* Every checkpoint is placed *before* the hard part, and checkpoints are 25–45 tiles apart.
* Falls onto lower ground never kill; only map-bottom pits, thorns, beetles and heavy falling
  objects do. Every physics puzzle has either a non-lethal fallback route or is recoverable with `R`.
* Levels are 100–170 wide, 24–64 tall; each has 15–55 bananas, one golden fig, 3–5 signposts.

### 4.3 The ten levels

For each level: biome, size, mechanics, structure, intent and the secret. The exact map is in the
file; signpost texts are in the file (listed here for review).

**Level 1 — Home Tree** (`canopy`, 100×24, `level01.txt`) — *teach: run, jump, hold-to-jump-higher, one-way branches, checkpoint, exit.*
Flat start with bananas, two low steps, a 3-wide pit, a branch (`=`) to jump through, checkpoint,
a 4-wide pit, a hill of +2 steps to a plateau, a step down to the exit. Secret: a branch above the
plateau's left edge holds the golden fig (dx 2, +3 — the hardest jump in the level, optional).
Signs: "Arrows or A/D walk. Space jumps. Hold Space to jump higher!" · "Mind the gap! You can clear five tiles at a run." · "Every journey ends in a hollow tree. Hop in when you're done."

```
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#.................................................................G................................#
#...............................................................=====..............................#
#.......................................................................b.b.b......................#
#..................................................................................................#
#..................................................................b..################.............#
#........................................bbb..........................################.bb..........#
#.....................b.b..........b.................bb.......b..#####################.............#
#................b................b.b...=====.......b..b.........#########################.........#
#........bbbb.......########.....b...b.............b....b...##############################.........#
#..P..S.........############...S................C...........##############################.S...E...#
##################################...###############....############################################
##################################...###############....############################################
##################################...###############....############################################
```

**Level 2 — Vine Grove** (`grove`, 130×30) — *teach: vines.* A short vine over a 6-wide pit (grab),
checkpoint, two vines over a 15-wide pit (swing to swing), a long vine hanging beside a 10-tall wall
(climb, then swing onto the top), checkpoint, then three vines across a 28-wide ravine (rhythm).
Secret: the middle ravine vine is longer; drop from it onto a hidden pillar with the fig, and climb
back up the same vine. Signs: grab/swing/release · climb & let go at the top of the swing · "Three vines, one gully. Trust the swing."

**Level 3 — The Gully** (`gully`, 140×30) — *teach: sagging bridges, thorns; test: precise jumps.*
Bridge over a drop (safe), checkpoint, a 14-plank bridge over a thorn bed (the sag is the tension),
ground thorns to hop, a climb up three floating rocks with thorns below, checkpoint on the cliff
top, a 16-plank bridge over thorns, two thorn hops, exit. Secret: two branches above the start lead
up-left to the fig (visible from the spawn — teaches "look up").

**Level 4 — Riverbend** (`river`, 150×28) — *teach: swimming, currents, logs.* River 1 has a gentle
right-flowing current and two logs (swim or float). Checkpoint. River 2 flows fast toward a
waterfall (a bottomless gap at its end): hop the five logs before the current takes you — the logs
drift too, slowly. Checkpoint. A still lake with logs and a branch (+3 from a log). Secret: the fig
lies on the lake bed — press Down to dive for it (no breath meter).

**Level 5 — Mushroom Hollow** (`hollow`, 120×40) — *teach: mushrooms (hold for a big bounce),
beetles (stomp).* Mushroom → branch → beetle patrol → mushroom up a 6-tall wall → plateau with
checkpoint and beetle → drop into the hollow: mushroom to a ledge, mushroom on the ledge to a
branch, then up to the upper plateau; a mushroom staircase (mushroom on ground → branch, mushroom
on the branch → exit tower). Secret: the fig in the hollow's far corner, guarded by a beetle.

**Level 6 — Boulder Ridge** (`ridge`, 150×32) — *teach: crates, see-saws.* Crate as a step over a
4-tall wall; a see-saw tilting bridge over a drop; **the see-saw puzzle**: a crate starts on the
right end of a 40°-limit see-saw, holding it down as a ramp — walk under the raised end, board the
low end, push the crate up past the pivot and the plank flips under you: the right end is now high
enough to jump to the branch (only from the raised end; a level plank falls 1.1 tiles short).
Branches up to the cliff, checkpoint, a drop to a shelf with two crates and another 4-tall wall,
a see-saw bridge over thorns, beetles, exit. Secret: a branch above the first wall's plateau.

**Level 7 — Windy Cliffs** (`cliffs`, 110×48, vertical) — *teach: wind, updrafts, crumbling
ledges.* A tailwind carries you over an 8-wide gap; checkpoint; updraft 1 lifts you 14 tiles to a
ledge; four crumbling ledges with 2-wide gaps *against* the wind (keep moving); checkpoint; updraft
2 lifts you 22 tiles to the top plateau and the exit. Secret: from the plateau a tailwind band
carries you left over a 6-wide gap to a crumbling ledge with the fig; it crumbles — you drop 22 tiles
straight into updraft 1 and ride back up. Falls land on ground (no death) except the first gap.

**Level 8 — Sighing Swamp** (`swamp`, 170×28) — *teach: movers, falling logs.* Drifting lily pads
across a pool (or paddle — the pool is swimmable); a hanging log that drops when you walk under it
(run, or wait); **trap-as-tool**: climb two branches, drop onto a hanging 8-tile log above a 6-wide
pit and ride it down — it bridges the pit; pool 2: a pad that rises 6 tiles to a branch, then a pad
that drifts 11 tiles across to the far bank (swim back to the near bank if you miss); a gauntlet of
three hanging logs and two beetles to the exit. Secret: the fig on the bed of pool 2.

**Level 9 — The Old Mill** (`mill`, 130×48) — *teach: pulleys, boulders, slopes.* Four beats:
(A) Step on lift 1 at the top-left slab: your weight (1 kg on a 1 kg platform vs a 1 kg counterweight
that rests on its floor) takes you down 18 tiles while the counterweight rises in its sealed shaft.
(B) The walkway: lift 3 (1.0 kg, ratio 0.5) sits flush in a 3-deep pit; its partner lift 4 (1.1 kg,
6 wide) rests on the floor of a 6-wide, 6-deep pit that blocks the walkway. Push the crate (1.5 kg)
onto lift 3: 2.5 > 2.2 so lift 3 sinks 3 tiles and lift 4 rises 6 to walkway level, bridging the
gap. Pip alone on lift 3 (2.0 < 2.2) does nothing — no softlock; Pip can jump out of the 3-deep pit.
(C) A 5-wide, 5-deep boulder trap pit, then an 8-tall 45° slope; a hanging log with two boulders sits
over the slope — walking under it drops them; they roll down the slope at you and settle in the trap.
Hop onto the branch to let them pass, or outrun them. Checkpoint on the plateau. (D) Lift 5 (ratio 2)
is flush with the plateau; its partner lift 6 hangs above a 6-deep pit. Run up the slope to the
ledge, push the boulder off — it drops onto lift 6 (4+1 kg × 6 tiles > 2 kg × 12 tiles) — then run
down and hop on lift 5 before it rises 12 tiles to the exit slab (damping 4 on lift 6 makes the
rise take ~5 s; if you miss it, `R`). Secret: the fig hangs in lift 5's shaft — you pass through it.
Note for the engineer: the counterweight of lift 1 starts resting on the floor of a sealed shaft
under the start slab (so the pair is stably balanced until Pip adds his weight); draw its rope over
the terrain (it visibly passes through a slot in the slab).

**Level 10 — The Storm Tree** (`storm`, 120×64, vertical) — *climax: everything, in gusts.* Wind
is `gust 2.5 3.5`. A giant trunk (12 wide) rises from the forest floor. Stage 1 (left): mushroom →
three branches → a vine → the left branch and the first hollow through the trunk (beetle). Stage 2
(right, gusts blow away from the tree): the right branch → a 12-plank rope bridge → platform →
three crumbling ledges up the outer wall → a lily-pad mover that drifts 30 tiles back to the upper
right branch, above the gust band → the second hollow (beetle). Stage 3 (left, gusts blow toward the
outer wall): a vine, a branch, a second vine (with the gust helping the swing), the wall ledge with a
checkpoint, two crumbles and a branch up the wall, a mover back to the top-left branch, then a 3-tile
hop onto the trunk's top where the golden leaf waits. Falling off anywhere lands on the forest floor
(mushrooms and branches on both sides bounce you back to the branches — the right side has thorn
patches and a beetle). Secret: the fig in the far bottom-right corner past the thorns.

Total play time target: 60–90 minutes for a first run.

---

## 5. Art Direction (all procedural)

Style: soft, painterly, storybook jungle. Big readable shapes, warm light, cool shadows, everything
slightly rounded. The monkey sprite is the only bitmap; every other visual is built from SFML
`VertexArray`s, `ConvexShape`s, `CircleShape`s and a few small generated `sf::Texture`s (noise,
leaf, dot) created at startup. Draw order: sky → parallax layers → water back → terrain →
objects/vines/bridges → player → water front → foreground foliage → particles → vignette/light →
HUD.

### 5.1 Palettes

Per biome (hex). `skyTop/skyBot` = sky gradient; `far/mid/near` = the three parallax silhouette
layers; `earth` = terrain fill; `earthDark` = terrain edge/shadow; `grass` = cap colour;
`grassLight` = blade highlight; `accent` = decorative flowers/glow; `water` = water base;
`fog` = distance haze tint (alpha applied per layer).

| biome | skyTop | skyBot | far | mid | near | earth | earthDark | grass | grassLight | accent | water | fog |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| canopy | `#7EC8E3` | `#F2E7B5` | `#8FBF9F` | `#4F9A5E` | `#2E6B3A` | `#7A5232` | `#4E3320` | `#6DBE45` | `#B4E86A` | `#F4A93B` | `#4FA5C9` | `#DFF1C8` |
| grove | `#9AD6C8` | `#F6F0C4` | `#7DBF9E` | `#3F8E63` | `#20553C` | `#6E4A2D` | `#46301C` | `#58B348` | `#A6E066` | `#E85D9A` | `#3F9BB5` | `#D6F0DA` |
| gully | `#A9C4D8` | `#E9D9B0` | `#8AA0A8` | `#5B6F62` | `#334A3A` | `#6B5B45` | `#3E3527` | `#7FA84A` | `#B8D46A` | `#C9433C` | `#5B93A8` | `#D9DDD0` |
| river | `#66B8E8` | `#FFF1BF` | `#8CCFB7` | `#3E9E7D` | `#1F6A4E` | `#6C4B2C` | `#43301B` | `#63C24E` | `#B2EA6C` | `#FF8A3D` | `#2F8FC2` | `#CFEFEA` |
| hollow | `#5F7A8C` | `#B9A6C7` | `#6E5F8A` | `#4A3F66` | `#2B2545` | `#5C4A40` | `#332822` | `#4E9E63` | `#8CD98A` | `#E3B04B` | `#3A6E7E` | `#B8A9C9` |
| ridge | `#8FC3E6` | `#F3DDB0` | `#B3A38A` | `#8A7256` | `#5B4A36` | `#8A6A47` | `#553F2A` | `#93B14D` | `#CBDD7A` | `#D9553F` | `#4C97B8` | `#E6DCC4` |
| cliffs | `#6FAEE0` | `#D9E9F5` | `#A5B8C8` | `#7189A0` | `#3F5568` | `#6F6A63` | `#3F3B36` | `#87A85B` | `#B6D278` | `#F2D268` | `#4A8FB8` | `#E4ECF2` |
| swamp | `#7E9C7A` | `#D6CF95` | `#6F8A62` | `#4D6942` | `#2E4429` | `#5E5238` | `#383021` | `#7FA43C` | `#B7CF5C` | `#B9E356` | `#5E7F4A` | `#C7C79E` |
| mill | `#8EA5B8` | `#E2CFA5` | `#8E7F70` | `#66594D` | `#3E362E` | `#7A6046` | `#4A3826` | `#7D9E4B` | `#B3C96E` | `#C48A3F` | `#4B7F96` | `#D8CDBA` |
| storm | `#3C4A63` | `#7F8FA6` | `#4E5B72` | `#35415A` | `#1F2738` | `#5A4A3A` | `#2E251C` | `#4F8A47` | `#86BF66` | `#FFD972` | `#365A73` | `#8892A6` |

The Golden Leaf and golden fig glow: `#FFD972` core, `#FFB13B` edge. UI cream `#FFF4D6`, UI ink
`#2E2418`, UI banana yellow `#FFD23F`, danger red `#E0463C`.

### 5.2 Layered parallax background

Five layers, each a `VertexArray` regenerated per level from a seeded RNG (seed = level index):

1. **Sky**: full-screen vertical gradient `skyTop → skyBot`. `storm`: add a slow-scrolling second
   gradient band (dark cloud strip, alpha 60) and 1.5-s lightning flashes (whole sky lerps 40 % to
   white for 0.1 s) every 12–20 s with a low rumble.
2. **Far** (parallax 0.15): distant mountains/canopy: a polyline of 12–20 points with heights from
   smoothed noise, filled to the bottom with `far` at alpha 200, blended toward `fog`. `river`/`swamp`
   use rounded hill bumps; `cliffs`/`mill` use jagged peaks; `hollow` uses giant mushroom
   silhouettes (circles on stems).
3. **Mid** (parallax 0.4): tree silhouettes: trunks (tapered quads) with 3–5 canopy blobs (circles of
   r 1.5–3 tiles) in `mid`; every 5th tree is a palm (fan of 7 curved leaves). Density 1 tree per 4
   tiles of level width.
4. **Near** (parallax 0.7): closer trunks and hanging vines (thin curves) in `near`, plus 2–3 large
   leaf clusters per screen.
5. **Foreground** (parallax 1.3, drawn *over* the player, alpha 170): occasional big leaves and
   fern fronds at the bottom edge, never covering more than 15 % of the screen height, none within
   4 tiles of the player's screen position (skip those).

Light shafts: in `canopy`, `grove`, `hollow` draw 3–4 translucent (alpha 25–40) tall
parallelograms in `#FFF6C8` slanting from top-right, slowly swaying (±2° over 6 s).

### 5.3 Terrain rendering

Terrain is rendered once per level into chunk `RenderTexture`s (chunks of 32×32 tiles) and then
drawn as sprites; nothing is regenerated per frame.

1. **Fill**: for every `#` tile draw a quad in `earth`. Then multiply in a tileable 128×128 noise
   texture (value noise, 3 octaves, contrast 0.25) sampled in world space (so it does not repeat
   visibly per tile) — this is the "textured earth".
2. **Rounded silhouette**: for each terrain corner that is convex (solid tile with two air
   neighbours forming a corner), overdraw the outer 0.15 tile of the corner with the sky/background
   colour and a quarter-circle of `earth` — cheaper version: draw a `CircleShape` of r=0.15 tile in
   `earth` and a masking square; simplest acceptable version: skip rounding and rely on the grass
   caps and edge shading (do rounding if time allows).
3. **Edge shading**: for every solid tile edge facing air draw a 0.18-tile-wide strip: top edges in
   a lighter tone (`earth` lerped 35 % to `grassLight`), bottom edges in `earthDark`, side edges in
   `earthDark` at alpha 120. Inner corners get a small dark triangle.
4. **Grass/moss caps**: on every top edge facing air draw a cap 0.35 tile tall in `grass` with a
   wavy bottom (3 bumps per tile), and 4–6 blades per tile: thin triangles 0.25–0.5 tile tall
   leaning ±25°, in `grass`/`grassLight` alternating. Blades sway: per-vertex offset of
   `sin(t*2 + x*0.7) * 2 px` (this is the one terrain thing recomputed per frame — keep blades in a
   separate `VertexArray` per chunk). Biome variants: `gully`/`ridge`/`cliffs`/`mill` use sparse
   dry tufts (2 blades, `#C9B36B`); `hollow` uses glowing moss dots instead of blades; `swamp`
   uses reeds (tall thin blades, 0.8 tile).
5. **Dangling roots**: on bottom edges facing air with ≥2 tiles of air below, 30 % chance per tile
   of 1–3 root curves (quadratic Béziers 0.5–1.5 tiles long, 3 px wide, `earthDark`) with a small
   knot at the tip; they sway slightly.
6. **Rock speckles**: 2 % of solid tiles get a lighter pebble ellipse; near edges, small darker
   stones.
7. **Slopes**: filled triangles with the same noise multiply; the grass cap follows the slope
   (a rotated cap strip).
8. **One-way branches `=`**: a horizontal branch — a rounded rectangle 0.3 tile thick in `earthDark`
   with a lighter top line, bark notches every tile, 1–2 leaf pairs per 2 tiles hanging off the
   underside, and small twig ends at both extremes.
9. **Crumbling `%`**: grey slab (`#8E8C86`), cracks (2–3 dark polylines), moss on top corners, tiny
   pebbles that drop while shaking.
10. **Mushroom `M`**: a stem 0.4×0.5 tile (`#F1E3C8`) and a cap: half-ellipse 1.0×0.5 tile in
    `accent`-ish red `#D84C3F` with 3 cream dots; `hollow` caps are teal `#3FB0A0` and glow.

### 5.4 Objects

* Crate: rounded square 0.48 tile in `#A9743F`, planks (3 horizontal lines), a darker X of straps,
  slight 2-px outline `earthDark`.
* Boulder: circle in `#8C8378` with 3 facet polygons (`#6F675D`), a moss patch (`grass`), rotated
  with the body.
* Log (floating/falling): rounded rectangle in `#8B5A2B`, end rings (concentric circles on the
  ends when seen from the side — draw as two small ellipses), moss on top, two vine lines for `F`.
* Beetle: as §3.10, colours shell `#26365C`, highlight `#5F79B5`, legs `#1A1A1A`.
* See-saw: plank `#B07A45` with grain lines; fulcrum a Y-shaped stump in `earthDark`.
* Lifts: platform planks with two iron bands (`#5B5B5B`), rope `#C9A66B` to a wheel (circle with 6
  spokes) under the beam; the rope across the beam is drawn as a straight line between wheels.
* Movers: lily pad (`#4F9A3C` ellipse with a notch, `#8ED46A` rim, a pink flower `accent` on one in
  three) or leaf raft (three overlapping leaves).
* Signpost: post + board (`#B07A45`), text bubble see §5.9.
* Checkpoint: totem 1.5 tiles tall (stacked rounded blocks, carved face) with a lantern: dark when
  inactive; when active the lantern glows `#FFD972` (additive circle, alpha 90, radius 1.5 tiles,
  pulsing 10 %) and 6 fireflies orbit.
* Exit: dark hollow oval 2×2.5 tiles (`#1F160E`) with an inner warm glow (`#FFB13B` alpha 70) and
  leaves around the rim; bananas rise out of it slowly (decorative). Storm biome: the Golden Leaf
  (leaf polygon 2×3 tiles, `#FFD972`, veins `#FFB13B`, 8 rotating rays alpha 40, sparkle particles).

### 5.5 Vines, ropes and bridges

* Vine: sample the segment centres, build a Catmull-Rom curve, draw as a triangle strip 5 px wide
  in `#3F6B2A` with a lighter 2 px core `#7BB05A`. Every 3rd segment gets a leaf pair: two small
  leaf polygons (0.3 tile) oriented with the segment tangent, alternating sides, in `grass`. The
  anchor is a knot (circle r 5 px) on a stub branch (`=`-style, 1 tile).
* Bridge: each plank a quad (0.46×0.10 m) in `#B07A45` with a dark edge, rotated with the body;
  two rope lines (2 px, `#C9A66B`) through the plank ends; end posts (0.3×0.8 tile) at the anchors.
* Pulley ropes and falling-log vines: 2-px lines, slight sag drawn as a quadratic curve.

### 5.6 Water

Water tiles are drawn in two passes. **Back pass** (before terrain objects): the water body as a
quad per column from the surface row to the bottom in `water` at alpha 200, darkening 30 % toward
the bottom. **Front pass** (after the player): the top 0.4 tile of each surface column at alpha 110
(so Pip reads as submerged), plus the surface line: a polyline across the surface with vertex y =
`surface + 3px * sin(x*1.3 + t*2) + 2px * sin(x*3.1 - t*3.4)`, drawn as a 3-px light line
(`#DCF5FF`, alpha 180) and a second darker line 2 px below. Currents: add streak sprites (thin 1×6
px light lines) drifting at 3 tiles/s in the flow direction, 0.3 per tile². Reflection: skip.
Splash: 12 droplets (small circles, `#DCF5FF`) with gravity, plus an expanding ring on the surface.

### 5.7 Particles

One pooled particle system (max 2000), each particle: position, velocity, life, size, colour,
kind (dot/leaf/streak). Kinds:

* **Ambient leaves**: 1 per 30 tile² of visible area, spawn at the top of the screen, fall at
  1 tile/s with horizontal sine drift, rotate; colour from `grass`/`accent`. In wind they move
  with the wind (§3.13).
* **Fireflies** (`hollow`, `swamp`, `storm` nights; also at checkpoints): 20 per screen, slow random
  walk, alpha pulsing 0.5–1 with 2-s period, additive `#DFF48A`.
* **Dust**: on landing 6–10 grey puffs; while running on ground 2 puffs/s behind the feet.
* **Splash/drips**: §5.6.
* **Sparkles**: banana pickup 8 yellow stars 0.3 s; golden fig 24 gold stars 0.8 s; the Golden Leaf
  continuous 3/s.
* **Spores** (mushroom bounce), **pebbles** (crumble shake), **leaf burst** (death, 20 leaves).
* **Rain** (`storm` only): 200 thin streaks falling at 25 tiles/s, angle following the current gust
  direction (±20°), alpha 90.

### 5.8 Lighting & post

* Vignette: a full-screen quad with a radial alpha gradient (0 at centre → 90 at corners) in
  `#1A1208`. `hollow`/`storm` use 140 at corners.
* Player light: in `hollow`, `mill`, `storm` an additive soft circle (r 4 tiles, alpha 35,
  `#FFE9B0`) around Pip.
* Checkpoint/exit/leaf glows: additive circles as described.
* Death: lerp the whole frame toward greyscale by rendering the world to a `RenderTexture` and
  drawing it with a `sf::Shader` (simple luminance mix) — if shaders are unavailable, draw a black
  quad at alpha 120 instead.

### 5.9 UI

Font: bundle one open-licence TTF (e.g. a rounded sans such as *Fredoka* or *Nunito* — the engineer
picks; put it in `data/fonts/` with its licence). Sizes: HUD 22 px, cards 22 px, titles 48 px.

* **HUD** (top-left, cream panel with rounded corners, alpha 200): banana icon + `12 / 28`; fig icon
  (grey outline, gold when collected); death counter with a small skull-leaf icon; a slim timer
  `m:ss` (top-right). HUD hides after 3 s idle in the level and fades in on any pickup/event.
* **Level title card**: on level start, over the game view, a wide cream ribbon slides in from the
  left for 1.8 s: `LEVEL 3 — THE GULLY` in ink, with the biome's `accent` underline.
* **Story cards**: full-screen `skyBot`-toned paper (`#FFF4D6`) with a subtle noise texture, a thin
  double border in `earthDark`, the 1–3 lines centred in 22 px ink, a small procedurally drawn
  vignette in the top third: a leaf (green polygon with veins) for Nana Fig cards, a swirl for wind
  cards, the Golden Leaf for the final card. "Space to continue" pulses at the bottom. Cards fade
  in/out 0.3 s.
* **Title screen**: the canopy palette background with parallax layers scrolling slowly (auto-pan
  along a looping strip of level 1), the monkey sprite (stand frame, 2× scaled with nearest
  filtering) sitting on a procedural green leaf like the concept art, gently bobbing; the title
  LEAFWIND in 48 px cream with a dark drop shadow and a leaf tucked into the F; menu: *Continue*
  (if a save exists) / *New Game* / *Level Select* / *Quit*. Ambient leaves and the bird/insect
  ambience loop.
* **Level select**: a 5×2 grid of cards, each card: level number, name, a colour swatch of the
  biome palette (three horizontal bands sky/mid/earth) as a mini-thumbnail, `bananas best/total`,
  a gold or grey fig, best time. Locked cards are dimmed with a vine-knot icon. Arrows/WASD move,
  Space enters, Esc back.
* **Pause menu** (Esc): dim the game (black alpha 140), a cream panel: *Resume* / *Restart from
  checkpoint* / *Restart level* / *Level select* / *Quit*. Shows the controls list at the bottom.
* **Results screen** (after outro card): bananas collected, fig yes/no, deaths, time, "New best!"
  tags; *Next* / *Replay* / *Level select*.
* **Credits**: after the epilogue cards, scroll: title, "a game by <owner>", "design", "code",
  "Pip's sprites (2010)", "thank you for playing", with leaves drifting; Esc/Space returns to the
  title.
* **Signpost bubble**: a cream rounded box with a small tail above the post, text 18 px ink, max
  width 320 px with word wrap, fades in/out over 0.15 s.

---

## 6. Audio (all synthesized at startup into `sf::SoundBuffer`s, 44.1 kHz mono 16-bit)

Write a tiny synth: oscillators (sine, triangle, square, saw, white noise), linear/exponential
frequency sweeps, ADSR envelopes, a one-pole low-pass, and additive mixing; render each SFX once.
Play through a pool of 16 `sf::Sound`s. Master volume in options (save file).

| SFX | Recipe |
|---|---|
| jump | Square 220→440 Hz sweep over 90 ms, duty 0.4; env A 2 ms / D 80 ms / no sustain; low-pass 3 kHz. Pitch ×1.0 normally, ×1.1 for a jump from a vine. |
| land (soft) | Noise burst 40 ms low-passed at 600 Hz, env A 1 ms D 40 ms, gain 0.4. **hard**: same + a 60 Hz sine thump 80 ms, gain 0.8. |
| step | Noise 15 ms, low-pass 1.2 kHz, gain 0.15, random pitch ±10 %; every 0.14 s while walking on ground (0.22 s when on wood: bridges/lifts use a 200 Hz triangle tap instead). |
| banana | Two sines: 880 Hz for 60 ms then 1320 Hz for 90 ms (a rising "ping"), env A 1 ms D 100 ms; each consecutive pickup within 1 s raises both notes by a semitone (up to +7). |
| golden fig | Arpeggio: triangle 523, 659, 784, 1047 Hz, 90 ms each with 60 ms overlapping decays, plus a soft sine shimmer at 2093 Hz 400 ms at gain 0.2. |
| rope grab | Short noise "slap" 25 ms (band 800–2000 Hz) + a 160 Hz triangle 60 ms; on release a reversed version at lower gain. |
| climb tick | 300 Hz triangle 20 ms, gain 0.2, each segment moved. |
| splash | Noise 250 ms, low-pass sweeping 4 kHz→400 Hz, env A 5 ms D 250 ms, plus 3 random sine blips (600–1200 Hz, 40 ms) in the first 100 ms. |
| hurt / death | Saw 440→110 Hz over 350 ms, duty-modulated by a 30 Hz square (buzz), env A 2 ms D 350 ms, low-pass 2.5 kHz; then a soft descending 3-note triangle (392, 330, 262 Hz, 120 ms each). |
| checkpoint | Sine chord 523+659+784 Hz 500 ms with a 6 Hz tremolo, A 20 ms, D 500 ms; plus a "lantern light" tick (2 kHz sine 20 ms). |
| level complete | Triangle melody 8 notes at 110 ms: C5 E5 G5 C6 G5 E5 G5 C6 (523, 659, 784, 1047, 784, 659, 784, 1047) with a sustained C4+G4 sine pad 1.2 s underneath, gain 0.5. |
| mushroom boing | Sine 150→600 Hz over 120 ms then 600→300 Hz over 180 ms, with a 12 Hz vibrato; pitch ×1.25 for a big bounce. |
| beetle squish | Noise 60 ms band-passed 300–900 Hz + a 90 Hz sine pop, env D 80 ms. |
| crumble | shake: noise 350 ms low-pass 300 Hz with 20 Hz amplitude wobble, gain 0.3; crack: a 2 ms click burst + noise 150 ms low-passed 1.5 kHz. |
| crate push | Looping noise low-passed 400 Hz gain 0.15 while a crate moves in contact with the player. |
| boulder | Roll: looping brown noise gain proportional to speed (max 0.4); impact: 50 Hz sine 150 ms + noise 100 ms, gain 0.8. |
| log creak / thud | Creak: 180 Hz saw with pitch wobble ±8 % at 5 Hz, 300 ms, low-pass 1 kHz, gain 0.3. Thud: like hard land, gain 1.0. |
| wind gust | Loop: white noise through a low-pass whose cutoff follows the gust envelope (200 Hz off → 1500 Hz on, 0.6 s ramp), gain 0.35 on / 0.08 off; updraft adds a whistly 1.2 kHz band-pass noise at gain 0.15 while Pip is inside. |
| UI move / confirm | 660 Hz sine 30 ms / 880+1320 Hz 80 ms. |

**Ambience loops** (per biome, 8-s loops synthesized once): a low pink-noise bed (gain 0.06) plus
biome sprinkles: canopy/grove — random bird chirps (2-note sine slides 1.5–3 kHz, every 2–6 s);
river — water noise (low-passed noise 900 Hz, gain 0.15, slow amplitude wobble); hollow — drips
(sine 1.8 kHz 30 ms with a 200 ms delayed quieter repeat, every 3–7 s); swamp — frogs (140 Hz
square ribbits 80 ms ×3, every 4–9 s) and insects (4 kHz buzz bursts); cliffs/storm — the wind
loop; mill — a slow wooden creak every 5–10 s and a distant wheel rhythm (low thump every 1.3 s).

**Music (optional, simple)**: a generative marimba-style piece: a pentatonic scale (C D E G A) over
a 2-bar chord loop (C–Am–F–G) at 96 bpm; each beat 60 % chance of a triangle note with a 400 ms
exponential decay, melody randomly walking ±2 scale steps; a soft sine bass on chord roots each bar.
Per biome change the root (canopy C, grove D, gully A minor, river G, hollow E minor, ridge F, cliffs
D, swamp A minor, mill G, storm E minor) and the tempo (storm 120 bpm). Fade out during story cards
and replace with the ambience bed only.

---

## 7. Game Flow, Controls & Persistence

### 7.1 Flow

```
Boot → Title ─┬─ New Game → Prologue card → Level 1 intro card → LEVEL → outro card → Results → next level ...
              ├─ Continue → Level select (cursor on the first unfinished level)
              ├─ Level select → intro card → LEVEL → outro card → Results → (Next / Replay / Select)
              └─ Quit
Level 10 outro → Epilogue cards (3) → [Bonus card if 10 figs] → Credits → Title
```

Level start sequence: fade from black 0.4 s, title ribbon, player control enabled immediately.
Pause (Esc) is available in-level only; story cards can be skipped with Esc.

### 7.2 Controls

| Action | Keyboard | Gamepad (SFML joystick 0, Xbox mapping) |
|---|---|---|
| Move | ← → or A D | Left stick X / D-pad |
| Jump / release vine / hop from water | Space or Z or K | A (button 0) |
| Climb vine / swim up / enter door | ↑ or W | Left stick up / D-pad up |
| Climb down / drop through `=` / dive / drop off vine | ↓ or S | Left stick down |
| Pause | Esc | Start (button 7) |
| Restart from checkpoint | R | Back (button 6) |
| Menu confirm / back | Space, Enter / Esc | A / B |

Replace the direct `sf::Keyboard::isKeyPressed` calls in `Player` with an `Input` struct filled
once per frame (needed for buffering, gamepad and the headless tests).

### 7.3 Persistence

`save.txt` next to the executable (or `$XDG_DATA_HOME/leafwind/save.txt` if that directory is
writable; try it first). Plain text, one line per level, rewritten atomically (write `save.tmp`,
rename):

```
leafwind-save 1
level 1 unlocked=1 done=1 bananas=25 total=28 fig=1 deaths=3 time=87.4
level 2 unlocked=1 done=0 bananas=0 total=42 fig=0 deaths=0 time=0
...
options volume=0.8 music=1
```

Level N+1 unlocks when level N is completed. Best bananas/fig/deaths/time are kept as the best
of each independently. The results screen shows the current run vs best.

---

## 8. Testability

### 8.1 Command-line modes (extend the existing `main.cpp` argument parsing)

* `--validate-levels` — headless (no window): loads every `data/levels/level*.txt` and checks the
  rules below; prints one line per level and exits non-zero on any failure. Add as a CTest.
* `--screenshot <levelIndex> <frames> <out.png>` — opens the window, loads that level, runs
  `frames` fixed steps with no input (frames ≥ 60 lets particles/idle settle), saves the frame,
  exits. Used for the README and the render smoke test (already wired through `xvfb-run` in
  `tests/CMakeLists.txt`; update that test's arguments to the new signature).
* `--test-physics [levelIndex]` — headless: load the level (default all), step 600 frames with no
  input and then 600 frames holding Right+Jump alternately; assert no NaN, the player never leaves
  the map horizontally, every dynamic body stays within map bounds ±2 m or is removed, and the
  player is either alive or respawned. Keep the existing test name `PhysicsIntegration`.
* `--play <levelIndex>` — skip menus, jump straight into a level (for iteration).

Rendering must be separable from simulation: `World` owns the physics and objects and can be
constructed without any `sf::RenderWindow`; only `Renderer` touches graphics. Sound is optional
(a null audio device must not crash headless runs).

### 8.2 Level validation rules (`--validate-levels`)

1. Header parses; `size` matches the row count and every row's length; only legend characters.
2. Exactly one `P`, one `E`, one `G`; ≥1 `C`; `S` count == `sign:` count; every `obj` digit used
   exactly once and every used digit defined; partner pulleys reference each other.
3. `|` has `R`/`|` above; every `-` run is bounded by `#`; `O` has no solid tile in its 8
   neighbours; `P E C S e` and `M` have support below; `P E C` have head room.
4. Size bounds: 60 ≤ width ≤ 250, 20 ≤ height ≤ 80.
5. Reachability (over-approximate flood fill from `P` over "standable" cells using the jump table
   in §2.2, vine reach, mushroom reach, water adjacency, updraft columns, wind bonuses, and a list
   of declared physics links for puzzles/movers): `E`, every `C`, `G`, `S` and every `b` must be
   reached. The generator that produced the shipped levels implements exactly this; porting the
   same rules to C++ is optional but recommended (declared links can be expressed as
   `link: x0 y0 x1 y1` header lines — the current files do not carry them, so an initial C++
   validator may implement rules 1–4 only and treat 5 as a warning).

### 8.3 Visual regression

Keep `tests/visual_test.sh`: take `--screenshot` of levels 1, 4, 7, 10 after 120 frames and compare
against `tests/reference_screenshots/` with a tolerance (ImageMagick `compare -metric RMSE`,
threshold 0.08 — particles are randomized, so seed the RNG from the level index in screenshot mode
and disable time-based animation by fixing `t = frames/60`).

---

## 9. Implementation notes for the engineer (scope guard)

* Order of work: level loader + validator → terrain/one-way/player feel (§2) → camera → vines →
  the rest of §3 in level order → renderer (§5) → UI/flow (§7) → audio (§6) → tests (§8).
  Each mechanic can be checked in its introducing level with `--play N`.
* Keep the existing class split (`World`, `Player`, `Rope`, `Bridge`, `Block`, `ContactListener`)
  but expect to rewrite their internals; the `UserData` contact-callback pattern is fine to keep.
  Use `b2Body::GetUserData().pointer` consistently; never store raw `this` on both fixture and
  body without a common base.
* Everything visual comes from ~10 small generated textures + vertex arrays; no image assets
  besides the monkey and the font. If something in §5 turns out expensive, drop in this order:
  rounded terrain corners, light shafts, rain, foreground foliage, reflections (already skipped).
* Not in scope: enemies beyond beetles, boss, dialogue trees, localisation, shaders beyond the
  optional greyscale, online anything, controller remapping UI.

---

## 10. Implementation notes (deviations and clarifications)

Recorded by the engineer while implementing this document. Everything not listed here follows the
spec above. All ten levels are verified by scripted playthroughs on the real physics
(`tests/test_levels.cpp`, run by `ctest` as `LevelRoutes`).

### 10.1 Level map edits (minimal, each found by a failing playthrough)

| Level | Change | Why |
|---|---|---|
| 3 | The three-rock climb (block 64–66 rows 23–25, rocks 61–63 rows 20–21 and 65–67 rows 17–18) became a rising staircase: block at 59–61, rocks at 63–65 and 67–69; the banana at (65,21) moved to (66,21). Same heights, same count, thorns still under the last gap. | Rock B overhung the only approach to block A (head room), and each +3 zigzag hop needed a frame-perfect sideways steer at the apex. |
| 5 | A mushroom added at (62,37) in the hollow's far corner. | The fig corner was a pit with no way out except `R`; now Pip can bounce back onto the pillar. |
| 6 | The puzzle crate moved from x=53 to x=51 (row 24). | With the crate at the plank's end there was no room for Pip to board the low end right of it. |
| 7 | The secret's crumbling ledge moved from x=61–63 to x=62–64 and the fig from (62,8) to (63,8). | The gap was 7 tiles, not the 6 the text describes, and the tailwind leap fell a hair short; now the ledge also sits straight above updraft 1 as §4.3 says. Note: reaching it means hopping over the exit sensor on the plateau (a full jump clears it). |
| 8 | The start bank (x=1–20) is one row higher; `P` and the first `S` moved up a row. | The bank sat a tile below pool 1's surface (a standing "wall of water"); every other bank is flush with its water. |
| 10 | The two wall crumbles and the branch above them moved down one row (22 → 19 → 16 → 13, then +1 onto mover 2). | From the wall-ledge checkpoint the first crumble was +4, which §2.2 forbids. |

### 10.2 Feel and physics

* **Launch velocities** (jump, rope hop, mushroom bounces, stomp, swim hop) get half a gravity step
  added (`cfg::launch`, e.g. −9.21 instead of −9.0 m/s). Box2D integrates semi-implicitly, which
  otherwise shaves v·dt/2 off every apex; measured heights now match §2.2/§3.9 (3.24-tile jump,
  1.40-tile tap, 4.0 / 6.77-tile bounces).
* **Wind** is a separate drift velocity (+6 m/s² in the air, decays 0.35/s) that the run controller
  does not cancel — otherwise holding a direction would nullify it. On the ground it only drifts an
  idle Pip (2 m/s²); while running it bleeds off quickly, so take-off speed is full and headwind
  jumps reach ≈3 tiles, tailwind ≈8–9, as designed.
* **Currents** use the same drift mechanism in water (3 m/s², terminal ≈2 m/s), so Pip can just
  about swim upstream.
* **Updrafts** act while Pip's centre *or feet* are in the column, so he leaves the top with his feet
  and coasts ≈0.6 tile above it (with centre-only sampling he cleared the ledge by 0.04 tile).
* **Slopes**: the controlled speed is measured along the surface (4.5 m/s along the slope) and the
  tangential part of gravity is cancelled so Pip stands still. Slope handling applies to static
  ground and see-saws; bridges, logs and crates take plain horizontal velocity.
* **Moving supports**: kinematic movers lend their full velocity. Dynamic supports (logs, planks,
  crates) lend their horizontal velocity only while Pip is idle — running on a light log otherwise
  fed Pip's push back into the log (runaway speed).
* **Vines**: while grabbed, an extra rope-length distance joint (anchor → Pip, max = length along the
  vine) keeps the 1 kg monkey from stretching the 0.03 kg segments; the grab anchor is on the
  segment's axis so Pip is pulled onto the vine. The top joint to the anchor is unlimited.
* **Bridges**: plank links are 1.02 × tile long (slack for the sag) and each plank has rope-limit
  distance joints to both anchors instead of one anchor-to-anchor joint (which would join two static
  points). Measured sag under Pip: 16-plank ≈1.6–1.7 tiles.
* **See-saws**: a crate spawned above a see-saw starts resting on the plank with the plank already at
  its limit (dropping it on a level plank flung it off). The 40° puzzle see-saw has a grippy plank
  (friction 1.6) and low end stops so the crate stays aboard.
* **Movers**: even-numbered movers start half a cycle out of phase, so neighbouring lily pads meet
  (level 8's two pads were otherwise always 11 tiles apart).
* **Pulleys**: platforms are 2 cm narrower than their shafts (no wall binding) and each pair's speed
  is capped at 2.4 / `damping` m/s, applied through the ratio. With the documented masses the boulder
  flung lift 5 up 12 tiles in 0.7 s; now it takes ≈5 s as §4.3 intends. Default lifts move ≤2.4 m/s.
* **Level 9 lift 4** cannot be a true bridge: with ratio 0.5, "Pip alone on lift 3 does nothing"
  and "lift 4 holds Pip" are contradictory for any masses. It rises to walkway level and works as a
  stepping stone (run-jump, touch down on its far end, keep running); falling in is recoverable
  with `R`. Masses are as documented.
* **Falling logs** crush only when they come down on top of Pip (contact normal pointing down onto
  him and his centre under the log's span); a clipped shoulder just shoves him. Crush checks use the
  object's pre-step velocity. Floating logs that go over a bottomless edge stop colliding with
  terrain (they tumbled and wedged in level 4's falls), and water columns that reach the map bottom
  get an invisible bed.
* **Sensors**: the foot sensor is an AABB query + `b2TestOverlap` each step; bananas, figs,
  checkpoints, thorns, the exit and beetle stomps/side hits are geometric overlap tests (beetles do
  not physically collide with Pip). Pip's body never sleeps.
* Level 7: falling from the crumbling ledges is lethal (the gap under them is bottomless, contrary to
  "falls land on ground"); unchanged — the checkpoint is right before it.

### 10.3 Presentation, flow and tooling

* Font: DejaVu Sans / Sans Bold (`data/fonts/`, licence included) rather than Fredoka/Nunito.
* Convex terrain corners are rounded with a 9 px (≈0.22 tile) radius; grass caps overhang edges.
* No options screen: volume (`-` / `=`) and music (`M`) are hotkeys, shown in the pause screen and
  stored in the save file. Title plays the canopy ambience and music.
* After level 10's outro the epilogue follows directly (no results screen), per §7.1.
* Validator: rules 1–4 of §8.2 plus building every level's physics world; rule 5 (reachability) is
  covered by the scripted playthroughs instead.
* Extra command-line modes: `--screenshot` also accepts `title`, `select`, `credits`,
  `card:prologue|<N>|out<N>|epilogue|bonus`, `pause:<N>`, `results:<N>` and `--place <tx> <ty>`;
  `--flow-test` (drives menus/cards/levels with synthetic keys), `--bench <N> <frames>`,
  `--dump-audio <dir>` (every synthesized sound as WAV) and `--mute`. `LEAFWIND_SAVE` overrides the
  save path, `LEAFWIND_DATA` the data directory.
