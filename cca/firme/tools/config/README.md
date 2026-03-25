# FIRME-ACS config files

## `sp_layout.json`

FIRME-ACS does not package Secure Partitions (SPs) by default, so `sp_layout.json` is intentionally empty
(`{}`).

- The suite runs a Secure shim at S-EL2 (BL32).
- SEL1 caller coverage (when required by the FIRME spec) is exercised via an internal S-EL2→S-EL1 entry path,
  without relying on a packaged SP.

TF-A uses `SP_LAYOUT_FILE` to generate `sp_gen.mk` via Python tooling (requires the `fdt` module). When
`sp_layout.json` is empty, `cca/firme/tools/scripts/build_tfa_fvp.sh` can build TF-A without `fdt` by
providing an empty `sp_gen.mk` fallback.

If you populate `sp_layout.json` to package SPs, ensure your Python environment provides `fdt` so TF-A can
generate `sp_gen.mk`.

