find_package(Python REQUIRED)

add_custom_target(
	charis-fort-options-pytest
	${Python_EXECUTABLE} -m unittest
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(check charis-fort-options-pytest)
