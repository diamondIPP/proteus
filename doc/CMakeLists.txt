find_package(Doxygen 1.6)
find_program(SPHINX_EXECUTABLE
             NAMES sphinx-build sphinx-build-3
             DOC "Sphinx documentation generator")

if(NOT Doxygen_FOUND OR NOT SPHINX_EXECUTABLE)
  message(WARNING "Disable documentation: Doxygen or Sphinx not available")
  return()
else()
  message(STATUS "Enable documentation")
endif()

set(DOC_SRC basic_steps.rst
            configuration_files.rst
            contribution_guide.rst
            index.rst)

configure_file(conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)
configure_file(doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY)
add_custom_target(
    doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
    COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "generate documentation with Doxygen and Sphinx"
    VERBATIM
    SOURCES conf.py.in doxyfile.in ${DOC_SRC})