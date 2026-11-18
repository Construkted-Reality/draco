# Changes in this fork

This fork holds patches that TileForge needs and that are not yet in
`google/draco`. Each entry names the branch, the reason, and the upstream
status.

Keep this file current. A patch with no entry here is a patch nobody can
review.

## fix/options-float-precision

Branched from `google/draco` `main` at 052a311. That commit is release 1.5.7
plus 49 `main` commits; it is not a release.

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

## sync/upstream-decoder-checks

Branched from `fix/rans-bit-encoder-destructor` at 673ee67. This branch holds
both patches above and the upstream commits below.

The sync point is upstream `main` at d6c1acf. The branch takes four changes
from `052a311..d6c1acf` by cherry-pick, in upstream order. Each pick carries
the upstream author and a `(cherry picked from commit ...)` line. The first
three are merge commits on upstream `main`, picked with `-m 1`. The fourth is
the commit inside the merge 9feaeae (upstream pull request 1231), with a
shorter message body.

| Upstream commit | Subject |
|---|---|
| dd4ec6c | Add bounds checks to decoder prediction schemes and point count |
| 0bc8abd | Add bounds checks on decoded face indices in sequential mesh decoder |
| 807217a | Check offset in rANS entropy decoder |
| 21bace6 | Add missing output-buffer bounds check to tex-coords prediction scheme decoders |

These commits add input validation on the decode path. They change no
encoder code. For a valid bitstream the decoder returns the same mesh as
before. The `draco_static` encoder objects are byte-identical to the build of
673ee67.

The branch does not take the other upstream commits in that range. Those
commits change encoder code, build files, file readers, point cloud decoders
only, or release packaging.

Two upstream decoder changes in that range are not taken and need a decision
of their own:

- c301b6a (upstream pull request 1238) makes the wrap transform of the
  decoder add in 64 bits. It changes the decoded result for a bitstream whose
  prediction plus correction leaves the signed 32-bit range. TileForge
  bitstreams hold at most 30 bits per value and cannot reach that case.
- The 1.5.8 release commit 13445d3 holds more decoder checks (a limit on the
  number of predicted values, and an axis check in the kd-tree decoder). The
  same commit changes encoder code, so a cherry-pick cannot take it. The
  checks need a manual port.

Upstream status: these are upstream commits. Nothing to report.

## How to take further upstream changes

1. Fetch upstream: `git fetch upstream --tags`.
2. List the commits after the last sync point:
   `git log --oneline --first-parent d6c1acf..upstream/main`. Read each
   commit, and read the commits inside each merged pull request.
3. Select only the commits whose hunks under `src/draco` are decode path
   validation. Do not take a commit that changes code the encoder runs. Do
   not take build file changes.
4. Make a branch from the current maintained branch. Cherry-pick the selected
   commits in upstream order with `git cherry-pick -x`.
5. Build `draco_static` for the old tip and for the new branch. Use the same
   source path, the same build path, and the same options for both builds.
   Compare the sha256 of every object file. Only decoder objects may differ.
6. Build with `-DDRACO_TESTS=ON` and run `draco_tests` on both. The results
   must be the same, and `options_test` must pass.
7. Update the sync point and the table in this file.
