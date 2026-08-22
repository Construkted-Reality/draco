# Changes in this fork

This fork holds patches that TileForge needs and that are not yet in
`google/draco`. Each entry names the branch, the reason, and the upstream
status.

Keep this file current. A patch with no entry here is a patch nobody can
review.

## fix/options-float-precision

Branched from `google/draco` `main` at 052a311 (release 1.5.7).

`Options::SetVector` and `Options::SetFloat` store every value with
`std::to_string`, which formats a float in fixed notation with 6 decimal
places. A binary32 whose unit in the last place is smaller than 1e-6, which is
any value below about 8.4, does not survive that round trip.

`ExpertEncoder::SetAttributeExplicitQuantization` passes the quantization
origin and the range through that same store. `SetAttributeGridQuantization`
calls it. So a mesh whose bounding box lies near the origin of the coordinate
frame gets an origin that is one unit in the last place wrong, and every
decoded position leaves the grid the caller asked for.

TileForge tiles a single model into thousands of pieces and relies on the
grid to keep a vertex shared by two neighbouring tiles in one place. Measured
on a 932 tile corpus: with the defect, one tile pair agreed on 0 of 169 shared
vertices; with the fix, on 169 of 169.

The patch prints a float with `%.9g` and a double with `%.17g`. Both
round-trip exactly. It keeps the Android fallback for the other types. It adds
`src/draco/core/options_test.cc`, which fails on unpatched Draco and passes
here.

Upstream status: not yet reported.
