cmake_minimum_required(VERSION 3.25)

project(SvgDom LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Set OFF to use ISO C++ without compiler extensions


add_executable(SvgDom
    main.cpp
    src/Timer.h
    src/SvgDocument.cpp
    src/SvgDocument.hpp
    src/SvgDocument_p.hpp
    src/SvgParser.hpp
    src/SvgElement.cpp
    src/SvgElement.hpp
    src/SvgBasicElement.cpp
    src/SvgBasicElement.hpp
    src/SvgCommentElement.cpp
    src/SvgCommentElement.hpp
    src/SvgRectElement.cpp
    src/SvgRectElement.hpp
    src/SvgSvgElement.cpp
    src/SvgSvgElement.hpp
    src/SvgParser.hpp
    src/XmlElement_p.hpp
    src/XmlAttributes_p.hpp
    src/utility_p.hpp
)

include(GNUInstallDirs)
install(TARGETS SvgDom
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include_directories(..\\3rdParty\\rapidxml\\
                    .\\) #worked