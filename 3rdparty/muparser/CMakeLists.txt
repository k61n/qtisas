cmake_minimum_required(VERSION 3.24)

project(muparser)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    src/muParser.cpp
    src/muParserBase.cpp
    src/muParserBytecode.cpp
    src/muParserCallback.cpp
    src/muParserDLL.cpp
    src/muParserError.cpp
    src/muParserInt.cpp
    src/muParserTest.cpp
    src/muParserTokenReader.cpp
)

set(HEADERS
    include/muParser.h
    include/muParserBase.h
    include/muParserBytecode.h
    include/muParserCallback.h
    include/muParserDef.h
    include/muParserDLL.h
    include/muParserError.h
    include/muParserFixes.h
    include/muParserInt.h
    include/muParserStack.h
    include/muParserTest.h
    include/muParserToken.h
    include/muParserTokenReader.h
)

add_library(muparser ${SOURCES} ${HEADERS})
target_include_directories(muparser PUBLIC include)
