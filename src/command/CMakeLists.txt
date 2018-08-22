
# Execute wrap ui files command here.

QT5_WRAP_CPP(COMMAND_MOC_HDRS command_file_viewer_dialog.h)
QT5_WRAP_UI(COMMAND_UI_FORMS qt/command_file_viewer_dialog.ui)

set(COMMAND_SRCS command_file_viewer_dialog.cpp)
set(COMMAND_HDRS command_file_viewer_dialog.h command_file_viewer_codes.h)

add_library(command_qt STATIC ${COMMAND_SRCS} ${COMMAND_HDRS} ${COMMAND_MOC_HDRS} ${COMMAND_UI_FORMS})
target_link_libraries(command_qt Qt5::Widgets)
target_include_directories(command_qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${Cmgui_SOURCE_DIR}/src)
