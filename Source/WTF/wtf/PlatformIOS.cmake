include(PlatformCocoa.cmake)

find_library(UIKIT_LIBRARY UIKit)
list(APPEND WTF_LIBRARIES
    ${UIKIT_LIBRARY}
)

target_compile_options(WTF PRIVATE ${WEBKIT_PRIVATE_FRAMEWORKS_COMPILE_FLAG})

list(APPEND WTF_SOURCES
    ios/WebCoreThread.cpp
)

ADD_WEBKIT_PREFIX_HEADERS(WTF WTFPrefix.h)
