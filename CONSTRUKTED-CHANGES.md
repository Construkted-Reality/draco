# Construkted-Reality fork of Google Draco

This fork carries one change on top of upstream `google/draco`:

## Flat open-addressing attribute dedup (`flat-hash-dedup` branch)

`PointAttribute::DeduplicateFormattedValues<float,3>` (the position
attribute dedup — the single hottest stage of mesh encode, ~30–40% of
encode self-time on high-vertex meshes) previously used
`std::unordered_map`, a node-based container with a heap allocation
per unique value. This fork replaces it with a flat open-addressing
hash table (`FlatDedupTableU32x3`), eliminating the per-node
allocation and improving cache locality.

- **Byte-identical output.** The flat table preserves the exact
  first-occurrence index assignment, so the encoded `.drc` bitstream
  is byte-for-byte identical to stock Draco. Verified on all 48 Draco
  testdata meshes plus varied quantization/compression parameters.
- **Faster encode.** Measured ~1.1–1.3× whole-encode on high-vertex
  meshes (win scales with vertex count / duplicate density; small
  meshes see little). The win comes from removing per-node allocation,
  not from the hashing itself.
- **Applies to the `float`/3-component (position) instantiation**;
  all other attribute types keep the stock `std::unordered_map` path.

No behavior, format, or API change — a pure performance optimization.
