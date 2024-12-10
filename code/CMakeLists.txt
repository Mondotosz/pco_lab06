cmake_minimum_required(VERSION 3.13)
project(PCO_LAB05)

set(CMAKE_CXX_STANDARD 17)

# Liste des fichiers sources avec chemins complets
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_threadpool.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/threadpool.h
)


add_executable(PCO_LAB06 ${SOURCES} ${HEADERS})
target_link_libraries(PCO_LAB06 PRIVATE gtest -lpcosynchro)

