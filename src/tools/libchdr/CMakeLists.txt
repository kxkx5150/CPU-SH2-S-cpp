cmake_minimum_required(VERSION 3.1)

project(chdr C)

# if(APPLE OR UNIX)
#   list(APPEND FLAC_DEFS HAVE_SYS_PARAM_H)
# endif()

find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIRS})

#--------------------------------------------------
# chdr
#--------------------------------------------------

set(CHDR_SOURCES
  bitstream.c
  bitstream.h
  cdrom.c
  cdrom.h
  chd.c
  chd.h
  coretypes.h
  # flac.c
  # flac.h
  huffman.c
  huffman.h)

# find_library(LIBFLAC_LIB NAMES FLAC)
# message("FLAC: ${LIBFLAC_INCLUDE_DIR} ${LIBFLAC_LIB}")

# list(APPEND CHDR_INCLUDES ${LIBFLAC_INCLUDE_DIR})
# list(APPEND CHDR_DEFS FLAC__NO_DLL)
# list(APPEND CHDR_LIBS ${LIBFLAC_LIB})


set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)


add_library(chdr-static STATIC ${CHDR_SOURCES})
target_include_directories(chdr-static PRIVATE ${CHDR_INCLUDES} ${OPENSSL_INCLUDE_DIR})
target_compile_definitions(chdr-static PRIVATE ${CHDR_DEFS})
target_link_libraries(chdr-static ${CHDR_LIBS} OpenSSL::Crypto z)

