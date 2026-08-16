cmake_minimum_required(VERSION 3.16)

project(WeqiTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Test)

qt_add_executable(test_movement
    test_movement.cpp
    ../src/GameState.cpp
    ../src/GameState.h
    ../src/ChessRules.cpp
    ../src/ChessRules.h
    ../src/Move.h
    ../src/MoveHistory.cpp
    ../src/MoveHistory.h
    ../src/GameController.cpp
    ../src/GameController.h
    ../src/ChessPiece.cpp
    ../src/ChessPiece.h
    ../src/data/AIProviderManager.cpp
    ../src/data/AIProviderManager.h
    ../src/data/DataPaths.cpp
    ../src/data/DataPaths.h
    ../src/ai/AIManager.cpp
    ../src/ai/AIManager.h
)

target_include_directories(test_movement PRIVATE
    ../src
    ../src/data
    ../src/ai
)
target_link_libraries(test_movement PRIVATE Qt6::Widgets Qt6::Test)

enable_testing()
add_test(NAME movement COMMAND test_movement)
