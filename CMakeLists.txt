# ------------------------------------------------------------------------------------------------ #
#                                This file is part of CosmoScout VR                                #
#       and may be used under the terms of the MIT license. See the LICENSE file for details.      #
#                         Copyright: (c) 2019 German Aerospace Center (DLR)                        #
# ------------------------------------------------------------------------------------------------ #

option(CSP_SIMPLE_WMS_BODIES "Enable compilation of this plugin" ON)

if (NOT CSP_SIMPLE_WMS_BODIES)
  return()
endif()

# build plugin -------------------------------------------------------------------------------------

file(GLOB SOURCE_FILES src/*.cpp)

add_library(csp-simple-wms-bodies SHARED
  ${SOURCE_FILES}
)

target_link_libraries(csp-simple-wms-bodies
  PUBLIC
    cs-core
)

# install plugin -----------------------------------------------------------------------------------

install(TARGETS   csp-simple-wms-bodies DESTINATION "share/plugins")
install(DIRECTORY "textures"        DESTINATION "share/resources")
install(DIRECTORY "gui"      DESTINATION "share/resources")
