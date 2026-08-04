/*
 * Drop the PMW3610 auto mouse layer as soon as a normal key is typed.
 *
 * The pmw3610 driver only releases the auto mouse layer on a timer
 * (CONFIG_PMW3610_AUTOMOUSE_TIMEOUT_MS). For that whole window the keys the
 * mouse layer overrides -- J = left click, K = right click, "," = F5,
 * "." = back -- stay overridden even when you meant to type. This listener
 * watches the keycodes that are actually emitted and drops the layer as soon
 * as one of them is an ordinary key.
 *
 * Looking at emitted keycodes rather than key positions gives the behaviour we
 * want for free:
 *   - mouse buttons (&mkp) are reported through the Zephyr input subsystem and
 *     never raise a keycode event at all, so clicking keeps the layer alive;
 *   - a held modifier emits a modifier keycode, which we ignore, so
 *     shift-click and ctrl-click still work;
 *   - tapping a mod-tap emits its tap keycode instead (LANG1 / LANG2 here),
 *     which drops the layer -- a tap means you went back to typing.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/keys.h>

LOG_MODULE_REGISTER(mona2_automouse_cancel, CONFIG_ZMK_LOG_LEVEL);

#define AUTOMOUSE_LAYER ((zmk_keymap_layer_id_t)CONFIG_MONA2_AUTOMOUSE_CANCEL_LAYER)

/* Keycodes produced by the mouse layer itself, which must therefore not cancel
 * it. Mouse buttons need no entry here: they never raise a keycode event. */
static const uint32_t automouse_keep_keycodes[] = {
    HID_USAGE_KEY_KEYBOARD_F5, /* "," on the mouse layer */
};

static int mona2_automouse_cancel_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!zmk_keymap_layer_active(AUTOMOUSE_LAYER)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* consumer / media / system usages: leave the layer alone */
    if (ev->usage_page != HID_USAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* a modifier is being held down: keep shift-click and ctrl-click working */
    if (is_mod(ev->usage_page, ev->keycode)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (size_t i = 0; i < ARRAY_SIZE(automouse_keep_keycodes); i++) {
        if (ev->keycode == automouse_keep_keycodes[i]) {
            return ZMK_EV_EVENT_BUBBLE;
        }
    }

    LOG_DBG("keycode 0x%02X typed, dropping auto mouse layer", ev->keycode);
    zmk_keymap_layer_deactivate(AUTOMOUSE_LAYER);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(mona2_automouse_cancel, mona2_automouse_cancel_listener);
ZMK_SUBSCRIPTION(mona2_automouse_cancel, zmk_keycode_state_changed);
