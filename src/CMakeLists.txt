
add_subdirectory(command)
add_subdirectory(graphics)

set(CMGUI_MODEL_SRCS
    command/commands.cpp
    command/cmgui_command_model.cpp
    command/execute_command.cpp
    command/parser.cpp
    general/list.cpp
    general/message.cpp
)

set(CMGUI_MODEL_HDRS
    command/commands.h
    command/cmgui_command_model.h
    command/execute_command.h
    command/parser.h
    general/list.h
    general/message.h
)

add_library(cmgui_model STATIC ${CMGUI_MODEL_SRCS} ${CMGUI_MODEL_HDRS})
target_include_directories(cmgui_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMISS_PERL_INTERPRETER_INCLUDE_DIRS})
target_link_libraries(cmgui_model PUBLIC Qt5::Core ${CMISS_PERL_INTERPRETER_LIBRARIES} zinc-static)
target_compile_definitions(cmgui_model PRIVATE ${DEFINITIONS_FOR_CMGUI} ${DEFINITIONS_FOR_PERL_INTERPRETER})

#add_custom_target(cmgui_model_headers SOURCES ${CMGUI_MODEL_HDRS})
