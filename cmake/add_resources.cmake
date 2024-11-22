include(CMakeParseArguments)

function(add_resources)
	set(ARGS_Options)

	set(ARGS_OneValue TARGET)

	set(ARGS_MultiValue RESOURCES)

	cmake_parse_arguments(
		ARGS "${ARGS_Options}" "${ARGS_OneValue}" "${ARGS_MultiValue}" ${ARGN}
	)

	if(NOT ARGS_TARGET)
		message(FATAL_ERROR "Missing TARGET argument")
	endif()

	if(NOT ARGS_RESOURCES)
		message(FATAL_ERROR "Missing RESOURCES argument")
	endif()

	set(RC_DEPENDS "")
	set(header_content "#pragma once\n\n")

	foreach(r ${ARGS_RESOURCES})
		string(MAKE_C_IDENTIFIER ${r} r_identifier)
		set(output "${CMAKE_CURRENT_BINARY_DIR}/${r_identifier}.o")
		add_custom_command(
			OUTPUT ${output}
			COMMAND ${CMAKE_LINKER} --relocatable --format binary --output
					${output} ${r}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${r}
		)

		set(RC_DEPENDS ${RC_DEPENDS} ${output})
		target_link_libraries(${ARGS_TARGET} PRIVATE ${output})

		string(
			APPEND
			header_content
			"\nextern char ${r_identifier}_start[] asm( \"_binary_${r_identifier}_start\" )\;\nextern char ${r_identifier}_end[]   asm( \"_binary_${r_identifier}_end\" )\;\nextern size_t ${r_identifier}_size  asm( \"_binary_${r_identifier}_size\" )\;\n"
		)
	endforeach()

	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${ARGS_TARGET}_rc.h
		 ${header_content}
	)

	add_custom_target(${ARGS_TARGET}_rc DEPENDS ${RC_DEPENDS})

	add_dependencies(${ARGS_TARGET} ${ARGS_TARGET}_rc)
endfunction()
