set ( SRC_FILES HelloWorldAlgorithm.cpp
                LorentzianTest.cpp
		Muon_ExpDecayOscTest.cpp
                ModifyData.cpp
                PropertyAlgorithm.cpp
                WorkspaceAlgorithm.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES HelloWorldAlgorithm.h
                LorentzianTest.h
		Muon_ExpDecayOscTest.h
                ModifyData.h
                PropertyAlgorithm.h
                WorkspaceAlgorithm.h
)

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(UserAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( UserAlgorithms EXCLUDE_FROM_ALL ${SRC_FILES} ${INC_FILES})

# Add to the 'Framework' group in VS
set_property ( TARGET UserAlgorithms PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( UserAlgorithms LINK_PRIVATE ${MANTIDLIBS} )
