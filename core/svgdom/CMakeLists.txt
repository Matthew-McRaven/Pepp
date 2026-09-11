cmake_minimum_required(VERSION 3.25)

add_executable(
  SvgDom
  main.cpp
  # Svg elements
  src/SvgDocument.cpp
  src/SvgDocument.hpp
  src/SvgParser.hpp
  src/SvgElement.cpp
  src/SvgElement.hpp
  src/SvgCommentElement.cpp
  src/SvgCommentElement.hpp
  src/SvgCDataElement.cpp
  src/SvgCDataElement.hpp
  src/SvgRectElement.cpp
  src/SvgRectElement.hpp
  src/SvgSvgElement.cpp
  src/SvgSvgElement.hpp
  src/SvgUseElement.cpp
  src/SvgUseElement.hpp
  # special functionality
  src/Timer.h
  src/SvgType.hpp
  src/SvgParser.hpp
  src/SvgFactory.cpp
  src/SvgFactory.hpp
  src/XmlAttributes_p.hpp
  src/utility_p.hpp)
target_link_libraries(SvgDom PUBLIC rapidxml fmt)

set_target_properties(SvgDom PROPERTIES FOLDER "qtc_runnable")
