/**
 * MIT License
 *
 * @brief Snapshot representing digital button input states.
 *
 * @file ButtonBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <ButtonHandler_Config.h>
#include <InputModel.h>
#include <SnapshotBus.h>
#include <Universal_Button.h>

#include <cstddef>
#include <cstdint>

// ---- Aliases ---- //

using ButtonState = snapshot::input::State<NUM_BUTTONS>; ///< Snapshot payload: bitset of button states + timestamp.
using ButtonBus = snapshot::SnapshotBus<ButtonState>;    ///< Snapshot bus transporting ButtonState frames.
using Button = ButtonHandler<NUM_BUTTONS>;               ///< Concrete button handler bound to NUM_BUTTONS.
using snapshot::input::for_each_edge;                    ///< Import edge-iteration helper for brevity.
using snapshot::input::idx;                              ///< Import generic enum→index caster for brevity.

// ---- Names table (generated from BUTTON_LIST) ---- //

/**
 * @brief String names for each logical button (index-aligned with ButtonIndex and bitset).
 * @note Generated from the BUTTON_LIST macro so logs remain readable and always in sync.
 */
#define INPUTTYPES_NAME_EXPAND(name, pin) #name,
static constexpr const char *kButtonNames[NUM_BUTTONS] = {BUTTON_LIST(INPUTTYPES_NAME_EXPAND)};
#undef INPUTTYPES_NAME_EXPAND

// ---- Static checks ---- //

static_assert(NUM_BUTTONS > 0, "Expected at least one button.");
static_assert(sizeof(kButtonNames) / sizeof(kButtonNames[0]) == NUM_BUTTONS,
              "kButtonNames must match NUM_BUTTONS.");

// ---- Utilities ---- //

/**
 * @brief Get a human-readable name for a ButtonIndex.
 * @note Assumes ButtonIndex values are index-aligned with BUTTON_LIST order.
 */
constexpr const char *to_name(ButtonIndex id) noexcept
{
    return kButtonNames[idx(id)];
}
