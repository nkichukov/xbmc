/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C"
{

struct AddonGlobalInterface;

namespace ADDON
{

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold general gui functions and initialize also all other gui related types not
   * related to a instance type and usable for every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/controls/Edit.h"
   */
  struct Interface_GUIControlEdit
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static void set_visible(void* kodiBase, void* handle, bool visible);
    static void set_enabled(void* kodiBase, void* handle, bool enabled);

    static void set_input_type(void* kodiBase, void* handle, int type, const char* heading);

    static void set_label(void* kodiBase, void* handle, const char* label);
    static char* get_label(void* kodiBase, void* handle);

    static void set_text(void* kodiBase, void* handle, const char* text);
    static char* get_text(void* kodiBase, void* handle);

    static void set_cursor_position(void* kodiBase, void* handle, unsigned int position);
    static unsigned int get_cursor_position(void* kodiBase, void* handle);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */
