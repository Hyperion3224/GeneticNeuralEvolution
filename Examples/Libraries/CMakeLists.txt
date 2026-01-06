# Create the math/neural library
add_library(CoreSystems STATIC
    NeuralNetwork.cpp
    System_Info.cpp
    ThreadPool.cpp
    # Headers included for IDE visibility
    NeuralNetwork.hpp
    System_Info.hpp
    ThreadPool.hpp
    Ops_Parallel.h
    ParallelFor.h
    ModelPartitioner.hpp
)

# Set include paths so other targets can find the headers and json.hpp
target_include_directories(CoreSystems PUBLIC 
    . 
    Third_Party
)