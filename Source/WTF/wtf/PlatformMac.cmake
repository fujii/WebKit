include(PlatformCocoa.cmake)

find_library(COCOA_LIBRARY Cocoa)
list(APPEND WTF_LIBRARIES
    ${COCOA_LIBRARY}
)

list(APPEND WTF_SOURCES
    mac/FileSystemMac.mm
)

ADD_WEBKIT_PREFIX_HEADERS(WTF WTFPrefix.h)
