##
## Tests for PythonAlgorithms
##

set ( TEST_PY_FILES
  CheckForSampleLogsTest.py
  CreateWorkspaceTest.py
  DakotaChiSquaredTest.py
  FindReflectometryLinesTest.py
  LoadFullprofFileTest.py
  MaskWorkspaceToCalFileTest.py
  RetrieveRunInfoTest.py
  Stitch1DTest.py
  SuggestTibCNCSTest.py
  SuggestTibHYSPECTest.py
  UpdatePeakParameterTableValueTest.py
)

# Prefix for test name=PythonAlgorithms
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonAlgorithms ${TEST_PY_FILES} )