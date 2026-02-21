cmake_minimum_required(VERSION 3.24)

project(ShipNetSimInstaller VERSION ${ShipNetSim_VERSION})

# Helper function to filter debug DLLs and ensure release versions exist
function(filter_debug_dlls input_dll_list output_dll_list)
    set(filtered_dlls "")
    foreach(dll_path ${${input_dll_list}})
        get_filename_component(dll_name "${dll_path}" NAME)
        # Check if this is a debug DLL (ends with d.dll or _d.dll)
        if(dll_name MATCHES ".*[_]?d\\.dll$")
            # Extract the base name without the debug suffix
            if(dll_name MATCHES ".*_d\\.dll$")
                # Handle _d.dll case
                string(REGEX REPLACE "_d\\.dll$" ".dll" release_dll_name "${dll_name}")
            else()
                # Handle d.dll case
                string(REGEX REPLACE "d\\.dll$" ".dll" release_dll_name "${dll_name}")
            endif()

            get_filename_component(dll_dir "${dll_path}" DIRECTORY)
            set(release_dll_path "${dll_dir}/${release_dll_name}")
            # Only exclude debug DLL if release version exists
            if(EXISTS "${release_dll_path}")
                message(STATUS "Excluding debug DLL (release version exists): ${dll_name}")
            else()
                message(WARNING "Debug DLL found but no release version: ${dll_name} - including anyway")
                list(APPEND filtered_dlls "${dll_path}")
            endif()
        else()
            # Include non-debug DLLs
            list(APPEND filtered_dlls "${dll_path}")
        endif()
    endforeach()
    set(${output_dll_list} "${filtered_dlls}" PARENT_SCOPE)
endfunction()

# Check if building of installer is enabled
if(BUILD_INSTALLER)

    # Define ShipNetSimInstaller as a custom target
    add_custom_target(ShipNetSimInstaller ALL
        COMMENT "Creating an installer for ShipNetSim."
    )

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_CORE_DLL_NAME})
add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_NAME})
add_dependencies(ShipNetSimInstaller ShipNetSimAdjBuilder)

if (BUILD_GUI)
    add_dependencies(ShipNetSimInstaller ShipNetSimGUI)
endif()

if (BUILD_SERVER)
    add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_SERVER_NAME})
    add_dependencies(ShipNetSimInstaller ShipNetSimRabbitMQConfig)
endif()

# Define install paths for the executables and libraries
install(TARGETS ${SHIPNETSIM_CORE_DLL_NAME}
    RUNTIME DESTINATION bin
    COMPONENT ShipNetSimCore_COMP
    LIBRARY DESTINATION lib
    COMPONENT ShipNetSimCore_COMP
    ARCHIVE DESTINATION lib/static
    COMPONENT ShipNetSimCore_COMP)

# Define install paths for the ShipNetSim Main Executable
install(TARGETS ${SHIPNETSIM_NAME}
    RUNTIME DESTINATION bin
    COMPONENT ShipNetSim_COMP
    LIBRARY DESTINATION lib
    COMPONENT ShipNetSim_COMP
    ARCHIVE DESTINATION lib/static
    COMPONENT ShipNetSim_COMP)

# Define install paths for the ShipNetSimAdjBuilder Executable
install(TARGETS ShipNetSimAdjBuilder
    RUNTIME DESTINATION bin
    COMPONENT ShipNetSimAdjBuilder_COMP)

# Conditionally include the server executable if BUILD_SERVER is ON
if(BUILD_SERVER)
    install(TARGETS ${SHIPNETSIM_SERVER_NAME}
        RUNTIME DESTINATION bin
        COMPONENT ShipNetSimServer_COMP
        LIBRARY DESTINATION lib
        COMPONENT ShipNetSimServer_COMP
        ARCHIVE DESTINATION lib/static
        COMPONENT ShipNetSimServer_COMP)

    install(TARGETS ShipNetSimRabbitMQConfig
        RUNTIME DESTINATION bin
        COMPONENT ShipNetSimServer_COMP)
endif()

# Conditionally include the GUI executable if BUILD_GUI is ON
if(BUILD_GUI)
    install(TARGETS ShipNetSimGUI
        RUNTIME DESTINATION bin
        COMPONENT ShipNetSimGUI_COMP
        LIBRARY DESTINATION lib
        COMPONENT ShipNetSimGUI_COMP
        ARCHIVE DESTINATION lib/static
        COMPONENT ShipNetSimGUI_COMP)
endif()

# Install necessary files for Windows
if(WIN32)
    # Get Qt parent directory for plugins and libs
    get_filename_component(QT_PARENT_DIR ${QT_BIN_DIR} DIRECTORY)

    # ========================================
    # VALIDATION: Check critical Qt DLLs exist
    # ========================================
    set(REQUIRED_QT_DLLS
        "${QT_BIN_DIR}/Qt6Core.dll"
        "${QT_BIN_DIR}/Qt6Network.dll"
        "${QT_BIN_DIR}/Qt6Sql.dll"
        "${QT_BIN_DIR}/Qt6Xml.dll"
        "${QT_BIN_DIR}/Qt6Concurrent.dll"
    )

foreach(QT_DLL ${REQUIRED_QT_DLLS})
    if(NOT EXISTS "${QT_DLL}")
        message(FATAL_ERROR "CRITICAL: Required Qt DLL not found: ${QT_DLL}")
    endif()
endforeach()

# ========================================
# VALIDATION: Check Qt plugins exist
# ========================================
if(NOT EXISTS "${QT_PARENT_DIR}/plugins/platforms/")
    message(FATAL_ERROR "CRITICAL: Qt platforms plugin directory not found: ${QT_PARENT_DIR}/plugins/platforms/")
endif()

if(NOT EXISTS "${QT_PARENT_DIR}/plugins/imageformats/")
    message(FATAL_ERROR "CRITICAL: Qt imageformats plugin directory not found: ${QT_PARENT_DIR}/plugins/imageformats/")
endif()

# ========================================
# CORE Qt6 DLL files (ESSENTIAL FOR ALL APPS)
# ========================================
filter_debug_dlls(REQUIRED_QT_DLLS FILTERED_QT_DLLS)
install(
    FILES ${FILTERED_QT_DLLS}
    DESTINATION bin
    COMPONENT ShipNetSimCore_COMP
)

# ========================================
# GUI-specific Qt6 DLL files
# ========================================
if(BUILD_GUI)
    set(REQUIRED_QT_GUI_DLLS
        "${QT_BIN_DIR}/Qt6Gui.dll"
        "${QT_BIN_DIR}/Qt6Widgets.dll"
    )

foreach(QT_GUI_DLL ${REQUIRED_QT_GUI_DLLS})
    if(NOT EXISTS "${QT_GUI_DLL}")
        message(FATAL_ERROR "CRITICAL: Required Qt GUI DLL not found: ${QT_GUI_DLL}")
    endif()
endforeach()

# Optional GUI DLLs (warn if missing but don't fail)
set(OPTIONAL_QT_GUI_DLLS
    "${QT_BIN_DIR}/Qt6OpenGL.dll"
    "${QT_BIN_DIR}/Qt6OpenGLWidgets.dll"
    "${QT_BIN_DIR}/Qt6PrintSupport.dll"
)

set(ALL_QT_GUI_DLLS ${REQUIRED_QT_GUI_DLLS})
foreach(OPTIONAL_DLL ${OPTIONAL_QT_GUI_DLLS})
    if(EXISTS "${OPTIONAL_DLL}")
        list(APPEND ALL_QT_GUI_DLLS "${OPTIONAL_DLL}")
    else()
        message(WARNING "Optional Qt GUI DLL not found: ${OPTIONAL_DLL}")
    endif()
endforeach()

filter_debug_dlls(ALL_QT_GUI_DLLS FILTERED_QT_GUI_DLLS)
install(
    FILES ${FILTERED_QT_GUI_DLLS}
    DESTINATION bin
    COMPONENT ShipNetSimGUI_COMP
)
endif()

# ========================================
# CRITICAL: Qt6 PLUGINS (REQUIRED FOR GUI APPS TO START)
# ========================================
# Platform plugins (ESSENTIAL)
install(
    DIRECTORY "${QT_PARENT_DIR}/plugins/platforms/"
    DESTINATION bin/plugins/platforms
    COMPONENT ShipNetSimCore_COMP
    FILES_MATCHING PATTERN "*.dll"
)

# Image format plugins (CRITICAL)
install(
    DIRECTORY "${QT_PARENT_DIR}/plugins/imageformats/"
    DESTINATION bin/plugins/imageformats
    COMPONENT ShipNetSimCore_COMP
    FILES_MATCHING PATTERN "*.dll"
)

# Optional plugin directories (warn if missing)
set(OPTIONAL_PLUGIN_DIRS "iconengines" "styles" "tls")
foreach(PLUGIN_DIR ${OPTIONAL_PLUGIN_DIRS})
    if(EXISTS "${QT_PARENT_DIR}/plugins/${PLUGIN_DIR}/")
        install(
            DIRECTORY "${QT_PARENT_DIR}/plugins/${PLUGIN_DIR}/"
            DESTINATION bin/plugins/${PLUGIN_DIR}
            COMPONENT ShipNetSimCore_COMP
            FILES_MATCHING PATTERN "*.dll"
        )
else()
    message(WARNING "Optional Qt plugin directory not found: ${QT_PARENT_DIR}/plugins/${PLUGIN_DIR}/")
endif()
endforeach()

# SQL drivers (REQUIRED)
if(NOT EXISTS "${QT_PARENT_DIR}/plugins/sqldrivers/")
    message(FATAL_ERROR "CRITICAL: Qt SQL drivers plugin directory not found: ${QT_PARENT_DIR}/plugins/sqldrivers/")
endif()
install(
    DIRECTORY "${QT_PARENT_DIR}/plugins/sqldrivers/"
    DESTINATION bin/plugins/sqldrivers
    COMPONENT ShipNetSimCore_COMP
    FILES_MATCHING PATTERN "*.dll"
)

# ========================================
# THIRD-PARTY LIBRARY DLLs (CRITICAL - REQUIRED FOR ALL TARGETS)
# ========================================

# GDAL DLLs (REQUIRED for ALL targets)
set(GDAL_DLL_PATHS "")
if(DEFINED GDAL_LIBRARY)
    get_filename_component(GDAL_DLL_DIR "${GDAL_LIBRARY}" DIRECTORY)
    list(APPEND GDAL_DLL_PATHS "${GDAL_DLL_DIR}/../bin" "${GDAL_DLL_DIR}")
endif()
if(DEFINED GDAL_INCLUDE_DIR)
    list(APPEND GDAL_DLL_PATHS "${GDAL_INCLUDE_DIR}/../bin" "${GDAL_INCLUDE_DIR}/../lib")
endif()
# Common GDAL installation paths
list(APPEND GDAL_DLL_PATHS
    "C:/Program Files/GDAL/bin"
    "C:/OSGeo4W64/bin"
    "C:/OSGeo4W/bin"
)

set(GDAL_FOUND FALSE)
foreach(GDAL_PATH ${GDAL_DLL_PATHS})
    get_filename_component(GDAL_BIN_DIR "${GDAL_PATH}" ABSOLUTE)
    if(EXISTS "${GDAL_BIN_DIR}")
        file(GLOB GDAL_DLLS
            "${GDAL_BIN_DIR}/gdal*.dll"
            "${GDAL_BIN_DIR}/proj*.dll"
            "${GDAL_BIN_DIR}/geos*.dll"
            "${GDAL_BIN_DIR}/sqlite3*.dll"
            "${GDAL_BIN_DIR}/expat*.dll"
            "${GDAL_BIN_DIR}/iconv*.dll"
            "${GDAL_BIN_DIR}/charset*.dll"
            "${GDAL_BIN_DIR}/curl*.dll"
            "${GDAL_BIN_DIR}/crypto*.dll"
            "${GDAL_BIN_DIR}/ssl*.dll"
            "${GDAL_BIN_DIR}/jpeg*.dll"
            "${GDAL_BIN_DIR}/png*.dll"
            "${GDAL_BIN_DIR}/tiff*.dll"
            "${GDAL_BIN_DIR}/zlib*.dll"
            "${GDAL_BIN_DIR}/lzma*.dll"
            "${GDAL_BIN_DIR}/hdf5*.dll"
            "${GDAL_BIN_DIR}/netcdf*.dll"
            "${GDAL_BIN_DIR}/openjp2*.dll"
        )
    if(GDAL_DLLS)
        message(STATUS "Found GDAL DLLs in: ${GDAL_BIN_DIR}")
        message(STATUS "GDAL DLLs: ${GDAL_DLLS}")
        filter_debug_dlls(GDAL_DLLS FILTERED_GDAL_DLLS)
        install(
            FILES ${FILTERED_GDAL_DLLS}
            DESTINATION bin
            COMPONENT ShipNetSimCore_COMP
        )
    set(GDAL_FOUND TRUE)
    break()
endif()
endif()
endforeach()

if(NOT GDAL_FOUND)
    message(FATAL_ERROR "CRITICAL: GDAL DLLs not found! Searched paths: ${GDAL_DLL_PATHS}")
endif()

# GeographicLib DLLs (REQUIRED for ALL targets)
set(GEO_DLL_PATHS "")
if(DEFINED GeographicLib_LIBRARY_DIRS)
    foreach(LIB_DIR ${GeographicLib_LIBRARY_DIRS})
        list(APPEND GEO_DLL_PATHS "${LIB_DIR}/../bin" "${LIB_DIR}")
    endforeach()
endif()
if(DEFINED GeographicLib_INCLUDE_DIRS)
    foreach(INC_DIR ${GeographicLib_INCLUDE_DIRS})
        list(APPEND GEO_DLL_PATHS "${INC_DIR}/../bin" "${INC_DIR}/../lib")
    endforeach()
endif()
# Common GeographicLib installation paths
list(APPEND GEO_DLL_PATHS
    "C:/Program Files/GeographicLib/bin"
    "C:/Program Files (x86)/GeographicLib/bin"
)

set(GEO_FOUND FALSE)
foreach(GEO_PATH ${GEO_DLL_PATHS})
    get_filename_component(GEO_BIN_DIR "${GEO_PATH}" ABSOLUTE)
    if(EXISTS "${GEO_BIN_DIR}")
        file(GLOB GEO_DLLS "${GEO_BIN_DIR}/*geographic*.dll" "${GEO_BIN_DIR}/*Geographic*.dll")
        if(GEO_DLLS)
            message(STATUS "Found GeographicLib DLLs in: ${GEO_BIN_DIR}")
            message(STATUS "GeographicLib DLLs: ${GEO_DLLS}")
            filter_debug_dlls(GEO_DLLS FILTERED_GEO_DLLS)
            install(
                FILES ${FILTERED_GEO_DLLS}
                DESTINATION bin
                COMPONENT ShipNetSimCore_COMP
            )
        set(GEO_FOUND TRUE)
        break()
    endif()
endif()
endforeach()

if(NOT GEO_FOUND)
    message(FATAL_ERROR "CRITICAL: GeographicLib DLLs not found! Searched paths: ${GEO_DLL_PATHS}")
endif()

# PROJ LIBRARY DLLs AND DATA FILES (REQUIRED FOR ALL TARGETS)
set(PROJ_DLL_PATHS "")
if(DEFINED PROJ_LIBRARY)
    get_filename_component(PROJ_DLL_DIR "${PROJ_LIBRARY}" DIRECTORY)
    list(APPEND PROJ_DLL_PATHS "${PROJ_DLL_DIR}/../bin" "${PROJ_DLL_DIR}")
endif()
if(DEFINED PROJ_INCLUDE_DIR)
    list(APPEND PROJ_DLL_PATHS "${PROJ_INCLUDE_DIR}/../bin" "${PROJ_INCLUDE_DIR}/../lib")
endif()
if(DEFINED PROJ_ROOT)
    list(APPEND PROJ_DLL_PATHS "${PROJ_ROOT}/bin")
endif()
# Common PROJ installation paths
list(APPEND PROJ_DLL_PATHS
    "C:/Program Files/PROJ/bin"
    "C:/Program Files (x86)/PROJ/bin"
    "C:/OSGeo4W64/bin"
    "C:/OSGeo4W/bin"
    "C:/ProgramData/Anaconda3/Library/bin"
    "C:/ProgramData/Miniconda3/Library/bin"
    "C:/Users/$ENV{USERNAME}/Anaconda3/Library/bin"
    "C:/Users/$ENV{USERNAME}/Miniconda3/Library/bin"
    "$ENV{CONDA_PREFIX}/Library/bin"
)

set(PROJ_FOUND FALSE)
foreach(PROJ_PATH ${PROJ_DLL_PATHS})
    get_filename_component(PROJ_BIN_DIR "${PROJ_PATH}" ABSOLUTE)
    if(EXISTS "${PROJ_BIN_DIR}")
        # Search for PROJ DLLs with version patterns
        file(GLOB PROJ_DLLS
            "${PROJ_BIN_DIR}/proj*.dll"
            "${PROJ_BIN_DIR}/proj_*.dll"
        )

        # Search for PROJ data files
        file(GLOB PROJ_DATA_FILES
            "${PROJ_BIN_DIR}/proj.db"
        )

        if(PROJ_DLLS)
            message(STATUS "Found PROJ DLLs in: ${PROJ_BIN_DIR}")
            message(STATUS "PROJ DLLs: ${PROJ_DLLS}")
            filter_debug_dlls(PROJ_DLLS FILTERED_PROJ_DLLS)
            install(
                FILES ${FILTERED_PROJ_DLLS}
                DESTINATION bin
                COMPONENT ShipNetSimCore_COMP
            )

            # Install PROJ data files if found
            if(PROJ_DATA_FILES)
                message(STATUS "Found PROJ data files: ${PROJ_DATA_FILES}")
                install(
                    FILES ${PROJ_DATA_FILES}
                    DESTINATION bin
                    COMPONENT ShipNetSimCore_COMP
                )
            else()
                message(WARNING "PROJ data files (proj.db) not found in: ${PROJ_BIN_DIR}")
            endif()

            # Check for PROJ share directory (contains additional data files)
            get_filename_component(PROJ_PARENT_DIR "${PROJ_BIN_DIR}" DIRECTORY)
            set(PROJ_SHARE_DIR "${PROJ_PARENT_DIR}/share/proj")
            if(EXISTS "${PROJ_SHARE_DIR}")
                message(STATUS "Found PROJ share directory: ${PROJ_SHARE_DIR}")
                install(
                    DIRECTORY "${PROJ_SHARE_DIR}/"
                    DESTINATION share/proj
                    COMPONENT ShipNetSimCore_COMP
                    FILES_MATCHING
                    PATTERN "*.db"
                    PATTERN "*.gsb"
                    PATTERN "*.gtx"
                    PATTERN "*.json"
                )
            else()
                message(STATUS "PROJ share directory not found at: ${PROJ_SHARE_DIR}")
            endif()

            set(PROJ_FOUND TRUE)
            break()
        endif()
    endif()
endforeach()

if(NOT PROJ_FOUND)
    message(FATAL_ERROR "CRITICAL: PROJ DLLs not found! Searched paths: ${PROJ_DLL_PATHS}")
endif()

# ========================================
# SERVER-SPECIFIC DLL FILES (REQUIRED IF BUILD_SERVER=ON)
# ========================================
if(BUILD_SERVER)
    # Container library DLLs
    set(CONTAINER_FOUND FALSE)
    if(DEFINED CONTAINER_BIN_DIR AND EXISTS "${CONTAINER_BIN_DIR}")
        file(GLOB CONTAINER_DLLS
            "${CONTAINER_BIN_DIR}/container*.dll"
            "${CONTAINER_BIN_DIR}/Container*.dll"
        )
    if(CONTAINER_DLLS)
        message(STATUS "Found Container DLLs: ${CONTAINER_DLLS}")
        filter_debug_dlls(CONTAINER_DLLS FILTERED_CONTAINER_DLLS)
        install(
            FILES ${FILTERED_CONTAINER_DLLS}
            DESTINATION bin
            COMPONENT ShipNetSimServer_COMP
        )
    set(CONTAINER_FOUND TRUE)
endif()
endif()

if(NOT CONTAINER_FOUND)
    message(FATAL_ERROR "CRITICAL: Container DLLs not found! CONTAINER_BIN_DIR=${CONTAINER_BIN_DIR}")
endif()

# RabbitMQ DLL
set(RABBITMQ_FOUND FALSE)
if(DEFINED RABBITMQ_BIN_DIR AND EXISTS "${RABBITMQ_BIN_DIR}")
    file(GLOB RABBITMQ_DLLS
        "${RABBITMQ_BIN_DIR}/rabbitmq*.dll"
        "${RABBITMQ_BIN_DIR}/librabbitmq*.dll"
    )
if(RABBITMQ_DLLS)
    message(STATUS "Found RabbitMQ DLLs: ${RABBITMQ_DLLS}")
    filter_debug_dlls(RABBITMQ_DLLS FILTERED_RABBITMQ_DLLS)
    install(
        FILES ${FILTERED_RABBITMQ_DLLS}
        DESTINATION bin
        COMPONENT ShipNetSimServer_COMP
    )
set(RABBITMQ_FOUND TRUE)
endif()
endif()

if(NOT RABBITMQ_FOUND)
    message(FATAL_ERROR "CRITICAL: RabbitMQ DLLs not found! RABBITMQ_BIN_DIR=${RABBITMQ_BIN_DIR}")
endif()

    # Qt GUI DLLs for ShipNetSimRabbitMQConfig (only needed when GUI is not being built)
    if(NOT BUILD_GUI)
        set(SERVER_QT_GUI_DLLS
            "${QT_BIN_DIR}/Qt6Gui.dll"
            "${QT_BIN_DIR}/Qt6Widgets.dll"
        )
        foreach(QT_GUI_DLL ${SERVER_QT_GUI_DLLS})
            if(NOT EXISTS "${QT_GUI_DLL}")
                message(FATAL_ERROR "CRITICAL: Required Qt GUI DLL for ShipNetSimRabbitMQConfig not found: ${QT_GUI_DLL}")
            endif()
        endforeach()
        filter_debug_dlls(SERVER_QT_GUI_DLLS FILTERED_SERVER_QT_GUI_DLLS)
        install(
            FILES ${FILTERED_SERVER_QT_GUI_DLLS}
            DESTINATION bin
            COMPONENT ShipNetSimServer_COMP
        )
    endif()
endif()

# ========================================
# GUI-SPECIFIC DLLs (REQUIRED IF BUILD_GUI=ON)
# ========================================
if(BUILD_GUI)
    # OpenSceneGraph DLLs (REQUIRED)
    set(OSG_DLL_PATHS "")
    if(DEFINED OpenSceneGraph_DIR)
        list(APPEND OSG_DLL_PATHS "${OpenSceneGraph_DIR}/../bin" "${OpenSceneGraph_DIR}/bin")
    endif()
    if(DEFINED OSG_ROOT)
        list(APPEND OSG_DLL_PATHS "${OSG_ROOT}/bin")
    endif()
    # Common OSG installation paths
    list(APPEND OSG_DLL_PATHS
        "C:/Program Files/OpenSceneGraph/bin"
        "C:/Program Files (x86)/OpenSceneGraph/bin"
        "C:/OpenSceneGraph/bin"
    )

    set(OSG_FOUND FALSE)
    foreach(OSG_PATH ${OSG_DLL_PATHS})
        get_filename_component(OSG_BIN_DIR "${OSG_PATH}" ABSOLUTE)
        if(EXISTS "${OSG_BIN_DIR}")
            file(GLOB OSG_DLLS
                "${OSG_BIN_DIR}/osg*.dll"
                "${OSG_BIN_DIR}/ot*.dll"
                "${OSG_BIN_DIR}/OpenThreads*.dll"
            )
            if(OSG_DLLS)
                message(STATUS "Found OpenSceneGraph DLLs in: ${OSG_BIN_DIR}")
                message(STATUS "OSG DLLs: ${OSG_DLLS}")
                filter_debug_dlls(OSG_DLLS FILTERED_OSG_DLLS)
                install(
                    FILES ${FILTERED_OSG_DLLS}
                    DESTINATION bin
                    COMPONENT ShipNetSimGUI_COMP
                )

                # CORRECTED: OpenSceneGraph plugins - copy individual DLL files directly to bin
                file(GLOB OSG_PLUGIN_DIRS "${OSG_BIN_DIR}/osgPlugins-*")
                if(NOT OSG_PLUGIN_DIRS)
                    message(FATAL_ERROR "CRITICAL: OpenSceneGraph plugins directory not found in: ${OSG_BIN_DIR}")
                endif()

                foreach(PLUGIN_DIR ${OSG_PLUGIN_DIRS})
                    if(IS_DIRECTORY "${PLUGIN_DIR}")
                        get_filename_component(PLUGIN_DIR_NAME "${PLUGIN_DIR}" NAME)
                        message(STATUS "Installing OSG plugin DLLs directly to bin from: ${PLUGIN_DIR}")

                        # Find all DLL files in the plugin directory
                        file(GLOB OSG_PLUGIN_DLLS "${PLUGIN_DIR}/*.dll")
                        if(OSG_PLUGIN_DLLS)
                            # Install individual plugin DLL files directly to bin (no subdirectory)
                            filter_debug_dlls(OSG_PLUGIN_DLLS FILTERED_OSG_PLUGIN_DLLS)
                            install(
                                FILES ${FILTERED_OSG_PLUGIN_DLLS}
                                DESTINATION bin
                                COMPONENT ShipNetSimGUI_COMP
                            )
                            list(LENGTH OSG_PLUGIN_DLLS OSG_PLUGIN_COUNT)
                            message(STATUS "Installed ${OSG_PLUGIN_COUNT} OSG plugin DLLs: ${OSG_PLUGIN_DLLS}")
                        else()
                            message(WARNING "No DLL files found in OSG plugin directory: ${PLUGIN_DIR}")
                        endif()
                    endif()
                endforeach()

                set(OSG_FOUND TRUE)
                break()
            endif()
        endif()
    endforeach()

    if(NOT OSG_FOUND)
        message(FATAL_ERROR "CRITICAL: OpenSceneGraph DLLs not found! Searched paths: ${OSG_DLL_PATHS}")
    endif()

    # osgEarth DLLs (REQUIRED)
    set(OSGEARTH_DLL_PATHS "")
    if(DEFINED osgEarth_DIR)
        list(APPEND OSGEARTH_DLL_PATHS "${osgEarth_DIR}/../bin" "${osgEarth_DIR}/bin")
    endif()
    # Common osgEarth installation paths
    list(APPEND OSGEARTH_DLL_PATHS
        "C:/Program Files/OSGEarth/bin"
        "C:/Program Files (x86)/OSGEarth/bin"
        "C:/OSGEarth/bin"
    )

    set(OSGEARTH_FOUND FALSE)
    foreach(OSGEARTH_PATH ${OSGEARTH_DLL_PATHS})
        get_filename_component(OSGEARTH_BIN_DIR "${OSGEARTH_PATH}" ABSOLUTE)
        if(EXISTS "${OSGEARTH_BIN_DIR}")
            file(GLOB OSGEARTH_DLLS "${OSGEARTH_BIN_DIR}/osgEarth*.dll")
            if(OSGEARTH_DLLS)
                message(STATUS "Found osgEarth DLLs in: ${OSGEARTH_BIN_DIR}")
                message(STATUS "osgEarth DLLs: ${OSGEARTH_DLLS}")
                filter_debug_dlls(OSGEARTH_DLLS FILTERED_OSGEARTH_DLLS)
                install(
                    FILES ${FILTERED_OSGEARTH_DLLS}
                    DESTINATION bin
                    COMPONENT ShipNetSimGUI_COMP
                )

                # CORRECTED: osgEarth plugins - copy individual DLL files directly to bin
                file(GLOB OSGEARTH_PLUGIN_DIRS "${OSGEARTH_BIN_DIR}/osgPlugins-*")
                if(OSGEARTH_PLUGIN_DIRS)
                    foreach(PLUGIN_DIR ${OSGEARTH_PLUGIN_DIRS})
                        if(IS_DIRECTORY "${PLUGIN_DIR}")
                            get_filename_component(PLUGIN_DIR_NAME "${PLUGIN_DIR}" NAME)
                            message(STATUS "Installing osgEarth plugin DLLs directly to bin from: ${PLUGIN_DIR}")

                            # Find all DLL files in the plugin directory
                            file(GLOB OSGEARTH_PLUGIN_DLLS "${PLUGIN_DIR}/*.dll")
                            if(OSGEARTH_PLUGIN_DLLS)
                                # Install individual plugin DLL files directly to bin (no subdirectory)
                                filter_debug_dlls(OSGEARTH_PLUGIN_DLLS FILTERED_OSGEARTH_PLUGIN_DLLS)
                                install(
                                    FILES ${FILTERED_OSGEARTH_PLUGIN_DLLS}
                                    DESTINATION bin
                                    COMPONENT ShipNetSimGUI_COMP
                                )
                                list(LENGTH OSGEARTH_PLUGIN_DLLS OSGEARTH_PLUGIN_COUNT)
                                message(STATUS "Installed ${OSGEARTH_PLUGIN_COUNT} osgEarth plugin DLLs: ${OSGEARTH_PLUGIN_DLLS}")
                            else()
                                message(WARNING "No DLL files found in osgEarth plugin directory: ${PLUGIN_DIR}")
                            endif()
                        endif()
                    endforeach()
                else()
                    message(STATUS "No osgEarth plugin directories found in: ${OSGEARTH_BIN_DIR}")
                endif()

                set(OSGEARTH_FOUND TRUE)
                break()
            endif()
        endif()
    endforeach()

    if(NOT OSGEARTH_FOUND)
        message(FATAL_ERROR "CRITICAL: osgEarth DLLs not found! Searched paths: ${OSGEARTH_DLL_PATHS}")
    endif()

    # OpenGL software rendering support (OPTIONAL)
    find_file(OPENGL_SOFTWARE_RENDERER
        NAMES "opengl32sw.dll" "d3dcompiler_47.dll"
        PATHS "${QT_BIN_DIR}" "C:/Windows/System32"
        DOC "OpenGL software renderer DLL"
    )
    if(OPENGL_SOFTWARE_RENDERER)
        message(STATUS "Found OpenGL software renderer: ${OPENGL_SOFTWARE_RENDERER}")
        install(
            FILES "${OPENGL_SOFTWARE_RENDERER}"
            DESTINATION bin
            COMPONENT ShipNetSimGUI_COMP
        )
    else()
        message(WARNING "OpenGL software renderer not found - GUI may not work on systems without proper graphics drivers")
    endif()

# OpenGL software rendering support (OPTIONAL)
find_file(OPENGL_SOFTWARE_RENDERER
    NAMES "opengl32sw.dll" "d3dcompiler_47.dll"
    PATHS "${QT_BIN_DIR}" "C:/Windows/System32"
    DOC "OpenGL software renderer DLL"
)
if(OPENGL_SOFTWARE_RENDERER)
    message(STATUS "Found OpenGL software renderer: ${OPENGL_SOFTWARE_RENDERER}")
    install(
        FILES "${OPENGL_SOFTWARE_RENDERER}"
        DESTINATION bin
        COMPONENT ShipNetSimGUI_COMP
    )
else()
    message(WARNING "OpenGL software renderer not found - GUI may not work on systems without proper graphics drivers")
endif()
endif()


# ========================================
# USER-CONFIGURABLE DLL SEARCH PATHS
# ========================================
# Allow users to specify additional search directories for missing DLLs
set(ADDITIONAL_DLL_SEARCH_PATHS "" CACHE STRING
    "Additional directories to search for missing DLL files (semicolon-separated list)")

# Define comprehensive list of missing DLL files with their expected locations
set(MISSING_DLLS_LIST
    # Anaconda DLLs
    "zlib.dll"
    "archive.dll"
    "libcurl.dll"
    "libxml2.dll"
    "libcrypto-1_1-x64.dll"
    "libssl-1_1-x64.dll"
    "liblzma.dll"
    "libdeflate.dll"
    "liblz4.dll"
    "blosc.dll"
    "zstd.dll"
    "Lerc.dll"
    "iconv.dll"
    "LIBBZ2.dll"
    "charset.dll"
    "libssh2.dll"
    "sqlite3.dll"
    "tiff.dll"
    # GEOS DLLs
    "geos_c.dll"
    "geos.dll"
    # Visual C++ Runtime DLLs
    "VCRUNTIME140D.dll"
    "ucrtbased.dll"
    # Additional graphics/image DLLs
    "freetype.dll"
    "libpng16.dll"
)

# Standard search paths for missing DLLs
set(STANDARD_DLL_SEARCH_PATHS
    # Anaconda paths
    "C:/ProgramData/Anaconda3/Library/bin"
    "C:/ProgramData/Miniconda3/Library/bin"
    "C:/Users/$ENV{USERNAME}/Anaconda3/Library/bin"
    "C:/Users/$ENV{USERNAME}/Miniconda3/Library/bin"
    "$ENV{CONDA_PREFIX}/Library/bin"
    # GEOS paths
    "C:/Program Files/GEOS/bin"
    "C:/Program Files (x86)/GEOS/bin"
    # Windows system paths
    "C:/Windows/System32"
    "C:/Windows/SysWOW64"
    # Visual Studio paths
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Redist/MSVC"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Redist/MSVC"
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Redist/MSVC"
    "C:/Program Files/Microsoft Visual Studio/2019/Community/VC/Redist/MSVC"
)

# Combine user-specified and standard search paths
set(ALL_DLL_SEARCH_PATHS ${ADDITIONAL_DLL_SEARCH_PATHS} ${STANDARD_DLL_SEARCH_PATHS})

# ========================================
# COMPREHENSIVE MISSING DLL DETECTION AND INSTALLATION
# ========================================
function(find_and_install_missing_dlls)
    set(FOUND_DLLS "")
    set(MISSING_DLLS "")

    message(STATUS "=== Searching for Missing DLL Files ===")

    foreach(DLL_NAME ${MISSING_DLLS_LIST})
        set(DLL_FOUND FALSE)
        set(DLL_PATH "")

        # Search in all specified paths
        foreach(SEARCH_PATH ${ALL_DLL_SEARCH_PATHS})
            # Handle Visual Studio redist paths specially (they have subdirectories)
            if(SEARCH_PATH MATCHES ".*VC.*Redist.*MSVC.*")
                file(GLOB VS_SUBDIRS "${SEARCH_PATH}/*")
                foreach(VS_SUBDIR ${VS_SUBDIRS})
                    if(IS_DIRECTORY "${VS_SUBDIR}")
                        set(VS_SEARCH_PATHS
                            "${VS_SUBDIR}/x64/Microsoft.VC143.CRT"
                            "${VS_SUBDIR}/x64/Microsoft.VC142.CRT"
                            "${VS_SUBDIR}/x64/Microsoft.VC141.CRT"
                        )
                        foreach(VS_PATH ${VS_SEARCH_PATHS})
                            if(EXISTS "${VS_PATH}/${DLL_NAME}")
                                set(DLL_PATH "${VS_PATH}/${DLL_NAME}")
                                set(DLL_FOUND TRUE)
                                break()
                            endif()
                        endforeach()
                        if(DLL_FOUND)
                            break()
                        endif()
                    endif()
                endforeach()
            else()
                # Standard path search
                if(EXISTS "${SEARCH_PATH}/${DLL_NAME}")
                    set(DLL_PATH "${SEARCH_PATH}/${DLL_NAME}")
                    set(DLL_FOUND TRUE)
                    break()
                endif()
            endif()
        endforeach()

        if(DLL_FOUND)
            list(APPEND FOUND_DLLS "${DLL_PATH}")
            message(STATUS "✓ Found: ${DLL_NAME} at ${DLL_PATH}")
        else()
            list(APPEND MISSING_DLLS "${DLL_NAME}")
            message(WARNING "✗ Missing: ${DLL_NAME}")
        endif()
    endforeach()

    # Install found DLLs
    if(FOUND_DLLS)
        # FIXED: Calculate length first, then use in message
        list(LENGTH FOUND_DLLS FOUND_DLLS_COUNT)
        message(STATUS "Installing ${FOUND_DLLS_COUNT} DLL files...")
        filter_debug_dlls(FOUND_DLLS FILTERED_FOUND_DLLS)
        install(
            FILES ${FILTERED_FOUND_DLLS}
            DESTINATION bin
            COMPONENT ShipNetSimCore_COMP
        )
    endif()

    # Report summary
    list(LENGTH FOUND_DLLS FOUND_COUNT)
    list(LENGTH MISSING_DLLS MISSING_COUNT)
    message(STATUS "=== DLL Search Summary ===")
    message(STATUS "Found: ${FOUND_COUNT} DLLs")
    message(STATUS "Missing: ${MISSING_COUNT} DLLs")

    if(MISSING_DLLS)
        message(STATUS "Missing DLLs: ${MISSING_DLLS}")
        message(STATUS "To resolve missing DLLs:")
        message(STATUS "1. Install the required software (GEOS, etc.)")
        message(STATUS "2. Add custom search paths using: -DADDITIONAL_DLL_SEARCH_PATHS=\"path1;path2\"")
        message(STATUS "3. Manually copy missing DLLs to your project's bin directory")
    endif()
    message(STATUS "========================")
endfunction()

# Call the function to find and install missing DLLs
find_and_install_missing_dlls()

# ========================================
# VISUAL C++ RUNTIME LIBRARIES (WARN IF MISSING)
# ========================================
set(VCREDIST_SEARCH_PATHS
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Redist/MSVC"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Redist/MSVC"
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Redist/MSVC"
    "C:/Program Files/Microsoft Visual Studio/2019/Community/VC/Redist/MSVC"
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/Professional/VC/Redist/MSVC"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Redist/MSVC"
)

set(VCREDIST_FOUND FALSE)
set(VCREDIST_INSTALLER_PATH "")
foreach(VCREDIST_BASE ${VCREDIST_SEARCH_PATHS})
    if(EXISTS "${VCREDIST_BASE}")
        file(GLOB VCREDIST_SUBDIRS "${VCREDIST_BASE}/*")
        if(VCREDIST_SUBDIRS)
            list(GET VCREDIST_SUBDIRS -1 LATEST_VCREDIST)
            set(VCREDIST_PATHS
                "${LATEST_VCREDIST}/x64/Microsoft.VC143.CRT"
                "${LATEST_VCREDIST}/x64/Microsoft.VC142.CRT"
                "${LATEST_VCREDIST}/x64/Microsoft.VC141.CRT"
            )
        # Look for redistributable installer
        file(GLOB VCREDIST_INSTALLER "${LATEST_VCREDIST}/vcredist_x64.exe")
        if(VCREDIST_INSTALLER)
            set(VCREDIST_INSTALLER_PATH ${VCREDIST_INSTALLER})
        endif()

        foreach(VCREDIST_PATH ${VCREDIST_PATHS})
            if(EXISTS "${VCREDIST_PATH}")
                file(GLOB VCREDIST_DLLS "${VCREDIST_PATH}/*.dll")
                if(VCREDIST_DLLS)
                    message(STATUS "Found VC++ Redistributable DLLs in: ${VCREDIST_PATH}")
                    filter_debug_dlls(VCREDIST_DLLS FILTERED_VCREDIST_DLLS)
                    install(
                        FILES ${FILTERED_VCREDIST_DLLS}
                        DESTINATION bin
                        COMPONENT ShipNetSimCore_COMP
                    )
                set(VCREDIST_FOUND TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(VCREDIST_FOUND)
        break()
    endif()
endif()
endif()
endforeach()

if(NOT VCREDIST_FOUND)
    message(WARNING "Visual C++ Redistributable DLLs not found. Target machines may need VC++ Redist installed.")
endif()

# Install VC++ Redistributable installer if found
if(VCREDIST_INSTALLER_PATH)
    message(STATUS "Found VC++ Redistributable installer: ${VCREDIST_INSTALLER_PATH}")
    install(FILES ${VCREDIST_INSTALLER_PATH}
        DESTINATION .
        COMPONENT ShipNetSimCore_COMP)
endif()

# ========================================
# OPTIONAL: ICU DLLs
# ========================================
file(GLOB ICU_DLLS "${QT_BIN_DIR}/icu*.dll")
if(ICU_DLLS)
    message(STATUS "Found ICU DLLs: ${ICU_DLLS}")
    filter_debug_dlls(ICU_DLLS FILTERED_ICU_DLLS)
    install(
        FILES ${FILTERED_ICU_DLLS}
        DESTINATION bin
        COMPONENT ShipNetSimCore_COMP
    )
else()
    message(STATUS "ICU DLLs not found (this is usually OK)")
endif()

# ========================================
# SUMMARY MESSAGE
# ========================================
message(STATUS "=== DLL Detection Summary ===")
message(STATUS "GDAL: ${GDAL_FOUND}")
message(STATUS "GeographicLib: ${GEO_FOUND}")
if(BUILD_SERVER)
    message(STATUS "Container: ${CONTAINER_FOUND}")
    message(STATUS "RabbitMQ: ${RABBITMQ_FOUND}")
endif()
if(BUILD_GUI)
    message(STATUS "OpenSceneGraph: ${OSG_FOUND}")
    message(STATUS "osgEarth: ${OSGEARTH_FOUND}")
endif()
message(STATUS "VC++ Redistributable: ${VCREDIST_FOUND}")
message(STATUS "============================")

elseif(UNIX AND NOT APPLE)
    # ========================================
    # LINUX: Shared Library (.so) Bundling
    # ========================================
    # Use file(GET_RUNTIME_DEPENDENCIES) at install time to discover and bundle
    # all .so dependencies automatically.

    # Collect all library search directories for dependency resolution
    set(_LIB_SEARCH_DIRS "${QT_LIB_DIR}")
    # Add build-type-specific lib dir (GDAL, GeographicLib, OSG, osgEarth, etc.)
    if(EXISTS "/usr/local/${CMAKE_BUILD_TYPE}/lib")
        list(APPEND _LIB_SEARCH_DIRS "/usr/local/${CMAKE_BUILD_TYPE}/lib")
    endif()
    list(APPEND _LIB_SEARCH_DIRS "/usr/local/lib")
    # Add OSG lib dir if available
    if(DEFINED OSG_DIR)
        list(APPEND _LIB_SEARCH_DIRS "${OSG_DIR}/lib")
    endif()
    # Add osgEarth lib dir if available
    if(DEFINED osgEarth_DIR)
        get_filename_component(_OSGEARTH_PREFIX "${osgEarth_DIR}/../../.." ABSOLUTE)
        list(APPEND _LIB_SEARCH_DIRS "${_OSGEARTH_PREFIX}/lib")
    endif()
    # Add GDAL lib dir if available
    if(DEFINED GDAL_DIR)
        get_filename_component(_GDAL_PREFIX "${GDAL_DIR}/../../.." ABSOLUTE)
        list(APPEND _LIB_SEARCH_DIRS "${_GDAL_PREFIX}/lib")
    endif()
    list(REMOVE_DUPLICATES _LIB_SEARCH_DIRS)

    # Build the DIRECTORIES string for file(GET_RUNTIME_DEPENDENCIES)
    set(_DIRS_ARGS "")
    foreach(_dir ${_LIB_SEARCH_DIRS})
        string(APPEND _DIRS_ARGS "                \"${_dir}\"\n")
    endforeach()
    message(STATUS "Library search directories for dependency resolution: ${_LIB_SEARCH_DIRS}")

    # Core dependencies (ShipNetSim, ShipNetSimAdjBuilder, ShipNetSimCore)
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES
                \$<TARGET_FILE:${SHIPNETSIM_NAME}>
                \$<TARGET_FILE:ShipNetSimAdjBuilder>
            LIBRARIES
                \$<TARGET_FILE:${SHIPNETSIM_CORE_DLL_NAME}>
            RESOLVED_DEPENDENCIES_VAR _resolved
            UNRESOLVED_DEPENDENCIES_VAR _unresolved
            PRE_EXCLUDE_REGEXES
                \"^libc\\\\.so\" \"^libm\\\\.so\" \"^libpthread\" \"^libdl\\\\.so\"
                \"^librt\\\\.so\" \"^ld-linux\" \"^libstdc\\\\+\\\\+\" \"^libgcc_s\"
                \"^libX\" \"^libxcb\" \"^libGL\" \"^libEGL\" \"^libdrm\"
            DIRECTORIES
${_DIRS_ARGS}        )
        foreach(dep \${_resolved})
            file(INSTALL \${dep} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib
                 FOLLOW_SYMLINK_CHAIN)
        endforeach()
        if(_unresolved)
            message(STATUS \"Unresolved core dependencies (may be system libs): \${_unresolved}\")
        endif()
    " COMPONENT ShipNetSimCore_COMP)

    # GUI dependencies
    if(BUILD_GUI)
        install(CODE "
            file(GET_RUNTIME_DEPENDENCIES
                EXECUTABLES
                    \$<TARGET_FILE:ShipNetSimGUI>
                RESOLVED_DEPENDENCIES_VAR _resolved_gui
                UNRESOLVED_DEPENDENCIES_VAR _unresolved_gui
                PRE_EXCLUDE_REGEXES
                    \"^libc\\\\.so\" \"^libm\\\\.so\" \"^libpthread\" \"^libdl\\\\.so\"
                    \"^librt\\\\.so\" \"^ld-linux\" \"^libstdc\\\\+\\\\+\" \"^libgcc_s\"
                    \"^libX\" \"^libxcb\" \"^libGL\" \"^libEGL\" \"^libdrm\"
                DIRECTORIES
${_DIRS_ARGS}            )
            foreach(dep \${_resolved_gui})
                file(INSTALL \${dep} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib
                     FOLLOW_SYMLINK_CHAIN)
            endforeach()
        " COMPONENT ShipNetSimGUI_COMP)
    endif()

    # Server dependencies
    if(BUILD_SERVER)
        install(CODE "
            file(GET_RUNTIME_DEPENDENCIES
                EXECUTABLES
                    \$<TARGET_FILE:${SHIPNETSIM_SERVER_NAME}>
                    \$<TARGET_FILE:ShipNetSimRabbitMQConfig>
                RESOLVED_DEPENDENCIES_VAR _resolved_srv
                UNRESOLVED_DEPENDENCIES_VAR _unresolved_srv
                PRE_EXCLUDE_REGEXES
                    \"^libc\\\\.so\" \"^libm\\\\.so\" \"^libpthread\" \"^libdl\\\\.so\"
                    \"^librt\\\\.so\" \"^ld-linux\" \"^libstdc\\\\+\\\\+\" \"^libgcc_s\"
                    \"^libX\" \"^libxcb\" \"^libGL\" \"^libEGL\" \"^libdrm\"
                DIRECTORIES
${_DIRS_ARGS}            )
            foreach(dep \${_resolved_srv})
                file(INSTALL \${dep} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib
                     FOLLOW_SYMLINK_CHAIN)
            endforeach()
        " COMPONENT ShipNetSimServer_COMP)
    endif()

    # ========================================
    # LINUX: Qt Plugins
    # ========================================
    if(DEFINED QT_PLUGINS_DIR)
        # Platform plugins (essential)
        install(DIRECTORY "${QT_PLUGINS_DIR}/platforms/"
            DESTINATION plugins/platforms
            COMPONENT ShipNetSimCore_COMP
            FILES_MATCHING PATTERN "*.so")

        # Image format plugins
        install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats/"
            DESTINATION plugins/imageformats
            COMPONENT ShipNetSimCore_COMP
            FILES_MATCHING PATTERN "*.so")

        # SQL driver plugins
        if(EXISTS "${QT_PLUGINS_DIR}/sqldrivers/")
            install(DIRECTORY "${QT_PLUGINS_DIR}/sqldrivers/"
                DESTINATION plugins/sqldrivers
                COMPONENT ShipNetSimCore_COMP
                FILES_MATCHING PATTERN "*.so")
        endif()

        # Optional plugins
        foreach(PLUGIN_DIR "tls" "iconengines" "xcbglintegrations" "wayland-shell-integration")
            if(EXISTS "${QT_PLUGINS_DIR}/${PLUGIN_DIR}/")
                install(DIRECTORY "${QT_PLUGINS_DIR}/${PLUGIN_DIR}/"
                    DESTINATION plugins/${PLUGIN_DIR}
                    COMPONENT ShipNetSimCore_COMP
                    FILES_MATCHING PATTERN "*.so")
            endif()
        endforeach()
    else()
        message(WARNING "QT_PLUGINS_DIR not defined - Qt plugins will not be bundled")
    endif()

    # ========================================
    # LINUX: Qt Plugin Dependencies
    # ========================================
    # Qt plugins (e.g., libqxcb.so) have their own dependencies (e.g., libQt6XcbQpa.so.6)
    # that are NOT direct dependencies of our executables. We must resolve and bundle these
    # separately. Also bundle libxcb-cursor which is excluded by the ^libxcb regex but is
    # required by the xcb platform plugin from Qt 6.5+.
    if(DEFINED QT_PLUGINS_DIR)
        install(CODE "
            # Gather source plugin .so files (not installed copies, to avoid RPATH conflicts)
            file(GLOB_RECURSE _plugin_libs
                \"${QT_PLUGINS_DIR}/platforms/*.so\"
                \"${QT_PLUGINS_DIR}/imageformats/*.so\"
                \"${QT_PLUGINS_DIR}/sqldrivers/*.so\"
                \"${QT_PLUGINS_DIR}/tls/*.so\"
                \"${QT_PLUGINS_DIR}/iconengines/*.so\"
                \"${QT_PLUGINS_DIR}/xcbglintegrations/*.so\"
                \"${QT_PLUGINS_DIR}/wayland-shell-integration/*.so\")

            if(_plugin_libs)
                file(GET_RUNTIME_DEPENDENCIES
                    LIBRARIES \${_plugin_libs}
                    RESOLVED_DEPENDENCIES_VAR _resolved_plugin_deps
                    UNRESOLVED_DEPENDENCIES_VAR _unresolved_plugin_deps
                    PRE_EXCLUDE_REGEXES
                        \"^libc\\\\.so\" \"^libm\\\\.so\" \"^libpthread\" \"^libdl\\\\.so\"
                        \"^librt\\\\.so\" \"^ld-linux\" \"^libstdc\\\\+\\\\+\" \"^libgcc_s\"
                        \"^libX\" \"^libGL\" \"^libEGL\" \"^libdrm\"
                    DIRECTORIES
${_DIRS_ARGS}                )
                foreach(dep \${_resolved_plugin_deps})
                    # Skip if already installed
                    get_filename_component(_depname \"\${dep}\" NAME)
                    if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/lib/\${_depname}\")
                        file(INSTALL \${dep} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib
                             FOLLOW_SYMLINK_CHAIN)
                    endif()
                endforeach()
                if(_unresolved_plugin_deps)
                    message(STATUS \"Unresolved plugin deps (may be system libs): \${_unresolved_plugin_deps}\")
                endif()
            endif()

            # Bundle libxcb-cursor (needed by Qt xcb platform plugin from 6.5+)
            # The plugin dep resolution above should catch it, but ensure it as a fallback
            if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/lib/libxcb-cursor.so.0\")
                foreach(_xcb_search_dir /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/lib)
                    file(GLOB _xcb_cursor_libs \"\${_xcb_search_dir}/libxcb-cursor.so*\")
                    if(_xcb_cursor_libs)
                        foreach(_xcb_lib \${_xcb_cursor_libs})
                            file(INSTALL \${_xcb_lib} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib
                                 FOLLOW_SYMLINK_CHAIN)
                        endforeach()
                        break()
                    endif()
                endforeach()
            endif()
        " COMPONENT ShipNetSimCore_COMP)
    endif()

    # ========================================
    # LINUX: PROJ Data Files
    # ========================================
    find_path(PROJ_DATA_DIR proj.db
        PATHS /usr/share/proj /usr/local/share/proj
              /usr/local/${CMAKE_BUILD_TYPE}/share/proj
        NO_DEFAULT_PATH)
    if(PROJ_DATA_DIR)
        message(STATUS "Found PROJ data directory: ${PROJ_DATA_DIR}")
        install(DIRECTORY "${PROJ_DATA_DIR}/"
            DESTINATION share/proj
            COMPONENT ShipNetSimCore_COMP
            FILES_MATCHING
            PATTERN "*.db"
            PATTERN "*.json"
            PATTERN "*.gsb"
            PATTERN "*.gtx")
    else()
        message(WARNING "PROJ data directory (proj.db) not found")
    endif()

    # ========================================
    # LINUX: GUI-Specific Libraries (OSG, osgEarth)
    # ========================================
    if(BUILD_GUI)
        # OpenSceneGraph plugin .so files
        set(OSG_PLUGIN_SEARCH_PATHS "")
        if(DEFINED OpenSceneGraph_DIR)
            get_filename_component(OSG_PREFIX "${OpenSceneGraph_DIR}/../../.." ABSOLUTE)
            list(APPEND OSG_PLUGIN_SEARCH_PATHS "${OSG_PREFIX}/lib")
        endif()
        list(APPEND OSG_PLUGIN_SEARCH_PATHS
            "/usr/local/${CMAKE_BUILD_TYPE}/lib"
            "/usr/local/lib"
            "/usr/lib"
            "/usr/lib/x86_64-linux-gnu")

        set(OSG_PLUGINS_INSTALLED FALSE)
        foreach(SEARCH_PATH ${OSG_PLUGIN_SEARCH_PATHS})
            file(GLOB OSG_PLUGIN_DIRS "${SEARCH_PATH}/osgPlugins-*")
            if(OSG_PLUGIN_DIRS)
                foreach(PLUGIN_DIR ${OSG_PLUGIN_DIRS})
                    if(IS_DIRECTORY "${PLUGIN_DIR}")
                        get_filename_component(PLUGIN_DIR_NAME "${PLUGIN_DIR}" NAME)
                        message(STATUS "Installing OSG plugins from: ${PLUGIN_DIR}")
                        file(GLOB OSG_PLUGIN_SOS "${PLUGIN_DIR}/*.so")
                        if(OSG_PLUGIN_SOS)
                            install(FILES ${OSG_PLUGIN_SOS}
                                DESTINATION lib/${PLUGIN_DIR_NAME}
                                COMPONENT ShipNetSimGUI_COMP)
                        endif()
                        set(OSG_PLUGINS_INSTALLED TRUE)
                    endif()
                endforeach()
                if(OSG_PLUGINS_INSTALLED)
                    break()
                endif()
            endif()
        endforeach()

        if(NOT OSG_PLUGINS_INSTALLED)
            message(WARNING "OpenSceneGraph plugin directory not found")
        endif()

        # osgEarth plugin .so files
        set(OSGEARTH_PLUGIN_SEARCH_PATHS "")
        if(DEFINED osgEarth_DIR)
            get_filename_component(OSGEARTH_PREFIX "${osgEarth_DIR}/../../.." ABSOLUTE)
            list(APPEND OSGEARTH_PLUGIN_SEARCH_PATHS "${OSGEARTH_PREFIX}/lib")
        endif()
        list(APPEND OSGEARTH_PLUGIN_SEARCH_PATHS
            "/usr/local/${CMAKE_BUILD_TYPE}/lib"
            "/usr/local/lib"
            "/usr/lib"
            "/usr/lib/x86_64-linux-gnu")

        set(OSGEARTH_PLUGINS_INSTALLED FALSE)
        foreach(SEARCH_PATH ${OSGEARTH_PLUGIN_SEARCH_PATHS})
            file(GLOB OSGEARTH_PLUGIN_DIRS "${SEARCH_PATH}/osgPlugins-*")
            if(OSGEARTH_PLUGIN_DIRS)
                foreach(PLUGIN_DIR ${OSGEARTH_PLUGIN_DIRS})
                    if(IS_DIRECTORY "${PLUGIN_DIR}")
                        get_filename_component(PLUGIN_DIR_NAME "${PLUGIN_DIR}" NAME)
                        file(GLOB OSGEARTH_PLUGIN_SOS "${PLUGIN_DIR}/osgdb_earth*.so")
                        if(OSGEARTH_PLUGIN_SOS)
                            message(STATUS "Installing osgEarth plugins from: ${PLUGIN_DIR}")
                            install(FILES ${OSGEARTH_PLUGIN_SOS}
                                DESTINATION lib/${PLUGIN_DIR_NAME}
                                COMPONENT ShipNetSimGUI_COMP)
                            set(OSGEARTH_PLUGINS_INSTALLED TRUE)
                        endif()
                    endif()
                endforeach()
                if(OSGEARTH_PLUGINS_INSTALLED)
                    break()
                endif()
            endif()
        endforeach()
    endif()

    # ========================================
    # LINUX: ICU Libraries (Optional)
    # ========================================
    if(DEFINED QT_LIB_DIR)
        file(GLOB ICU_LIBS "${QT_LIB_DIR}/libicu*.so*")
        if(ICU_LIBS)
            message(STATUS "Found ICU libraries: ${ICU_LIBS}")
            install(FILES ${ICU_LIBS} DESTINATION lib COMPONENT ShipNetSimCore_COMP)
        else()
            message(STATUS "ICU libraries not found (this is usually OK)")
        endif()
    endif()

    # ========================================
    # LINUX: Fix RPATH on Bundled Shared Libraries
    # ========================================
    # Bundled third-party .so files (Qt, GDAL, OSG, etc.) have hardcoded
    # RPATHs pointing to their original install locations. Use patchelf to
    # set $ORIGIN-based RPATHs so transitive dependencies resolve from the
    # installed lib/ directory on any machine.
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        message(STATUS "Found patchelf: ${PATCHELF_EXECUTABLE}")
        install(CODE "
            # Fix RPATH on shared libraries in lib/
            file(GLOB _all_libs \"\${CMAKE_INSTALL_PREFIX}/lib/*.so*\")
            foreach(_lib \${_all_libs})
                if(IS_SYMLINK \"\${_lib}\")
                    continue()
                endif()
                execute_process(
                    COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \\$ORIGIN \"\${_lib}\"
                    ERROR_QUIET)
            endforeach()

            # Fix RPATH on OSG/osgEarth plugins in lib/osgPlugins-*/
            file(GLOB_RECURSE _osg_plugins \"\${CMAKE_INSTALL_PREFIX}/lib/osgPlugins-*/*.so\")
            foreach(_plugin \${_osg_plugins})
                execute_process(
                    COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \\$ORIGIN/.. \"\${_plugin}\"
                    ERROR_QUIET)
            endforeach()

            # Fix RPATH on Qt plugins in plugins/<type>/
            file(GLOB_RECURSE _qt_plugins \"\${CMAKE_INSTALL_PREFIX}/plugins/*.so\")
            foreach(_plugin \${_qt_plugins})
                execute_process(
                    COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \\$ORIGIN/../../lib \"\${_plugin}\"
                    ERROR_QUIET)
            endforeach()
        " COMPONENT ShipNetSimCore_COMP)
    else()
        message(WARNING "patchelf not found - bundled .so RPATHs will not be fixed for portability. Install with: sudo apt install patchelf")
    endif()

    # ========================================
    # LINUX: Summary
    # ========================================
    message(STATUS "=== Linux Library Detection Summary ===")
    message(STATUS "QT_LIB_DIR: ${QT_LIB_DIR}")
    message(STATUS "QT_PLUGINS_DIR: ${QT_PLUGINS_DIR}")
    message(STATUS "PROJ_DATA_DIR: ${PROJ_DATA_DIR}")
    if(BUILD_GUI)
        message(STATUS "OSG plugins installed: ${OSG_PLUGINS_INSTALLED}")
    endif()
    message(STATUS "=======================================")

endif() # WIN32 / UNIX

# ========================================
# PLATFORM-INDEPENDENT: Data Directory Install
# ========================================
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/data/")
    message(FATAL_ERROR "CRITICAL: Data directory not found: ${CMAKE_SOURCE_DIR}/src/data/")
endif()

# Small configuration and essential asset files with core component
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/data/
    DESTINATION data
    COMPONENT ShipNetSimCore_COMP
    FILES_MATCHING
    PATTERN "*.txt"
    PATTERN "*.ini"
    PATTERN "*.cfg"
    PATTERN "*.xml"
    PATTERN "*.json"
    PATTERN "*.png"
    PATTERN "*.sns"
    PATTERN "*.dat"
    PATTERN "*.earth"
    PATTERN "*.geojson"
    PATTERN "*.gltf"
    PATTERN "*.bin"
    PATTERN "*.cpg"
    PATTERN "*.dbf"
    PATTERN "*.prj"
    PATTERN "*.shp"
    PATTERN "*.shx"
    PATTERN "*.html"
    PATTERN "*.md"
    PATTERN "*.hvg_adj"
)

# Large geographic data files (TIF) in separate component
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/data/
    DESTINATION data
    COMPONENT ShipNetSimData_COMP
    FILES_MATCHING
    PATTERN "*.tif"
)

# ========================================
# PLATFORM-INDEPENDENT: Config Directory
# ========================================
# Create an empty config/ directory so ShipNetSimRabbitMQConfig and ShipNetSimServer
# save/load ShipNetSim_rabbitmq.xml relative to the installation rather than falling
# back to the user's AppConfigLocation (~/.config or AppData).
install(DIRECTORY DESTINATION config COMPONENT ShipNetSimServer_COMP)

# ========================================
# PLATFORM-SPECIFIC: qt.conf for Plugin Discovery
# ========================================
if(WIN32)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPlugins = plugins\n")
elseif(UNIX AND NOT APPLE)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPrefix = ..\nPlugins = plugins\nLibraries = lib\n")
endif()
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    DESTINATION bin
    COMPONENT ShipNetSimCore_COMP
)

# Include CPack module to generate the installer
set(CPACK_GENERATOR "IFW")
set(CPACK_PACKAGE_VENDOR "Virginia Tech Transportation Institute - Center for Sustainable Mobility")
set(CPACK_PACKAGE_DESCRIPTION "An installer for ShipNetSim simulation software.")
set(CPACK_PACKAGE_VERSION_MAJOR "${ShipNetSim_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${ShipNetSim_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${ShipNetSim_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_NSIS_MODIFY_PATH ON)

# ========================================
# CONFIGURE ADMIN vs NON-ADMIN INSTALLATION (QtIFW 4.10 Compatible)
# ========================================
if(WIN32)
    # QtIFW 4.10 Enhanced Configuration
    set(CPACK_IFW_PACKAGE_TITLE "ShipNetSim")
    set(CPACK_IFW_PACKAGE_PUBLISHER "Virginia Tech Transportation Institute")
    set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH 800)
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT 600)
    set(CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS ON)
    set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH ON)

    # QtIFW 4.10-specific features
    set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST ON)
    set(CPACK_IFW_PACKAGE_PRODUCT_IMAGES "")
    if(BUILD_GUI)
        set(CPACK_IFW_PACKAGE_RUN_PROGRAM "@TargetDir@/bin/ShipNetSimGUI.exe")
        set(CPACK_IFW_PACKAGE_RUN_PROGRAM_DESCRIPTION "Launch ShipNetSim GUI now")
    endif()
    set(CPACK_IFW_PACKAGE_RUN_PROGRAM_ARGUMENTS "")

    # QtIFW 4.10 Performance Optimizations (Reduced compression to fix LZMA errors)
    set(CPACK_IFW_PACKAGE_COMPRESSION_LEVEL "3")  # Medium compression instead of 9
    set(CPACK_IFW_VERBOSE ON)
    set(CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR ON)

    # Package-level configuration for proper updates
    set(CPACK_IFW_PACKAGE_NAME "ShipNetSim")
    set(CPACK_IFW_PACKAGE_VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}")
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME "ShipNetSimMaintenanceTool")

    # Create the UI form for the admin selection page
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/adminselectionform.ui" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
    <ui version=\"4.0\">
     <class>AdminSelectionForm</class>
     <widget class=\"QWidget\" name=\"AdminSelectionForm\">
      <layout class=\"QVBoxLayout\">
       <item>
        <widget class=\"QLabel\">
         <property name=\"text\">
          <string>&lt;h3&gt;Installation Type&lt;/h3&gt;</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class=\"QRadioButton\" name=\"adminRadioButton\">
         <property name=\"text\">
          <string>Install for all users (Recommended - Requires Administrator)</string>
         </property>
         <property name=\"checked\">
          <bool>true</bool>
         </property>
        </widget>
       </item>
       <item>
        <widget class=\"QLabel\">
         <property name=\"text\">
          <string>• Installs to Program Files\\n• Available to all users\\n• Requires administrator privileges</string>
         </property>
         <property name=\"indent\">
          <number>20</number>
         </property>
        </widget>
       </item>
       <item>
        <widget class=\"QRadioButton\" name=\"userRadioButton\">
         <property name=\"text\">
          <string>Install for current user only</string>
         </property>
        </widget>
       </item>
       <item>
        <widget class=\"QLabel\">
         <property name=\"text\">
          <string>• Installs to user profile\\n• Available only to current user\\n• No administrator privileges required</string>
         </property>
         <property name=\"indent\">
          <number>20</number>
         </property>
        </widget>
       </item>
      </layout>
     </widget>
    </ui>")

# Create control script with QtIFW 4.10 corrected admin detection
set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/controlscript.qs")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/controlscript.qs" "
function Controller() {
   installer.setValue('StartMenuDir', 'ShipNetSim');
   installer.setMessageBoxAutomaticAnswer('OverwriteTargetDirectory', QMessageBox.Yes);
   installer.setMessageBoxAutomaticAnswer('stopProcessesForUpdates', QMessageBox.Ignore);

   // Set initial default values
   installer.setValue('InstallationType', 'SystemWide');
   installer.setValue('TargetDir', 'C:/Program Files/ShipNetSim');
}

Controller.prototype.IntroductionPageCallback = function() {
   console.log('DEBUG: IntroductionPageCallback - setting defaults');
   installer.setValue('InstallationType', 'SystemWide');
   console.log('DEBUG: Set initial InstallationType to SystemWide');
}

// CORRECTED: Following documentation pattern for custom pages
Controller.prototype.DynamicAdminSelectionFormCallback = function() {
   console.log('DEBUG: DynamicAdminSelectionFormCallback triggered');

   var page = gui.pageWidgetByObjectName('DynamicAdminSelectionForm');
   if (!page) {
       console.log('ERROR: Could not get page widget');
       return;
   }

   var adminButton = page.adminRadioButton;
   var userButton = page.userRadioButton;

   // Set initial state based on current InstallationType
   var currentType = installer.value('InstallationType');
   if (currentType === 'UserOnly' && userButton) {
       userButton.checked = true;
   } else if (adminButton) {
       adminButton.checked = true;
   }

   console.log('DEBUG: Set initial radio button state for: ' + currentType);

   // CORRECTED: Connect signals directly to update values immediately
   if (adminButton) {
       adminButton.toggled.connect(function(checked) {
           if (checked) {
               console.log('DEBUG: Admin installation selected');
               installer.setValue('InstallationType', 'SystemWide');
               installer.setValue('TargetDir', 'C:/Program Files/ShipNetSim');
               console.log('DEBUG: Updated TargetDir to: C:/Program Files/ShipNetSim');
           }
       });
   }

   if (userButton) {
       userButton.toggled.connect(function(checked) {
           if (checked) {
               console.log('DEBUG: User installation selected');
               installer.setValue('InstallationType', 'UserOnly');
               var userPath = installer.value('HomeDir') + '/AppData/Local/ShipNetSim';
               installer.setValue('TargetDir', userPath);
               console.log('DEBUG: Updated TargetDir to: ' + userPath);
           }
       });
   }
}

// CORRECTED: Use TargetDirectoryPageCallback to handle admin elevation
Controller.prototype.TargetDirectoryPageCallback = function() {
   console.log('DEBUG: TargetDirectoryPageCallback triggered');

   var installType = installer.value('InstallationType');
   console.log('DEBUG: Current installation type: ' + installType);

   if (installType === 'SystemWide') {
       // Check for admin rights and elevate if needed
       try {
           var isAdmin = false;
           if (typeof installer.hasAdminRights === 'function') {
               isAdmin = installer.hasAdminRights();
           }

           if (!isAdmin) {
               var result = QMessageBox.question('admin_required', 'Administrator Required',
                   'Administrator privileges are required for system-wide installation.\\n\\n' +
                   'Click OK to restart with administrator privileges, or Cancel to install for current user only.',
                   QMessageBox.Ok | QMessageBox.Cancel, QMessageBox.Ok);

               if (result === QMessageBox.Ok) {
                   if (typeof installer.gainAdminRights === 'function') {
                       if (!installer.gainAdminRights()) {
                           // Elevation failed, fall back to user installation
                           QMessageBox.information('elevation_failed', 'Elevation Failed',
                               'Could not obtain administrator privileges.\\n\\nContinuing with user installation.');
                           installer.setValue('InstallationType', 'UserOnly');
                           var userPath = installer.value('HomeDir') + '/AppData/Local/ShipNetSim';
                           installer.setValue('TargetDir', userPath);

                           // Update the target directory display
                           var widget = gui.currentPageWidget();
                           if (widget && widget.TargetDirectoryLineEdit) {
                               widget.TargetDirectoryLineEdit.setText(userPath);
                           }
                       }
                   }
               } else {
                   // User cancelled, fall back to user installation
                   installer.setValue('InstallationType', 'UserOnly');
                   var userPath = installer.value('HomeDir') + '/AppData/Local/ShipNetSim';
                   installer.setValue('TargetDir', userPath);

                   // Update the target directory display
                   var widget = gui.currentPageWidget();
                   if (widget && widget.TargetDirectoryLineEdit) {
                       widget.TargetDirectoryLineEdit.setText(userPath);
                   }
               }
           }
       } catch (e) {
           console.log('DEBUG: Admin rights check failed: ' + e.message);
       }
   }

   // Update the displayed path to match the current selection
   var widget = gui.currentPageWidget();
   if (widget && widget.TargetDirectoryLineEdit) {
       var currentPath = installer.value('TargetDir');
       widget.TargetDirectoryLineEdit.setText(currentPath);
       console.log('DEBUG: Updated display path to: ' + currentPath);
   }
}")

   # Create component script optimized for QtIFW 4.10
   file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/core_component.qs" "
   function Component() {
      console.log('DEBUG: Adding admin selection page before target directory');
      installer.addWizardPage(component, 'AdminSelectionForm', QInstaller.TargetDirectory);
   }

   Component.prototype.createOperations = function() {
      try {
          component.createOperations();

          var installType = installer.value('InstallationType');
          console.log('DEBUG: Creating operations for: ' + installType);

          if (installType === 'SystemWide') {
              // System-wide operations
              component.addOperation('RegisterFileType',
                  'sns',
                  '@TargetDir@/bin/ShipNetSim.exe \"%1\"',
                  'ShipNetSim Simulation File',
                  'application/x-shipnetsim');

              component.addOperation('CreateShortcut',
                  '@TargetDir@/bin/ShipNetSim.exe',
                  '@AllUsersStartMenuProgramsPath@/ShipNetSim/ShipNetSim.lnk',
                  'workingDirectory=@TargetDir@',
                  'iconPath=@TargetDir@/bin/ShipNetSim.exe',
                  'description=ShipNetSim Simulation Software');

              component.addOperation('CreateShortcut',
                  '@TargetDir@/bin/ShipNetSim.exe',
                  '@AllUsersDesktopPath@/ShipNetSim.lnk',
                  'workingDirectory=@TargetDir@',
                  'iconPath=@TargetDir@/bin/ShipNetSim.exe',
                  'description=ShipNetSim Simulation Software');

              if (installer.fileExists('@TargetDir@/vcredist_x64.exe')) {
                  component.addOperation('Execute',
                      '@TargetDir@/vcredist_x64.exe',
                      '/install', '/quiet', '/norestart',
                      'UNDOEXECUTE',
                      '@TargetDir@/vcredist_x64.exe', '/uninstall', '/quiet');
              }
          } else {
              // User-only operations
              component.addOperation('RegisterFileType',
                  'sns',
                  '@TargetDir@/bin/ShipNetSim.exe \"%1\"',
                  'ShipNetSim Simulation File',
                  'application/x-shipnetsim',
                  'CurrentUser');

              component.addOperation('CreateShortcut',
                  '@TargetDir@/bin/ShipNetSim.exe',
                  '@UserStartMenuProgramsPath@/ShipNetSim/ShipNetSim.lnk',
                  'workingDirectory=@TargetDir@',
                  'iconPath=@TargetDir@/bin/ShipNetSim.exe',
                  'description=ShipNetSim Simulation Software');

              component.addOperation('CreateShortcut',
                  '@TargetDir@/bin/ShipNetSim.exe',
                  '@UserDesktopPath@/ShipNetSim.lnk',
                  'workingDirectory=@TargetDir@',
                  'iconPath=@TargetDir@/bin/ShipNetSim.exe',
                  'description=ShipNetSim Simulation Software');
          }
      } catch (e) {
          console.log('ERROR in createOperations: ' + e.message);
      }
   }")

   # GUI component script
   if(BUILD_GUI)
       file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gui_component.qs" "
function Component() {
   // GUI Component constructor
}

Component.prototype.createOperations = function() {
   try {
       component.createOperations();

       var installType = installer.value('InstallationType');

       if (installType === 'SystemWide') {
           // Create GUI shortcuts system-wide
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimGUI.exe',
               '@AllUsersStartMenuProgramsPath@/ShipNetSim/ShipNetSim GUI.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimGUI.exe',
               'description=ShipNetSim GUI Application');

           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimGUI.exe',
               '@AllUsersDesktopPath@/ShipNetSim GUI.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimGUI.exe',
               'description=ShipNetSim GUI Application');
       } else {
           // Create GUI shortcuts for current user
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimGUI.exe',
               '@UserStartMenuProgramsPath@/ShipNetSim/ShipNetSim GUI.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimGUI.exe',
               'description=ShipNetSim GUI Application');

           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimGUI.exe',
               '@UserDesktopPath@/ShipNetSim GUI.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimGUI.exe',
               'description=ShipNetSim GUI Application');
       }
   } catch (e) {
       console.log('Error in GUI createOperations: ' + e.message);
   }
}")
   endif()

   # Server component script
   if(BUILD_SERVER)
       file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/server_component.qs" "
function Component() {
   // Server Component constructor
}

Component.prototype.createOperations = function() {
   try {
       component.createOperations();
       var installType = installer.value('InstallationType');

       if (installType === 'SystemWide') {
           // Create Server shortcuts system-wide
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}.exe',
               '@AllUsersStartMenuProgramsPath@/ShipNetSim/ShipNetSim Server.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}.exe',
               'description=ShipNetSim Server Application');

           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimRabbitMQConfig.exe',
               '@AllUsersStartMenuProgramsPath@/ShipNetSim/RabbitMQ Config.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimRabbitMQConfig.exe',
               'description=RabbitMQ Connection Configuration');
       } else {
           // Create Server shortcuts for current user
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}.exe',
               '@UserStartMenuProgramsPath@/ShipNetSim/ShipNetSim Server.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}.exe',
               'description=ShipNetSim Server Application');

           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimRabbitMQConfig.exe',
               '@UserStartMenuProgramsPath@/ShipNetSim/RabbitMQ Config.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimRabbitMQConfig.exe',
               'description=RabbitMQ Connection Configuration');
       }
   } catch (e) {
       console.log('Error in Server createOperations: ' + e.message);
   }
}")
   endif()

   # AdjBuilder component script
   file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/adjbuilder_component.qs" "
function Component() {
   // AdjBuilder Component constructor
}

Component.prototype.createOperations = function() {
   try {
       component.createOperations();
       var installType = installer.value('InstallationType');

       if (installType === 'SystemWide') {
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimAdjBuilder.exe',
               '@AllUsersStartMenuProgramsPath@/ShipNetSim/ShipNetSim Adjacency Builder.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimAdjBuilder.exe',
               'description=ShipNetSim Adjacency Cache Builder');
       } else {
           component.addOperation('CreateShortcut',
               '@TargetDir@/bin/ShipNetSimAdjBuilder.exe',
               '@UserStartMenuProgramsPath@/ShipNetSim/ShipNetSim Adjacency Builder.lnk',
               'workingDirectory=@TargetDir@',
               'iconPath=@TargetDir@/bin/ShipNetSimAdjBuilder.exe',
               'description=ShipNetSim Adjacency Cache Builder');
       }
   } catch (e) {
       console.log('Error in AdjBuilder createOperations: ' + e.message);
   }
}")

elseif(UNIX AND NOT APPLE)
    # ========================================
    # LINUX: QtIFW Package Settings
    # ========================================
    set(CPACK_IFW_PACKAGE_TITLE "ShipNetSim")
    set(CPACK_IFW_PACKAGE_PUBLISHER "Virginia Tech Transportation Institute")
    set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH 800)
    set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT 600)
    set(CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS ON)
    set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH ON)
    set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST ON)
    set(CPACK_IFW_PACKAGE_COMPRESSION_LEVEL "3")
    set(CPACK_IFW_VERBOSE ON)
    set(CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR ON)
    set(CPACK_IFW_PACKAGE_NAME "ShipNetSim")
    set(CPACK_IFW_PACKAGE_VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}")
    set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME "ShipNetSimMaintenanceTool")
    # Linux-specific: default install to home directory
    set(CPACK_IFW_PACKAGE_TARGET_DIR "@HomeDir@/ShipNetSim")

    # ========================================
    # LINUX: Control Script (no admin selection needed)
    # ========================================
    set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/controlscript.qs")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/controlscript.qs" "
function Controller() {
    installer.setMessageBoxAutomaticAnswer('OverwriteTargetDirectory', QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer('stopProcessesForUpdates', QMessageBox.Ignore);
}

Controller.prototype.IntroductionPageCallback = function() {
    console.log('ShipNetSim Linux Installer');
}

Controller.prototype.TargetDirectoryPageCallback = function() {
    var widget = gui.currentPageWidget();
    if (widget && widget.TargetDirectoryLineEdit) {
        var currentPath = installer.value('TargetDir');
        widget.TargetDirectoryLineEdit.setText(currentPath);
    }
}")

    # ========================================
    # LINUX: Core Component Script
    # ========================================
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/core_component.qs" "
function Component() {
    // Core Component constructor - no admin selection page on Linux
}

Component.prototype.createOperations = function() {
    try {
        component.createOperations();

        if (installer.value('os') === 'x11') {
            // Create desktop entry for ShipNetSim
            component.addOperation('CreateDesktopEntry',
                'ShipNetSim.desktop',
                'Version=1.0\\nType=Application\\nTerminal=true\\n' +
                'Exec=@TargetDir@/bin/${SHIPNETSIM_NAME}\\n' +
                'Name=ShipNetSim\\nCategories=Education;Science;\\n' +
                'Comment=Ship Network Simulator');

            // Make executables executable
            component.addOperation('Execute', 'chmod', '+x',
                '@TargetDir@/bin/${SHIPNETSIM_NAME}');
            component.addOperation('Execute', 'chmod', '+x',
                '@TargetDir@/bin/ShipNetSimAdjBuilder');
        }
    } catch (e) {
        console.log('Error in Core createOperations: ' + e.message);
    }
}")

    # ========================================
    # LINUX: GUI Component Script
    # ========================================
    if(BUILD_GUI)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gui_component.qs" "
function Component() {
    // GUI Component constructor
}

Component.prototype.createOperations = function() {
    try {
        component.createOperations();

        if (installer.value('os') === 'x11') {
            component.addOperation('CreateDesktopEntry',
                'ShipNetSimGUI.desktop',
                'Version=1.0\\nType=Application\\nTerminal=false\\n' +
                'Exec=@TargetDir@/bin/ShipNetSimGUI\\n' +
                'Name=ShipNetSim GUI\\nCategories=Education;Science;\\n' +
                'Comment=ShipNetSim GUI Application');

            component.addOperation('Execute', 'chmod', '+x',
                '@TargetDir@/bin/ShipNetSimGUI');
        }
    } catch (e) {
        console.log('Error in GUI createOperations: ' + e.message);
    }
}")
    endif()

    # ========================================
    # LINUX: Server Component Script
    # ========================================
    if(BUILD_SERVER)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/server_component.qs" "
function Component() {
    // Server Component constructor
}

Component.prototype.createOperations = function() {
    try {
        component.createOperations();

        if (installer.value('os') === 'x11') {
            component.addOperation('CreateDesktopEntry',
                'ShipNetSimServer.desktop',
                'Version=1.0\\nType=Application\\nTerminal=true\\n' +
                'Exec=@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}\\n' +
                'Name=ShipNetSim Server\\nCategories=Education;Science;\\n' +
                'Comment=ShipNetSim Server Application');

            component.addOperation('Execute', 'chmod', '+x',
                '@TargetDir@/bin/${SHIPNETSIM_SERVER_NAME}');
            component.addOperation('Execute', 'chmod', '+x',
                '@TargetDir@/bin/ShipNetSimRabbitMQConfig');
        }
    } catch (e) {
        console.log('Error in Server createOperations: ' + e.message);
    }
}")
    endif()

    # ========================================
    # LINUX: AdjBuilder Component Script
    # ========================================
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/adjbuilder_component.qs" "
function Component() {
    // AdjBuilder Component constructor
}

Component.prototype.createOperations = function() {
    try {
        component.createOperations();

        if (installer.value('os') === 'x11') {
            component.addOperation('CreateDesktopEntry',
                'ShipNetSimAdjBuilder.desktop',
                'Version=1.0\\nType=Application\\nTerminal=true\\n' +
                'Exec=@TargetDir@/bin/ShipNetSimAdjBuilder\\n' +
                'Name=ShipNetSim Adjacency Builder\\nCategories=Education;Science;\\n' +
                'Comment=ShipNetSim Adjacency Cache Builder');
        }
    } catch (e) {
        console.log('Error in AdjBuilder createOperations: ' + e.message);
    }
}")

endif() # WIN32 / UNIX QtIFW config

# ========================================
# QtIFW PATH CONFIGURATION
# ========================================
# Enhanced QtIFW path detection for version 4.10
if(NOT DEFINED CPACK_IFW_ROOT)
    if(WIN32)
        set(POSSIBLE_IFW_ROOTS
            "C:/Qt/Tools/QtInstallerFramework/4.10"
            "C:/Qt/Tools/QtInstallerFramework/4.1"
            "C:/QtIFW-4.10.0"
            "C:/QtIFW"
            "$ENV{QTIFWDIR}"
        )
    elseif(UNIX)
        set(POSSIBLE_IFW_ROOTS
            "$ENV{HOME}/Qt/Tools/QtInstallerFramework/4.10"
            "$ENV{HOME}/Qt/Tools/QtInstallerFramework/4.8"
            "/opt/Qt/Tools/QtInstallerFramework/4.10"
            "/opt/Qt/Tools/QtInstallerFramework/4.8"
            "/usr/local/bin"
            "$ENV{QTIFWDIR}"
        )
    endif()

    foreach(IFW_PATH ${POSSIBLE_IFW_ROOTS})
        if(EXISTS "${IFW_PATH}/bin/binarycreator" OR EXISTS "${IFW_PATH}/bin/binarycreator.exe")
            set(CPACK_IFW_ROOT "${IFW_PATH}")
            message(STATUS "Found QtIFW at: ${IFW_PATH}")
            break()
        endif()
    endforeach()

    if(NOT DEFINED CPACK_IFW_ROOT)
        message(WARNING "QtIFW not found. Please set CPACK_IFW_ROOT manually.")
        if(WIN32)
            set(CPACK_IFW_ROOT "C:/Qt/Tools/QtInstallerFramework/4.10")
        elseif(UNIX)
            set(CPACK_IFW_ROOT "$ENV{HOME}/Qt/Tools/QtInstallerFramework/4.10")
        endif()
    endif()
endif()

include(CPack)
include(CPackIFW)

# ========================================
# COMPONENT DEFINITIONS AND CONFIGURATION
# ========================================

# Add separate data component to handle large files
cpack_add_component(ShipNetSimData_COMP
   DISPLAY_NAME "ShipNetSim Data Files"
   DESCRIPTION "Geographic and simulation data files for ShipNetSim."
   REQUIRED)

cpack_add_component(ShipNetSimCore_COMP
   DISPLAY_NAME "ShipNetSim Core"
   DESCRIPTION "Core libraries and essential files for ShipNetSim."
   REQUIRED)

cpack_add_component(ShipNetSim_COMP
   DISPLAY_NAME "ShipNetSim Main Executable"
   DESCRIPTION "Main ShipNetSim executable."
   REQUIRED)

# Enhanced component configuration with proper versioning and update support
cpack_ifw_configure_component(ShipNetSimData_COMP
   ESSENTIAL ON
   FORCED_INSTALLATION ON
   VISIBLE ON
   NAME "shipnetsim.data"
   VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
   SORTING_PRIORITY 50)

# Core component: USER_INTERFACES only needed on Windows (admin selection page)
if(WIN32)
    cpack_ifw_configure_component(ShipNetSimCore_COMP
       ESSENTIAL ON
       FORCED_INSTALLATION ON
       VISIBLE ON
       NAME "shipnetsim.core"
       VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
       SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/core_component.qs"
       USER_INTERFACES "${CMAKE_CURRENT_BINARY_DIR}/adminselectionform.ui"
       LICENSES "GPL-3.0 License" ${CPACK_RESOURCE_FILE_LICENSE}
       SORTING_PRIORITY 100)
else()
    cpack_ifw_configure_component(ShipNetSimCore_COMP
       ESSENTIAL ON
       FORCED_INSTALLATION ON
       VISIBLE ON
       NAME "shipnetsim.core"
       VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
       SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/core_component.qs"
       LICENSES "GPL-3.0 License" ${CPACK_RESOURCE_FILE_LICENSE}
       SORTING_PRIORITY 100)
endif()

cpack_ifw_configure_component(ShipNetSim_COMP
   ESSENTIAL ON
   FORCED_INSTALLATION ON
   VISIBLE ON
   NAME "shipnetsim.main"
   VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
   SORTING_PRIORITY 90)

if(BUILD_SERVER)
   cpack_add_component(ShipNetSimServer_COMP
       DISPLAY_NAME "ShipNetSim Server"
       DESCRIPTION "ShipNetSim Server executable.")

   cpack_ifw_configure_component(ShipNetSimServer_COMP
       VISIBLE ON
       NAME "shipnetsim.server"
       VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
       SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/server_component.qs"
       SORTING_PRIORITY 70)
endif()

if(BUILD_GUI)
   cpack_add_component(ShipNetSimGUI_COMP
       DISPLAY_NAME "ShipNetSim GUI"
       DESCRIPTION "ShipNetSim GUI executable.")

   cpack_ifw_configure_component(ShipNetSimGUI_COMP
       VISIBLE ON
       NAME "shipnetsim.gui"
       VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
       SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/gui_component.qs"
       SORTING_PRIORITY 80)
endif()

cpack_add_component(ShipNetSimAdjBuilder_COMP
   DISPLAY_NAME "ShipNetSim Adjacency Builder"
   DESCRIPTION "Standalone tool for pre-building adjacency caches for faster pathfinding.")

cpack_ifw_configure_component(ShipNetSimAdjBuilder_COMP
   VISIBLE ON
   NAME "shipnetsim.adjbuilder"
   VERSION "${ShipNetSim_VERSION_MAJOR}.${ShipNetSim_VERSION_MINOR}.${ShipNetSim_VERSION_PATCH}"
   SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/adjbuilder_component.qs"
   SORTING_PRIORITY 85)

endif()
