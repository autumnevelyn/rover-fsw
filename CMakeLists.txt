cmake_minimum_required(VERSION 3.16)
# IDF specific

# Infrastructure and apps
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/osal"
                                 "${CMAKE_CURRENT_LIST_DIR}/apps"
                                 "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(rover_fsw)
