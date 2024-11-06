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
		COMPONENT ShipNetSimCore_COMP  # Applies to the RUNTIME files
		LIBRARY DESTINATION lib
		COMPONENT ShipNetSimCore_COMP  # Applies to the LIBRARY files
		ARCHIVE DESTINATION lib/static
		COMPONENT ShipNetSimCore_COMP)  # Applies to the ARCHIVE files
			
	# Define install paths for the ShipNetSim Main Executable
	install(TARGETS ${SHIPNETSIM_NAME}
		RUNTIME DESTINATION bin
		COMPONENT ShipNetSim_COMP  # Applies to the RUNTIME files
		LIBRARY DESTINATION lib
		COMPONENT ShipNetSim_COMP  # Applies to the LIBRARY files
		ARCHIVE DESTINATION lib/static
		COMPONENT ShipNetSim_COMP)  # Applies to the ARCHIVE files


    # Conditionally include the server executable if BUILD_SERVER is ON
    if(BUILD_SERVER)
		install(TARGETS ${SHIPNETSIM_SERVER_NAME}
			RUNTIME DESTINATION bin
			COMPONENT ShipNetSimServer_COMP  # Applies to the RUNTIME files
			LIBRARY DESTINATION lib
			COMPONENT ShipNetSimServer_COMP  # Applies to the LIBRARY files
			ARCHIVE DESTINATION lib/static
			COMPONENT ShipNetSimServer_COMP)  # Applies to the ARCHIVE files
    endif()

    # Conditionally include the GUI executable if BUILD_GUI is ON
    if(BUILD_GUI)
		install(TARGETS ShipNetSimGUI
			RUNTIME DESTINATION bin
			COMPONENT ShipNetSimGUI_COMP  # Applies to the RUNTIME files
			LIBRARY DESTINATION lib
			COMPONENT ShipNetSimGUI_COMP  # Applies to the LIBRARY files
			ARCHIVE DESTINATION lib/static
			COMPONENT ShipNetSimGUI_COMP)  # Applies to the ARCHIVE files
    endif()

    # Install necessary Qt6 DLL files assuming they are in the bin directory of Qt installation
    # Adjust the paths according to where Qt binaries are actually located on your system
    if(WIN32)
        install(
            FILES
                "${QT_BIN_DIR}/Qt6Core.dll"
                "${QT_BIN_DIR}/Qt6Gui.dll"
                "${QT_BIN_DIR}/Qt6Widgets.dll"
                "${QT_BIN_DIR}/Qt6Network.dll"
                "${QT_BIN_DIR}/Qt6Sql.dll"
                "${QT_BIN_DIR}/Qt6Xml.dll"
                "${QT_BIN_DIR}/Qt6Concurrent.dll"

            DESTINATION bin
            COMPONENT ShipNetSimCore_COMP
        )

        install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/src/data/
            DESTINATION data
            COMPONENT ShipNetSimCore_COMP
            PATTERN "*"
        )

        # If BUILD_SERVER is ON, also install the container.dll
        if(BUILD_SERVER)
            # Install container.dll
            install(
                FILES "${CONTAINER_BIN_DIR}/container.dll"
                DESTINATION bin
                COMPONENT ShipNetSimCore_COMP
            )
        endif()

        get_filename_component(QT_PARENT_DIR ${QT_BIN_DIR} DIRECTORY)


        # Install TLS plugins
        install(
            DIRECTORY "${QT_PARENT_DIR}/plugins/tls"
            DESTINATION bin/plugins
            COMPONENT ShipNetSimCore_COMP
        )

    endif()




    # Include CPack module to generate the installer
    set(CPACK_GENERATOR "IFW")
    set(CPACK_PACKAGE_VENDOR "Virginia Tech Transportation Institute - Center for Sustainable Mobility")
    set(CPACK_PACKAGE_DESCRIPTION "An installer for ShipNetSim simulation software.")
    set(CPACK_PACKAGE_VERSION_MAJOR "${ShipNetSim_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${ShipNetSim_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${ShipNetSim_VERSION_PATCH}")
    # Set the executables to include in the package based on BUILD_SERVER and BUILD_GUI options
    # set(CPACK_PACKAGE_EXECUTABLES "ShipNetSim;ShipNetSim")
    # if(BUILD_SERVER)
    #     list(APPEND CPACK_PACKAGE_EXECUTABLES "ShipNetSimServer;ShipNetSim Server")
    # endif()
    # if(BUILD_GUI)
    #     list(APPEND CPACK_PACKAGE_EXECUTABLES "ShipNetSimGUI;ShipNetSim GUI")
    # endif()
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

    # Configure the Core component
    cpack_ifw_configure_component(ShipNetSimCore_COMP
        essential ON
        forced_installation ON
        visible ON)  # This component must be installed and visible in the installer GUI

    # Define components for each target
    cpack_add_component(ShipNetSim_COMP
        DISPLAY_NAME "ShipNetSim Main Executable"
        DESCRIPTION "Main ShipNetSim executable."
		REQUIRED)
	
	cpack_ifw_configure_component(ShipNetSim_COMP
        essential ON
        forced_installation ON
        visible ON)  # This component must be installed and visible in the installer GUI

    # Define components and descriptions
    if(BUILD_SERVER)
        cpack_add_component(ShipNetSimServer_COMP
            DISPLAY_NAME "ShipNetSim Server"
            DESCRIPTION "ShipNetSim Server executable.")

        cpack_ifw_configure_component(ShipNetSimServer_COMP
            visible ON)  # Make this component visible and optional
    endif()

    # Optional GUI component
    if(BUILD_GUI)
        cpack_add_component(ShipNetSimGUI_COMP
            DISPLAY_NAME "ShipNetSim GUI"
            DESCRIPTION "ShipNetSim GUI executable.")

        cpack_ifw_configure_component(ShipNetSimGUI_COMP
            visible ON)  # Make this component visible and optional
    endif()
	
	cpack_ifw_configure_component(ShipNetSimCore_COMP LICENSES "GPL-3.0 License" ${CPACK_RESOURCE_FILE_LICENSE})

endif()
