cmake_minimum_required(VERSION 3.24)

project(ShipNetSimInstaller VERSION ${ShipNetSim_VERSION})

# Check if building of installer is enabled
if(BUILD_INSTALLER)

    # Define ShipNetSimInstaller as a custom target
    add_custom_target(ShipNetSimInstaller ALL
        COMMENT "Creating an installer for ShipNetSim."
    )

    # Ensure that ShipNetSimCore is built first by specifying it as a dependency
    add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_CORE_DLL_NAME})
    add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_NAME})

    if (BUILD_GUI)
        add_dependencies(ShipNetSimInstaller ShipNetSimGUI)
    endif()

    if (BUILD_SERVER)
        add_dependencies(ShipNetSimInstaller ${SHIPNETSIM_SERVER_NAME})
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

    # Conditionally include the server executable if BUILD_SERVER is ON
    if(BUILD_SERVER)
        install(TARGETS ${SHIPNETSIM_SERVER_NAME}
            RUNTIME DESTINATION bin
            COMPONENT ShipNetSimServer_COMP
            LIBRARY DESTINATION lib
            COMPONENT ShipNetSimServer_COMP
            ARCHIVE DESTINATION lib/static
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
        install(
            FILES ${REQUIRED_QT_DLLS}
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

            install(
                FILES ${ALL_QT_GUI_DLLS}
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
                    install(
                        FILES ${GDAL_DLLS}
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
                    install(
                        FILES ${GEO_DLLS}
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
                    install(
                        FILES ${CONTAINER_DLLS}
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
                    install(
                        FILES ${RABBITMQ_DLLS}
                        DESTINATION bin
                        COMPONENT ShipNetSimServer_COMP
                    )
                    set(RABBITMQ_FOUND TRUE)
                endif()
            endif()

            if(NOT RABBITMQ_FOUND)
                message(FATAL_ERROR "CRITICAL: RabbitMQ DLLs not found! RABBITMQ_BIN_DIR=${RABBITMQ_BIN_DIR}")
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
                        install(
                            FILES ${OSG_DLLS}
                            DESTINATION bin
                            COMPONENT ShipNetSimGUI_COMP
                        )

                        # CRITICAL: OpenSceneGraph plugins
                        file(GLOB OSG_PLUGIN_DIRS "${OSG_BIN_DIR}/osgPlugins-*")
                        if(NOT OSG_PLUGIN_DIRS)
                            message(FATAL_ERROR "CRITICAL: OpenSceneGraph plugins directory not found in: ${OSG_BIN_DIR}")
                        endif()

                        foreach(PLUGIN_DIR ${OSG_PLUGIN_DIRS})
                            if(IS_DIRECTORY "${PLUGIN_DIR}")
                                get_filename_component(PLUGIN_DIR_NAME "${PLUGIN_DIR}" NAME)
                                message(STATUS "Installing OSG plugins from: ${PLUGIN_DIR}")
                                install(
                                    DIRECTORY "${PLUGIN_DIR}/"
                                    DESTINATION "bin/${PLUGIN_DIR_NAME}"
                                    COMPONENT ShipNetSimGUI_COMP
                                    FILES_MATCHING PATTERN "*.dll"
                                )
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
                        install(
                            FILES ${OSGEARTH_DLLS}
                            DESTINATION bin
                            COMPONENT ShipNetSimGUI_COMP
                        )
                        set(OSGEARTH_FOUND TRUE)
                        break()
                    endif()
                endif()
            endforeach()

            if(NOT OSGEARTH_FOUND)
                message(FATAL_ERROR "CRITICAL: osgEarth DLLs not found! Searched paths: ${OSGEARTH_DLL_PATHS}")
            endif()

            # osgQt DLLs (REQUIRED)
            set(OSGQT_DLL_PATHS "")
            if(DEFINED OSGQT_INCLUDE_DIR)
                list(APPEND OSGQT_DLL_PATHS "${OSGQT_INCLUDE_DIR}/../bin" "${OSGQT_INCLUDE_DIR}/../lib")
            endif()
            if(DEFINED OSGQOPENGL_LIB)
                get_filename_component(OSGQT_LIB_DIR "${OSGQOPENGL_LIB}" DIRECTORY)
                list(APPEND OSGQT_DLL_PATHS "${OSGQT_LIB_DIR}/../bin" "${OSGQT_LIB_DIR}")
            endif()
            # Common osgQt installation paths
            list(APPEND OSGQT_DLL_PATHS
                "C:/Program Files/osgQt/bin"
                "C:/Program Files (x86)/osgQt/bin"
                "C:/osgQt/bin"
            )

            # set(OSGQT_FOUND FALSE)
            # foreach(OSGQT_PATH ${OSGQT_DLL_PATHS})
            #     get_filename_component(OSGQT_BIN_DIR "${OSGQT_PATH}" ABSOLUTE)
            #     if(EXISTS "${OSGQT_BIN_DIR}")
            #         file(GLOB OSGQT_DLLS
            #             "${OSGQT_BIN_DIR}/osg*Qt*.dll"
            #             "${OSGQT_BIN_DIR}/osg*QOpenGL*.dll"
            #             "${OSGQT_BIN_DIR}/osgQOpenGL*.dll"
            #         )
            #         if(OSGQT_DLLS)
            #             message(STATUS "Found osgQt DLLs in: ${OSGQT_BIN_DIR}")
            #             message(STATUS "osgQt DLLs: ${OSGQT_DLLS}")
            #             install(
            #                 FILES ${OSGQT_DLLS}
            #                 DESTINATION bin
            #                 COMPONENT ShipNetSimGUI_COMP
            #             )
            #             set(OSGQT_FOUND TRUE)
            #             break()
            #         endif()
            #     endif()
            # endforeach()

            # if(NOT OSGQT_FOUND)
            #     message(FATAL_ERROR "CRITICAL: osgQt DLLs not found! Searched paths: ${OSGQT_DLL_PATHS}")
            # endif()

            # KDReports DLLs (REQUIRED)
            set(KDREPORTS_DLL_PATHS "")
            if(DEFINED KDREPORTS_DIR)
                list(APPEND KDREPORTS_DLL_PATHS "${KDREPORTS_DIR}/../../bin" "${KDREPORTS_DIR}/../bin")
            endif()
            # Common KDReports installation paths
            list(APPEND KDREPORTS_DLL_PATHS
                "C:/Program Files/KDAB/KDReports/bin"
                "C:/Program Files (x86)/KDAB/KDReports/bin"
                "C:/KDAB/KDReports-2.3.95/bin"
            )

            # set(KDREPORTS_FOUND FALSE)
            # foreach(KDREPORTS_PATH ${KDREPORTS_DLL_PATHS})
            #     get_filename_component(KDREPORTS_BIN_DIR "${KDREPORTS_PATH}" ABSOLUTE)
            #     if(EXISTS "${KDREPORTS_BIN_DIR}")
            #         file(GLOB KDREPORTS_DLLS
            #             "${KDREPORTS_BIN_DIR}/*kdreports*.dll"
            #             "${KDREPORTS_BIN_DIR}/*KDReports*.dll"
            #         )
            #         if(KDREPORTS_DLLS)
            #             message(STATUS "Found KDReports DLLs in: ${KDREPORTS_BIN_DIR}")
            #             message(STATUS "KDReports DLLs: ${KDREPORTS_DLLS}")
            #             install(
            #                 FILES ${KDREPORTS_DLLS}
            #                 DESTINATION bin
            #                 COMPONENT ShipNetSimGUI_COMP
            #             )
            #             set(KDREPORTS_FOUND TRUE)
            #             break()
            #         endif()
            #     endif()
            # endforeach()

            # if(NOT KDREPORTS_FOUND)
            #     message(FATAL_ERROR "CRITICAL: KDReports DLLs not found! Searched paths: ${KDREPORTS_DLL_PATHS}")
            # endif()

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
        # INSTALL DATA DIRECTORY
        # ========================================
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/data/")
            message(FATAL_ERROR "CRITICAL: Data directory not found: ${CMAKE_SOURCE_DIR}/src/data/")
        endif()

        install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/src/data/
            DESTINATION data
            COMPONENT ShipNetSimCore_COMP
            PATTERN "*"
        )

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
                    foreach(VCREDIST_PATH ${VCREDIST_PATHS})
                        if(EXISTS "${VCREDIST_PATH}")
                            file(GLOB VCREDIST_DLLS "${VCREDIST_PATH}/*.dll")
                            if(VCREDIST_DLLS)
                                message(STATUS "Found VC++ Redistributable DLLs in: ${VCREDIST_PATH}")
                                install(
                                    FILES ${VCREDIST_DLLS}
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

        # ========================================
        # OPTIONAL: ICU DLLs
        # ========================================
        file(GLOB ICU_DLLS "${QT_BIN_DIR}/icu*.dll")
        if(ICU_DLLS)
            message(STATUS "Found ICU DLLs: ${ICU_DLLS}")
            install(
                FILES ${ICU_DLLS}
                DESTINATION bin
                COMPONENT ShipNetSimCore_COMP
            )
        else()
            message(STATUS "ICU DLLs not found (this is usually OK)")
        endif()

        # ========================================
        # Create qt.conf file for proper plugin discovery
        # ========================================
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
            "[Paths]\nPlugins = plugins\n")
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
            DESTINATION bin
            COMPONENT ShipNetSimCore_COMP
        )

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
            message(STATUS "osgQt: ${OSGQT_FOUND}")
            message(STATUS "KDReports: ${KDREPORTS_FOUND}")
        endif()
        message(STATUS "VC++ Redistributable: ${VCREDIST_FOUND}")
        message(STATUS "============================")

    endif() # WIN32

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

    include(CPack)
    include(CPackIFW)

    # Define components and descriptions
    cpack_add_component(ShipNetSimCore_COMP
       DISPLAY_NAME "ShipNetSim Core"
       DESCRIPTION "Core libraries for ShipNetSim."
       REQUIRED)

    cpack_ifw_configure_component(ShipNetSimCore_COMP
       essential ON
       forced_installation ON
       visible ON)

    cpack_add_component(ShipNetSim_COMP
       DISPLAY_NAME "ShipNetSim Main Executable"
       DESCRIPTION "Main ShipNetSim executable."
       REQUIRED)

    cpack_ifw_configure_component(ShipNetSim_COMP
       essential ON
       forced_installation ON
       visible ON)

    if(BUILD_SERVER)
       cpack_add_component(ShipNetSimServer_COMP
           DISPLAY_NAME "ShipNetSim Server"
           DESCRIPTION "ShipNetSim Server executable.")

       cpack_ifw_configure_component(ShipNetSimServer_COMP
           visible ON)
    endif()

    if(BUILD_GUI)
       cpack_add_component(ShipNetSimGUI_COMP
           DISPLAY_NAME "ShipNetSim GUI"
           DESCRIPTION "ShipNetSim GUI executable.")

       cpack_ifw_configure_component(ShipNetSimGUI_COMP
           visible ON)
    endif()

    cpack_ifw_configure_component(ShipNetSimCore_COMP LICENSES "GPL-3.0 License" ${CPACK_RESOURCE_FILE_LICENSE})

endif()
