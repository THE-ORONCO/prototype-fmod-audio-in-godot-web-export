# Audio sources

## Bundled (in `audio/` and the FMOD Studio project)

| Files                                            | Source                                                                      | Licence | Used as                                      |
|--------------------------------------------------|-----------------------------------------------------------------------------|---------|----------------------------------------------|
| `singularity_calm.wav`, `singularity_action.wav` | [Singularity](https://opengameart.org/content/singularity-0) by vitalezzz   | CC0     | Calm/action layers, designed for crossfading |
| `click_001–005.ogg`                              | [Kenney Interface Sounds](https://opengameart.org/content/interface-sounds) | CC0     | `click`                                      |
| `back_001–004.ogg`                               | [Kenney Interface Sounds](https://opengameart.org/content/interface-sounds) | CC0     | `footstep` (stand-in)                        |

## Candidates for new demos

Researched on 2026-10-03 and not downloaded. Check the licence on each page before using anything. CC-BY requires
attribution.

Music:

- [Tension Based Loops](https://opengameart.org/content/tension-based-loops-6-loops-full-mix) (VividReality, CC-BY 3.0):
  6 synth loop layers in C minor. Good for testing more than two vertical layers.
- [Gravity Turn](https://opengameart.org/content/gravity-turn) (vitalezzz, CC0): calm and action loops, but the calm
  loop is shorter. Suits horizontal switching rather than synced layers.
- [Exploration and Chase](https://opengameart.org/content/exploration-and-chase-orchestral-symphonic-music-with-a-bit-of-synthesizer-sound)
  (boite obscure, CC-BY 4.0): horizontal sections plus vertical layers. MP3 only.
- [Dynamic Action Music Pack](https://opengameart.org/content/dynamic-action-music-pack) (ViRiX, CC-BY 3.0): calm, tense
  1–3, climax, and endings. Suits horizontal re-sequencing with `AudioStreamInteractive` or FMOD transitions.
- [Interactive JRPG music in FMOD Studio](https://opengameart.org/content/interactive-jrpg-music-in-fmod-studio)
  (SubspaceAudio, CC0): a complete FMOD Studio project, useful as a reference.

SFX (all CC0):

- [Platformer Sounds](https://opengameart.org/content/platformer-sounds-terminal-interaction-door-shots-bang-and-footsteps)
  (yd): terminal beeps, doors, footsteps
- [Fantozzi's Footsteps (Grass/Sand & Stone)](https://opengameart.org/content/fantozzis-footsteps-grasssand-stone): 12
  single steps. These could replace the `footstep` stand-in.
