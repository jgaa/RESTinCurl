
set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

include(ExternalProject)

ExternalProject_Add(
    externalLest
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/martinmoene/lest.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )
    
ExternalProject_Add(
    externalJson
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )

ExternalProject_Add(
    externalLog
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/jgaa/logfault.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )
    
include_directories(
     ${EXTERNAL_PROJECTS_PREFIX}/src/externalLest/include/
     ${EXTERNAL_PROJECTS_PREFIX}/src/externalJson/include/
     ${EXTERNAL_PROJECTS_PREFIX}/src/externalLog/include/
    )
