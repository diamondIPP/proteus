set(gbl_VERSION_MAJOR 2)
set(gbl_VERSION_MINOR 1)
set(gbl_VERSION_PATCH 5)
set(gbl_SOURCES src/BorderedBandMatrix.cpp
                src/GblData.cpp
                src/GblPoint.cpp
                src/GblTrajectory.cpp
                src/MilleBinary.cpp
                src/VMatrix.cpp)
set(gbl_HEADERS include/BorderedBandMatrix.h
                include/GblData.h
                include/GblPoint.h
                include/GblTrajectory.h
                include/MilleBinary.h
                include/VMatrix.h)

# include headers to make them visible in QtCreator
add_library(gbl STATIC ${gbl_SOURCES} ${gbl_HEADERS})
# Eigen as SYSTEM include to suppress warnings
target_include_directories(gbl SYSTEM PUBLIC ${EIGEN_INCLUDE_DIRS})
target_include_directories(gbl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
