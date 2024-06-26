cmake_minimum_required(VERSION 3.20)
projecT(hwloc)

# Source:
# https://github.com/microsoft/vcpkg/blob/7e495c0773353de69f24f3af8aea0ae9a129508c/ports/hwloc/CMakeLists.txt

cmake_minimum_required(VERSION 3.0)
project(hwloc
    LANGUAGES C
    VERSION 2.2.0)

configure_file(contrib/windows/hwloc_config.h include/hwloc/autogen/config.h COPYONLY)
configure_file(contrib/windows/static-components.h include/static-components.h COPYONLY)
configure_file(contrib/windows/private_config.h include/private/autogen/config.h COPYONLY)

file(READ ${CMAKE_CURRENT_BINARY_DIR}/include/private/autogen/config.h PRIVATE_CONFIG_H)
string(REPLACE "#define HAVE_DECL_SNPRINTF 0" "#define HAVE_DECL_SNPRINTF 1" PRIVATE_CONFIG_H "${PRIVATE_CONFIG_H}")
string(REPLACE "#define HAVE_DECL_STRTOULL 0" "#define HAVE_DECL_STRTOULL 1" PRIVATE_CONFIG_H "${PRIVATE_CONFIG_H}")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    string(REPLACE "/* #undef HWLOC_X86_32_ARCH */" "#define HWLOC_X86_32_ARCH 1"    PRIVATE_CONFIG_H "${PRIVATE_CONFIG_H}")
    string(REPLACE "#define HWLOC_X86_64_ARCH 1"    "/* #undef HWLOC_X86_64_ARCH */" PRIVATE_CONFIG_H "${PRIVATE_CONFIG_H}")
    string(REPLACE "#define SIZEOF_VOID_P 8"        "#define SIZEOF_VOID_P 4"        PRIVATE_CONFIG_H "${PRIVATE_CONFIG_H}")
endif()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/private/autogen/config.h "${PRIVATE_CONFIG_H}")

add_library(libhwloc
    hwloc/base64.c
    hwloc/bind.c
    hwloc/bitmap.c
    hwloc/components.c
    hwloc/distances.c
    hwloc/diff.c
    hwloc/misc.c
    hwloc/pci-common.c
    hwloc/shmem.c
    hwloc/topology-noos.c
    hwloc/topology-synthetic.c
    hwloc/topology-windows.c
    hwloc/topology-x86.c
    hwloc/topology-xml-nolibxml.c
    hwloc/topology-xml.c
    hwloc/topology.c
    hwloc/traversal.c
    hwloc/dolib.c)

set_target_properties(libhwloc PROPERTIES DEFINE_SYMBOL _USRDLL)

add_executable(hwloc-info
    utils/hwloc/hwloc-info.c)

target_link_libraries(hwloc-info PRIVATE libhwloc)