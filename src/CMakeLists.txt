# Set the variable 'units_LIB_TARGET_NAME' with the value "unitsLib".
# This can be used as the name of a library target in the project
set(units_LIB_TARGET_NAME "unitsLib")

# Add the subdirectory 'ShipNetSim' to the build.
add_subdirectory(ShipNetSim)

# Add the subdirectory 'ShipNetSimGUI' to the build.
add_subdirectory(ShipNetSimGUI)

# Add the subdirectory 'third_party' to the build.
add_subdirectory(third_party)

# Add the sudirectory 'test' to the build.
#add_subdirectory(test)

# Add the subdirectory 'ShipNetSimInstaller' to the build.
#add_subdirectory(ShipNetSimInstaller)

