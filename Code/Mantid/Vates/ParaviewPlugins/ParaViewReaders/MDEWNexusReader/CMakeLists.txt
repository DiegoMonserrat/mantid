project( MantidParaViewMDEWNexusReader )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
add_paraview_plugin( MantidParaViewMDEWNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWNexusReader.xml
	SERVER_MANAGER_SOURCES vtkMDEWNexusReader.cxx
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWNexusReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewMDEWNexusReaderSMPlugin LINK_PRIVATE 
${MANTID_SUBPROJECT_LIBS} 
${POCO_LIBRARIES}
${Boost_LIBRARIES}
${JSONCPP_LIBRARIES}
${QWT_LIBRARIES})

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewMDEWNexusReaderSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDEWNexusReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewMDEWNexusReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


