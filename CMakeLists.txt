# SPDX-License-Identifier: MIT

if(CONFIG_MONA2_AUTOMOUSE_CANCEL)
  zephyr_library_named(mona2_automouse_cancel)
  zephyr_library_sources(src/automouse_cancel.c)
endif()
