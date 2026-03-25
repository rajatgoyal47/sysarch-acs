..
.. Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
..
.. SPDX-License-Identifier: BSD-3-Clause
..

********************************************************************************
FIRME Architecture Compliance Suite - Scenario Document (FIRME-ACS)
********************************************************************************

.. contents:: Contents
  :depth: 2

Overview
========

FIRME-ACS validates the TF-A BL31 (EL3) implementation of the FIRME service by exercising FIRME ABIs from the
caller security states BL31 expects in real deployments:

- Normal world: NS-EL2 dispatcher payload (BL33)
- Secure world: S-EL2 shim payload (BL32) with optional S-EL1 caller bodies
- Realm world: R-EL2 shim payload (Realm payload image), reached via RMMD forwarding

Testcase checklist
==================

.. list-table::
  :header-rows: 1
  :widths: 22 20 58

  * - **Test Name**
    - **Validated rule(s)**
    - **Algorithm**

  * -

      `gpt_transition_feat_rme <../test/gpt/gpt_transition_feat_rme.c>`_

    -

      R0085: FEAT_RME permitted transitions (Table 3.1) via ``FIRME_GM_GPI_SET``.

    -

      1. Check FEAT_RME presence.
      2. From the required caller worlds, attempt the Table 3.1 transitions:

         - Secure caller: NS <-> Secure
         - Realm caller:  NS <-> Realm

      3. For each transition:
         a) Ensure the test granule is at the **Current GPI**.
         b) Invoke ``FIRME_GM_GPI_SET`` from the specified world.
         c) Expect SUCCESS and verify enforcement using cross-world access probes.

  * -

      `gpt_transition_feat_gpc2 <../test/gpt/gpt_transition_feat_gpc2.c>`_

    -

      R0086: FEAT_RME_GPC2 additional NS<->NSO transitions (Table 3.2).

    -

      1. Check FEAT_RME_GPC2 presence.
      2. From NS-EL2, attempt:

         - NS -> NSO
         - NSO -> NS

      3. Expect SUCCESS only when FEAT_RME_GPC2 is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement by probing the transitioned granule from Secure and Realm payloads and confirming
         the expected fault/no-fault behavior.

  * -

      `gpt_transition_feat_gdi <../test/gpt/gpt_transition_feat_gdi.c>`_

    -

      R0089: FEAT_RME_GDI NSP/SA transition policy (Table 3.3).

    -

      1. Check FEAT_RME_GDI presence.
      2. From NS-EL2, attempt the Table 3.3 permitted transitions:

         - NS/NSO -> NSP
         - NS/NSO -> SA
         - NSP    -> NS/NSO
         - SA     -> NS/NSO

      3. Expect SUCCESS only when FEAT_RME_GDI is present; otherwise expect DENIED/NOT_SUPPORTED.
      4. Verify enforcement using cross-world probes and confirm no transitions beyond the union of Table 3.1–3.3
         are permitted.

Notes
=====

- The tests are designed to be deterministic and self-contained (no dependency on full RMM/SPMC runtimes).

License
=======

FIRME-ACS is distributed under the BSD-3-Clause License.

*Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.*
