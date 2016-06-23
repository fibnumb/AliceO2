set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/detectors/tpc
)

set(SYSTEM_INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}
  ${BASE_INCLUDE_DIRECTORIES}
  ${Boost_INCLUDE_DIRS}
  ${FAIRROOT_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
)

include_directories( ${INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
  ${ROOT_LIBRARY_DIR}
  ${FAIRROOT_LIBRARY_DIR}
)

link_directories( ${LINK_DIRECTORIES})

set(SRCS

)

set(HEADERS

)
Set(LINKDEF src/tpcReconstructionLinkDef.h)
Set(LIBRARY_NAME tpcReconstruction)
Set(DEPENDENCIES
    tpcBase
)

GENERATE_LIBRARY()