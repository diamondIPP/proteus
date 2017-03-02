find_package(Doxygen 1.6)
find_program(SPHINX_EXECUTABLE
             NAMES sphinx-build sphinx-build-3
             DOC "Sphinx documentation generator")

set(DOC_SRC basic_steps.rst
            configuration_files.rst
            contribution_guide.rst
            index.rst)

if(DOXYGEN_FOUND AND SPHINX_EXECUTABLE)
  message(STATUS "Using Sphinx: ${SPHINX_EXECUTABLE}")
  message(STATUS "Enable documentation")

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
else()
  message(WARNING "Disable documentation")
  message(WARNING "Doxygen: ${Doxygen_FOUND}")
  message(WARNING "Sphinx: ${SPHINX_EXECUTABLE}")
endif()
