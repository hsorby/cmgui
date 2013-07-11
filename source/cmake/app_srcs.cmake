
SET(APP_SRCS
    source/image_processing/computed_field_binary_erode_image_filter_app.cpp
    source/image_processing/computed_field_binary_dilate_image_filter_app.cpp
    source/image_processing/computed_field_fast_marching_image_filter_app.cpp
    source/image_processing/computed_field_gradient_magnitude_recursive_gaussian_image_filter_app.cpp
    source/image_processing/computed_field_connected_threshold_image_filter_app.cpp
    source/image_processing/computed_field_rescale_intensity_image_filter_app.cpp
    source/image_processing/computed_field_derivative_image_filter_app.cpp
    source/image_processing/computed_field_curvature_anisotropic_diffusion_image_filter_app.cpp
    source/image_processing/computed_field_histogram_image_filter_app.cpp
    source/image_processing/computed_field_discrete_gaussian_image_filter_app.cpp
    source/image_processing/computed_field_sigmoid_image_filter_app.cpp
    source/image_processing/computed_field_mean_image_filter_app.cpp
    source/image_processing/computed_field_canny_edge_detection_filter_app.cpp
    source/image_processing/computed_field_binary_threshold_image_filter_app.cpp
    source/image_processing/computed_field_threshold_image_filter_app.cpp
    source/computed_field/computed_field_time_app.cpp
    source/image_processing/computed_field_image_resample_app.cpp
    source/computed_field/computed_field_string_constant_app.cpp
    source/computed_field/computed_field_deformation_app.cpp
    source/computed_field/computed_field_finite_element_app.cpp
    source/computed_field/computed_field_integration_app.cpp
    source/computed_field/computed_field_image_app.cpp
    source/computed_field/computed_field_vector_operators_app.cpp
    source/computed_field/computed_field_nodeset_operators_app.cpp
    source/computed_field/computed_field_matrix_operators_app.cpp
    source/computed_field/computed_field_lookup_app.cpp
    source/computed_field/computed_field_logical_operators_app.cpp
    source/computed_field/computed_field_function_app.cpp
    source/computed_field/computed_field_fibres_app.cpp
    source/computed_field/computed_field_derivatives_app.cpp
    source/computed_field/computed_field_curve_app.cpp
    source/computed_field/computed_field_composite_app.cpp
    source/mesh/cmiss_element_private_app.cpp
    source/computed_field/computed_field_compose_app.cpp
    source/computed_field/computed_field_format_output_app.cpp
    source/computed_field/computed_field_trigonometry_app.cpp
    source/computed_field/computed_field_arithmetic_operators_app.cpp
    source/graphics/environment_map_app.cpp
    source/minimise/minimise_app.cpp
    source/computed_field/computed_field_alias_app.cpp
    source/computed_field/computed_field_coordinate_app.cpp
    source/graphics/element_point_ranges_app.cpp
    source/finite_element/export_finite_element_app.cpp
    source/graphics/render_to_finite_elements_app.cpp
    source/finite_element/finite_element_conversion_app.cpp
    source/finite_element/finite_element_app.cpp
    source/finite_element/finite_element_region_app.cpp
    source/graphics/graphics_object_app.cpp
    source/graphics/graphic_app.cpp
    source/graphics/font_app.cpp
    source/computed_field/computed_field_conditional_app.cpp
    source/graphics/tessellation_app.cpp
    source/curve/curve_app.cpp
    source/graphics/texture_app.cpp
    source/three_d_drawing/graphics_buffer_app.cpp
    source/general/geometry_app.cpp
    source/computed_field/computed_field_app.cpp
    source/graphics/light_model_app.cpp
    source/graphics/graphics_filter_app.cpp
    source/computed_field/computed_field_set_app.cpp
    source/general/multi_range_app.cpp
    source/graphics/auxiliary_graphics_types_app.cpp
    source/graphics/light_app.cpp
    source/graphics/scene_app.cpp
    source/graphics/spectrum_settings_app.cpp
    source/graphics/spectrum_app.cpp
    source/graphics/colour_app.cpp
    source/graphics/material_app.cpp
    source/region/cmiss_region_app.cpp
    source/graphics/scene_viewer_app.cpp
    source/cmgui.cpp
    source/comfile/comfile.cpp
    source/command/cmiss.cpp
    source/command/command.cpp
    source/command/console.cpp
    source/command/example_path.cpp
    source/command/parser.cpp
    source/context/context_app.cpp
    source/context/user_interface_module.cpp
    source/element/element_point_tool.cpp
    source/element/element_tool.cpp
    source/element/element_point_viewer_wx.cpp
    source/emoter/emoter_dialog.cpp
    source/graphics/transform_tool.cpp
    source/dialog/tessellation_dialog.cpp
    source/graphics/region_tree_viewer_wx.cpp
    source/graphics/spectrum_editor_wx.cpp
    source/graphics/spectrum_editor_dialog_wx.cpp
    source/interaction/interactive_tool.cpp
    source/io_devices/matrix.cpp
    source/node/node_tool.cpp
    source/three_d_drawing/window_system_extensions.c
    source/user_interface/confirmation.cpp
    source/user_interface/event_dispatcher.cpp
    source/user_interface/filedir.cpp
    source/user_interface/user_interface.cpp
    source/colour/colour_editor_wx.cpp
    source/comfile/comfile_window_wx.cpp
    source/command/command_window.cpp
    source/computed_field/computed_field_scene_viewer_projection_app.cpp
    source/graphics/graphics_window.cpp
    source/material/material_editor_wx.cpp
    source/node/node_viewer_wx.cpp
    source/region/cmiss_region_chooser_wx.cpp
    source/region/cmiss_region_app.cpp
    source/time/time_keeper_app.cpp
    source/transformation/transformation_editor_wx.cpp
    source/user_interface/idle.cpp)


