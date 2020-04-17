cmake_minimum_required (VERSION 3.15)

add_executable (Iroha "Iroha.cpp" "Iroha.h")

set_target_properties(Iroha PROPERTIES CXX_STANDARD 17)

find_package(fmt CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

target_link_libraries(Iroha PRIVATE 
		fmt::fmt fmt::fmt-header-only
		yaml-cpp)
