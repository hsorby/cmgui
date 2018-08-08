
# Execute wrap ui files command here.

QT5_WRAP_CPP(GRAPHICS_MOC_HDRS graphics_window_dialog.h)
QT5_WRAP_UI(GRAPHICS_UI_FORMS graphics_window_dialog.ui)

set(GRAPHICS_SRCS graphics_window_dialog.cpp)
set(GRAPHICS_HDRS graphics_window_dialog.h)

add_library(graphics_qt STATIC ${GRAPHICS_SRCS} ${GRAPHICS_HDRS} ${GRAPHICS_MOC_HDRS} ${GRAPHICS_UI_FORMS})
target_link_libraries(graphics_qt Qt5::Widgets)
target_include_directories(graphics_qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${Cmgui_SOURCE_DIR}/source)
