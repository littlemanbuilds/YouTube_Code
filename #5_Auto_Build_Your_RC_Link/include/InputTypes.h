/**
 * MIT License
 *
 * @brief Button snapshot aliases and simple helpers.
 *
 * @file InputTypes.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <cstddef>
#include <app_config.h>
#include <Universal_Button.h>
#include <ButtonHandler_Config.h>
#include <SnapshotBus.h>
#include <SnapshotModel.h>

// ---- Aliases ---- //

using Button = ButtonHandler<NUM_BUTTONS>;              ///< Concrete button handler bound to NUM_BUTTONS.
using InputState = snapshot::model::State<NUM_BUTTONS>; ///< Snapshot payload: bitset of button states + timestamp.
using InputBus = snapshot::SnapshotBus<InputState>;     ///< Snapshot bus that transports InputState frames.
using snapshot::model::for_each_edge;                   ///< Import edge-iteration helper for brevity.
using snapshot::model::idx;                             ///< Import generic enum→index caster for brevity.

// ---- Names table (generated from BUTTON_LIST) ---- //

/**
 * @brief String names for each logical button (index-aligned with ButtonId and bitset).
 * Generated from the BUTTON_LIST macro so logs remain readable and always in sync.
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
 */
constexpr const char *to_name(ButtonIndex id) noexcept
{
    return kButtonNames[idx(id)];
}

/**
 * @brief Print human-readable button edges (press/release) between two snapshots.
 *
 * @param prev Previous snapshot.
 * @param cur Current snapshot.
 */
inline void logButtonEvents(const InputState &prev, const InputState &cur) noexcept
{
    for_each_edge<NUM_BUTTONS>(
        prev, cur,
        [](std::size_t i, bool pressed, std::uint32_t t_ms)
        {
            debug(kButtonNames[i]);                          ///< Button name.
            debug(pressed ? " pressed @ " : " released @ "); ///< Edge type.
            debugln(t_ms);                                   ///< Timestamp (ms).
        });
}