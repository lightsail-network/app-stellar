/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2024 Ledger SAS.
 *   (c) 2024 overcat.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/
#ifdef HAVE_BAGL
#include "glyphs.h"

// clang-format off
UX_STEP_NOCB(
    ux_blind_signing_warning_step,
    pbb,
    {
      &C_icon_warning,
#ifdef TARGET_NANOS
      "Transaction",
      "not trusted",
#else
      "This transaction",
      "cannot be trusted",
#endif
    });
#ifndef TARGET_NANOS
UX_STEP_NOCB(
    ux_blind_signing_text1_step,
    nnnn,
    {
      "Your Ledger cannot",
      "decode this",
      "transaction. If you",
      "sign it, you could",
    });
UX_STEP_NOCB(
    ux_blind_signing_text2_step,
    nnnn,
    {
      "be authorizing",
      "malicious actions",
      "that can drain your",
      "wallet.",
    });
#endif
UX_STEP_NOCB(
    ux_blind_signing_link_step,
    nn,
    {
      "Learn more:",
      "ledger.com/e8",
    });
// clang-format on
#endif