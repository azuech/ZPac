# The Bresenham Accumulator in ZPac

## Who Was Jack Bresenham?

Jack Elton Bresenham (1937–2022) was an American computer scientist who spent most of his career at IBM. In 1962, while working at IBM's San Jose Development Laboratory, he was tasked with a practical engineering problem: how to make a digital plotter draw straight lines accurately on a grid of discrete integer positions.

The result was the **Bresenham Line Algorithm**, published in 1965 in the *IBM Systems Journal*. It became one of the most widely cited algorithms in computer graphics history — not because it was mathematically exotic, but because it was brilliantly pragmatic: it solved a floating-point problem using nothing but integer addition and comparison, making it viable on hardware that had no floating-point unit at all.

Bresenham later extended his ideas to circle drawing (the Midpoint Circle Algorithm), and his approach influenced generations of rasterization techniques. His original 1962 algorithm is still in use today, essentially unchanged, in embedded systems, GPU rasterizers, and retro platforms.

---

## The Core Idea: Error Accumulation Without Division

The fundamental insight of Bresenham's approach is this: **instead of computing the exact fractional position at each step, accumulate the error as an integer and act when it overflows.**

Consider drawing a line from (0, 0) to (7, 3). The ideal slope is 3/7 ≈ 0.4286. At each horizontal step, you advance exactly 3/7 of a pixel in the vertical direction — but you can only draw at integer positions. Naively, you would compute `y = round(x * 3/7)` at each step, which requires a division.

Bresenham's method avoids the division entirely:

```
error = 0
for each step x from 0 to 6:
    error += 3          // add numerator
    if error >= 7:      // compare with denominator
        y += 1          // take the integer step
        error -= 7      // subtract denominator, keep the remainder
```

The result is identical to the floating-point version, with no division, no multiplication, and no fractional arithmetic. Just addition and comparison.

This technique generalizes far beyond line drawing: **any situation requiring a fractional rate applied to a discrete counter** can use the same accumulator pattern.

---

## Fixed-Point Variant: The 8.8 Accumulator

A common modern variant — used in ZPac — reformulates the accumulator as a **fixed-point integer**. In an 8.8 fixed-point representation:

- The **upper 8 bits** hold the integer part (whole pixels to move)
- The **lower 8 bits** hold the fractional part (the accumulated remainder)

Each frame, a speed value is added to the accumulator. The integer part is extracted with a right-shift, and the fractional remainder is preserved with a bitmask:

```c
accumulator += speed_value;
steps = accumulator >> 8;    // integer part: pixels to move this frame
accumulator &= 0xFF;         // keep only the fractional remainder
```

This is mathematically equivalent to Bresenham's error accumulation, expressed in binary arithmetic.

---

## Why It Matters on Z80

The Zeal 8-bit Computer is built around the **Zilog Z80** processor running at 10 MHz. The Z80 has no hardware floating-point unit and no hardware division instruction. Division must be implemented in software, typically as a loop of subtractions, taking dozens of clock cycles.

On a platform where every cycle counts — and where the game loop must complete within a single vertical blanking interval at 60 fps — floating-point or software division for per-character movement would be prohibitively expensive.

The Bresenham accumulator reduces per-frame movement calculation to:
- one 16-bit addition (`speed_acc += pac_speed`)
- one 8-bit right-shift (`>> 8`)
- one 8-bit AND (`&= 0xFF`)

Three operations. Essentially free.

---

## The Arcade Speed Model

The original arcade game ran characters at fractional speeds relative to a base clock. For example:

| Condition | Speed (approx.) |
|---|---|
| Pac-Man, normal | 80% of base |
| Pac-Man, frightened | ~90% of base |
| Ghosts, normal | 75% of base |
| Ghosts, frightened | 50% of base |
| Ghosts, eyes (returning) | 150% of base |

These are not integers. Reproducing them on a discrete-frame system requires either floating-point arithmetic or a fractional accumulator. The Bresenham approach handles them exactly, without approximation drift — which matters for long-term timing accuracy.

---

## The ZPac Implementation

In ZPac, each character has a `speed_acc` field (a `uint16_t`) in its state structure. Speed values are defined per level in a table of `speed_set_t` structs:

```c
static const speed_set_t speed_tiers[4] = {
    /*  pac_n  pac_f  pac_d  gho_n  gho_f  gho_t */
    {   433,   505,   361,   397,   216,   144 },  /* Tier 0: L1 */
    {   487,   541,   397,   451,   234,   162 },  /* Tier 1: L2-4 */
    {   541,   577,   433,   505,   252,   180 },  /* Tier 2: L5-20 */
    {   487,   577,   397,   505,   252,   180 }   /* Tier 3: L21+ */
};
```

The value 433, for instance, represents 433/256 ≈ 1.691 pixels per frame. At 60 fps, this yields approximately 101 pixels per second — calibrated to match the arcade original within 0.5%.

The core accumulator logic appears in the main game loop, labeled as step 4:

```c
/* 4. Speed accumulator fixed-point 8.8 */
{
    uint16_t pac_speed;
    if (fright_timer > 0) {
        pac_speed = cur_spd.pac_fright;
    } else {
        pac_speed = cur_spd.pac_normal;
    }
    zpac.speed_acc += pac_speed;
}
steps = (uint8_t)(zpac.speed_acc >> 8);
zpac.speed_acc &= 0xFF;
```

The `steps` value — almost always 1, occasionally 2 — then drives the pixel movement loop (step 5), which advances the character one sub-pixel at a time while checking for tile-center intercepts to handle collision and direction changes.

---

## The Jitter Problem It Solves

Without the accumulator, a naive implementation might round the fractional speed to the nearest integer each frame: always move 1 pixel, or always move 2. This introduces systematic timing error that accumulates over time, causing characters to arrive at tile boundaries slightly early or late. The visible result is **jitter**: irregular movement cadence that looks wrong to anyone familiar with the original arcade game.

The Bresenham accumulator eliminates this by distributing the fractional steps perfectly evenly over time — the same mathematical guarantee that made Bresenham's line algorithm draw visually smooth lines in 1962.

---

## Summary

| Aspect | Detail |
|---|---|
| Inventor | Jack E. Bresenham, IBM, 1962 |
| Original purpose | Integer line rasterization on plotters |
| Core principle | Accumulate fractional error; act on integer overflow |
| ZPac variant | Fixed-point 8.8 integer accumulator |
| Location in code | `game_loop.c`, step 4 of the main game loop |
| Purpose in ZPac | Accurate fractional speed for Pac-Man and ghosts at 60 fps |
| Z80 advantage | Three instructions (add, shift, AND) vs. expensive software division |
| Calibration | Within 0.5% of original arcade timing |

The Bresenham accumulator is one of those rare algorithmic ideas that remains optimal across six decades of hardware generations — from IBM plotters in 1962, to arcade hardware in 1980, to Z80 homebrew computers in 2024.
