# List contains only add-on related headers not present in
# ./addons/kodi-dev-kit/include/kodi
#

# Keep this in alphabetical order
set(CORE_ADDON_BINDINGS_FILES
    ${CORE_SOURCE_DIR}/xbmc/input/actions/ActionIDs.h
    ${CORE_SOURCE_DIR}/xbmc/input/XBMC_vkeys.h
)

set(CORE_ADDON_BINDINGS_DIRS
    ${CORE_SOURCE_DIR}/xbmc/addons/kodi-dev-kit/include/kodi/
    ${CORE_SOURCE_DIR}/xbmc/cores/VideoPlayer/Interface/Addon
)
