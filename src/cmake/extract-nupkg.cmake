cmake_minimum_required(VERSION 3.0.0)

find_program(UNZIP unzip REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)

set(URL "https://www.nuget.org/api/v2/package/${name}/${version}")
set(NUPKG "${local_dir}/${name}-${version}.nupkg")
set(DIR "${local_dir}/${name}-${version}")
if(NOT EXISTS "${NUPKG}")
  file(DOWNLOAD "${URL}" "${NUPKG}")
endif()

if(NOT EXISTS "${DIR}")
  execute_process(
    COMMAND ${UNZIP} -d "${DIR}" "${NUPKG}"
    WORKING_DIRECTORY "${local_dir}"
    ERROR_QUIET
  )
endif()
