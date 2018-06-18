cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -fPIC")

include_directories( ${INCLUDE_DIRECTORIES} ${CMAKE_SOURCE_DIR}/include)

project(ocr-passport)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
#set (SOURCES "src/ocr_area_io.cpp" "src/passport_ocr.cpp")
add_executable(${PROJECT_NAME} "main.cpp" "src/ocr_area_io.cpp" "src/passport_ocr.cpp")



target_link_libraries(${PROJECT_NAME} "boost_system" "boost_thread" "boost_filesystem" "lept" "pthread")

