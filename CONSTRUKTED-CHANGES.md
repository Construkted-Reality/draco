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

## fix/rans-bit-encoder-destructor

Branched from `fix/options-float-precision` at 618e9bf. This branch holds both
patches.

The destructor of `RAnsBitEncoder` calls `Clear`. `Clear` calls
`bit_counts_.assign(2, 0)`, which allocates when the vector has no storage yet.
A destructor is `noexcept`, so a failed allocation there terminates the
process.

The vector has no storage when `StartEncoding` did not run yet, or when its own
allocation failed. When an encode fails with `std::bad_alloc` and memory stays
exhausted, the stack unwinds through the destructor, the destructor allocates,
and the process aborts. The caller cannot catch the failure. The backtraces
show two owners: `MeshEdgebreakerEncoderImpl` and
`MeshPredictionSchemeTexCoordsPortableEncoder::EncodePredictionData`.

TileForge measured this with the allocation probe in
`tileforge-draco/tests/native_allocation.cc`. The probe fails one allocation,
or one allocation and every later allocation, at each `operator new` call of
one encode and one decode. On the unpatched pin, every one of the 119
terminating attempts stopped in this destructor. The 119 attempts came from
four encode configurations. With the patch, the probe reports 0 terminating
attempts, and decode never terminated.

The patch removes the call to `Clear` from the destructor. The members release
their own storage, so the destructor does nothing that the patch removes. The
patch does not change encoded output. Measured on the 932 tile corpus in five
encode configurations: 4,650 of 4,650 encoded buffers are byte-identical to
the unpatched build.

The patch adds no Draco unit test. The probe needs a replaced global
`operator new`, which the Draco test binary cannot hold.

Upstream status: not yet reported.
