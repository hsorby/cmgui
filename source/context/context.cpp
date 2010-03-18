/***************************************************************************//**
 * context.cpp
 * 
 * The main root structure of cmgui.
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

extern "C" {
#include "time/time_keeper.h"
#include "api/cmiss_graphics_module.h"
#include "command/cmiss.h"
#include "context/context.h"
#include "curve/curve.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/cmiss_rendition.h"
#include "selection/any_object_selection.h"
#include "region/cmiss_region.h"
/* following is temporary until circular references are removed for Cmiss_region  */
#include "region/cmiss_region_private.h"
}

struct Context
{
	int access_count;
	const char *id;
	struct Cmiss_region *root_region;
	/* Always want the entry for graphics_buffer_package even if it will
		not be available on this implementation */
	struct Cmiss_graphics_module *graphics_module;
	struct Cmiss_command_data  *default_command_data;
 	struct User_interface_module *UI_module;
	struct Any_object_selection *any_object_selection;
	struct Element_point_ranges_selection *element_point_ranges_selection;
	struct FE_element_selection *element_selection;
	struct FE_node_selection *node_selection, *data_selection;
	struct Event_dispatcher *event_dispatcher;
	struct IO_stream_package *io_stream_package;
	struct MANAGER(Curve) *curve_manager;
};

struct Context *Cmiss_context_create(const char *id)
{
	struct Context *context = NULL;
	if (ALLOCATE(context, struct Context, 1))
	{
		context->graphics_module = NULL;
		context->root_region = NULL;
		context->id = duplicate_string(id);
		context->default_command_data = NULL;
		context->UI_module = NULL;
		context->any_object_selection = NULL;
		context->element_point_ranges_selection = NULL;
		context->element_selection = NULL;
		context->node_selection = NULL;
		context->data_selection = NULL;
		context->event_dispatcher = NULL;
		context->io_stream_package = NULL;
		context->curve_manager = NULL;
		context->access_count = 1;
	}

	return context;
}

int Cmiss_context_destroy(struct Context **context_address)
{
	int return_code = 0;
	struct Context *context = NULL;

	if (NULL != (context = *context_address))
	{
		context->access_count--;
		if (0 == context->access_count)
		{
			if (context->id)
				DEALLOCATE(context->id);
			if (context->default_command_data)
				Cmiss_command_data_destroy(&context->default_command_data);
			if (context->UI_module)
				User_interface_module_destroy(&context->UI_module);
			if (context->graphics_module)
				Cmiss_graphics_module_destroy(&context->graphics_module);
			if (context->any_object_selection)
			{
				DESTROY(Any_object_selection)(&context->any_object_selection);
			}
			if (context->element_point_ranges_selection)
			{
				DESTROY(Element_point_ranges_selection)(&context->element_point_ranges_selection);
			}
			if (context->element_selection)
			{
				DESTROY(FE_element_selection)(&context->element_selection);
			}
			if (context->node_selection)
			{
				DESTROY(FE_node_selection)(&context->node_selection);
			}
			if (context->data_selection)
			{
				DESTROY(FE_node_selection)(&context->data_selection);
			}
			if (context->root_region)
			{
				/* need the following due to circular references where field owned by region references region itself;
				 * when following removed also remove #include "region/cmiss_region_private.h" */
				Cmiss_region_detach_fields_hierarchical(context->root_region);
				DEACCESS(Cmiss_region)(&context->root_region);
			}
			if (context->curve_manager)
			{
				DESTROY(MANAGER(Curve))(&context->curve_manager);
			}
			if (context->io_stream_package)
			{
				DESTROY(IO_stream_package)(&context->io_stream_package);
			}
			if (context->event_dispatcher)
			{
				DESTROY(Event_dispatcher)(&context->event_dispatcher);
			}
			DEALLOCATE(*context_address);
		}
		*context_address = NULL;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_destroy.  Missing graphics module");
		return_code = 0;
	}

	/* Write out any memory blocks still ALLOCATED when MEMORY_CHECKING is
		on.  When MEMORY_CHECKING is off this function does nothing */
	list_memory(/*count_number*/0, /*show_pointers*/0, /*increment_counter*/0,
		/*show_structures*/1);

	return return_code;
}

struct Context *Cmiss_context_access(struct Context *context)
{
	if (context)
	{
	  context->access_count++;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_access.  Missing context");
	}
	return context;
}

struct Cmiss_graphics_module *Cmiss_context_get_default_graphics_module(struct Context *context)
{	
	if (context)
	{
		if (!context->graphics_module)
		{
			context->graphics_module = Cmiss_graphics_module_create(context);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_graphics_module.  Missing context");
	}

	return Cmiss_graphics_module_access(context->graphics_module);
}

struct Cmiss_graphics_module *Cmiss_context_create_graphics_module(struct Context *context)
{	
	struct Cmiss_graphics_module *graphics_module = NULL;

	if (context)
	{
		graphics_module = Cmiss_graphics_module_create(context);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_create_graphics_module.  Missing context");
	}

	return graphics_module;
}

struct Cmiss_region *Cmiss_context_get_default_region(struct Context *context)
{
	if (context)
	{
		if (!context->root_region)
		{
			context->root_region = Cmiss_region_create();
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_region.  Missing context");
	}
	
	return ACCESS(Cmiss_region)(context->root_region);
}

struct Cmiss_region *Cmiss_context_create_region(struct Context *context)
{
	Cmiss_region *region = NULL;

	if (context)
	{
		region = Cmiss_region_create();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_create_region.  Missing context");
	}
	return region;
}

struct Cmiss_command_data *Cmiss_context_get_default_command_interpreter(struct Context *context)
{
	struct Cmiss_command_data *command_data = NULL;

	if (context && context->UI_module) 
	{
		if (!context->default_command_data)
		{
			context->default_command_data = CREATE(Cmiss_command_data)(context, context->UI_module);
		}
		if (context->default_command_data)
		{
			command_data = Cmiss_command_data_access(context->default_command_data);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_command_interpreter.  Missing context "
			"or user interface has not been enable yet.");
	}

	return command_data;
}

#if !defined (WIN32_USER_INTERFACE)
struct User_interface_module *Cmiss_context_create_user_interface(
	struct Context *context, int in_argc, const char *in_argv[])
#else
struct User_interface_module *Cmiss_context_create_user_interface(
	struct Context *context, int in_argc, const char *in_argv[],
	HINSTANCE current_instance, HINSTANCE previous_instance, 
	LPSTR command_line,int initial_main_window_state)
#endif
{
	struct User_interface_module *UI_module = NULL;

	if (context)
	{
		if (!context->UI_module)
		{
#if !defined (WIN32_USER_INTERFACE)
			context->UI_module = User_interface_module_create(
				context, in_argc, in_argv);
#else
		  context->UI_module = User_interface_module_create(
				context, in_argc, in_argv, current_instance,
				previous_instance, command_line, initial_main_window_state);
#endif
		}
		if (context->UI_module)
		{
			UI_module = User_interface_module_access(context->UI_module);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_create_user_interface.  Missing context.");
	}

	return UI_module;
}

struct Any_object_selection *Cmiss_context_get_any_object_selection(
	struct Context *context)
{
	struct Any_object_selection *any_object_selection = NULL;
	if (context)
	{
		if (!context->any_object_selection)
		{
			context->any_object_selection = CREATE(Any_object_selection)();
		}
		any_object_selection = context->any_object_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_any_object_selection.  Missing context.");
	}
	return any_object_selection;
}

struct Element_point_ranges_selection *Cmiss_context_get_element_point_ranges_selection(
	struct Context *context)
{
	struct Element_point_ranges_selection *element_point_ranges_selection = NULL;
	if (context)
	{
		if (!context->element_point_ranges_selection)
		{
			context->element_point_ranges_selection = CREATE(Element_point_ranges_selection)();
		}
		element_point_ranges_selection = context->element_point_ranges_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_element_point_ranges_selection.  Missing context.");
	}
	return element_point_ranges_selection;
}

struct FE_element_selection *Cmiss_context_get_element_selection(
	struct Context *context)
{
	struct FE_element_selection *element_selection = NULL;
	if (context)
	{
		if (!context->element_selection)
		{
			Cmiss_region *root_region = 
				Cmiss_context_get_default_region(context);
			context->element_selection = CREATE(FE_element_selection)(
				Cmiss_region_get_FE_region(root_region));
			Cmiss_region_destroy(&root_region);
		}
		element_selection = context->element_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_element_selection.  Missing context.");
	}
	return element_selection;
}

struct FE_node_selection *Cmiss_context_get_node_selection(
	struct Context *context)
{
	struct FE_node_selection *node_selection = NULL;
	if (context)
	{
		if (!context->node_selection)
		{
			Cmiss_region *root_region = 
				Cmiss_context_get_default_region(context);
			context->node_selection = CREATE(FE_node_selection)(
				Cmiss_region_get_FE_region(root_region));
			Cmiss_region_destroy(&root_region);
		}
		node_selection = context->node_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_node_selection.  Missing context.");
	}
	return node_selection;
}

struct FE_node_selection *Cmiss_context_get_data_selection(
	struct Context *context)
{
	struct FE_node_selection *data_selection = NULL;
	if (context)
	{
		if (!context->data_selection)
		{
			Cmiss_region *root_region = 
				Cmiss_context_get_default_region(context);
			context->data_selection = CREATE(FE_node_selection)(
				FE_region_get_data_FE_region(Cmiss_region_get_FE_region(root_region)));
			Cmiss_region_destroy(&root_region);
		}
		data_selection = context->data_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_data_selection.  Missing context.");
	}

	return data_selection;
}

struct IO_stream_package *Cmiss_context_get_default_IO_stream_package(
	struct Context *context)
{
	struct IO_stream_package *io_stream_package = NULL;
	if (context)
	{
		if (!context->io_stream_package)
		{
			context->io_stream_package = CREATE(IO_stream_package)();
		}
		io_stream_package = context->io_stream_package;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_IO_stream_package.  Missing context.");
	}

	return io_stream_package;
}


struct Event_dispatcher *Cmiss_context_get_default_event_dispatcher(struct Context *context)
{
	struct Event_dispatcher *event_dispatcher = NULL;
	if (context)
	{
		if (!context->event_dispatcher)
		{
			context->event_dispatcher = CREATE(Event_dispatcher)();
		}
		event_dispatcher = context->event_dispatcher;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_event_dispatcher.  Missing context.");
	}
	return event_dispatcher;
}

int Cmiss_context_execute_command(Cmiss_context_id context, 
	const char *command)
{
	int return_code = 0;
	if (context && context->UI_module && command)
	{
		struct Cmiss_command_data *command_data = 
			Cmiss_context_get_default_command_interpreter(context);
		return_code = cmiss_execute_command(command, (void *)command_data);
		Cmiss_command_data_destroy(&command_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_execute_command.  Missing context or user interface or"
			"command is empty.");
	}

	return return_code;
}

int Cmiss_context_run_main_loop(Cmiss_context_id context)
{
	int return_code = 0;
	if (context && context->UI_module)
	{
		struct Cmiss_command_data *command_data = 
			Cmiss_context_get_default_command_interpreter(context);
		return_code = Cmiss_command_data_main_loop(command_data);
		Cmiss_command_data_destroy(&command_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_start_main_loop.  Missing context or user interface");
	}

	return return_code;
}

Cmiss_scene_viewer_id Cmiss_context_get_graphics_window_pane_by_name(
	Cmiss_context_id context, const char *name, int pane_number)
{
	Cmiss_scene_viewer_id scene_viewer = NULL;

	if (context && context->UI_module && name)
	{
#if defined (USE_CMGUI_GRAPHICS_WINDOW)
		struct Graphics_window *window = NULL;
		struct MANAGER(Graphics_window) *graphics_window_manager =
			context->UI_module->graphics_window_manager;
		if (graphics_window_manager)
		{
			if (NULL != (window=FIND_BY_IDENTIFIER_IN_MANAGER(Graphics_window,name)(
										 (char *)name, graphics_window_manager)))
			{
				scene_viewer = Graphics_window_get_Scene_viewer(window, pane_number);
			}
			else
			{
				display_message(WARNING_MESSAGE,"Could not find window named %s",
					name);
				scene_viewer = (struct Scene_viewer *)NULL;
			}
		}
#else /* defined (USE_CMGUI_GRAPHICS_WINDOW) */
		display_message(WARNING_MESSAGE,"No graphics windows in this cmgui");
		scene_viewer = (struct Scene_viewer *)NULL;
		USE_PARAMETER(pane_number);
#endif /* defined (USE_CMGUI_GRAPHICS_WINDOW) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_.  Missing context or user interface or"
			"command is empty.");
	}

	return scene_viewer;
}

Cmiss_time_keeper_id Cmiss_context_get_default_time_keeper(
	Cmiss_context_id context)
{
	Cmiss_time_keeper *time_keeper = NULL;
	if (context && context->UI_module && context->UI_module->default_time_keeper)
	{
		time_keeper = ACCESS(Time_keeper)(context->UI_module->default_time_keeper);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_time_keeper.  Missing context or user interface");
	}
	return time_keeper;
}

Cmiss_scene_viewer_package_id Cmiss_context_get_default_scene_viewer_package(
	Cmiss_context_id context)
{
	Cmiss_scene_viewer_package *scene_viewer_package = NULL;
	if (context && context->UI_module && context->UI_module->scene_viewer_package)
	{
		scene_viewer_package = context->UI_module->scene_viewer_package;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_scene_viewer_package.  "
			"Missing context or user interface");
	}
	return scene_viewer_package;
}

struct MANAGER(Curve) *Cmiss_context_get_default_curve_manager(
	Cmiss_context_id context)
{
	MANAGER(Curve) *curve_manager = NULL;
	if (context)
	{
		if (!context->curve_manager)
		{
			context->curve_manager = CREATE(MANAGER(Curve))();
		}
		curve_manager = context->curve_manager;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_context_get_default_curve_manager.  "
			"Missing context");
	}
	return curve_manager;
}