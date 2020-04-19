cmake_minimum_required (VERSION 3.15)

add_executable (Iroha "Iroha.cpp" "Iroha.h" "HTTP_Client.cpp")

set_target_properties(Iroha PROPERTIES CXX_STANDARD 17)

target_include_directories(Iroha PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

find_package(fmt CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system date_time)
find_package(nlohmann_json CONFIG REQUIRED)

if(Boost_FOUND)
  target_include_directories(Iroha PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(Iroha PRIVATE ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_link_libraries(Iroha PRIVATE 
		fmt::fmt fmt::fmt-header-only
		yaml-cpp
		OpenSSL::SSL
		OpenSSL::Crypto
		nlohmann_json nlohmann_json::nlohmann_json)

if (MSVC)
	target_compile_definitions(Iroha PRIVATE _WIN32_WINNT=0x0A00)
endif (MSVC)

if (UNIX)
	target_link_libraries(Iroha PRIVATE stdc++fs)
endif (UNIX)