# Corner legibility experiments

Variant A was selected on October 31, 2026 and promoted to `projection/approved-mesh.json`.
The images below are offline comparisons; the compositor must be rebuilt to use the new mesh.
All images use a synthetic 720 × 720 source card, sampled once through mesh triangles.
The enlarged corner views use the same pixel scale for every variant.

## Options

- **Reference:** approved M5.
- **A — fuller corners:** corner-weighted stretch/area optimization with a curvature
  penalty. The central radius 0.4 and the circular boundary are fixed to M5.
  The top-left marker's local area increases by 36.5%; its thin axis grows by 39.8%.
  Stretch falls from 2.97:1 to 2.08:1. Curvature residuals are subpixel at 960 px,
  rather than a claim of mathematically exact convexity.
- **B — content margin:** use the inner 84% of the original parameter domain.
  All content remains visible; the unused outer region is shown in gray.
  Stretch falls to 1.90:1, but the center is 16% smaller. This is an explicit
  space-for-shape tradeoff, not a crop or a full-disk projection.

The four measured source markers are `(30,60)`, `(690,60)`, `(30,675)`, and `(690,675)`.
`metrics.json` contains local Jacobian singular values and areas in normalized coordinates.
Metrics use symmetric finite differences of the bilinear mesh; the previews use the
compositor's triangle split. All bilinear face-corner determinants are positive.

## Reproduce

Requires NumPy and Pillow. Code and data remain separate from the approved projection.

```sh
python3 experiments/corner-legibility/compare.py
# Redraw previews from the stored experimental A mesh without refitting.
python3 experiments/corner-legibility/compare.py --reuse
```

Outputs: `comparison.png`, `corner-zoom.png`, individual previews, experimental meshes,
and `metrics.json`. A is selected; B remains an unselected experiment.
