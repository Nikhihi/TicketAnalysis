cmake_minimum_required(VERSION 3.16)
project(TicketAnalysis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ticket_analysis
    src/main.cpp
    src/ticket.cpp
    src/analyzer.cpp
    src/report.cpp
)

target_include_directories(ticket_analysis PRIVATE src third_party)

if(MSVC)
    target_compile_options(ticket_analysis PRIVATE /utf-8 /W3)
else()
    target_compile_options(ticket_analysis PRIVATE -Wall)
endif()
