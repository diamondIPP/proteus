set(SOURCES pt-align.cpp correlationsaligner.cpp residualsaligner.cpp)
set(HEADERS aligner.h correlationsaligner.h residualsaligner.h)

add_tool(align ${SOURCES} ${HEADERS})
