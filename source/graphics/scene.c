/*******************************************************************************
FILE : scene.c

LAST MODIFIED : 5 July 2000

DESCRIPTION :
Structure for storing the collections of objects that make up a 3-D graphical
model - lights, materials, primitives, etc.
Also contains interface routines for having these converted to display lists,
and for these to be assembled into a single display list of the whole scene.
The display list for the whole scene lives and dies with the scene, but it is
up to others - ie. the Scene_viewer - to display.

???RC In hierarchical model a Scene should be a type derived from a
Scene_object/base graphics object - that way they can be stored in the same
containers - and in a scene themselves. Furthermore, I think Graphical
Finite Elements/Regions should be a class derived from a Scene, not just
extra functionality for it as is currently the case.

HISTORY :
November 1997. Created from Scene description part of Drawing.
==============================================================================*/
#include <stdio.h>
#include <string.h>
/*
#include <math.h>
#include <time.h>
#include "view/coord.h"
*/
#include "command/parser.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
/*#include "finite_element/finite_element_to_streamlines.h"*/
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/list_private.h"
#include "general/manager_private.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/object.h"
/*#include "graphics/auxiliary_graphics_types.h"*/
#include "graphics/element_group_settings.h"
/*#include "graphics/graphics_library.h"*/
#include "graphics/glyph.h"
#include "graphics/graphics_object.h"
#include "graphics/graphical_element.h"
#include "graphics/light.h"
#include "graphics/makegtobj.h"
#include "graphics/scene.h"
#include "graphics/texture.h"
#include "three_d_drawing/ThreeDDraw.h"
#include "time/time.h"
#include "user_interface/message.h"
#include "user_interface/user_interface.h"

/*
Module constants
----------------
*/

/*
Module variables
----------------
*/

/* select buffer size grows in increments of SELECT_BUFFER_SIZE_INCREMENT when
	 select buffer overflows. Hence, always large enough to fit all picked objects
	 in the scene */
#define SELECT_BUFFER_SIZE_INCREMENT 10000
static int select_buffer_size=10000;

/*
Module types
------------
*/

struct Scene_object
/*******************************************************************************
LAST MODIFIED : 22 February 2000

DESCRIPTION :
Scenes store a list of these wrappers to GT_objects so that the same
graphics object may have different visibility on different scenes.
==============================================================================*/
{
	enum Scene_object_type type;
	/* these are defined for all scene objects */
	char *name;
	int position;
	gtMatrix *transformation;
	/* these are defined only for SCENE_OBJECT_GRAPHICS_OBJECT */
	struct GT_object *gt_object;
	enum GT_visibility_type visibility;
	struct Time_object *time_object;
	/* these are defined only for SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP */
	struct GT_element_group *gt_element_group;
	/* this is the child scene for SCENE_OBJECT_SCENE */
	struct Scene *child_scene;
	void *scene_manager_callback_id;
	struct MANAGER(Scene) *scene_manager;
	/* this is the scene to which the scene_object belongs */
	struct Scene *scene;
	int access_count;
}; /* struct Scene_object */

FULL_DECLARE_INDEXED_LIST_TYPE(Scene_object);

struct Scene
/*******************************************************************************
LAST MODIFIED : 28 April 2000

DESCRIPTION :
Stores the collections of objects that make up a 3-D graphical model.
==============================================================================*/
{
	/* the name of the scene */
	char *name;
	/* keep pointer to this scene's manager since can pass on manager change */
	/* messages if member manager changes occur (eg. materials) */
	struct MANAGER(Scene) *scene_manager;
	/* list of graphics objects in the scene (plus visibility flag) */
	struct LIST(Scene_object) *scene_object_list;
	/* need following managers for autocreation of graphical finite elements, */
	/* material/light changes, etc. */
	enum Scene_graphical_element_mode graphical_element_mode;
	/* fields and computed_fields */
	struct MANAGER(Computed_field) *computed_field_manager;
	void *computed_field_manager_callback_id;
	struct MANAGER(FE_field) *fe_field_manager;
	/* elements and element groups: */
	struct MANAGER(FE_element) *element_manager;
	struct MANAGER(GROUP(FE_element)) *element_group_manager;
	/* have callbacks from element_group_manager for automatic creating and
		 destroying of GT_element_groups */
	void *element_group_manager_callback_id;
	/* nodes and node groups: */
	struct MANAGER(FE_node) *node_manager;
	struct MANAGER(GROUP(FE_node)) *node_group_manager;
	/* data and data groups: */
	struct MANAGER(FE_node) *data_manager;
	struct MANAGER(GROUP(FE_node)) *data_group_manager;

	/* global stores of selected objects */
	struct Element_point_ranges_selection *element_point_ranges_selection;
	struct FE_element_selection *element_selection;
	struct FE_node_selection *data_selection,*node_selection;

	/* graphics object representing axes */
	struct GT_object *axis_object;

	/* attribute managers and defaults: */
	struct LIST(GT_object) *glyph_list;
	struct MANAGER(Graphical_material) *graphical_material_manager;
	struct Graphical_material *default_material;
	void *graphical_material_manager_callback_id;
	struct MANAGER(Interactive_streamline) *streamline_manager;
	void *streamline_manager_callback_id;
	struct MANAGER(Light) *light_manager;
	struct LIST(Light) *list_of_lights;
	void *light_manager_callback_id;
	struct MANAGER(Spectrum) *spectrum_manager;
	void *spectrum_manager_callback_id;
	struct Spectrum *default_spectrum;
	struct MANAGER(Texture) *texture_manager;
	void *texture_manager_callback_id;
	struct Time_keeper *default_time_keeper;
	struct User_interface *user_interface;
	/* routine to call and data to pass to a module that handles scene input */
	/* such as picking and mouse drags */
	struct Scene_input_callback input_callback;
	/* display list identifier for the scene */
	GLuint display_list;
	int display_list_current;
	/* the number of objects accessing this scene. The scene cannot be removed
		from manager unless it is 1 (ie. only the manager is accessing it) */
	int access_count;
}; /* struct Scene */

FULL_DECLARE_LIST_TYPE(Scene);
FULL_DECLARE_MANAGER_TYPE(Scene);

struct Scene_picked_object
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Describes a single picked item in a format compatible with objects in our
display hierarchy.
==============================================================================*/
{
	/* the number of this picking event */
	int hit_no;
	/* path of scene objects to picked graphic in display hierarchy */
	int number_of_scene_objects;
	struct Scene_object **scene_objects;
	/* integer names identifying parts of picked graphic, eg. node numbers */
	int number_of_subobjects;
	int *subobjects; /*???RC unsigned int instead? */
	/* z-range of picked part of graphics_object/sub_objects: */
	unsigned int nearest,farthest;
	/* so LISTs etc. can be used: */
	int access_count;
}; /* struct Scene_picked_object */

FULL_DECLARE_LIST_TYPE(Scene_picked_object);

/*
Module functions
----------------
*/
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Scene_object,position,int,compare_int)

DECLARE_LOCAL_MANAGER_FUNCTIONS(Scene)

static int Scene_refresh(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 22 June 2000

DESCRIPTION :
Tells the scene it has changed, forcing it to send the manager message
MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER.
???RC Hopefully temporary. Manually sends manager change message. Could use
manager modify not identifier instead.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_refresh);
	if (scene)
	{
		/* display list is assumed to be current */
		if (scene->scene_manager&&IS_MANAGED(Scene)(scene,scene->scene_manager))
		{
			MANAGER_NOTE_CHANGE(Scene)(
				MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Scene),scene,scene->scene_manager);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_refresh.  Scene not managed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_refresh.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_refresh */

static int Scene_notify_object_changed(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 13 October 1998

DESCRIPTION :
Scene functions such as add/remove graphics_object and set_visibility clear
the display_list_current flag of the scene. Changes to objects in the scene only
require a rebuild of those objects themselves, not the scene. This latter case
is signified by a display_list_current value of 2.
This function sets display_list_current to 2 if it is not already 0, then sends
the manager message MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER to instruct clients of
the scene that it has changed. They must call compile_Scene to make sure its
display list is made up to date.
Private to the Scene and Scene_objects
==============================================================================*/
{
	int return_code;

	ENTER(Scene_notify_object_changed);
	if (scene)
	{
		if (0 != scene->display_list_current)
		{
			/* objects in scene need recompiling */
			scene->display_list_current=2;
		}
#if defined (DEBUG)
		/*???debug*/
		printf("Scene %s changed %i\n",scene->name,scene->display_list_current);
#endif /* defined (DEBUG) */
		if (scene->scene_manager)
		{
			return_code = Scene_refresh( scene );
		}
		else
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_notify_object_changed.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_notify_object_changed */

static int Scene_changed_private(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 22 June 1998

DESCRIPTION :
Tells the scene it has changed, forcing it to send the manager message
MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER.  Recompiles the scene display list as well
as the objects in the scene unlike the public Scene_changed which only compiles
the component objects.
Private to the Scene and Scene_objects.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_changed_private);
	if (scene)
	{
		scene->display_list_current=0;
#if defined (DEBUG)
		/*???debug*/
		printf("Scene %s changed %i\n",scene->name,scene->display_list_current);
#endif /* defined (DEBUG) */
		if (scene->scene_manager)
		{
			return_code = Scene_refresh( scene );
		}
		else
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_changed_private.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_changed_private */

static int Scene_object_time_update_callback(struct Time_object *time_object,
	double current_time, void *scene_object_void)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Responds to changes in the time object.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_object_time_update_callback);
	USE_PARAMETER(current_time);
	if (time_object && (scene_object=(struct Scene_object *)scene_object_void))
	{
		switch (scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				GT_object_changed(scene_object->gt_object);
				Scene_notify_object_changed(scene_object->scene);
				return_code=1;
			} break;
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			case SCENE_OBJECT_SCENE:
			{
				/* Nothing to do currently */
				return_code=1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"Scene_object_time_update_callback.  "
					"Unknown scene object type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_time_update_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_time_update_callback */

static int Scene_object_graphics_object_update_callback(
	struct GT_object *graphics_object, void *scene_object_void)
/*******************************************************************************
LAST MODIFIED : 13 October 1998

DESCRIPTION :
Responds to changes in the graphics object.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_object_graphics_object_update_callback);
	if (graphics_object && (scene_object=(struct Scene_object *)scene_object_void))
	{
		Scene_update_time_behaviour(scene_object->scene, graphics_object);		
		Scene_notify_object_changed(scene_object->scene);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_graphics_object_update_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_graphics_object_update_callback */

static int Scene_object_element_group_update_callback(
	struct GT_element_group *gt_element_group, void *scene_object_void)
/*******************************************************************************
LAST MODIFIED : 6 April 2000

DESCRIPTION :
Responds to changes in GT_element_groups. Only updates scene if group is
visible.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_object_element_group_update_callback);
	if (gt_element_group&&(scene_object=(struct Scene_object *)scene_object_void))
	{
		if (g_VISIBLE==scene_object->visibility)
		{
			Scene_changed_private(scene_object->scene);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_element_group_update_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_element_group_update_callback */

static int compile_Scene_object(struct Scene_object *scene_object, void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Rebuilds the display list for each uncreated or morphing graphics_object in the
linked list contained in the scene_object.
==============================================================================*/
{
	float time;
	int return_code;

	ENTER(compile_Scene_object);
	USE_PARAMETER(dummy_void);
	if (scene_object)
	{
		return_code=1;
		/* only compile visible objects */
		if (g_VISIBLE==scene_object->visibility)
		{
			switch(scene_object->type)
			{
				case SCENE_OBJECT_GRAPHICS_OBJECT:
				{
					if(Scene_object_has_time(scene_object))
					{
						time = Scene_object_get_time(scene_object);
					}
					else
					{
						time = 0.0;
					}
					return_code=compile_GT_object(scene_object->gt_object,(void *)&time);
				} break;
				case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
				{
					if(Scene_object_has_time(scene_object))
					{
						time = Scene_object_get_time(scene_object);
					}
					else
					{
						time = 0.0;
					}
					return_code=compile_GT_element_group(scene_object->gt_element_group,time);
				} break;
				case SCENE_OBJECT_SCENE:
				{
					return_code=compile_Scene(scene_object->child_scene);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compile_Scene_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* compile_Scene_object */

static int execute_Scene_object(struct Scene_object *scene_object,void *dummy_void)
/*******************************************************************************
LAST MODIFIED :9 October 1998

DESCRIPTION :
Calls the display lists of all created graphics_objects in the linked list
contained in the scene_object, if it is visible.
???RC  Adding names for scene_objects and graphics_objects attached by the
linked-list. Scene_object names equal their position in the scene_object_list
while graphics_objects in graphical finite elements are numbered sequentially
from 1 to assist conversion back to graphics_object addresses in picking.
==============================================================================*/
{
	float time;
	int return_code;

	ENTER(execute_Scene_object);
	USE_PARAMETER(dummy_void);
	if (scene_object)
	{
		return_code=1;
		if (g_VISIBLE==scene_object->visibility)
		{
			/* put out the name (position) of the scene_object: */
			glLoadName((GLuint)scene_object->position);

			/* save a matrix multiply when identity transformation */
			if(scene_object->transformation)
			{
				/* Save starting modelview matrix */
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				/* perform individual object transformation */
				wrapperMultiplyCurrentMatrix(scene_object->transformation);
			}
			switch(scene_object->type)
			{
				case SCENE_OBJECT_GRAPHICS_OBJECT:
				{
					if(Scene_object_has_time(scene_object))
					{
						time = Scene_object_get_time(scene_object);
					}
					else
					{
						time = 0.0;
					}
					return_code=execute_GT_object(scene_object->gt_object,(void *)&time);
				} break;
				case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
				{
					if(Scene_object_has_time(scene_object))
					{
						time = Scene_object_get_time(scene_object);
					}
					else
					{
						time = 0.0;
					}
					return_code=execute_GT_element_group(scene_object->gt_element_group,
						time);
				} break;
				case SCENE_OBJECT_SCENE:
				{
					return_code=execute_Scene(scene_object->child_scene);
				}
			}
			if(scene_object->transformation)	
			{
				/* Restore starting modelview matrix */
				glPopMatrix();
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"execute_Scene_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* execute_Scene_object */

static void Scene_object_child_scene_change(
	struct MANAGER_MESSAGE(Scene) *message,void *scene_object_void)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Something has changed globally in the scene manager. If the contents of the
scene on this window are modified, redraw.
==============================================================================*/
{
	struct Scene_object *scene_object;

	ENTER(Scene_object_scene_change);

	if (message&&(scene_object=(struct Scene_object *)scene_object_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(Scene):
			{
				Scene_notify_object_changed(scene_object->scene);
			} break;
			case MANAGER_CHANGE_OBJECT(Scene):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Scene):
			{
				if (message->object_changed==scene_object->child_scene)
				{
					Scene_notify_object_changed(scene_object->scene);
				}
			} break;
			case MANAGER_CHANGE_ADD(Scene):
			case MANAGER_CHANGE_DELETE(Scene):
			case MANAGER_CHANGE_IDENTIFIER(Scene):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_scene_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_object_scene_change */

static struct Scene_object *CREATE(Scene_object)(char *name,struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 10 February 2000

DESCRIPTION :
Creates a vanilla Scene_object that is able to be DESTROYed, must be properly
defined by a specific creator. It is given the <name> and the parent <scene>
and is visible.
==============================================================================*/
{
	struct Scene_object *scene_object;
	ENTER(CREATE(Scene_object));
	if (name && scene)
	{
		if (ALLOCATE(scene_object,struct Scene_object,1) &&
			ALLOCATE(scene_object->name, char, strlen(name) + 1))
		{
			scene_object->type = SCENE_OBJECT_TYPE_INVALID;
			strcpy(scene_object->name, name);
			scene_object->position=0;
			scene_object->gt_object=(struct GT_object *)NULL;
			scene_object->visibility=g_VISIBLE;
			scene_object->time_object = (struct Time_object *)NULL;
			scene_object->transformation = (gtMatrix *)NULL;
			scene_object->gt_element_group = (struct GT_element_group *)NULL;
			scene_object->child_scene = (struct Scene *)NULL;	
			/* The scene_object is not accessing the scene as the
				scene owns these objects and destroys them all before
				destroying itself */
			scene_object->scene = scene;
			scene_object->scene_manager = (struct MANAGER(Scene) *)NULL;
			scene_object->scene_manager_callback_id = NULL;
			scene_object->access_count=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Scene_object).  Insufficient memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Scene_object).  Invalid arguments");
		scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (scene_object);
} /* CREATE(Scene_object) */

static struct Scene_object *create_Scene_object_with_Graphics_object(
	char *name, struct GT_object *gt_object, struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 10 February 2000

DESCRIPTION :
Creates a Scene_object with ACCESSed <gt_object> and visibility on.
==============================================================================*/
{
	struct Scene_object *scene_object;

	ENTER(CREATE(Scene_object));
	if (gt_object)
	{
		if (scene_object=CREATE(Scene_object)(name,scene))
		{
			scene_object->type = SCENE_OBJECT_GRAPHICS_OBJECT;
			scene_object->gt_object=ACCESS(GT_object)(gt_object);
			GT_object_add_callback(gt_object,
				Scene_object_graphics_object_update_callback,(void *)scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_Scene_object_with_Graphics_object.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Scene_object_with_Graphics_object.  "
			"Missing graphics object");
		scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (scene_object);
} /* create_Scene_object_with_Graphics_object */

static struct Scene_object *create_Scene_object_with_Graphical_element_group(
	char *name, struct GT_element_group *gt_element_group, struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 10 February 2000

DESCRIPTION :
Creates a Scene_object with <gt_element_group> and visibility on.
==============================================================================*/
{
	struct Scene_object *scene_object;

	ENTER(create_Scene_object_with_Graphical_element_group);
	if (gt_element_group)
	{
		if (scene_object=CREATE(Scene_object)(name,scene))
		{
			scene_object->type = SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP;
			scene_object->gt_element_group=
				ACCESS(GT_element_group)(gt_element_group);
			GT_element_group_add_callback(gt_element_group,
				Scene_object_element_group_update_callback,
				(void *)scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_Scene_object_with_Graphical_element_group.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Scene_object_with_Graphical_element_group.  "
			"Missing graphical element group");
		scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (scene_object);
} /* create_Scene_object_with_Graphical_element_group */

static struct Scene_object *create_Scene_object_with_Scene(
	char *name, struct Scene *child_scene, struct Scene *scene,
	struct MANAGER(Scene) *scene_manager)
/*******************************************************************************
LAST MODIFIED : 10 February 2000

DESCRIPTION :
Creates a Scene_object with ACCESSed <gt_object> and visibility on.
==============================================================================*/
{
	struct Scene_object *scene_object;

	ENTER(create_Scene_object_with_Scene);
	if (child_scene)
	{
		if (scene_object=CREATE(Scene_object)(name,scene))
		{
			scene_object->type = SCENE_OBJECT_SCENE;
			scene_object->child_scene = ACCESS(Scene)(child_scene);
			/* register for any scene changes */
			scene_object->scene_manager = scene_manager;
			scene_object->scene_manager_callback_id=
				MANAGER_REGISTER(Scene)(Scene_object_child_scene_change,
					(void *)scene_object, scene_manager);
		}
		else
		{
			display_message(ERROR_MESSAGE,"create_Scene_object_with_Scene.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Scene_object_with_Scene.  Missing Scene");
		scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (scene_object);
} /* create_Scene_object_with_Scene */

static int DESTROY(Scene_object)(struct Scene_object **scene_object_ptr)
/*******************************************************************************
LAST MODIFIED : 10 February 2000

DESCRIPTION :
DEACCESSes the member GT_object and removes any other dynamic fields.
==============================================================================*/
{
	struct Scene_object *scene_object;
	int return_code;

	ENTER(DESTROY(Scene_object));
	if (scene_object_ptr)
	{
		return_code=1;
		if (scene_object= *scene_object_ptr)
		{
			if (0==scene_object->access_count)
			{
				if (scene_object->transformation)
				{
					DEALLOCATE(scene_object->transformation);
				}
				if (scene_object->child_scene)
				{
					DEACCESS(Scene)(&(scene_object->child_scene));
				}
				/* The scene_object is not accessing the scene as the
					scene owns these objects and destroys them all before
					destroying itself */
				scene_object->scene = (struct Scene *)NULL;
				if (scene_object->scene_manager_callback_id &&
					scene_object->scene_manager)
				{
					MANAGER_DEREGISTER(Scene)(
						scene_object->scene_manager_callback_id,
						scene_object->scene_manager);
					scene_object->scene_manager_callback_id=(void *)NULL;
				}
				if(scene_object->gt_object)
				{
					GT_object_remove_callback(scene_object->gt_object, 
						Scene_object_graphics_object_update_callback,
						(void *)scene_object);
					DEACCESS(GT_object)(&(scene_object->gt_object));
				}
				if(scene_object->gt_element_group)
				{
					DEACCESS(GT_element_group)(&(scene_object->gt_element_group));
				}
				if(scene_object->time_object)
				{
					Time_object_remove_callback(scene_object->time_object, 
						Scene_object_time_update_callback, scene_object);
					DEACCESS(Time_object)(&(scene_object->time_object));
				}
				if(scene_object->name)
				{
					DEALLOCATE(scene_object->name);
				}
				DEALLOCATE(*scene_object_ptr);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"DESTROY(Scene_object).  Non-zero access_count");
				*scene_object_ptr=(struct Scene_object *)NULL;
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Scene_object).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene_object) */

static int Scene_object_get_position(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Returns the position identifier of <scene_object>, or 0 in case of error.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_get_position);
	if (scene_object)
	{
		return_code=scene_object->position;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_position.  Missing scene_object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_get_position */

static int Scene_object_set_position(struct Scene_object *scene_object,
	int position)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Sets the position identifier of <scene_object>. <position> must be positive.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_set_position);
	if (scene_object&&(0<position))
	{
		scene_object->position=position;
		Scene_changed_private(scene_object->scene);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_set_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_position */

static int Scene_object_copy_to_list(struct Scene_object *scene_object,
	void *scene_object_list_void)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Scene_object iterator function for duplicating lists of scene_objects.
Copies the scene_object and adds it to the list.
==============================================================================*/
{
	int i, j, return_code;
	struct LIST(Scene_object) *scene_object_list;
	struct Scene_object *scene_object_copy;

	ENTER(Scene_object_copy_to_list);
	if (scene_object&&(scene_object_list=
		(struct LIST(Scene_object) *)scene_object_list_void))
	{
		return_code=0;
		switch(scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				if (scene_object_copy=create_Scene_object_with_Graphics_object(
					scene_object->name, scene_object->gt_object, scene_object->scene))
				{
					return_code = 1;
				}
			} break;
			case SCENE_OBJECT_SCENE:
			{
				if (scene_object_copy=create_Scene_object_with_Scene(
					scene_object->name, scene_object->child_scene,
					scene_object->scene, scene_object->scene_manager))
				{
					return_code = 1;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Scene_object_copy_to_list.  Unknown Scene_object type");
				return_code=0;
			} break;
		}
		if( return_code )
		{
			scene_object_copy->type = scene_object->type;
			scene_object_copy->visibility=scene_object->visibility;
			if(scene_object->transformation)
			{
				if(ALLOCATE(scene_object_copy->transformation, gtMatrix, 1))
				{
					for (i=0;i<4;i++)
					{
						for (j=0;j<4;j++)
						{
							(*scene_object_copy->transformation)[i][j] 
								= (*scene_object->transformation)[i][j];
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_object_copy_to_list.  Unable to allocate transformation");
					return_code=0;
				}
			}
			if(scene_object->time_object)
			{
				scene_object_copy->time_object = ACCESS(Time_object)
					(scene_object->time_object);
			}
			if(return_code)
			{
				return_code=ADD_OBJECT_TO_LIST(Scene_object)(scene_object_copy,
					scene_object_list);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_copy_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_copy_to_list */

static struct GT_object *make_axis_graphics_object(
	struct Graphical_material *default_material,
	struct LIST(GT_object) *glyph_list)
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Creates a graphics object of type g_GLYPH_SET which contains a single reference
to the "axes" glyph used for displaying axes with the scene.
==============================================================================*/
{
	struct GT_glyph_set *glyph_set;
	struct GT_object *glyph,*graphics_object;
	Triple *axis1_list,*axis2_list,*axis3_list,*point_list;

	ENTER(make_axis_graphics_object);
	if (default_material&&glyph_list)
	{
		glyph_set=(struct GT_glyph_set *)NULL;
		if ((glyph=FIND_BY_IDENTIFIER_IN_LIST(GT_object,name)("axes",glyph_list))&&
			ALLOCATE(point_list,Triple,1)&&ALLOCATE(axis1_list,Triple,1)&&
			ALLOCATE(axis2_list,Triple,1)&&ALLOCATE(axis3_list,Triple,1))
		{
			(*point_list)[0]=0.0;
			(*point_list)[1]=0.0;
			(*point_list)[2]=0.0;
			(*axis1_list)[0]=1.0;
			(*axis1_list)[1]=0.0;
			(*axis1_list)[2]=0.0;
			(*axis2_list)[0]=0.0;
			(*axis2_list)[1]=1.0;
			(*axis2_list)[2]=0.0;
			(*axis3_list)[0]=0.0;
			(*axis3_list)[1]=0.0;
			(*axis3_list)[2]=1.0;
			if (!(glyph_set=CREATE(GT_glyph_set)(1,point_list,axis1_list,axis2_list,
				axis3_list,glyph,(char **)NULL,g_NO_DATA,(GTDATA *)NULL,
				/*object_name*/0,/*names*/(int *)NULL)))
			{
				DEALLOCATE(point_list);
				DEALLOCATE(axis1_list);
				DEALLOCATE(axis2_list);
				DEALLOCATE(axis3_list);
			}
		}
		if (glyph_set)
		{
			if (graphics_object=CREATE(GT_object)("axes",g_GLYPH_SET,
				default_material))
			{
				if (!GT_OBJECT_ADD(GT_glyph_set)(graphics_object,/*time*/0.0,glyph_set))
				{
					DESTROY(GT_object)(&graphics_object);
					DESTROY(GT_glyph_set)(&glyph_set);
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"make_axis_graphics_object.  Could not create glyph_set");
			graphics_object=(struct GT_object *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"make_axis_graphics_object.  Invalid argument(s)");
		graphics_object=(struct GT_object *)NULL;
	}
	LEAVE;

	return (graphics_object);
} /* make_axis_graphics_object */

static int Scene_add_scene_object(struct Scene *scene,
	struct Scene_object *scene_object,int position)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Adds <scene_object> to the list of objects on <scene> at <position>.
A position of 1 indicates the top of the list, while less than 1 or greater
than the number of graphics objects in the list puts it at the end.
The <name> is used for the scene_object and must be unique for the scene.
These routines are static as modules other than the scene should be using
functions such as Scene_add_graphics_object rather creating their own scene_object.
==============================================================================*/
{
	int return_code,number_in_list;
	struct Scene_object *scene_object_in_way;
	
	ENTER(Scene_add_scene_object);
	/* check arguments */
	if (scene&&scene_object)
	{
		if(!IS_OBJECT_IN_LIST(Scene_object)(scene_object, scene->scene_object_list))
		{
			number_in_list=NUMBER_IN_LIST(Scene_object)(scene->scene_object_list);
			if ((1>position)||(number_in_list<position))
			{
				position=number_in_list+1;
			}
			ACCESS(Scene_object)(scene_object);
			return_code=1;
			while (return_code&&scene_object)
			{
				Scene_object_set_position(scene_object,position);
				/* is there already a scene_object with that position? */
				if (scene_object_in_way=FIND_BY_IDENTIFIER_IN_LIST(Scene_object,
					position)(position,scene->scene_object_list))
				{
					/* remove the old scene_object to make way for the new */
					ACCESS(Scene_object)(scene_object_in_way);
					REMOVE_OBJECT_FROM_LIST(Scene_object)(
						scene_object_in_way,scene->scene_object_list);
				}
				if (ADD_OBJECT_TO_LIST(Scene_object)(scene_object,
					scene->scene_object_list))
				{
					DEACCESS(Scene_object)(&scene_object);
					/* the old in-the-way scene_object is now the new scene_object */
					scene_object=scene_object_in_way;
					position++;
				}
				else
				{
					DEACCESS(Scene_object)(&scene_object);
					if (scene_object_in_way)
					{
						DEACCESS(Scene_object)(&scene_object_in_way);
					}
					display_message(ERROR_MESSAGE,"Scene_add_scene_object.  "
						"Could not add object - list may have changed");
					return_code=0;
				}
			}
			/* display list of scene needs rebuilding */
			scene->display_list_current=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_add_scene_object.  "
				"Object already in list");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_add_scene_object.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_add_scene_object */

static int Scene_remove_scene_object(struct Scene *scene,
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Removes <scene object> from the list of objects on <scene>.
These routines are static as modules other than the scene should be using
functions such as Scene_add_graphics_object rather creating their own
scene_object.
==============================================================================*/
{
	int return_code,position,visibility;

	ENTER(Scene_remove_scene_object);
	/* check arguments */
	if (scene&&scene_object)
	{
		if (IS_OBJECT_IN_LIST(Scene_object)(scene_object,
			scene->scene_object_list))
		{
			/* now perform the remove */
			position=Scene_object_get_position(scene_object);
			visibility=Scene_object_get_visibility(scene_object);
			return_code=REMOVE_OBJECT_FROM_LIST(Scene_object)(scene_object,
				scene->scene_object_list);
			/* decrement positions of all remaining scene_objects */
			return_code=1;
			while (return_code&&(scene_object=FIND_BY_IDENTIFIER_IN_LIST(
				Scene_object,position)(position+1,scene->scene_object_list)))
			{
				ACCESS(Scene_object)(scene_object);
				REMOVE_OBJECT_FROM_LIST(Scene_object)(scene_object,
					scene->scene_object_list);
				Scene_object_set_position(scene_object,position);
				if (ADD_OBJECT_TO_LIST(Scene_object)(scene_object,
					scene->scene_object_list))
				{
					position++;
				}
				else
				{
					display_message(ERROR_MESSAGE,"Scene_object_list_remove_object.  "
						"Could not readjust positions - list may have changed");
					return_code=0;
				}
				DEACCESS(Scene_object)(&scene_object);
			}
			if (visibility)
			{
				/* display list of scene needs rebuilding */
				scene->display_list_current=0;
				/*???RC I think this should be here... */
				Scene_notify_object_changed(scene);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_remove_scene_object.  Object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_remove_scene_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_remove_scene_object */

int element_group_to_scene(struct GROUP(FE_element) *element_group,
	void *scene_void)
/*******************************************************************************
LAST MODIFIED : 4 April 2000

DESCRIPTION :
Iterator function for adding graphical element_groups to <scene>.
First checks <element_group> not already in scene.
==============================================================================*/
{
	int return_code;
	struct Scene *scene;

	ENTER(element_group_to_scene);
	if (element_group&&(scene=(struct Scene *)scene_void))
	{
		if (!Scene_has_graphical_element_group(scene,element_group))
		{
			Scene_add_graphical_finite_element(scene,element_group,(char *)NULL);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"element_group_to_window.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* element_group_to_scene */

static void Scene_element_group_change(
	struct MANAGER_MESSAGE(GROUP(FE_element)) *message,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 6 April 2000

DESCRIPTION :
Element group manager change callback. Adds/removes graphical element groups
from <scene> in response to manager messages.
==============================================================================*/
{
	struct GROUP(FE_element) *element_group;
	struct Scene *scene;
	struct Scene_object *scene_object;

	ENTER(Scene_element_group_change);
	/* checking arguments */
	if (message&&(scene=(struct Scene *)scene_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(GROUP(FE_element)):
			{
				if ((GRAPHICAL_ELEMENT_NONE != scene->graphical_element_mode)&&
					(GRAPHICAL_ELEMENT_MANUAL != scene->graphical_element_mode))
				{
					/* draw any new element_groups on window */
					FOR_EACH_OBJECT_IN_MANAGER(GROUP(FE_element))(
						element_group_to_scene,(void *)scene,
						scene->element_group_manager);
				}
				/* remove GT_element_groups for unmanaged element groups */
				while (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
					Scene_object_has_unmanaged_element_group,
					(void *)scene->element_group_manager,scene->scene_object_list))
				{
					Scene_remove_scene_object(scene,scene_object);
				}
			} break;
			case MANAGER_CHANGE_ADD(GROUP(FE_element)):
			{
				if ((GRAPHICAL_ELEMENT_NONE != scene->graphical_element_mode)&&
					(GRAPHICAL_ELEMENT_MANUAL != scene->graphical_element_mode))
				{
					Scene_add_graphical_finite_element(scene,message->object_changed,
						(char *)NULL);
				}
			} break;
			case MANAGER_CHANGE_DELETE(GROUP(FE_element)):
			{
				/* if there is a GT_element_group for the element_group that is being
					 deleted, remove it from the scene */
				if ((element_group=message->object_changed)&&
					(scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
					Scene_object_has_element_group,(void *)element_group,
					scene->scene_object_list)))
				{
					Scene_remove_scene_object(scene,scene_object);
				}
			} break;
			case MANAGER_CHANGE_OBJECT(GROUP(FE_element)):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(GROUP(FE_element)):
			case MANAGER_CHANGE_IDENTIFIER(GROUP(FE_element)):
			{
				/* nothing: object changes are handled by graphical element itself  */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_element_group_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_element_group_change */

static void Scene_streamline_change(
	struct MANAGER_MESSAGE(Interactive_streamline) *message,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 10 December 1997

DESCRIPTION :
Streamline manager change callback.
==============================================================================*/
{
	struct Scene *scene;
	struct GT_object *gt_object;

	ENTER(Scene_streamline_change);
	/* checking arguments */
	if (message&&(scene=(struct Scene *)scene_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(Interactive_streamline):
			{
				/*???debug*/
				printf("Scene_streamline_change: CHANGE_ALL?? scene %s\n",scene->name);
			} break;
			case MANAGER_CHANGE_ADD(Interactive_streamline):
			{
				/*???debug*/
				printf("Scene_streamline_change: CHANGE_ADD scene %s\n",scene->name);
				if ( get_streamline_gt_object ( message->object_changed, &gt_object ))
				{
					Scene_add_graphics_object(scene,gt_object,0,gt_object->name);
				}
			} break;
			case MANAGER_CHANGE_OBJECT(Interactive_streamline):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Interactive_streamline):
			{
				/*???debug*/
				printf("Scene_streamline_change: CHANGE_MODIFY scene %s\n",scene->name);
				interactive_streamline_set_changed( message->object_changed );
				Scene_notify_object_changed(scene);
			} break;
			case MANAGER_CHANGE_DELETE(Interactive_streamline):
			{
				/*???debug*/
				printf("Scene_streamline_change: CHANGE_DELETE?? scene %s\n",
					scene->name);
			} break;
			case MANAGER_CHANGE_IDENTIFIER(Interactive_streamline):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_streamline_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_streamline_change */

static int Scene_object_set_not_created_if_material(
	struct Scene_object *scene_object,void *changed_material_void)
/*******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Since spectrums combine their colour with the material at compile time, just
recompiling the display list of changed materials does not always produce the
right effect when combined with spectrums. This function tells graphics objects
in such situations that their display lists need to be updated.
==============================================================================*/
{
	int return_code;
	struct Graphical_material *changed_material;
	struct GT_object *gt_object;

	ENTER(Scene_object_set_not_created_if_material);
	if (scene_object)
	{
		changed_material=(struct Graphical_material *)changed_material_void;
		switch (scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			{
				if (first_settings_in_GT_element_group_that(
					scene_object->gt_element_group,
					GT_element_settings_uses_material_with_spectrum,
					changed_material_void))
				{
					for_each_settings_in_GT_element_group(scene_object->gt_element_group,
						GT_element_settings_material_change,changed_material_void);
				}
			} break;
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				for (gt_object=Scene_object_get_gt_object(scene_object);gt_object != NULL;
					  gt_object=gt_object->nextobject)
				{
					if (gt_object->spectrum&&gt_object->default_material&&
						(!changed_material||(gt_object->default_material==changed_material)))
					{
						GT_object_changed(gt_object);
					}
				}
			} break;
			case SCENE_OBJECT_SCENE:
			{
				/* Each scene should get its own changed message */
				return_code=1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Scene_object_set_not_created_if_material.  Unknown scene_object type");
				return_code=0;
			} break;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_set_not_created_if_material.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_not_created_if_material */

static void Scene_graphical_material_change(
	struct MANAGER_MESSAGE(Graphical_material) *message,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 17 June 1998

DESCRIPTION :
Something has changed globally in the material manager.
Tell the scene it has changed and it will rebuild affected materials too.
==============================================================================*/
{
	struct Scene *scene;

	ENTER(Scene_graphical_material_change);
	if (message&&(scene=(struct Scene *)scene_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(Graphical_material):
			case MANAGER_CHANGE_OBJECT(Graphical_material):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Graphical_material):
			{
				/* tell objects using material with spectrums to rebuild */
				FOR_EACH_OBJECT_IN_LIST(Scene_object)(
					Scene_object_set_not_created_if_material,
					(void *)(message->object_changed),scene->scene_object_list);
				/*???RC Inefficient if modified material not used in scene */
				Scene_notify_object_changed(scene);
			} break;
			case MANAGER_CHANGE_ADD(Graphical_material):
			case MANAGER_CHANGE_DELETE(Graphical_material):
			case MANAGER_CHANGE_IDENTIFIER(Graphical_material):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_graphical_material_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_graphical_material_change */

static int Scene_object_set_not_created_if_spectrum(
	struct Scene_object *scene_object,void *changed_spectrum_void)
/*******************************************************************************
LAST MODIFIED : 18 June 1998

DESCRIPTION :
Marks as g_NOT_CREATED all graphics objects in the linked list starting at
scene_object->gt_object that use <changed_spectrum>. If <changed_spectrum> is
NULL, sets all objects that have a non-NULL spectrum to g_NOT_CREATED.
???RC Will need further work if spectrum not just held in <spectrum> member of
graphics_object - eg. for multiple spectra.
==============================================================================*/
{
	int return_code;
	struct Spectrum *changed_spectrum;
	struct GT_object *gt_object;

	ENTER(Scene_object_set_not_created_if_spectrum);
	if (scene_object)
	{
		changed_spectrum=(struct Spectrum *)changed_spectrum_void;
		switch (scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			{
				if (first_settings_in_GT_element_group_that(scene_object->gt_element_group,
					GT_element_settings_uses_spectrum,changed_spectrum_void))
				{
					for_each_settings_in_GT_element_group(scene_object->gt_element_group,
						GT_element_settings_spectrum_change,changed_spectrum_void);
				}
			} break;
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				for (gt_object=Scene_object_get_gt_object(scene_object);gt_object != NULL;
					  gt_object=gt_object->nextobject)
				{
					if ((changed_spectrum&&(gt_object->spectrum == changed_spectrum))||
						(!changed_spectrum&&(gt_object->spectrum)))
					{
						GT_object_changed(gt_object);
					}
				}
			} break;
			case SCENE_OBJECT_SCENE:
			{
				/* Each scene should get its own changed message */
				return_code=1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Scene_object_set_not_created_if_spectrum.  Unknown scene_object type");
				return_code=0;
			} break;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_set_not_created_if_spectrum.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_not_created_if_spectrum */

static void Scene_spectrum_change(
	struct MANAGER_MESSAGE(Spectrum) *message,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Something has changed globally in the spectrum manager. Mark status of all
graphics_obects using the affected spectrums as g_NOT_CREATED.
==============================================================================*/
{
	struct Scene *scene;

	ENTER(Scene_spectrum_change);
	if (message&&(scene=(struct Scene *)scene_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(Spectrum):
			case MANAGER_CHANGE_OBJECT(Spectrum):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Spectrum):
			{
				FOR_EACH_OBJECT_IN_LIST(Scene_object)(
					Scene_object_set_not_created_if_spectrum,
					(void *)(message->object_changed),scene->scene_object_list);
				Scene_notify_object_changed(scene);
			} break;
			case MANAGER_CHANGE_ADD(Spectrum):
			case MANAGER_CHANGE_DELETE(Spectrum):
			case MANAGER_CHANGE_IDENTIFIER(Spectrum):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_spectrum_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_spectrum_change */

static void Scene_texture_change(
	struct MANAGER_MESSAGE(Texture) *message,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 15 June 1998

DESCRIPTION :
Something has changed globally in the texture manager.
Tell the scene it has changed and it will rebuild affected textures too.
==============================================================================*/
{
	struct Scene *scene;

	ENTER(Scene_texture_change);
	if (message&&(scene=(struct Scene *)scene_void))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_ALL(Texture):
			case MANAGER_CHANGE_OBJECT(Texture):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Texture):
			{
				/*???RC difficult to work out which materials are used in scene, so
					 following inefficient */
				/* update scene if changed texture is used in any materials */
				if (FIRST_OBJECT_IN_MANAGER_THAT(Graphical_material)(
					Graphical_material_uses_texture,(void *)message->object_changed,
					scene->graphical_material_manager))
				{
					Scene_notify_object_changed(scene);
				}
			} break;
			case MANAGER_CHANGE_ADD(Texture):
			case MANAGER_CHANGE_DELETE(Texture):
			case MANAGER_CHANGE_IDENTIFIER(Texture):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_texture_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Scene_texture_change */

static int Scene_time_update_callback(struct Time_object *time_object,
	double current_time, void *scene_void)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Responds to changes in the time object.
==============================================================================*/
{
	int return_code;
	struct Scene *scene;

	ENTER(Scene_time_update_callback);
	USE_PARAMETER(current_time);
	if (time_object && (scene=(struct Scene *)scene_void))
	{
		Scene_notify_object_changed(scene);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_time_update_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_time_update_callback */

static int Scene_object_update_time_behaviour(
	struct Scene_object *scene_object, void *time_object_void)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Time_object *time_object;

	ENTER(Scene_object_update_time_behaviour);
	/* check arguments */
	if (scene_object && (time_object = (struct Time_object *)time_object_void))
	{
		switch (scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				/* Ensure the Scene object has a time object if the graphics
					object has more than one time */
				if(1 < GT_object_get_number_of_times(scene_object->gt_object))
				{
					if(!Scene_object_has_time(scene_object))
					{
						Scene_object_set_time_object(scene_object, time_object);
					}
				}
				return_code=1;
			} break;
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			case SCENE_OBJECT_SCENE:
			{
				/* Nothing to do currently */
				return_code=1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"Scene_object_update_time_behaviour.  "
					"Unknown scene object type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_object_update_time_behaviour.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_update_time_behaviour */

struct Scene_picked_object_get_nearest_element_data
{
	/* "nearest" value from Scene_picked_object for picked_element */
	unsigned int nearest;
	struct FE_element *nearest_element;
	/* group that the element must be in, or any group if NULL */
	struct GROUP(FE_element) *element_group;
	/* information about the nearest element */
	struct Scene_picked_object *scene_picked_object;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *gt_element_settings;
};

static int Scene_picked_object_get_nearest_element(
	struct Scene_picked_object *scene_picked_object,
	void *nearest_element_data_void)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
If the <scene_picked_object> refers to an element, the "nearest" value is
compared with that for the current nearest element in the
<nearest_element_data>. If there was no current nearest element or the new
element is nearer, it becomes the nearest element and its "nearest" value is
stored in the nearest_element_data.
==============================================================================*/
{
	int return_code;
	struct CM_element_information cm;
	struct FE_element *element;
	struct GROUP(FE_element) *element_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene_object *scene_object;
	struct Scene_picked_object_get_nearest_element_data	*nearest_element_data;

	ENTER(Scene_picked_object_get_nearest_element);
	if (scene_picked_object&&(nearest_element_data=
		(struct Scene_picked_object_get_nearest_element_data	*)
		nearest_element_data_void))
	{
		return_code=1;
		/* proceed only if there is no picked_element or object is nearer */
		if (((struct FE_element *)NULL==nearest_element_data->nearest_element)||
			(Scene_picked_object_get_nearest(scene_picked_object) <
				nearest_element_data->nearest))
		{
			/* is the last scene_object a Graphical_element wrapper, and does the
				 settings for the graphic refer to elements? */
			if ((scene_object=Scene_picked_object_get_Scene_object(
				scene_picked_object,
				Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
				&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
					Scene_object_get_type(scene_object))&&(gt_element_group=
						Scene_object_get_graphical_element_group(scene_object))&&
				(element_group=GT_element_group_get_element_group(gt_element_group))&&
				(2<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
				(settings=get_settings_at_position_in_GT_element_group(gt_element_group,
					Scene_picked_object_get_subobject(scene_picked_object,0)))&&
				(GT_element_settings_selects_elements(settings)))
			{
				if (CM_element_information_from_graphics_name(&cm,
					Scene_picked_object_get_subobject(scene_picked_object,1))&&
					(element=FIND_BY_IDENTIFIER_IN_GROUP(FE_element,identifier)(
						&cm,element_group)))
				{
					if ((!nearest_element_data->element_group)||
						((struct FE_element *)NULL != IS_OBJECT_IN_GROUP(FE_element)(
							element,nearest_element_data->element_group)))
					{
						nearest_element_data->nearest_element=element;
						nearest_element_data->scene_picked_object=scene_picked_object;
						nearest_element_data->gt_element_group=gt_element_group;
						nearest_element_data->gt_element_settings=settings;
						nearest_element_data->nearest=
							Scene_picked_object_get_nearest(scene_picked_object);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_picked_object_get_nearest_element.  "
						"Invalid element %s %d",CM_element_type_string(cm.type),cm.number);
					return_code=0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_nearest_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_nearest_element */

static int Scene_picked_object_get_picked_elements(
	struct Scene_picked_object *scene_picked_object,
	void *picked_elements_list_void)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
If the <scene_picked_object> refers to an element, it is converted into
an FE_element and added to the <picked_elements_list>.
==============================================================================*/
{
	int return_code;
	struct CM_element_information cm;
	struct FE_element *element;
	struct GROUP(FE_element) *element_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene_object *scene_object;

	ENTER(Scene_picked_object_get_picked_elements);
	if (scene_picked_object&&picked_elements_list_void)
	{
		return_code=1;
		/* is the last scene_object a Graphical_element wrapper, and does the
			 settings for the graphic refer to elements? */
		if ((scene_object=Scene_picked_object_get_Scene_object(scene_picked_object,
			Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
			&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
				Scene_object_get_type(scene_object))&&(gt_element_group=
					Scene_object_get_graphical_element_group(scene_object))&&
			(element_group=GT_element_group_get_element_group(gt_element_group))&&
			(2<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
			(settings=get_settings_at_position_in_GT_element_group(
				gt_element_group,
				Scene_picked_object_get_subobject(scene_picked_object,0)))&&
			(GT_element_settings_selects_elements(settings)))
		{
			if (CM_element_information_from_graphics_name(&cm,
				Scene_picked_object_get_subobject(scene_picked_object,1))&&
				(element=FIND_BY_IDENTIFIER_IN_GROUP(FE_element,identifier)(
					&cm,element_group)))
			{
				return_code=
					ensure_FE_element_is_in_list(element,picked_elements_list_void);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_picked_object_get_picked_elements.  "
					"Invalid element %s %d",CM_element_type_string(cm.type),cm.number);
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_picked_elements.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_picked_elements */

struct Scene_picked_object_get_nearest_element_point_data
{
	/* "nearest" value from Scene_picked_object for picked_element_point */
	unsigned int nearest;
	struct Element_point_ranges *nearest_element_point;
	/* group that the element_point must be in, or any group if NULL */
	struct GROUP(FE_element) *element_group;
	/* information about the nearest element_point */
	struct Scene_picked_object *scene_picked_object;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *gt_element_settings;
};

static int Scene_picked_object_get_nearest_element_point(
	struct Scene_picked_object *scene_picked_object,
	void *nearest_element_point_data_void)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
If the <scene_picked_object> refers to an element_point, the "nearest" value is
compared with that for the current nearest element_point in the
<nearest_element_point_data>. If there was no current nearest element_point or
the new element_point is nearer, it becomes the nearest element_point and its
"nearest" value is stored in the nearest_element_point_data.
Note that the <nearest_element_point> is an Element_point_ranges structure
created to store the nearest point; it is up to the calling function to manage
and destroy it once returned.
==============================================================================*/
{
	int element_point_number,face_number,i,return_code,
		top_level_number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	struct CM_element_information cm;
	struct Element_discretization element_discretization;
	struct Element_point_ranges *element_point_ranges;
	struct Element_point_ranges_identifier element_point_ranges_identifier;
	struct FE_element *element,*top_level_element;
	struct FE_field *native_discretization_field;
	struct GROUP(FE_element) *element_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene_object *scene_object;
	struct Scene_picked_object_get_nearest_element_point_data
		*nearest_element_point_data;
	Triple xi;

	ENTER(Scene_picked_object_get_nearest_element_point);
	if (scene_picked_object&&(nearest_element_point_data=
		(struct Scene_picked_object_get_nearest_element_point_data	*)
		nearest_element_point_data_void))
	{
		return_code=1;
		/* proceed only if there is no picked_element_point or object is nearer */
		if (((struct Element_point_ranges *)NULL==
			nearest_element_point_data->nearest_element_point)||
			(Scene_picked_object_get_nearest(scene_picked_object) <
				nearest_element_point_data->nearest))
		{
			/* is the last scene_object a Graphical_element wrapper, and does the
				 settings for the graphic refer to element_points? */
			if ((scene_object=Scene_picked_object_get_Scene_object(
				scene_picked_object,
				Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
				&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
					Scene_object_get_type(scene_object))&&(gt_element_group=
						Scene_object_get_graphical_element_group(scene_object))&&
				(element_group=GT_element_group_get_element_group(gt_element_group))&&
				(3<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
				(settings=get_settings_at_position_in_GT_element_group(
					gt_element_group,
					Scene_picked_object_get_subobject(scene_picked_object,0)))&&
				(GT_ELEMENT_SETTINGS_ELEMENT_POINTS==
					GT_element_settings_get_settings_type(settings)))
			{
				if (CM_element_information_from_graphics_name(&cm,
					Scene_picked_object_get_subobject(scene_picked_object,1))&&
					(element=FIND_BY_IDENTIFIER_IN_GROUP(FE_element,identifier)(&cm,
						element_group)))
				{
					if ((!nearest_element_point_data->element_group)||
						((struct FE_element *)NULL != IS_OBJECT_IN_GROUP(FE_element)(
							element,nearest_element_point_data->element_group)))
					{
						/* determine discretization of element for graphic */
						top_level_element=(struct FE_element *)NULL;
						GT_element_settings_get_discretization(settings,
							&element_discretization);
						top_level_number_in_xi[0]=element_discretization.number_in_xi1;
						top_level_number_in_xi[1]=element_discretization.number_in_xi2;
						top_level_number_in_xi[2]=element_discretization.number_in_xi3;
						GT_element_settings_get_face(settings,&face_number);
						native_discretization_field=
							GT_element_settings_get_native_discretization_field(settings);
						if (get_FE_element_discretization(element,element_group,face_number,
							native_discretization_field,top_level_number_in_xi,
							&top_level_element,element_point_ranges_identifier.number_in_xi))
						{
							element_point_ranges_identifier.element=element;
							element_point_ranges_identifier.top_level_element=
								top_level_element;
							element_point_ranges_identifier.xi_discretization_mode=
								GT_element_settings_get_xi_discretization_mode(settings);
							if (XI_DISCRETIZATION_EXACT_XI==
								element_point_ranges_identifier.xi_discretization_mode)
							{
								for (i=0;i<MAXIMUM_ELEMENT_XI_DIMENSIONS;i++)
								{
									element_point_ranges_identifier.number_in_xi[i]=1;
								}
							}
							GT_element_settings_get_seed_xi(settings,xi);
							/*???RC temporary, hopefully */
							for (i=0;i<3;i++)
							{
								element_point_ranges_identifier.exact_xi[i]=xi[i];
							}
							if (element_point_ranges=CREATE(Element_point_ranges)(
								&element_point_ranges_identifier))
							{
								element_point_number=
									Scene_picked_object_get_subobject(scene_picked_object,2);
								if (Element_point_ranges_add_range(element_point_ranges,
									element_point_number,element_point_number))
								{
									if (nearest_element_point_data->nearest_element_point)
									{
										DESTROY(Element_point_ranges)(
											&(nearest_element_point_data->nearest_element_point));
									}
									nearest_element_point_data->nearest_element_point=
										element_point_ranges;
									nearest_element_point_data->scene_picked_object=
										scene_picked_object;
									nearest_element_point_data->gt_element_group=gt_element_group;
									nearest_element_point_data->gt_element_settings=settings;
									nearest_element_point_data->nearest=
										Scene_picked_object_get_nearest(scene_picked_object);
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"Scene_picked_object_get_nearest_element_point.  "
										"Could not add element point range");
									DESTROY(Element_point_ranges)(&element_point_ranges);
									return_code=0;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"Scene_picked_object_get_nearest_element_point.  "
									"Could not create Element_point_ranges");
								return_code=0;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"Scene_picked_object_get_nearest_element_point.  "
								"Could not get discretization");
							return_code=0;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_picked_object_get_nearest_element_point.  "
						"Invalid element %s %d",CM_element_type_string(cm.type),cm.number);
					return_code=0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_nearest_element_point.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_nearest_element_point */

static int Scene_picked_object_get_picked_element_points(
	struct Scene_picked_object *scene_picked_object,
	void *picked_element_points_list_void)
/*******************************************************************************
LAST MODIFIED : 27 June 2000

DESCRIPTION :
If the <scene_picked_object> refers to an element_point, it is converted into
an Element_point_ranges and added to the <picked_element_points_list>.
==============================================================================*/
{
	int element_point_number,face_number,i,return_code,
		top_level_number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	struct CM_element_information cm;
	struct Element_discretization element_discretization;
	struct Element_point_ranges *element_point_ranges;
	struct Element_point_ranges_identifier element_point_ranges_identifier;
	struct FE_element *element,*top_level_element;
	struct FE_field *native_discretization_field;
	struct GROUP(FE_element) *element_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene_object *scene_object;
	Triple xi;

	ENTER(Scene_picked_object_get_picked_element_points);
	if (scene_picked_object&&picked_element_points_list_void)
	{
		return_code=1;
		/* is the last scene_object a Graphical_element wrapper, and does the
			 settings for the graphic refer to element_points? */
		if ((scene_object=Scene_picked_object_get_Scene_object(scene_picked_object,
			Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
			&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
				Scene_object_get_type(scene_object))&&(gt_element_group=
					Scene_object_get_graphical_element_group(scene_object))&&
			(element_group=GT_element_group_get_element_group(gt_element_group))&&
			(3<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
			(settings=get_settings_at_position_in_GT_element_group(
				gt_element_group,
				Scene_picked_object_get_subobject(scene_picked_object,0)))&&
			(GT_ELEMENT_SETTINGS_ELEMENT_POINTS==
				GT_element_settings_get_settings_type(settings)))
		{
			if (CM_element_information_from_graphics_name(&cm,
				Scene_picked_object_get_subobject(scene_picked_object,1))&&
				(element=FIND_BY_IDENTIFIER_IN_GROUP(FE_element,identifier)(&cm,
					element_group)))
			{
				/* determine discretization of element for graphic */
				top_level_element=(struct FE_element *)NULL;
				GT_element_settings_get_discretization(settings,
					&element_discretization);
				top_level_number_in_xi[0]=element_discretization.number_in_xi1;
				top_level_number_in_xi[1]=element_discretization.number_in_xi2;
				top_level_number_in_xi[2]=element_discretization.number_in_xi3;
				GT_element_settings_get_face(settings,&face_number);
				native_discretization_field=
					GT_element_settings_get_native_discretization_field(settings);
				if (get_FE_element_discretization(element,element_group,face_number,
					native_discretization_field,top_level_number_in_xi,
					&top_level_element,element_point_ranges_identifier.number_in_xi))
				{
					element_point_ranges_identifier.element=element;
					element_point_ranges_identifier.top_level_element=top_level_element;
					element_point_ranges_identifier.xi_discretization_mode=
						GT_element_settings_get_xi_discretization_mode(settings);
					if (XI_DISCRETIZATION_EXACT_XI==
						element_point_ranges_identifier.xi_discretization_mode)
					{
						for (i=0;i<MAXIMUM_ELEMENT_XI_DIMENSIONS;i++)
						{
							element_point_ranges_identifier.number_in_xi[i]=1;
						}
					}
					GT_element_settings_get_seed_xi(settings,xi);
					/*???RC temporary, hopefully */
					for (i=0;i<3;i++)
					{
						element_point_ranges_identifier.exact_xi[i]=xi[i];
					}
					if (element_point_ranges=CREATE(Element_point_ranges)(
						&element_point_ranges_identifier))
					{
						element_point_number=
							Scene_picked_object_get_subobject(scene_picked_object,2);
						if (!(Element_point_ranges_add_range(element_point_ranges,
							element_point_number,element_point_number)&&
							Element_point_ranges_add_to_list(element_point_ranges,
								picked_element_points_list_void)))
						{
							display_message(ERROR_MESSAGE,
								"Scene_picked_object_get_picked_element_points.  "
								"Could not add element point to picked list");
							return_code=0;
						}
						DESTROY(Element_point_ranges)(&element_point_ranges);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Scene_picked_object_get_picked_element_points.  "
							"Could not create Element_point_ranges");
						return_code=0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_picked_object_get_picked_element_points.  "
						"Could not get discretization");
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_picked_object_get_picked_element_points.  "
					"Invalid element %s %d",CM_element_type_string(cm.type),cm.number);
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_picked_element_points.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_picked_element_points */

struct Scene_picked_object_get_nearest_node_data
{
	/* "nearest" value from Scene_picked_object for picked_node */
	unsigned int nearest;
	struct FE_node *nearest_node;
	/* flag set when searching for nearest data point rather than node */
	int use_data;
	/* group that the node must be in, or any group if NULL */
	struct GROUP(FE_node) *node_group;
	/* information about the nearest node */
	struct Scene_picked_object *scene_picked_object;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *gt_element_settings;
};

static int Scene_picked_object_get_nearest_node(
	struct Scene_picked_object *scene_picked_object,void *nearest_node_data_void)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
If the <scene_picked_object> refers to a node, the "nearest" value is compared
with that for the current nearest node in the <nearest_node_data>. If there was
no current nearest node or the new node is nearer, it becomes the picked node
and its "nearest" value is stored in the nearest_node_data.
==============================================================================*/
{
	enum GT_element_settings_type settings_type;
	int node_number,return_code;
	struct FE_node *node;
	struct Scene_object *scene_object;
	struct Scene_picked_object_get_nearest_node_data *nearest_node_data;
	struct GROUP(FE_node) *node_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;

	ENTER(Scene_picked_object_get_nearest_node);
	if (scene_picked_object&&(nearest_node_data=
		(struct Scene_picked_object_get_nearest_node_data	*)nearest_node_data_void))
	{
		return_code=1;
		/* proceed only if there is no picked_node or object is nearer */
		if (((struct FE_node *)NULL==nearest_node_data->nearest_node)||
			(Scene_picked_object_get_nearest(scene_picked_object) <
				nearest_node_data->nearest))
		{
			/* is the last scene_object a Graphical_element wrapper, and does the
				 settings for the graphic refer to node_points or data_points? */
			if ((scene_object=Scene_picked_object_get_Scene_object(
				scene_picked_object,
				Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
				&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
					Scene_object_get_type(scene_object))&&(gt_element_group=
						Scene_object_get_graphical_element_group(scene_object))&&
				(3<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
				(settings=get_settings_at_position_in_GT_element_group(
					gt_element_group,
					Scene_picked_object_get_subobject(scene_picked_object,0)))&&
				(((GT_ELEMENT_SETTINGS_NODE_POINTS==
					(settings_type=GT_element_settings_get_settings_type(settings)))&&
					(!(nearest_node_data->use_data))&&
					(node_group=GT_element_group_get_node_group(gt_element_group)))||
					((GT_ELEMENT_SETTINGS_DATA_POINTS==settings_type)&&
						nearest_node_data->use_data&&
					(node_group=GT_element_group_get_data_group(gt_element_group)))))
			{
				node_number=Scene_picked_object_get_subobject(scene_picked_object,2);
				if (node=FIND_BY_IDENTIFIER_IN_GROUP(FE_node,cm_node_identifier)(
					node_number,node_group))
				{
					/* is the node in the nearest_node_data->node_group, if supplied */
					if ((!nearest_node_data->node_group)||((struct FE_node *)NULL !=
						IS_OBJECT_IN_GROUP(FE_node)(node,nearest_node_data->node_group)))
					{
						nearest_node_data->nearest_node=node;
						nearest_node_data->scene_picked_object=scene_picked_object;
						nearest_node_data->gt_element_group=gt_element_group;
						nearest_node_data->gt_element_settings=settings;
						nearest_node_data->nearest=
							Scene_picked_object_get_nearest(scene_picked_object);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_picked_object_get_nearest_node.  Invalid node %d",
						node_number);
					return_code=0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_nearest_node.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_nearest_node */

struct Scene_picked_object_get_picked_nodes_data
{
	struct LIST(FE_node) *node_list;
	/* flag set when searching for nearest data point rather than node */
	int use_data;
};

static int Scene_picked_object_get_picked_nodes(
	struct Scene_picked_object *scene_picked_object,void *picked_nodes_data_void)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
If the <scene_picked_object> refers to a node and the node is in the given
manager, ensures it is in the list.
==============================================================================*/
{
	enum GT_element_settings_type settings_type;
	int node_number,return_code;
	struct FE_node *node;
	struct Scene_object *scene_object;
	struct Scene_picked_object_get_picked_nodes_data *picked_nodes_data;
	struct GROUP(FE_node) *node_group;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;

	ENTER(Scene_picked_object_get_picked_nodes);
	if (scene_picked_object&&(picked_nodes_data=
		(struct Scene_picked_object_get_picked_nodes_data	*)picked_nodes_data_void))
	{
		return_code=1;
		/* is the last scene_object a Graphical_element wrapper, and does the
			 settings for the graphic refer to node_points? */
		if ((scene_object=Scene_picked_object_get_Scene_object(
			scene_picked_object,
			Scene_picked_object_get_number_of_scene_objects(scene_picked_object)-1))
			&&(SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==
				Scene_object_get_type(scene_object))&&(gt_element_group=
					Scene_object_get_graphical_element_group(scene_object))&&
			(3<=Scene_picked_object_get_number_of_subobjects(scene_picked_object))&&
			(settings=get_settings_at_position_in_GT_element_group(
				gt_element_group,
				Scene_picked_object_get_subobject(scene_picked_object,0)))&&
			(((GT_ELEMENT_SETTINGS_NODE_POINTS==
				(settings_type=GT_element_settings_get_settings_type(settings)))&&
				(!(picked_nodes_data->use_data))&&
				(node_group=GT_element_group_get_node_group(gt_element_group)))||
				((GT_ELEMENT_SETTINGS_DATA_POINTS==settings_type)&&
					picked_nodes_data->use_data&&
					(node_group=GT_element_group_get_data_group(gt_element_group)))))
		{
			node_number=Scene_picked_object_get_subobject(scene_picked_object,2);
			if (node=FIND_BY_IDENTIFIER_IN_GROUP(FE_node,cm_node_identifier)(
				node_number,node_group))
			{
				return_code=ensure_FE_node_is_in_list(node,
					(void *)(picked_nodes_data->node_list));
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_picked_object_get_picked_nodes.  Invalid node %d",node_number);
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_picked_nodes.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_picked_nodes */

/*
Global functions
----------------
*/

char *Scene_graphical_element_mode_string(enum Scene_graphical_element_mode graphical_element_mode)
/*******************************************************************************
LAST MODIFIED : 4 February 2000

DESCRIPTION :
Returns a pointer to a static string describing the <graphical_element_mode>.
This string should match the command used to create that type of settings.
The returned string must not be DEALLOCATEd!
==============================================================================*/
{
	char *return_string;

	ENTER(Scene_graphical_element_mode_string);
	switch (graphical_element_mode)
	{
		case GRAPHICAL_ELEMENT_NONE:
		{
			return_string="no_g_element";
		} break;
		case GRAPHICAL_ELEMENT_INVISIBLE:
		{
			return_string="invisible_g_element";
		} break;
		case GRAPHICAL_ELEMENT_EMPTY:
		{
			return_string="empty_g_element";
		} break;
		case GRAPHICAL_ELEMENT_LINES:
		{
			return_string="g_element_lines";
		} break;
		case GRAPHICAL_ELEMENT_MANUAL:
		{
			return_string="manual_g_element";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Scene_graphical_element_mode_string.  Unknown graphical_element_mode");
			return_string=(char *)NULL;
		} break;
	}
	LEAVE;

	return (return_string);
} /* Scene_graphical_element_mode_string */

char **Scene_graphical_element_mode_get_valid_strings(int *number_of_valid_strings)
/*******************************************************************************
LAST MODIFIED : 4 February 2000

DESCRIPTION :
Returns and allocated array of pointers to all static strings for valid
Scene_graphical_element_modes - obtained from function Scene_graphical_element_mode_string.
Up to calling function to deallocate returned array - but not the strings in it!
==============================================================================*/
{
	char **valid_strings;
	enum Scene_graphical_element_mode graphical_element_mode;
	int i;

	ENTER(Scene_graphical_element_mode_get_valid_strings);
	if (number_of_valid_strings)
	{
		*number_of_valid_strings=0;
		graphical_element_mode=GRAPHICAL_ELEMENT_MODE_BEFORE_FIRST;
		graphical_element_mode++;
		while (graphical_element_mode<GRAPHICAL_ELEMENT_MODE_AFTER_LAST)
		{
			(*number_of_valid_strings)++;
			graphical_element_mode++;
		}
		if (ALLOCATE(valid_strings,char *,*number_of_valid_strings))
		{
			graphical_element_mode=GRAPHICAL_ELEMENT_MODE_BEFORE_FIRST;
			graphical_element_mode++;
			i=0;
			while (graphical_element_mode<GRAPHICAL_ELEMENT_MODE_AFTER_LAST)
			{
				valid_strings[i]=Scene_graphical_element_mode_string(graphical_element_mode);
				i++;
				graphical_element_mode++;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_graphical_element_mode_get_valid_strings.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_graphical_element_mode_get_valid_strings.  Invalid argument");
		valid_strings=(char **)NULL;
	}
	LEAVE;

	return (valid_strings);
} /* Scene_graphical_element_mode_get_valid_strings */

enum Scene_graphical_element_mode Scene_graphical_element_mode_from_string(
	char *graphical_element_mode_string)
/*******************************************************************************
LAST MODIFIED : 4 February 2000

DESCRIPTION :
Returns the <Scene_graphical_element_mode> described by <graphical_element_mode_string>, or
INVALID if not recognized.
==============================================================================*/
{
	enum Scene_graphical_element_mode graphical_element_mode;

	ENTER(Scene_graphical_element_mode_from_string);
	if (graphical_element_mode_string)
	{
		graphical_element_mode=GRAPHICAL_ELEMENT_MODE_BEFORE_FIRST;
		graphical_element_mode++;
		while ((graphical_element_mode<GRAPHICAL_ELEMENT_MODE_AFTER_LAST)&&
			(!fuzzy_string_compare_same_length(graphical_element_mode_string,
				Scene_graphical_element_mode_string(graphical_element_mode))))
		{
			graphical_element_mode++;
		}
		if (GRAPHICAL_ELEMENT_MODE_AFTER_LAST==graphical_element_mode)
		{
			graphical_element_mode=GRAPHICAL_ELEMENT_MODE_INVALID;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_graphical_element_mode_from_string.  Invalid argument");
		graphical_element_mode=GRAPHICAL_ELEMENT_MODE_INVALID;
	}
	LEAVE;

	return (graphical_element_mode);
} /* Scene_graphical_element_mode_from_string */

DECLARE_OBJECT_FUNCTIONS(Scene_object)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Scene_object)
DECLARE_INDEXED_LIST_FUNCTIONS(Scene_object)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Scene_object, \
	position,int,compare_int)

enum GT_visibility_type Scene_object_get_visibility(
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Returns the visibility of <scene_object>.
==============================================================================*/
{
	enum GT_visibility_type visibility;

	ENTER(Scene_object_get_visibility);
	if (scene_object)
	{
		visibility=scene_object->visibility;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_visibility.  Invalid argument(s)");
		visibility=g_INVISIBLE;
	}
	LEAVE;

	return (visibility);
} /* Scene_object_get_visibility */

int Scene_object_set_visibility(struct Scene_object *scene_object,
	enum GT_visibility_type visibility)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
Sets the visibility of <scene_object>.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_set_visibility);
	if (scene_object)
	{
		scene_object->visibility=visibility;
		Scene_changed_private(scene_object->scene);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_set_visibility.  Missing scene_object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_visibility */

int Scene_object_has_transformation(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 8 October 1998

DESCRIPTION :
Returns 1 if the <scene_object> has a nonidentity transformation matrix
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_has_transformation);
	if (scene_object)
	{
		if(scene_object->transformation)
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_transformation.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_transformation */

int Scene_object_get_transformation(struct Scene_object *scene_object,
	gtMatrix *transformation)
/*******************************************************************************
LAST MODIFIED : 8 October 1998

DESCRIPTION :
Returns the transformation of <scene_object>.
==============================================================================*/
{
	int i, j, return_code;

	ENTER(Scene_object_get_transformation);
	if (scene_object)
	{
		if(scene_object->transformation)
		{
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					(*transformation)[i][j] = (*scene_object->transformation)[i][j];
				}
			}
		}
		else
		{
			/* Set the identity */
			(*transformation)[0][0] = 1.0;
			(*transformation)[0][1] = 0.0;
			(*transformation)[0][2] = 0.0;
			(*transformation)[0][3] = 0.0;
			(*transformation)[1][0] = 0.0;
			(*transformation)[1][1] = 1.0;
			(*transformation)[1][2] = 0.0;
			(*transformation)[1][3] = 0.0;
			(*transformation)[2][0] = 0.0;
			(*transformation)[2][1] = 0.0;
			(*transformation)[2][2] = 1.0;
			(*transformation)[2][3] = 0.0;
			(*transformation)[3][0] = 0.0;
			(*transformation)[3][1] = 0.0;
			(*transformation)[3][2] = 0.0;
			(*transformation)[3][3] = 1.0;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_transformation.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_get_transformation */

int Scene_object_set_transformation(struct Scene_object *scene_object,
	gtMatrix *transformation)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
Sets the transformation of <scene_object>.
==============================================================================*/
{
	int i, j, return_code;

	ENTER(Scene_object_set_transformation);
	if (scene_object)
	{
		if(!scene_object->transformation)
		{
			if(!ALLOCATE(scene_object->transformation, gtMatrix, 1))
			{
				display_message(ERROR_MESSAGE,
					"Scene_object_set_transformation.  Unable to allocate transformation");
				return_code=0;				
			}
		}
		if(scene_object->transformation)
		{
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					(*scene_object->transformation)[i][j] = (*transformation)[i][j];
				}
			}
			Scene_changed_private(scene_object->scene);
			return_code = 1;
		}
		  
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_set_transformation.  Missing scene_object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_transformation */

int Scene_object_iterator_set_visibility(struct Scene_object *scene_object,
	void *visibility_void)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
List iterator function for setting the visibility of <scene_object>.
Note: second parameter should be passed as (void *)visibility, where
visibility is of type enum GT_visibility_type.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_iterator_set_visibility);
	if (scene_object)
	{
		scene_object->visibility=(enum GT_visibility_type)visibility_void;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_iterator_set_visibility.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_iterator_set_visibility */

int Scene_object_has_name(struct Scene_object *scene_object,
	void *name_void)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> has the 
specified <name>.
==============================================================================*/
{
	char *name;
	int return_code;

	ENTER(Scene_object_has_name);
	if (scene_object&&(name=(char *)name_void))
	{
		return_code=!strcmp(name,scene_object->name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_name.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_name */

struct GT_object *Scene_object_get_gt_object(
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
Returns the GT_object referenced by <scene_object>.
==============================================================================*/
{
	struct GT_object *return_gt_object;

	ENTER(Scene_object_get_gt_object);
	if (scene_object && (SCENE_OBJECT_GRAPHICS_OBJECT==scene_object->type))
	{
		return_gt_object=scene_object->gt_object;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_gt_object.  Invalid scene_object");
		return_gt_object=(struct GT_object *)NULL;
	}
	LEAVE;

	return (return_gt_object);
} /* Scene_object_get_gt_object */

int Scene_object_set_gt_object(struct Scene_object *scene_object,
	struct GT_object *gt_object)
/*******************************************************************************
LAST MODIFIED : 21 July 1998

DESCRIPTION :
Changes the GT_object referenced by <scene_object>. Use to point copied scene
objects to graphics object specific to a scene, eg. scene->axis_object.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_set_gt_object);
	if (scene_object&&gt_object&&(SCENE_OBJECT_GRAPHICS_OBJECT==scene_object->type))
	{
		ACCESS(GT_object)(gt_object);
		if (scene_object->gt_object)
		{
			DEACCESS(GT_object)(&(scene_object->gt_object));
		}
		scene_object->gt_object=gt_object;
		Scene_changed_private(scene_object->scene);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_gt_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_gt_object */

int Scene_object_has_gt_object(struct Scene_object *scene_object,
	void *gt_object_void)
/*******************************************************************************
LAST MODIFIED : 7 July 1999

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> is of type
SCENE_OBJECT_GRAPHICS_OBJECT and references <gt_object>.  If <gt_object_void>
is NULL then returns true if <scene_object> contains any graphics_object.
==============================================================================*/
{
	int return_code;
	struct GT_object *gt_object;

	ENTER(Scene_object_has_gt_object);
	if (scene_object)
	{
		if (gt_object=(struct GT_object *)gt_object_void)
		{
			return_code = (SCENE_OBJECT_GRAPHICS_OBJECT==scene_object->type)&&
				((scene_object->gt_object)==gt_object);
		}
		else
		{
			return_code = (SCENE_OBJECT_GRAPHICS_OBJECT==scene_object->type)&&
				(scene_object->gt_object);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_gt_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_gt_object */

struct Scene *Scene_object_get_child_scene(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 7 July 1999

DESCRIPTION :
==============================================================================*/
{
	struct Scene *return_scene;

	ENTER(Scene_object_get_child_scene);
	if (scene_object && (SCENE_OBJECT_SCENE==scene_object->type))
	{
		return_scene = scene_object->child_scene;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_child_scene.  Invalid scene_object");
		return_scene = (struct Scene *)NULL;
	}
	LEAVE;

	return (return_scene);
} /* Scene_object_get_child_scene */

int Scene_object_has_child_scene(struct Scene_object *scene_object,
	void *child_scene_void)
/*******************************************************************************
LAST MODIFIED : 20 November 1998

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> is of type
SCENE_OBJECT_SCENE and references <child_scene> (or any child_scene if NULL).
==============================================================================*/
{
	int return_code;
	struct Scene *child_scene;

	ENTER(Scene_object_has_child_scene);
	if (scene_object)
	{
		child_scene=(struct Scene *)child_scene_void;
		return_code=((SCENE_OBJECT_SCENE == scene_object->type)&&
			((!child_scene)||(scene_object->child_scene == child_scene)));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_child_scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_child_scene */

double Scene_object_get_time(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Returns the actual time referenced by <scene_object>.
==============================================================================*/
{
	double return_time;

	ENTER(Scene_object_get_time);
	if (scene_object)
	{
		if(scene_object->time_object)
		{
			return_time = Time_object_get_current_time(scene_object->time_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_object_get_time.  Missing time_object");
			return_time=0.0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_time.  Missing scene_object");
		return_time=0.0;
	}
	LEAVE;

	return (return_time);
} /* Scene_object_get_time */

int Scene_object_has_time(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Returns 1 if the Scene object has a time dependence, 0 if not
==============================================================================*/
{
	double return_code;

	ENTER(Scene_object_has_time);
	if (scene_object)
	{
		if(scene_object->time_object)
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_time.  Missing scene_object");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_time */

struct Time_object *Scene_object_get_time_object(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Returns the Time_object referenced by <scene_object>.
==============================================================================*/
{
	struct Time_object *return_time;

	ENTER(Scene_object_get_time_object);
	if (scene_object)
	{
		return_time=scene_object->time_object;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_time_object.  Missing scene_object");
		return_time=(struct Time_object *)NULL;
	}
	LEAVE;

	return (return_time);
} /* Scene_object_get_time_object */

int Scene_object_set_time_object(struct Scene_object *scene_object,
	struct Time_object *time)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
Changes the Time_object referenced by <scene_object>.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_object_set_time_object);
	if (scene_object&&time)
	{
		if(scene_object->type == SCENE_OBJECT_GRAPHICS_OBJECT)
		{
			ACCESS(Time_object)(time);
			if (scene_object->time_object)
			{
				DEACCESS(Time_object)(&(scene_object->time_object));
			}
			scene_object->time_object = ACCESS(Time_object)(time);
			Time_object_add_callback(time, Scene_object_time_update_callback,
				scene_object);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_object_time_object.  This object cannot be associated with a time object");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_time_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_set_time_object */

enum Scene_object_type Scene_object_get_type(struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the Scene_object_type of <scene_object>.
==============================================================================*/
{
	enum Scene_object_type scene_object_type;

	ENTER(Scene_object_get_type);
	if (scene_object)
	{
		scene_object_type=scene_object->type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_type.  Missing scene_object");
		scene_object_type=SCENE_OBJECT_TYPE_INVALID;
	}
	LEAVE;

	return (scene_object_type);
} /* Scene_object_get_type */

int Scene_object_has_data_group(struct Scene_object *scene_object,
	void *data_group_void)
/*******************************************************************************
LAST MODIFIED : 15 May 2000

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> contains a
g_ELEMENT_GROUP gt_object referencing the given data_group.
==============================================================================*/
{
	int return_code;
	struct GROUP(FE_node) *data_group;

	ENTER(Scene_object_has_data_group);
	if (scene_object&&(data_group=(struct GROUP(FE_node) *)data_group_void))
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			return_code=(data_group==
				GT_element_group_get_data_group(scene_object->gt_element_group));
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_data_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_data_group */

int Scene_object_has_element_group(struct Scene_object *scene_object,
	void *element_group_void)
/*******************************************************************************
LAST MODIFIED : 5 July 1999

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> contains a
g_ELEMENT_GROUP gt_object referencing the given element_group.
==============================================================================*/
{
	int return_code;
	struct GROUP(FE_element) *element_group;

	ENTER(Scene_object_has_element_group);
	if (scene_object&&(element_group=(struct GROUP(FE_element) *)
		element_group_void))
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			return_code=(GT_element_group_get_element_group(
				scene_object->gt_element_group)==element_group);
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_element_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_element_group */

int Scene_object_has_node_group(struct Scene_object *scene_object,
	void *node_group_void)
/*******************************************************************************
LAST MODIFIED : 15 May 2000

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> contains a
g_ELEMENT_GROUP gt_object referencing the given node_group.
==============================================================================*/
{
	int return_code;
	struct GROUP(FE_node) *node_group;

	ENTER(Scene_object_has_node_group);
	if (scene_object&&(node_group=(struct GROUP(FE_node) *)node_group_void))
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			return_code=(node_group==
				GT_element_group_get_node_group(scene_object->gt_element_group));
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_node_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_node_group */

int Scene_object_has_graphical_element_group(struct Scene_object *scene_object,
	void *gt_element_group_void)
/*******************************************************************************
LAST MODIFIED : 5 July 1999

DESCRIPTION :
Scene_object iterator function returning true if <scene_object> contains a
gt_ELEMENT_GROUP which matches <gt_element_group_void>. If
<gt_element_group_void> is NULL the function returns true if the scene_object
contains any gt_element_group.
==============================================================================*/
{
	int return_code;
	struct GT_element_group *gt_element_group;

	ENTER(Scene_object_has_element_group);
	if (scene_object)
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			if (gt_element_group=(struct GT_element_group *)gt_element_group_void)
			{
				return_code=(scene_object->gt_element_group==gt_element_group);
			}
			else
			{
				return_code = 1;
			}
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_graphical_element_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_graphical_element_group */

struct GT_element_group *Scene_object_get_graphical_element_group(
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 5 July 1999

DESCRIPTION :
Returns the GT_element_group referenced by <scene_object>.
==============================================================================*/
{
	struct GT_element_group *return_gt_element_group;

	ENTER(Scene_object_get_gt_object);
	if (scene_object && (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type))
	{
		return_gt_element_group=scene_object->gt_element_group;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_gt_element_group.  Missing scene_object");
		return_gt_element_group=(struct GT_element_group *)NULL;
	}
	LEAVE;

	return (return_gt_element_group);
} /* Scene_object_get_gt_element_group */

int Scene_object_has_unmanaged_element_group(
	struct Scene_object *scene_object,void *element_group_manager_void)
/*******************************************************************************
LAST MODIFIED : 16 June 1998

DESCRIPTION :
Returns true if the <scene_object> contains a GFE for an element group no
longer in <element_group_manager>.
???RC Should be static?
==============================================================================*/
{
	int return_code;
	struct MANAGER(GROUP(FE_element)) *element_group_manager;

	ENTER(Scene_object_has_unmanaged_element_group);
	if (scene_object&&scene_object->gt_object&&(element_group_manager=
		(struct MANAGER(GROUP(FE_element)) *)element_group_manager_void))
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			if (IS_MANAGED(GROUP(FE_element))(
				GT_element_group_get_element_group(scene_object->gt_element_group),
				element_group_manager))
			{
				return_code=0;
			}
			else
			{
				return_code=1;
			}
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_unmanaged_element_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_unmanaged_element_group */

int Scene_object_has_unmanaged_node_group(
	struct Scene_object *scene_object,void *node_group_manager_void)
/*******************************************************************************
LAST MODIFIED : 21 April 1999

DESCRIPTION :
Returns true if the <scene_object> contains a GFE for an element group no
longer in <node_group_manager>.
???RC Should be static?
==============================================================================*/
{
	int return_code;
	struct MANAGER(GROUP(FE_node)) *node_group_manager;

	ENTER(Scene_object_has_unmanaged_node_group);
	if (scene_object&&scene_object->gt_object&&(node_group_manager=
		(struct MANAGER(GROUP(FE_node)) *)node_group_manager_void))
	{
		if (SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP==scene_object->type)
		{
			if (IS_MANAGED(GROUP(FE_node))(GT_element_group_get_node_group(
				scene_object->gt_element_group),node_group_manager))
			{
				return_code=0;
			}
			else
			{
				return_code=1;
			}
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_has_unmanaged_node_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_has_unmanaged_node_group */

int Scene_object_get_range(struct Scene_object *scene_object,
	void *graphics_object_range_void)
/*******************************************************************************
LAST MODIFIED : 6 April 2000

DESCRIPTION :
Scene_object list iterator function. If <scene_object> is visible, expands
the <graphics_object_range> to include the range of the linked list of
graphics objects in scene_object.
==============================================================================*/
{
	float coordinates[4],transformed_coordinates[4];
	gtMatrix *transformation;
	int i,j,k,return_code;
	struct Graphics_object_range_struct *graphics_object_range,
		temp_graphics_object_range;
	struct GT_object *graphics_object;
	void *use_range_void;

	ENTER(Scene_object_get_range);
	if (scene_object&&(graphics_object_range=
		(struct Graphics_object_range_struct *)graphics_object_range_void))
	{
		if (transformation=scene_object->transformation)
		{
			temp_graphics_object_range.first=1;
			use_range_void=(void *)&temp_graphics_object_range;
		}
		else
		{
			use_range_void=graphics_object_range_void;
		}
		switch(scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			{
				return_code=1;
				if (g_VISIBLE==scene_object->visibility)
				{
					return_code=for_each_settings_in_GT_element_group(
						scene_object->gt_element_group,
						GT_element_settings_get_visible_graphics_object_range,
						use_range_void);
				}
			} break;
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				return_code=1;
				if (g_VISIBLE==scene_object->visibility)
				{
					for (graphics_object=scene_object->gt_object;
						return_code&&(graphics_object != NULL);
						graphics_object=graphics_object->nextobject)
					{
						return_code=
							get_graphics_object_range(graphics_object,use_range_void);
					}
				}
			} break;
			case SCENE_OBJECT_SCENE:
			{
				for_each_Scene_object_in_Scene(scene_object->child_scene,
					Scene_object_get_range,use_range_void);
			} break;
		}
		if (transformation&&(!temp_graphics_object_range.first))
		{
			coordinates[3]=1.0;
			/* transform and compare ranges of each of 8 corners of the cube */
			for (i=0;i<8;i++)
			{
				if (i & 1)
				{
					coordinates[0]=temp_graphics_object_range.maximum[0];
				}
				else
				{
					coordinates[0]=temp_graphics_object_range.minimum[0];
				}
				if (i & 2)
				{
					coordinates[1]=temp_graphics_object_range.maximum[1];
				}
				else
				{
					coordinates[1]=temp_graphics_object_range.minimum[1];
				}
				if (i & 4)
				{
					coordinates[2]=temp_graphics_object_range.maximum[2];
				}
				else
				{
					coordinates[2]=temp_graphics_object_range.minimum[2];
				}
				for (j=0;j<4;j++)
				{
					transformed_coordinates[j]=0.0;
					for (k=0;k<4;k++)
					{
						transformed_coordinates[j] +=
							(*transformation)[k][j]*coordinates[k];
					}
				}
				if (0.0<transformed_coordinates[3])
				{
					transformed_coordinates[0] /= transformed_coordinates[3];
					transformed_coordinates[1] /= transformed_coordinates[3];
					transformed_coordinates[2] /= transformed_coordinates[3];
					for (j=0;j<3;j++)
					{
						if (graphics_object_range->first)
						{
							graphics_object_range->minimum[j]=
								graphics_object_range->maximum[j]=transformed_coordinates[j];
						}
						else
						{
							if (transformed_coordinates[j] >
								graphics_object_range->maximum[j])
							{
								graphics_object_range->maximum[j]=transformed_coordinates[j];
							}
							else if (transformed_coordinates[j] <
								graphics_object_range->minimum[j])
							{
								graphics_object_range->minimum[j]=transformed_coordinates[j];
							}
						}
					}
					graphics_object_range->first=0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_get_range */

int Scene_object_get_time_range(struct Scene_object *scene_object,
	void *graphics_object_time_range_void)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
Scene_object list iterator function. Enlarges the minimum and maximum time
ranges by those of the graphics_objects contained in the <scene_object>.
==============================================================================*/
{
	int return_code;
	struct GT_object *graphics_object;

	ENTER(Scene_object_get_time_range);
	if (scene_object&&scene_object->gt_object)
	{
		return_code=1;
		for (graphics_object=scene_object->gt_object;(graphics_object != NULL)&&
			return_code;graphics_object=graphics_object->nextobject)
		{
			return_code=get_graphics_object_time_range(graphics_object,
				graphics_object_time_range_void);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_object_get_time_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_object_get_time_range */

int list_Scene_object(struct Scene_object *scene_object,void *dummy)
/*******************************************************************************
LAST MODIFIED : 26 April 1999

DESCRIPTION :
Iterator function called by list_Scene. Writes the scene object position,
name, visibility and information about the object in it to the command window.
???RC list transformation? Have separate gfx list transformation commands.
==============================================================================*/
{
	char *gt_element_group_name, *time_object_name;
	int return_code;

	ENTER(list_Scene_object);
	USE_PARAMETER(dummy);
	if (scene_object)
	{
		return_code=1;
		/* four spaces before position to align with list_Scene */
		display_message(INFORMATION_MESSAGE,"    %d. %s",scene_object->position,
			scene_object->name);
		if (g_VISIBLE != scene_object->visibility)
		{
			display_message(INFORMATION_MESSAGE," [INVISIBLE]");
		}
		switch (scene_object->type)
		{
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			{
				if (scene_object->gt_object&&scene_object->gt_object->name)
				{
					display_message(INFORMATION_MESSAGE," = graphics object %s",
						scene_object->gt_object->name);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"list_Scene_object.  Invalid gt_object");
					return_code=0;
				}
			} break;
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
			{
				if(GET_NAME(GT_element_group)(scene_object->gt_element_group,
					&gt_element_group_name))
				{
					display_message(INFORMATION_MESSAGE,
						" = graphical finite element group %s",
						gt_element_group_name);
					DEALLOCATE(gt_element_group_name);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"list_Scene_object.  Unable to get graphical element group name");
					return_code=0;
				}
			} break;
			case SCENE_OBJECT_SCENE:
			{
				if (scene_object->child_scene&&scene_object->child_scene->name)
				{
					display_message(INFORMATION_MESSAGE," = child scene %s",
						scene_object->child_scene->name);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"list_Scene_object.  Invalid child scene");
					return_code=0;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"list_Scene_object.  Invalid Scene_object_type");
				return_code=0;
			} break;
		}
		if (scene_object->time_object)
		{
			if (GET_NAME(Time_object)(scene_object->time_object,&time_object_name))
			{
				display_message(INFORMATION_MESSAGE,"; time object %s\n",
					time_object_name);
				DEALLOCATE(time_object_name);
			}
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"; no time object\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Scene_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Scene_object */

int list_Scene_object_transformation(struct Scene_object *scene_object,
	void *dummy)
/*******************************************************************************
LAST MODIFIED : 26 April 1999

DESCRIPTION :
Iterator function for writing the transformation in effect for <scene_object>
in an easy-to-interpret matrix multiplication form.
==============================================================================*/
{
	char *coordinate_symbol="xyzh";
	int i,return_code;
	gtMatrix transformation_matrix;

	ENTER(list_Scene_object_transformation);
	USE_PARAMETER(dummy);
	if (scene_object)
	{
		if (return_code=Scene_object_get_transformation(scene_object,
			&transformation_matrix))
		{
			display_message(INFORMATION_MESSAGE,"%d. %s transformation:\n",
				scene_object->position,scene_object->name);
			for (i=0;i<4;i++)
			{
				display_message(INFORMATION_MESSAGE,
					"  |%c.out| = | %13.6e %13.6e %13.6e %13.6e | . |%c.in|\n",
					coordinate_symbol[i],
					transformation_matrix[0][i],transformation_matrix[1][i],
					transformation_matrix[2][i],transformation_matrix[3][i],
					coordinate_symbol[i]);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Scene_object_transformation.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Scene_object_transformation */

int list_Scene_object_transformation_commands(struct Scene_object *scene_object,
	void *command_prefix_void)
/*******************************************************************************
LAST MODIFIED : 26 April 1999

DESCRIPTION :
Iterator function for writing the transformation in effect for <scene_object>
as a command, using the given <command_prefix>.
==============================================================================*/
{
	char *command_prefix;
	int i,j,return_code;
	gtMatrix transformation_matrix;
 
	ENTER(list_Scene_object_transformation_commands);
	if (scene_object&&(command_prefix=(char *)command_prefix_void))
	{
		if (return_code=Scene_object_get_transformation(scene_object,
			&transformation_matrix))
		{
			display_message(INFORMATION_MESSAGE,"%s %s",command_prefix,
				scene_object->name);
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					display_message(INFORMATION_MESSAGE," %g",
						(transformation_matrix)[i][j]);
				}
			}
			display_message(INFORMATION_MESSAGE,"\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Scene_object_transformation_commands.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Scene_object_transformation_commands */

struct Scene *CREATE(Scene)(char *name)
/*******************************************************************************
LAST MODIFIED : 28 April 2000

DESCRIPTION :
Scene now has pointer to its scene_manager, and it uses manager modify
messages to inform its clients of changes. The pointer to the scene_manager
is set and unset by the add/remove object from manager routines, overwritten
from the default versions of these functions.
==============================================================================*/
{
	struct Scene *scene;

	ENTER(CREATE(Scene));
	if (name)
	{
		/* allocate memory for the scene structure */
		if (ALLOCATE(scene,struct Scene,1)&&
			ALLOCATE(scene->name,char,strlen(name)+1)&&
			(scene->list_of_lights=CREATE(LIST(Light))())&&
			(scene->scene_object_list=CREATE(LIST(Scene_object))()))
		{
			/* assign values to the fields */
			strcpy(scene->name,name);
			scene->access_count=0;
			scene->scene_manager=(struct MANAGER(Scene) *)NULL;
			/* fields, elements, nodes and data */
			scene->computed_field_manager=(struct MANAGER(Computed_field) *)NULL;
			scene->fe_field_manager=(struct MANAGER(FE_field) *)NULL;
			scene->element_manager=(struct MANAGER(FE_element) *)NULL;
			scene->element_group_manager=(struct MANAGER(GROUP(FE_element)) *)NULL;
			scene->element_group_manager_callback_id=(void *)NULL;
			scene->node_manager=(struct MANAGER(FE_node) *)NULL;
			scene->node_group_manager=(struct MANAGER(GROUP(FE_node)) *)NULL;
			scene->data_manager=(struct MANAGER(FE_node) *)NULL;
			scene->data_group_manager=(struct MANAGER(GROUP(FE_node)) *)NULL;
			/* defaults to not adding GFEs - besides, need managers anyway */
			scene->graphical_element_mode=GRAPHICAL_ELEMENT_NONE;
			/* global stores of selected objects */
			scene->element_point_ranges_selection=
				(struct Element_point_ranges_selection *)NULL;
			scene->element_selection=(struct FE_element_selection *)NULL;
			scene->node_selection=(struct FE_node_selection *)NULL;
			scene->data_selection=(struct FE_node_selection *)NULL;
			/* axes created once graphics enabled */
			scene->axis_object=(struct GT_object *)NULL;
			/* attributes: */
			scene->glyph_list=(struct LIST(GT_object) *)NULL;
			scene->graphical_material_manager=
				(struct MANAGER(Graphical_material) *)NULL;
			scene->graphical_material_manager_callback_id=(void *)NULL;
			scene->default_material=(struct Graphical_material *)NULL;
			scene->light_manager=(struct MANAGER(Light) *)NULL;
			scene->light_manager_callback_id=(void *)NULL;
			scene->spectrum_manager=(struct MANAGER(Spectrum) *)NULL;
			scene->spectrum_manager_callback_id=(void *)NULL;
			scene->default_spectrum=(struct Spectrum *)NULL;
			scene->streamline_manager=(struct MANAGER(Interactive_streamline) *)NULL;
			scene->streamline_manager_callback_id=(void *)NULL;
			scene->texture_manager=(struct MANAGER(Texture) *)NULL;
			scene->texture_manager_callback_id=(void *)NULL;
			scene->default_time_keeper=(struct Time_keeper *)NULL;
			scene->user_interface=(struct User_interface *)NULL;
			/* display list index and current flag: */
			scene->display_list=0;
			scene->display_list_current=0;
			/* input callback handling information: */
			scene->input_callback.procedure=(Scene_input_callback_procedure *)NULL;
			scene->input_callback.data=(void *)NULL;
		}
		else
		{
			if (scene)
			{
				if (scene->name)
				{
					DEALLOCATE(scene->name);
					if (scene->list_of_lights)
					{
						DESTROY(LIST(Light))(&(scene->list_of_lights));
					}
				}
				DEALLOCATE(scene);
			}
			display_message(ERROR_MESSAGE,"CREATE(Scene).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Scene).  Invalid argument(s)");
		scene=(struct Scene *)NULL;
	}
	LEAVE;

	return (scene);
} /* CREATE(Scene) */

int DESTROY(Scene)(struct Scene **scene_address)
/*******************************************************************************
LAST MODIFIED : 28 April 2000

DESCRIPTION :
Closes the scene and disposes of the scene data structure.
==============================================================================*/
{
	int return_code;
	struct Scene *scene;

	ENTER(DESTROY(Scene));
	if (scene_address&&(scene= *scene_address))
	{
		if (0==scene->access_count)
		{
			Scene_disable_time_behaviour(scene);
			Scene_disable_interactive_streamlines(scene);
			Scene_set_graphical_element_mode(scene,
				GRAPHICAL_ELEMENT_NONE,
				(struct MANAGER(Computed_field) *)NULL,
				(struct MANAGER(FE_element) *)NULL,
				(struct MANAGER(GROUP(FE_element)) *)NULL,
				(struct MANAGER(FE_field) *)NULL,
				(struct MANAGER(FE_node) *)NULL,
				(struct MANAGER(GROUP(FE_node)) *)NULL,
				(struct MANAGER(FE_node) *)NULL,
				(struct MANAGER(GROUP(FE_node)) *)NULL,
				(struct Element_point_ranges_selection *)NULL,
				(struct FE_element_selection *)NULL,
				(struct FE_node_selection *)NULL,
				(struct FE_node_selection *)NULL,
				(struct User_interface *)NULL);
			Scene_disable_graphics(scene);
			DEALLOCATE(scene->name);
			/* must destroy the display list */
			if (scene->display_list)
			{
				glDeleteLists(scene->display_list,1);
			}
			DESTROY(LIST(Light))(&(scene->list_of_lights));
			DESTROY(LIST(Scene_object))(&(scene->scene_object_list));
			DEALLOCATE(*scene_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"DESTROY(Scene).  Non-zero access count!");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Scene).  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene) */

int Scene_enable_graphics(struct Scene *scene,
	struct LIST(GT_object) *glyph_list,
	struct MANAGER(Graphical_material) *graphical_material_manager,
	struct Graphical_material *default_material,
	struct MANAGER(Light) *light_manager,
	struct MANAGER(Spectrum) *spectrum_manager,
	struct Spectrum *default_spectrum,
	struct MANAGER(Texture) *texture_manager)
/*******************************************************************************
LAST MODIFIED : 12 February 1999

DESCRIPTION :
The scene is initially incapable of generating any graphics, since it does not
have access to the material, light and spectrum managers. This routine must be
called soon after creating the scene to give the scene these managers.
Do not have to call this routine if MANAGER_COPY_WITHOUT_IDENTIFIER is used to
create a scene from an existing scene with graphics enabled.
NOTE: The light_manager is not currently used by the scene.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_enable_graphics);
	if (scene&&glyph_list&&graphical_material_manager&&default_material&&
		light_manager&&spectrum_manager&&default_spectrum&&texture_manager)
	{
		if (scene->graphical_material_manager)
		{
			display_message(WARNING_MESSAGE,
				"Scene_enable_graphics.  Graphics already enabled");
		}
		else
		{
			scene->glyph_list=glyph_list;
			scene->graphical_material_manager=graphical_material_manager;
			scene->default_material=ACCESS(Graphical_material)(default_material);
			scene->light_manager=light_manager;
			scene->spectrum_manager=spectrum_manager;
			scene->default_spectrum=ACCESS(Spectrum)(default_spectrum);
			scene->texture_manager=texture_manager;
			/* register for any graphical_material changes */
			scene->graphical_material_manager_callback_id=
				MANAGER_REGISTER(Graphical_material)(Scene_graphical_material_change,
				(void *)scene,scene->graphical_material_manager);
			/* register for any spectrum changes */
			scene->spectrum_manager_callback_id=
				MANAGER_REGISTER(Spectrum)(Scene_spectrum_change,
				(void *)scene,scene->spectrum_manager);
			/* register for any texture changes */
			scene->texture_manager_callback_id=
				MANAGER_REGISTER(Texture)(Scene_texture_change,
				(void *)scene,scene->texture_manager);
			/* get axis glyph and material */
			scene->axis_object=make_axis_graphics_object(default_material,glyph_list);
			ACCESS(GT_object)(scene->axis_object);
			Scene_add_graphics_object(scene,scene->axis_object,0,
				scene->axis_object->name);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_enable_graphics.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_enable_graphics */

int Scene_disable_graphics(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 20 July 1998

DESCRIPTION :
Removes links to all objects required to display graphics.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_disable_graphics);
	if (scene)
	{
		/* turn off manager messages */
		if (scene->graphical_material_manager_callback_id)
		{
			MANAGER_DEREGISTER(Graphical_material)(
				scene->graphical_material_manager_callback_id,
				scene->graphical_material_manager);
		}
		if (scene->spectrum_manager_callback_id)
		{
			MANAGER_DEREGISTER(Spectrum)(
				scene->spectrum_manager_callback_id,scene->spectrum_manager);
		}
		if (scene->texture_manager_callback_id)
		{
			MANAGER_DEREGISTER(Texture)(
				scene->texture_manager_callback_id,scene->texture_manager);
		}
		if (scene->default_material)
		{
			DEACCESS(Graphical_material)(&(scene->default_material));
		}
		if (scene->default_spectrum)
		{
			DEACCESS(Spectrum)(&(scene->default_spectrum));
		}
		if (scene->axis_object)
		{
			Scene_remove_graphics_object(scene,scene->axis_object);
			DEACCESS(GT_object)(&(scene->axis_object));
		}
		scene->graphical_material_manager=
			(struct MANAGER(Graphical_material) *)NULL;
		scene->graphical_material_manager_callback_id=(void *)NULL;
		scene->default_material=(struct Graphical_material *)NULL;
		scene->light_manager=(struct MANAGER(Light) *)NULL;
		scene->light_manager_callback_id=(void *)NULL;
		scene->spectrum_manager=(struct MANAGER(Spectrum) *)NULL;
		scene->spectrum_manager_callback_id=(void *)NULL;
		scene->default_spectrum=(struct Spectrum *)NULL;
		scene->axis_object=(struct GT_object *)NULL;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_disable_graphics.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_disable_graphics */

int Scene_enable_time_behaviour(struct Scene *scene,
	struct Time_keeper *default_time_keeper)
/*******************************************************************************
LAST MODIFIED : 12 February 1999

DESCRIPTION :
The scene is initially incapable of varying objects with time as it has no
time_keeper to put time_objects into.
Do not have to call this routine if MANAGER_COPY_WITHOUT_IDENTIFIER is used to
create a scene from an existing scene with graphics enabled.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_enable_graphics);
	if (scene&&default_time_keeper)
	{
		if (scene->default_time_keeper)
		{
			display_message(WARNING_MESSAGE,
				"Scene_enable_time_behaviour.  Time already enabled");
		}
		else
		{
			scene->default_time_keeper = ACCESS(Time_keeper)(default_time_keeper);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_enable_time_behaviour.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_enable_time_behaviour */

struct Time_keeper *Scene_get_default_time_keeper(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 18 February 1999

DESCRIPTION :
==============================================================================*/
{
	struct Time_keeper *return_time_keeper;

	ENTER(Scene_get_default_time_keeper);
	if (scene)
	{
		return_time_keeper = scene->default_time_keeper;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_default_time_keeper.  Invalid argument(s)");
		return_time_keeper = (struct Time_keeper *)NULL;
	}
	LEAVE;

	return (return_time_keeper);
} /* Scene_get_default_time_keeper */

int Scene_disable_time_behaviour(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 12 February 1999

DESCRIPTION :
Removes links to all objects required to vary graphics objects with time.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_disable_time_behaviour);
	if (scene)
	{
		if (scene->default_time_keeper)
		{
			DEACCESS(Time_keeper)(&(scene->default_time_keeper));
		}
		scene->default_time_keeper=(struct Time_keeper *)NULL;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_disable_time_behaviour.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_disable_time_behaviour */

int Scene_enable_interactive_streamlines(struct Scene *scene,
	struct MANAGER(Interactive_streamline) *streamline_manager)
/*******************************************************************************
LAST MODIFIED : 8 February 1998

DESCRIPTION :
Allows scenes to automatically draw interactive streamlines when they are
modified.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_enable_interactive_streamlines);
	if (scene&&streamline_manager)
	{
		if (scene->streamline_manager)
		{
			display_message(WARNING_MESSAGE,
				"Scene_enable_interactive_streamlines.  Streamlines already enabled");
		}
		else
		{
			scene->streamline_manager=streamline_manager;
			/* register for interactive streamline changes */
			scene->streamline_manager_callback_id=
				MANAGER_REGISTER(Interactive_streamline)(Scene_streamline_change,
				(void *)scene,scene->streamline_manager);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_enable_interactive_streamlines.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_enable_interactive_streamlines */

int Scene_disable_interactive_streamlines(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 8 February 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Scene_disable_interactive_streamlines);
	if (scene)
	{
		/* turn off manager messages */
		if (scene->streamline_manager_callback_id)
		{
			MANAGER_DEREGISTER(Interactive_streamline)(
				scene->streamline_manager_callback_id,
				scene->streamline_manager);
		}
		scene->streamline_manager=(struct MANAGER(Interactive_streamline) *)NULL;
		scene->streamline_manager_callback_id=(void *)NULL;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_disable_interactive_streamlines.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_disable_interactive_streamlines */

enum Scene_graphical_element_mode Scene_get_graphical_element_mode(
	struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 4 February 2000

DESCRIPTION :
Returns the mode controlling how graphical element groups are displayed in the
scene.
==============================================================================*/
{
	enum Scene_graphical_element_mode graphical_element_mode;

	ENTER(Scene_get_graphical_element_mode);
	if (scene)
	{
		graphical_element_mode = scene->graphical_element_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_graphical_element_mode.  Invalid argument");
		graphical_element_mode = GRAPHICAL_ELEMENT_MODE_INVALID;
	}
	LEAVE;

	return (graphical_element_mode);
} /* enum Scene_graphical_element_mode graphical_element_mode */

int Scene_set_graphical_element_mode(struct Scene *scene,
	enum Scene_graphical_element_mode graphical_element_mode,
	struct MANAGER(Computed_field) *computed_field_manager,
	struct MANAGER(FE_element) *element_manager,
	struct MANAGER(GROUP(FE_element)) *element_group_manager,
	struct MANAGER(FE_field) *fe_field_manager,
	struct MANAGER(FE_node) *node_manager,
	struct MANAGER(GROUP(FE_node)) *node_group_manager,
	struct MANAGER(FE_node) *data_manager,
	struct MANAGER(GROUP(FE_node)) *data_group_manager,
	struct Element_point_ranges_selection *element_point_ranges_selection,
	struct FE_element_selection *element_selection,
	struct FE_node_selection *node_selection,
	struct FE_node_selection *data_selection,
	struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 28 April 2000

DESCRIPTION :
Sets the mode controlling how graphical element groups are displayed in the
scene. Passes the managers and other data required to create and update the
graphical elements.
Must be called after Scene_enable_graphics since GFEs require the default
material and spectrum.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_graphical_element_mode);
	if (scene&&((GRAPHICAL_ELEMENT_NONE == graphical_element_mode)||(
		computed_field_manager&&element_manager&&element_group_manager&&
		fe_field_manager&&node_manager&&node_group_manager&&data_manager&&
		data_group_manager&&element_point_ranges_selection&&element_selection&&
		node_selection&&data_selection)))
	{
		return_code=1;
		if (GRAPHICAL_ELEMENT_NONE == graphical_element_mode)
		{
			scene->graphical_element_mode=graphical_element_mode;
			scene->computed_field_manager=(struct MANAGER(Computed_field) *)NULL;
			scene->element_manager=(struct MANAGER(FE_element) *)NULL;
			/* turn off element_group_manager callbacks */
			if (scene->element_group_manager_callback_id)
			{
				MANAGER_DEREGISTER(GROUP(FE_element))(
					scene->element_group_manager_callback_id,
					scene->element_group_manager);
				scene->element_group_manager_callback_id=(void *)NULL;
			}
			scene->element_group_manager=(struct MANAGER(GROUP(FE_element)) *)NULL;
			/* remove all graphical elements from scene */
			while (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
				Scene_object_has_graphical_element_group,(void *)NULL,
				scene->scene_object_list))
			{
				Scene_remove_scene_object(scene,scene_object);
			}
			scene->node_manager=(struct MANAGER(FE_node) *)NULL;
			scene->node_group_manager=(struct MANAGER(GROUP(FE_node)) *)NULL;
			scene->data_manager=(struct MANAGER(FE_node) *)NULL;
			scene->data_group_manager=(struct MANAGER(GROUP(FE_node)) *)NULL;
			scene->element_point_ranges_selection=
				(struct Element_point_ranges_selection *)NULL;
			scene->element_selection=(struct FE_element_selection *)NULL;
			scene->node_selection=(struct FE_node_selection *)NULL;
			scene->data_selection=(struct FE_node_selection *)NULL;
			scene->user_interface=(struct User_interface *)NULL;
		}
		else
		{
			/* check managers consistent current mode - unless this is
				 GRAPHICAL_ELEMENT_NONE so setting for the first time */
			if ((GRAPHICAL_ELEMENT_NONE == scene->graphical_element_mode)||(
				(computed_field_manager == scene->computed_field_manager)&&
				(element_manager == scene->element_manager)&&
				(element_group_manager == scene->element_group_manager)&&
				(fe_field_manager == scene->fe_field_manager)&&
				(node_manager == scene->node_manager)&&
				(node_group_manager == scene->node_group_manager)&&
				(data_manager == scene->data_manager)&&
				(data_group_manager == scene->data_group_manager)))
			{
				if (scene->graphical_material_manager)
				{
					if (GRAPHICAL_ELEMENT_NONE == scene->graphical_element_mode)
					{
						scene->graphical_element_mode=graphical_element_mode;
						scene->computed_field_manager=computed_field_manager;
						scene->element_manager=element_manager;
						scene->element_group_manager=element_group_manager;
						scene->fe_field_manager=fe_field_manager;
						scene->node_manager=node_manager;
						scene->node_group_manager=node_group_manager;
						scene->data_manager=data_manager;
						scene->data_group_manager=data_group_manager;
						scene->element_point_ranges_selection=
							element_point_ranges_selection;
						scene->element_selection=element_selection;
						scene->node_selection=node_selection;
						scene->data_selection=data_selection;
						scene->user_interface=user_interface;
						if (GRAPHICAL_ELEMENT_MANUAL != graphical_element_mode)
						{
							/* add all current element_groups to new scene */
							FOR_EACH_OBJECT_IN_MANAGER(GROUP(FE_element))(
								element_group_to_scene,(void *)scene,
								scene->element_group_manager);
						}
						/* register for any element_group_manager changes */
						scene->element_group_manager_callback_id=
							MANAGER_REGISTER(GROUP(FE_element))(Scene_element_group_change,
								(void *)scene,scene->element_group_manager);
					}
					else
					{
						scene->graphical_element_mode=graphical_element_mode;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_set_graphical_element_mode.  Graphics not yet enabled");
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_set_graphical_element_mode.  Inconsistent managers");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_graphical_element_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_graphical_element_mode */

DECLARE_OBJECT_FUNCTIONS(Scene)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Scene)
DECLARE_LIST_FUNCTIONS(Scene)
DECLARE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Scene,name,char *,strcmp)

PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(Scene,name)
{
	char *name;
	int return_code;

	ENTER(MANAGER_COPY_WITH_IDENTIFIER(Scene,name));
	/* check arguments */
	if (source&&destination)
	{
		if (source->name)
		{
			if (ALLOCATE(name,char,strlen(source->name)+1))
			{
				strcpy(name,source->name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(Scene,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			if (return_code = MANAGER_COPY_WITHOUT_IDENTIFIER(Scene,name)(
				destination, source))
			{
				/* copy values */
				DEALLOCATE(destination->name);
				destination->name=name;
			}
			else
			{
				DEALLOCATE(name);
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(Scene,name).  Could not copy without identifier");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_WITH_IDENTIFIER(Scene,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITH_IDENTIFIER(Scene,name) */

PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(Scene,name)
{
	int return_code;
	struct LIST(Light) *temp_list_of_lights;
	struct LIST(Scene_object) *temp_scene_object_list;
	struct Scene_object *scene_object;
	Triple axis_lengths;

	ENTER(MANAGER_COPY_WITHOUT_IDENTIFIER(Scene,name));
	/* check arguments */
	if (source&&destination)
	{
#if defined (OLD_CODE)
		Scene_disable_graphical_finite_elements(destination);
#endif /* defined (OLD_CODE) */
		Scene_disable_interactive_streamlines(destination);
		Scene_disable_graphics(destination);
		if (source->graphical_material_manager)
		{
			Scene_enable_graphics(destination,source->glyph_list,
				source->graphical_material_manager,source->default_material,
				source->light_manager,source->spectrum_manager,source->default_spectrum,
				source->texture_manager);
			/* make sure the destination axes have same size and material as source */
			Scene_get_axis_lengths(source,axis_lengths);
			Scene_set_axis_lengths(destination,axis_lengths);
			Scene_set_axis_material(destination,Scene_get_axis_material(source));
		}
		if (source->default_time_keeper)
		{
			Scene_enable_time_behaviour(destination,
				source->default_time_keeper);
		}
		if (source->streamline_manager)
		{
			Scene_enable_interactive_streamlines(destination,
				source->streamline_manager);
		}
		Scene_set_graphical_element_mode(destination,source->graphical_element_mode,
			source->computed_field_manager,source->element_manager,
			source->element_group_manager,source->fe_field_manager,
			source->node_manager,source->node_group_manager,
			source->data_manager,source->data_group_manager,
			source->element_point_ranges_selection,
			source->element_selection,source->node_selection,source->data_selection,
			source->user_interface);
		/* copy list of lights to destination */
		/* duplicate each scene_object in source and put in destination list */
		if ((temp_list_of_lights=CREATE(LIST(Light))())&&
			(temp_scene_object_list=CREATE(LIST(Scene_object))())&&
			(FOR_EACH_OBJECT_IN_LIST(Light)(Light_to_list,
				(void *)temp_list_of_lights,source->list_of_lights))&&
			(FOR_EACH_OBJECT_IN_LIST(Scene_object)(Scene_object_copy_to_list,
				(void *)temp_scene_object_list,source->scene_object_list)))
		{
			DESTROY(LIST(Light))(&(destination->list_of_lights));
			destination->list_of_lights=temp_list_of_lights;
			DESTROY(LIST(Scene_object))(&(destination->scene_object_list));
			destination->scene_object_list=temp_scene_object_list;
			/* must make sure the axes scene_object in scene_object_list points to
				 the axis_object for the destination scene - not the source one */
			if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
				Scene_object_has_gt_object,(void *)source->axis_object,
				destination->scene_object_list))
			{
				if (destination->axis_object)
				{
					Scene_object_set_gt_object(scene_object,destination->axis_object);
				}
				else
				{
					Scene_remove_graphics_object(destination,source->axis_object);
				}
			}
			/* NOTE: MUST NOT COPY SCENE_MANAGER! */
			destination->display_list_current=0;
			return_code=1;
		}
		else
		{
			if (temp_list_of_lights)
			{
				DESTROY(LIST(Light))(&temp_list_of_lights);
				if (temp_scene_object_list)
				{
					DESTROY(LIST(Scene_object))(&temp_scene_object_list);
				}
			}
			display_message(ERROR_MESSAGE,
				"MANAGER_COPY_WITHOUT_IDENTIFIER(Scene,name).  Could not copy lists");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_WITHOUT_IDENTIFIER(Scene,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITHOUT_IDENTIFIER(Scene,name) */

PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(Scene,name,char *)
{
	char *destination_name;
	int return_code;

	ENTER(MANAGER_COPY_IDENTIFIER(Scene,name));
	/* check arguments */
	if (name&&destination)
	{
		if (name)
		{
			if (ALLOCATE(destination_name,char,strlen(name)+1))
			{
				strcpy(destination_name,name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(Scene,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			/* copy name */
			DEALLOCATE(destination->name);
			destination->name=destination_name;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(Scene,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_IDENTIFIER(Scene,name) */

/* NOTE: Using special ADD_OBJECT_TO_MANAGER function so that object keeps */
/*       pointer to its manager while it is managed. */

DECLARE_MANAGER_FUNCTIONS(Scene)
DECLARE_OBJECT_WITH_MANAGER_MANAGER_IDENTIFIER_FUNCTIONS( \
	Scene,name,char *,scene_manager)

int Scene_get_number_of_scene_objects(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Gets the number of scene_objects in a scene.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_get_number_of_scene_objects);
	if (scene)
	{
		return_code=NUMBER_IN_LIST(Scene_object)(scene->scene_object_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_number_of_scene_objects.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_number_of_scene_objects */

int for_each_Scene_object_in_Scene(struct Scene *scene,
	LIST_ITERATOR_FUNCTION(Scene_object) *iterator_function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 23 December 1997

DESCRIPTION :
Allows clients of the <scene> to perform functions with the scene_objects in
it. For example, rendervrml.c needs to output all the window objects in a scene.
==============================================================================*/
{
	int return_code;

	ENTER(for_each_Scene_object_in_Scene);
	if (scene&&iterator_function)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(Scene_object)(iterator_function,
			user_data,scene->scene_object_list);	
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_Scene_object_in_Scene.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* for_each_Scene_object_in_Scene */

struct Scene_object *first_Scene_object_in_Scene_that(struct Scene *scene,
	LIST_CONDITIONAL_FUNCTION(Scene_object) *conditional_function,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 15 May 2000

DESCRIPTION :
Wrapper for FIRST_OBJECT_IN_LIST_THAT function for Scene_object.
==============================================================================*/
{
	struct Scene_object *return_object;

	ENTER(first_Scene_object_in_Scene_that);
	if (scene)
	{
		return_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(conditional_function,
			user_data, scene->scene_object_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"first_Scene_object_in_Scene_that.  Missing scene");
		return_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (return_object);
} /* first_Scene_object_in_Scene_that */

struct Scene_picked_object *CREATE(Scene_picked_object)(int hit_no)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Creates a Scene_picked_object for storing picking information in a format
compatible with objects in our display hierarchy. Creates a blank object that
must be filled with appropriate data.
==============================================================================*/
{
	struct Scene_picked_object *scene_picked_object;

	ENTER(CREATE(Scene_picked_object));
	if (ALLOCATE(scene_picked_object,struct Scene_picked_object,1))
	{
		scene_picked_object->hit_no=hit_no;
		scene_picked_object->number_of_scene_objects=0;
		scene_picked_object->scene_objects=(struct Scene_object **)NULL;
		scene_picked_object->number_of_subobjects=0;
		scene_picked_object->subobjects=(int *)NULL;
		scene_picked_object->nearest=0;
		scene_picked_object->farthest=0;
		scene_picked_object->access_count=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Scene_picked_object).  Not enough memory");
	}
	LEAVE;

	return (scene_picked_object);
} /* CREATE(Scene_picked_object) */

int DESTROY(Scene_picked_object)(
	struct Scene_picked_object **scene_picked_object_address)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Destroys the Scene_picked_object.
==============================================================================*/
{
	int i,return_code;
	struct Scene_picked_object *scene_picked_object;

	ENTER(DESTROY(Scene_picked_object));
	if (scene_picked_object_address&&
		(scene_picked_object= *scene_picked_object_address))
	{
		if (0==scene_picked_object->access_count)
		{
			for (i=0;i<scene_picked_object->number_of_scene_objects;i++)
			{
				DEACCESS(Scene_object)(&(scene_picked_object->scene_objects[i]));
			}
			DEALLOCATE(scene_picked_object->scene_objects);
			DEALLOCATE(scene_picked_object->subobjects);
			DEALLOCATE(*scene_picked_object_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Scene_picked_object).  Non-zero access count!");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Scene_picked_object).  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene_picked_object) */

int Scene_picked_object_add_Scene_object(
	struct Scene_picked_object *scene_picked_object,
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Adds the <scene_object> to the end of the list specifying the path to the
picked graphic represented by the <scene_picked_object>.
==============================================================================*/
{
	int return_code;
	struct Scene_object **temp_scene_objects;

	ENTER(Scene_picked_object_add_Scene_object);
	if (scene_picked_object&&scene_object)
	{
		if (REALLOCATE(temp_scene_objects,scene_picked_object->scene_objects,
			struct Scene_object *,scene_picked_object->number_of_scene_objects+1))
		{
			scene_picked_object->scene_objects = temp_scene_objects;
			scene_picked_object->
				scene_objects[scene_picked_object->number_of_scene_objects]=
				ACCESS(Scene_object)(scene_object);
			scene_picked_object->number_of_scene_objects++;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_picked_object_add_Scene_object.  Not enough memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_add_Scene_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_add_Scene_object */

int Scene_picked_object_get_number_of_scene_objects(
	struct Scene_picked_object *scene_picked_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the number of scene objects in the path of our display heirarchy to the
<scene_picked_object>.
==============================================================================*/
{
	int number_of_scene_objects;

	ENTER(Scene_picked_object_get_number_of_scene_objects);
	if (scene_picked_object)
	{
		number_of_scene_objects = scene_picked_object->number_of_scene_objects;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_number_of_scene_objects.  Invalid argument(s)");
		number_of_scene_objects=0;
	}
	LEAVE;

	return (number_of_scene_objects);
} /* Scene_picked_object_get_number_of_scene_objects */

struct Scene_object *Scene_picked_object_get_Scene_object(
	struct Scene_picked_object *scene_picked_object,int scene_object_no)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the scene_object at position <scene_object_no> - where 0 is the first -
in the list of scene_objects in the path of our display heirarchy to the
<scene_picked_object>.
==============================================================================*/
{
	struct Scene_object *scene_object;

	ENTER(Scene_picked_object_get_Scene_object);
	if (scene_picked_object&&(0<=scene_object_no)&&
		(scene_object_no<scene_picked_object->number_of_scene_objects))
	{
		scene_object = scene_picked_object->scene_objects[scene_object_no];
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_Scene_object.  Invalid argument(s)");
		scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (scene_object);
} /* Scene_picked_object_get_Scene_object */

int Scene_picked_object_add_subobject(
	struct Scene_picked_object *scene_picked_object,int subobject)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Adds the <subobject> name to the end of the list of names identifying the
particular picked graphic represented by the <scene_picked_object>.
==============================================================================*/
{
	int return_code,*temp_subobjects;

	ENTER(Scene_picked_object_add_subobject);
	if (scene_picked_object)
	{
		if (REALLOCATE(temp_subobjects,scene_picked_object->subobjects,
			int,scene_picked_object->number_of_subobjects+1))
		{
			scene_picked_object->subobjects = temp_subobjects;
			scene_picked_object->subobjects[scene_picked_object->number_of_subobjects]
				= subobject;
			scene_picked_object->number_of_subobjects++;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_picked_object_add_subobject.  Not enough memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_add_subobject.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_add_subobject */

int Scene_picked_object_get_number_of_subobjects(
	struct Scene_picked_object *scene_picked_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the number of integer subobject names identifying the
<scene_picked_object>.
==============================================================================*/
{
	int number_of_subobjects;

	ENTER(Scene_picked_object_get_number_of_subobjects);
	if (scene_picked_object)
	{
		number_of_subobjects = scene_picked_object->number_of_subobjects;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_number_of_subobjects.  Invalid argument(s)");
		number_of_subobjects=0;
	}
	LEAVE;

	return (number_of_subobjects);
} /* Scene_picked_object_get_number_of_subobjects */

int Scene_picked_object_get_subobject(
	struct Scene_picked_object *scene_picked_object,int subobject_no)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the subobject at position <subobject_no> - where 0 is the first - in
the list of integer subobject names identifying the <scene_picked_object>.
==============================================================================*/
{
	int subobject;

	ENTER(Scene_picked_object_get_subobject);
	if (scene_picked_object&&(0<=subobject_no)&&
		(subobject_no<scene_picked_object->number_of_subobjects))
	{
		subobject = scene_picked_object->subobjects[subobject_no];
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_subobject.  Invalid argument(s)");
		subobject=0;
	}
	LEAVE;

	return (subobject);
} /* Scene_picked_object_get_subobject */

unsigned int Scene_picked_object_get_farthest(
	struct Scene_picked_object *scene_picked_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the <farthest> position at which the <scene_picked_object> was picked.
==============================================================================*/
{
	unsigned int farthest;

	ENTER(Scene_picked_object_get_farthest);
	if (scene_picked_object)
	{
		farthest = scene_picked_object->farthest;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_farthest.  Invalid argument(s)");
		farthest=0;
	}
	LEAVE;

	return (farthest);
} /* Scene_picked_object_get_farthest */

int Scene_picked_object_set_farthest(
	struct Scene_picked_object *scene_picked_object,unsigned int farthest)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Sets the <farthest> position at which the <scene_picked_object> was picked.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_picked_object_set_farthest);
	if (scene_picked_object)
	{
		scene_picked_object->farthest = farthest;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_set_farthest.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_set_farthest */

unsigned int Scene_picked_object_get_nearest(
	struct Scene_picked_object *scene_picked_object)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Returns the <nearest> position at which the <scene_picked_object> was picked.
==============================================================================*/
{
	unsigned int nearest;

	ENTER(Scene_picked_object_get_nearest);
	if (scene_picked_object)
	{
		nearest = scene_picked_object->nearest;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_nearest.  Invalid argument(s)");
		nearest=0;
	}
	LEAVE;

	return (nearest);
} /* Scene_picked_object_get_nearest */

int Scene_picked_object_set_nearest(
	struct Scene_picked_object *scene_picked_object,unsigned int nearest)
/*******************************************************************************
LAST MODIFIED : 15 July 1999

DESCRIPTION :
Sets the <nearest> position at which the <scene_picked_object> was picked.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_picked_object_set_nearest);
	if (scene_picked_object)
	{
		scene_picked_object->nearest = nearest;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_set_nearest.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_set_nearest */

int Scene_picked_object_write(struct Scene_picked_object *scene_picked_object)
/*******************************************************************************
LAST MODIFIED : 19 July 1999

DESCRIPTION :
Writes the contents of the <scene_picked_object> as:
hit_no: scene_object_name[.scene_object_name...] subobject_number...
==============================================================================*/
{
	int i,return_code;

	ENTER(Scene_picked_object_write);
	if (scene_picked_object)
	{
		display_message(INFORMATION_MESSAGE,"%d: ",scene_picked_object->hit_no);
		for (i=0;i<scene_picked_object->number_of_scene_objects;i++)
		{
			if (0<i)
			{
				display_message(INFORMATION_MESSAGE,".");
			}
			display_message(INFORMATION_MESSAGE,"%s",
				scene_picked_object->scene_objects[i]->name);
		}
		for (i=0;i<scene_picked_object->number_of_subobjects;i++)
		{
			display_message(INFORMATION_MESSAGE," %d",
				scene_picked_object->subobjects[i]);
		}
		display_message(INFORMATION_MESSAGE,", near=%d far=%d\n",
			scene_picked_object->nearest,scene_picked_object->farthest);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_write.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_write */

int Scene_picked_objects_have_same_transformation(
	struct Scene_picked_object *scene_picked_object1,
	struct Scene_picked_object *scene_picked_object2)
/*******************************************************************************
LAST MODIFIED : 23 July 1999

DESCRIPTION :
Returns true if <scene_picked_object1> and <scene_picked_object2> have the
same total transformation.
==============================================================================*/
{
	double transformation_matrix1[16],transformation_matrix2[16];
	int i,return_code,transformation_required1,transformation_required2;

	ENTER(Scene_picked_objects_have_same_transformation);
	return_code=0;
	if (scene_picked_object1&&scene_picked_object2)
	{
		if (Scene_picked_object_get_total_transformation_matrix(
			scene_picked_object1,&transformation_required1,transformation_matrix1)&&
			Scene_picked_object_get_total_transformation_matrix(
				scene_picked_object2,&transformation_required2,transformation_matrix2)&&
			(transformation_required1==transformation_required2))
		{
			return_code=1;
			for (i=0;(i>16)&&return_code;i++)
			{
				return_code=(transformation_matrix1[i] == transformation_matrix2[i]);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_objects_have_same_transformation.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_objects_have_same_transformation */

int Scene_picked_object_get_total_transformation_matrix(
	struct Scene_picked_object *scene_picked_object,int *transformation_required,
	double *transformation_matrix)
/*******************************************************************************
LAST MODIFIED : 23 July 1999

DESCRIPTION :
Multiplies the transformation matrices for all the scene_objects in the
<scene_picked_object>, returning the overall <matrix>. The matrix has 16 values
in the order of along rows first, which operate on the untransformed homogeneous
coordinates [x y z h(=1)] to give [x' y' z' h'], with xm = x'/h', etc. as in:
|x'| |M11 M12 M13 M14| |x|
|y'|=|M21 M22 M23 M24|.|y|
|z'| |M31 M32 M33 M34| |z|
|h'| |M41 M42 M43 M44| |h|
However, if none of the scene objects have transformations, the flag
<transformation_required> will be set to 0 and the <transformation_matrix> will
be set to the identity.
==============================================================================*/
{
	double mat1[6],mat2[16];
	int i,j,k,number_of_transformations,return_code;
	gtMatrix gt_transformation;

	ENTER(Scene_picked_object_get_total_transformation_matrix);
	if (scene_picked_object&&transformation_required&&transformation_matrix)
	{
		number_of_transformations=0;
		for (k=0;k<scene_picked_object->number_of_scene_objects;k++)
		{
			if (Scene_object_has_transformation(
				scene_picked_object->scene_objects[k]))
			{
				number_of_transformations++;
				Scene_object_get_transformation(scene_picked_object->scene_objects[k],
					&gt_transformation);
				if (1==number_of_transformations)
				{
					/* transpose gtMatrix */
					for (i=0;i<4;i++)
					{
						for (j=0;j<4;j++)
						{
							transformation_matrix[i*4+j] = gt_transformation[j][i];
						}
					}
				}
				else
				{
					/* transpose gtMatrix */
					for (i=0;i<4;i++)
					{
						for (j=0;j<4;j++)
						{
							mat1[i*4+j] = gt_transformation[j][i];
						}
					}
					multiply_matrix(4,4,4,transformation_matrix,mat1,mat2);
					copy_matrix(4,4,mat2,transformation_matrix);
				}
			}
		}
		if (!((*transformation_required)=(0<number_of_transformations)))
		{
			/* return the identity matrix - just in case */
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					if (i==j)
					{
						transformation_matrix[i*4+j] = 1.0;
					}
					else
					{
						transformation_matrix[i*4+j] = 0.0;
					}
				}
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_get_total_transformation_matrix.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_picked_object_get_total_transformation_matrix */

DECLARE_OBJECT_FUNCTIONS(Scene_picked_object)

DECLARE_LIST_FUNCTIONS(Scene_picked_object)

struct FE_element *Scene_picked_object_list_get_nearest_element(
	struct LIST(Scene_picked_object) *scene_picked_object_list,
	struct GROUP(FE_element) *element_group,
	struct Scene_picked_object **scene_picked_object_address,
	struct GT_element_group **gt_element_group_address,
	struct GT_element_settings **gt_element_settings_address)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
Returns the nearest picked element in <scene_picked_object_list> that is in
<element_group> (or any group if NULL). If any of the remaining address
arguments are not NULL, they are filled with the appropriate information
pertaining to the nearest element.
==============================================================================*/
{
	struct Scene_picked_object_get_nearest_element_data nearest_element_data;

	ENTER(Scene_picked_object_list_get_nearest_element);
	nearest_element_data.nearest=0;
	nearest_element_data.nearest_element=(struct FE_element *)NULL;
	nearest_element_data.element_group=element_group;
	nearest_element_data.scene_picked_object=(struct Scene_picked_object *)NULL;
	nearest_element_data.gt_element_group=(struct GT_element_group *)NULL;
	nearest_element_data.gt_element_settings=(struct GT_element_settings *)NULL;
	if (scene_picked_object_list)
	{
		FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
			Scene_picked_object_get_nearest_element,(void *)&nearest_element_data,
			scene_picked_object_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_nearest_element.  Invalid argument(s)");
	}
	if (scene_picked_object_address)
	{
		*scene_picked_object_address=nearest_element_data.scene_picked_object;
	}
	if (gt_element_group_address)
	{
		*gt_element_group_address=nearest_element_data.gt_element_group;
	}
	if (gt_element_settings_address)
	{
		*gt_element_settings_address=nearest_element_data.gt_element_settings;
	}
	LEAVE;

	return (nearest_element_data.nearest_element);
} /* Scene_picked_object_list_get_nearest_element */

struct LIST(FE_element) *Scene_picked_object_list_get_picked_elements(
	struct LIST(Scene_picked_object) *scene_picked_object_list)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
Returns the list of all elements identified in the <scene_picked_object_list>. 
==============================================================================*/
{
	struct LIST(FE_element) *picked_element_list;

	ENTER(Scene_picked_object_list_get_picked_elements);
	if (scene_picked_object_list)
	{	
		if (picked_element_list=CREATE(LIST(FE_element))())
		{
			FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
				Scene_picked_object_get_picked_elements,(void *)picked_element_list,
				scene_picked_object_list);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_picked_object_list_get_picked_elements.  "
				"Could not create element list");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_picked_elements.  Invalid argument(s)");
		picked_element_list=(struct LIST(FE_element) *)NULL;
	}
	LEAVE;

	return (picked_element_list);
} /* Scene_picked_object_list_get_picked_elements */

struct Element_point_ranges *Scene_picked_object_list_get_nearest_element_point(
	struct LIST(Scene_picked_object) *scene_picked_object_list,
	struct GROUP(FE_element) *element_group,
	struct Scene_picked_object **scene_picked_object_address,
	struct GT_element_group **gt_element_group_address,
	struct GT_element_settings **gt_element_settings_address)
/*******************************************************************************
LAST MODIFIED : 1 March 2000

DESCRIPTION :
Returns the nearest picked element point in <scene_picked_object_list> that is
in <element_group> (or any group if NULL). If any of the remaining address
arguments are not NULL, they are filled with the appropriate information
pertaining to the nearest element point.
The returned Element_point_ranges structure should be used or destroyed by the
calling function.
==============================================================================*/
{
	struct Scene_picked_object_get_nearest_element_point_data
		nearest_element_point_data;

	ENTER(Scene_picked_object_list_get_nearest_element_point);
	nearest_element_point_data.nearest=0;
	nearest_element_point_data.nearest_element_point=
		(struct Element_point_ranges *)NULL;
	nearest_element_point_data.element_group=element_group;
	nearest_element_point_data.scene_picked_object=
		(struct Scene_picked_object *)NULL;
	nearest_element_point_data.gt_element_group=(struct GT_element_group *)NULL;
	nearest_element_point_data.gt_element_settings=
		(struct GT_element_settings *)NULL;
	if (scene_picked_object_list)
	{
		FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
			Scene_picked_object_get_nearest_element_point,
			(void *)&nearest_element_point_data,scene_picked_object_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_nearest_element_point.  "
			"Invalid argument(s)");
	}
	if (scene_picked_object_address)
	{
		*scene_picked_object_address=nearest_element_point_data.scene_picked_object;
	}
	if (gt_element_group_address)
	{
		*gt_element_group_address=nearest_element_point_data.gt_element_group;
	}
	if (gt_element_settings_address)
	{
		*gt_element_settings_address=nearest_element_point_data.gt_element_settings;
	}
	LEAVE;

	return (nearest_element_point_data.nearest_element_point);
} /* Scene_picked_object_list_get_nearest_element_point */

struct LIST(Element_point_ranges) *Scene_picked_object_list_get_picked_element_points(
	struct LIST(Scene_picked_object) *scene_picked_object_list)
/*******************************************************************************
LAST MODIFIED : 18 May 2000

DESCRIPTION :
Returns the list of all element_points in the <scene_picked_object_list>.
==============================================================================*/
{
	struct LIST(Element_point_ranges) *picked_element_points_list;

	ENTER(Scene_picked_object_list_get_picked_element_points);
	if (scene_picked_object_list)
	{	
		if (picked_element_points_list=CREATE(LIST(Element_point_ranges))())
		{
			FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
				Scene_picked_object_get_picked_element_points,
				(void *)picked_element_points_list,scene_picked_object_list);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_picked_object_list_get_picked_element_points.  "
				"Could not create element point ranges list");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_picked_element_points.  "
			"Invalid argument(s)");
		picked_element_points_list=(struct LIST(Element_point_ranges) *)NULL;
	}
	LEAVE;

	return (picked_element_points_list);
} /* Scene_picked_object_list_get_picked_element_points */

struct FE_node *Scene_picked_object_list_get_nearest_node(
	struct LIST(Scene_picked_object) *scene_picked_object_list,
	int use_data,struct GROUP(FE_node) *node_group,
	struct Scene_picked_object **scene_picked_object_address,
	struct GT_element_group **gt_element_group_address,
	struct GT_element_settings **gt_element_settings_address)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
Returns the nearest picked node in <scene_picked_object_list> that is in
<node_group> (or any group if NULL). If any of the remaining address arguments
are not NULL, they are filled with the appropriate information pertaining to
the nearest node.
The <use_data> flag indicates that we are searching for a data point instead of
a node, needed since different settings type used for each.
==============================================================================*/
{
	struct Scene_picked_object_get_nearest_node_data nearest_node_data;

	ENTER(Scene_picked_object_list_get_nearest_node);
	nearest_node_data.nearest=0;
	nearest_node_data.nearest_node=(struct FE_node *)NULL;
	nearest_node_data.use_data=use_data;
	nearest_node_data.node_group=node_group;
	nearest_node_data.scene_picked_object=(struct Scene_picked_object *)NULL;
	nearest_node_data.gt_element_group=(struct GT_element_group *)NULL;
	nearest_node_data.gt_element_settings=(struct GT_element_settings *)NULL;
	if (scene_picked_object_list)
	{
		FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
			Scene_picked_object_get_nearest_node,(void *)&nearest_node_data,
			scene_picked_object_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_nearest_node.  Invalid argument(s)");
	}
	if (scene_picked_object_address)
	{
		*scene_picked_object_address=nearest_node_data.scene_picked_object;
	}
	if (gt_element_group_address)
	{
		*gt_element_group_address=nearest_node_data.gt_element_group;
	}
	if (gt_element_settings_address)
	{
		*gt_element_settings_address=nearest_node_data.gt_element_settings;
	}
	LEAVE;

	return (nearest_node_data.nearest_node);
} /* Scene_picked_object_list_get_nearest_node */

struct LIST(FE_node) *Scene_picked_object_list_get_picked_nodes(
	struct LIST(Scene_picked_object) *scene_picked_object_list,int use_data)
/*******************************************************************************
LAST MODIFIED : 5 July 2000

DESCRIPTION :
Returns the list of all nodes in the <scene_picked_object_list>. 
The <use_data> flag indicates that we are searching for data points instead of
nodes, needed since different settings type used for each.
==============================================================================*/
{
	struct Scene_picked_object_get_picked_nodes_data picked_nodes_data;

	ENTER(Scene_picked_object_list_get_picked_nodes);
	if (scene_picked_object_list)
	{	
		picked_nodes_data.use_data=use_data;
		if (picked_nodes_data.node_list=CREATE(LIST(FE_node))())
		{
			FOR_EACH_OBJECT_IN_LIST(Scene_picked_object)(
				Scene_picked_object_get_picked_nodes,(void *)&picked_nodes_data,
				scene_picked_object_list);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_picked_object_list_get_picked_nodes.  "
				"Could not create node list");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_picked_object_list_get_picked_nodes.  Invalid argument(s)");
		picked_nodes_data.node_list=(struct LIST(FE_node) *)NULL;
	}
	LEAVE;

	return (picked_nodes_data.node_list);
} /* Scene_picked_object_list_get_picked_nodes */

int Scene_get_input_callback(struct Scene *scene,
	struct Scene_input_callback *scene_input_callback)
/*******************************************************************************
LAST MODIFIED : 25 March 1999

DESCRIPTION :
Fills <scene_input_callback> with the current scene input_callback information.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_get_input_callback);
	if (scene&&scene_input_callback)
	{
		scene_input_callback->procedure=scene->input_callback.procedure;
		scene_input_callback->data=scene->input_callback.data;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_input_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_input_callback */

int Scene_set_input_callback(struct Scene *scene,
	struct Scene_input_callback *scene_input_callback)
/*******************************************************************************
LAST MODIFIED : 25 March 1999

DESCRIPTION :
Sets the function that will be called to pass on input information, and the data
that that function wants to receive.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_set_input_callback);
	if (scene&&scene_input_callback)
	{
		scene->input_callback.procedure=scene_input_callback->procedure;
		scene->input_callback.data=scene_input_callback->data;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_input_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_input_callback */

int Scene_input(struct Scene *scene,enum Scene_input_type input_type,
	int button_number,int input_modifier,double viewx,double viewy,double viewz,
	double nearx,double neary,double nearz,double farx,double fary,double farz,
	int num_hits,GLuint *select_buffer)
/*******************************************************************************
LAST MODIFIED : 15 February 2000

DESCRIPTION :
Routine called by Scene_viewers - and in future possibly other Scene clients -
to tell about input. At present only mouse input is handled; the <input_type>
says whether it is a mouse button press, motion or button release event, and the
button_number and whether modifier keys were depressed at the time of the
event are given in subsequent parameters. The coordinates of two points in
space, near and far, are given to identify the ray under the mouse pointer at
the time of the event. The view direction is also needed for clients of pick and
drag information. Finally, if the event involved selection/picking, the number
of hits and the select buffer will be set accordingly. The main function of
this routine is to convert picking information into a list of easy-to-interpret
Scene_picked_objects to pass to clients of the scene, eg. node editor.
==============================================================================*/
{
	GLuint *select_buffer_ptr,scene_object_no;
	int hit_no,number_of_names,return_code;
	struct Scene_input_callback_data scene_input_data;
	struct Scene_picked_object *scene_picked_object;
	struct Scene_object *scene_object;

	ENTER(Scene_input);
	if (scene)
	{
		/* processing successful unless otherwise specified */
		return_code=1;
		switch (input_type)
		{
			case SCENE_BUTTON_PRESS:
			case SCENE_MOTION_NOTIFY:
			case SCENE_BUTTON_RELEASE:
			{
				if (scene_input_data.picked_object_list=
					CREATE(LIST(Scene_picked_object))())
				{
					select_buffer_ptr=select_buffer;
					for (hit_no=0;(hit_no<num_hits)&&return_code;hit_no++)
					{
						if (scene_picked_object=CREATE(Scene_picked_object)(hit_no))
						{
							number_of_names=(int)(*select_buffer_ptr);
							select_buffer_ptr++;
							Scene_picked_object_set_nearest(scene_picked_object,
								(unsigned int)(*select_buffer_ptr));
							select_buffer_ptr++;
							Scene_picked_object_set_farthest(scene_picked_object,
								(unsigned int)(*select_buffer_ptr));
							select_buffer_ptr++;

							/* first part of names identifies list of scene_objects in path
								 to picked graphic. Must be at least one; only more that one
								 if contains child_scene */
							do
							{
								scene_object_no= *select_buffer_ptr;
								select_buffer_ptr++;
								number_of_names--;
								if (scene_object=FIND_BY_IDENTIFIER_IN_LIST(Scene_object,
									position)(scene_object_no,scene->scene_object_list))
								{
									return_code=Scene_picked_object_add_Scene_object(
										scene_picked_object,scene_object);
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"Scene_input.  No scene object at position %d",
										scene_object_no);
									return_code=0;
								}
							}
							while (return_code&&(SCENE_OBJECT_SCENE==scene_object->type));

							if (return_code)
							{
								for (;0<number_of_names&&return_code;number_of_names--)
								{
									return_code=Scene_picked_object_add_subobject(
										scene_picked_object,(int)(*select_buffer_ptr));
									select_buffer_ptr++;
								}
							}
							if (return_code)
							{
								if (!ADD_OBJECT_TO_LIST(Scene_picked_object)(
									scene_picked_object,
									scene_input_data.picked_object_list))
								{
									return_code=0;
								}
							}
							if (!return_code)
							{
								display_message(ERROR_MESSAGE,
									"Scene_input.  Failed to build Scene_picked_object");
								DESTROY(Scene_picked_object)(&scene_picked_object);
							}
						}
					}
					if (return_code)
					{
						/* now send the callback */
						if ((scene->input_callback).procedure)
						{
							/* complete members of callback data structure */
							scene_input_data.viewx=viewx;
							scene_input_data.viewy=viewy;
							scene_input_data.viewz=viewz;
							scene_input_data.nearx=nearx;
							scene_input_data.neary=neary;
							scene_input_data.nearz=nearz;
							scene_input_data.farx=farx;
							scene_input_data.fary=fary;
							scene_input_data.farz=farz;
							scene_input_data.input_type=input_type;
							scene_input_data.button_number=button_number;
							scene_input_data.input_modifier=input_modifier;
							if (scene->input_callback.procedure)
							{
								(scene->input_callback.procedure)(scene,
									scene->input_callback.data,&scene_input_data);
							}
						}
					}
					DESTROY(LIST(Scene_picked_object))(
						&(scene_input_data.picked_object_list));
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_input.  Could not create picked object list");
					return_code=0;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"Scene_input.  Invalid input_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_input.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_input */

struct LIST(Scene_picked_object) *Scene_pick_objects(struct Scene *scene,
	struct Interaction_volume *interaction_volume)
/*******************************************************************************
LAST MODIFIED : 1 May 2000

DESCRIPTION :
Returns a list of all the graphical entities in the <interaction_volume> of
<scene>. The nearest member of each scene_picked_object will be adjusted as
understood for the type of <interaction_volume> passed.
==============================================================================*/
{
	double depth,modelview_matrix[16],normalised_z,projection_matrix[16];
	GLdouble opengl_modelview_matrix[16],opengl_projection_matrix[16];
	GLuint *select_buffer,*select_buffer_ptr;
	int hit_no,i,j,num_hits,number_of_names,return_code,scene_object_no;
	struct LIST(Scene_picked_object) *scene_picked_object_list;
	struct Scene_picked_object *scene_picked_object;
	struct Scene_object *scene_object;

	ENTER(Scene_pick_objects);
	scene_picked_object_list=(struct LIST(Scene_picked_object) *)NULL;
	if (scene&&interaction_volume)
	{
		if (scene_picked_object_list=CREATE(LIST(Scene_picked_object))())
		{
			if (X3dThreeDDrawingGetCurrent()&&compile_Scene(scene))
			{
				select_buffer=(GLuint *)NULL;
				num_hits=-1;
				while (0>num_hits)
				{
					if (ALLOCATE(select_buffer,GLuint,select_buffer_size))
					{
						select_buffer_ptr=select_buffer;
						Interaction_volume_get_modelview_matrix(interaction_volume,
							modelview_matrix);
						Interaction_volume_get_projection_matrix(interaction_volume,
							projection_matrix);
						/* transpose projection matrix for OpenGL */
						for (i=0;i<4;i++)
						{
							for (j=0;j<4;j++)
							{
								opengl_modelview_matrix[j*4+i] = modelview_matrix[i*4+j];
								opengl_projection_matrix[j*4+i] = projection_matrix[i*4+j];
							}
						}
						glSelectBuffer(select_buffer_size,select_buffer);
						glRenderMode(GL_SELECT);
						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
						glMultMatrixd(opengl_projection_matrix);
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
						glMultMatrixd(opengl_modelview_matrix);
						/* set an arbitrary viewport - not really needed */
						glViewport(0,0,1024,1024);
						glDepthRange((GLclampd)0,(GLclampd)1);
						execute_Scene(scene);
						glFlush();
						num_hits=glRenderMode(GL_RENDER);
						if (0<=num_hits)
						{
							return_code=1;
							for (hit_no=0;(hit_no<num_hits)&&return_code;hit_no++)
							{
								if (scene_picked_object=CREATE(Scene_picked_object)(hit_no))
								{
									number_of_names=(int)(*select_buffer_ptr);
									select_buffer_ptr++;
									/* get range of depth of picked object */
									depth=(double)(*select_buffer_ptr);
									normalised_z=2.0*depth - 1.0;
									Scene_picked_object_set_nearest(scene_picked_object,
										Interaction_volume_get_closeness_from_normalised_z(
											interaction_volume,normalised_z));
									select_buffer_ptr++;
									depth=(double)(*select_buffer_ptr);
									normalised_z=2.0*depth - 1.0;
									Scene_picked_object_set_farthest(scene_picked_object,
										Interaction_volume_get_closeness_from_normalised_z(
											interaction_volume,normalised_z));
									select_buffer_ptr++;
									/* first part of names identifies list of scene_objects in
										 path to picked graphic. Must be at least one; only more
										 that one if contains child_scene */
									do
									{
										scene_object_no=(int)(*select_buffer_ptr);
										select_buffer_ptr++;
										number_of_names--;
										if (scene_object=FIND_BY_IDENTIFIER_IN_LIST(Scene_object,
											position)(scene_object_no,scene->scene_object_list))
										{
											return_code=Scene_picked_object_add_Scene_object(
												scene_picked_object,scene_object);
										}
										else
										{
											display_message(ERROR_MESSAGE,
												"Scene_pick_objects.  No scene object at position %d",
												scene_object_no);
											return_code=0;
										}
									}
									while (return_code&&(SCENE_OBJECT_SCENE==scene_object->type));
									if (return_code)
									{
										for (;0<number_of_names&&return_code;number_of_names--)
										{
											return_code=Scene_picked_object_add_subobject(
												scene_picked_object,(int)(*select_buffer_ptr));
											select_buffer_ptr++;
										}
									}
									if (return_code)
									{
										if (!ADD_OBJECT_TO_LIST(Scene_picked_object)(
											scene_picked_object,scene_picked_object_list))
										{
											return_code=0;
										}
									}
									if (!return_code)
									{
										display_message(ERROR_MESSAGE,"Scene_pick_objects.  "
											"Failed to build Scene_picked_object");
										DESTROY(Scene_picked_object)(&scene_picked_object);
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,"Scene_pick_objects.  "
										"Could not create Scene_picked_object");
									return_code=0;
								}
							}
						}
						else
						{
							/* select buffer overflow; enlarge and repeat */
							select_buffer_size += SELECT_BUFFER_SIZE_INCREMENT;
						}
						DEALLOCATE(select_buffer);
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_pick_objects.  No current X3d drawing widget");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_pick_objects.  Could not create picked object list");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_pick_objects.  Invalid argument(s)");
	}
	LEAVE;

	return (scene_picked_object_list);
} /* Scene_pick_objects */

int Scene_add_light(struct Scene *scene,struct Light *light)
/*******************************************************************************
LAST MODIFIED : 12 December 1997

DESCRIPTION :
Adds a light to the Scene list_of_lights.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_add_light);
	if (scene&&light)
	{
		if (!IS_OBJECT_IN_LIST(Light)(light,scene->list_of_lights))
		{
			return_code=ADD_OBJECT_TO_LIST(Light)(light,scene->list_of_lights);
			Scene_changed_private(scene);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_add_light.  Light already in list");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_add_light.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_add_light */

int Scene_has_light(struct Scene *scene,struct Light *light)
/*******************************************************************************
LAST MODIFIED : 12 December 1997

DESCRIPTION :
Returns true if <Scene> has <light> in its list_of_lights, OR if <light>
is NULL, returns true if <scene> has any lights.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_has_light);
	if (scene)
	{
		if (light)
		{
			return_code=IS_OBJECT_IN_LIST(Light)(light,scene->list_of_lights);
		}
		else
		{
			return_code=NUMBER_IN_LIST(Light)(scene->list_of_lights);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_has_light.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_has_light */

int Scene_remove_light(struct Scene *scene,struct Light *light)
/*******************************************************************************
LAST MODIFIED : 12 December 1997

DESCRIPTION :
Removes a light from the Scene list_of_lights.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_remove_light);
	if (scene&&light)
	{
		if (IS_OBJECT_IN_LIST(Light)(light,scene->list_of_lights))
		{
			return_code=REMOVE_OBJECT_FROM_LIST(Light)(light,
				scene->list_of_lights);
			Scene_changed_private(scene);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_remove_light.  Light not in list");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_remove_light.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_remove_light */

int for_each_Light_in_Scene(struct Scene *scene,
	LIST_ITERATOR_FUNCTION(Light) *iterator_function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 18 December 1997

DESCRIPTION :
Allows clients of the <scene> to perform functions with the lights in it. The
most common task will be to call execute_Light.
==============================================================================*/
{
	int return_code;

	ENTER(for_each_Light_in_Scene);
	if (scene&&iterator_function)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(Light)(iterator_function,user_data,
			scene->list_of_lights);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_Light_in_Scene.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* for_each_Light_in_Scene */

int compile_Scene(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 27 June 2000

DESCRIPTION :
Assembles the display list containing the whole scene. Before that, however, it
compiles the display lists of objects that will be executed in the scene.
Note that lights are not included in the scene and must be handled separately!
==============================================================================*/
{
	float glyph_time;
	int return_code;

	ENTER(compile_Scene);
	/* checking arguments */
	if (scene&&scene->graphical_material_manager)
	{
		return_code=1;
		/* 1 = display list current, 0 = not current. Any other number denotes
			 display list of member(s) of scene not current, but scene itself OK */
		if (1 != scene->display_list_current)
		{
#if defined (DEBUG)
			if (0 !=  scene->display_list_current)
			{
				printf("--- compiling members of scene %s\n",scene->name);
			}
#endif /* defined (DEBUG) */
			/* compile all the objects that make up the scene */
			/* compile textures first since they may be used by materials */
			FOR_EACH_OBJECT_IN_MANAGER(Texture)(
				compile_Texture,(void *)NULL,scene->texture_manager);
			/* compile graphical materials */
			FOR_EACH_OBJECT_IN_MANAGER(Graphical_material)(
				compile_Graphical_material,(void *)NULL,
				scene->graphical_material_manager);
			/* compile glyphs */
			glyph_time = 0.0;
			FOR_EACH_OBJECT_IN_LIST(GT_object)(compile_GT_object,&glyph_time,
				scene->glyph_list);
			/* compile graphics_objects in the scene */
			FOR_EACH_OBJECT_IN_LIST(Scene_object)(compile_Scene_object,
				NULL,scene->scene_object_list);
			/* compile lights in the scene */
			FOR_EACH_OBJECT_IN_LIST(Light)(compile_Light,(void *)NULL,
				scene->list_of_lights);
			if (0==scene->display_list_current)
			{
				if (scene->display_list||(scene->display_list=glGenLists(1)))
				{
#if defined (DEBUG)
					/*???debug*/printf("--- compiling scene %s\n",scene->name);
#endif /* defined (DEBUG) */
					/* now compile the whole scene */
					glNewList(scene->display_list,GL_COMPILE);
					/* initialize the names stack at the start of the scene */
					glInitNames();
					/* push a dummy name to be overloaded with scene_object
						identifiers */
					glPushName(0);
					FOR_EACH_OBJECT_IN_LIST(Scene_object)(execute_Scene_object,
						(void *)scene,scene->scene_object_list);
					glPopName();
					glEndList();
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"compile_Scene.  Could not generate display list");
					return_code=0;
				}
			}
			if (return_code)
			{
				scene->display_list_current=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compile_Scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* compile_Scene */

int execute_Scene(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Calls the display list for <scene>. If the display list is not current, an
an error is reported.
Note that lights are not included in the scene and must be handled separately!
==============================================================================*/
{
	int return_code;

	ENTER(execute_Scene);
	if (scene)
	{
		if (scene->display_list_current)
		{
			glCallList(scene->display_list);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"execute_Scene.  display list not current");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"execute_Scene.  Missing scene");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* execute_Scene */

int Scene_add_graphics_object(struct Scene *scene,
	struct GT_object *graphics_object,int position, char *name)
/*******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Adds <graphics_object> to the list of objects on <scene> at <position>.
A position of 1 indicates the top of the list, while less than 1 or greater
than the number of graphics objects in the list puts it at the end.
The <name> is used for the scene_object and must be unique for the scene.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;
	
	ENTER(Scene_add_graphics_object);
	/* check arguments */
	if (scene&&graphics_object&&name)
	{
		if (!(scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_name, (void *)name,
			scene->scene_object_list)))
		{
			if (scene_object=create_Scene_object_with_Graphics_object(
				name, graphics_object, scene))
			{
				return_code = Scene_add_scene_object(scene, scene_object,position);
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			/* not really an error */
#if defined (OLD_CODE)
			display_message(WARNING_MESSAGE,
				"Scene_add_graphics_object.  Object with that name already in scene");
#endif /* defined (OLD_CODE) */
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_add_graphics_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_add_graphics_object */

int Scene_remove_graphics_object(struct Scene *scene,
	struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 15 June 1998

DESCRIPTION :
Removes <graphics object> from the list of objects on <scene>.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_remove_graphics_object);
	/* check arguments */
	if (scene&&graphics_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)graphics_object,
			scene->scene_object_list))
		{
			Scene_remove_scene_object(scene, scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_remove_graphics_object.  Object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_remove_graphics_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_remove_graphics_object */

int Scene_add_child_scene(struct Scene *scene, struct Scene *child_scene,
	int position, char *name, struct MANAGER(Scene) *scene_manager)
/*******************************************************************************
LAST MODIFIED : 20 November 1998

DESCRIPTION :
Adds <child_scene> to the list of objects on <scene> at <position>.
A position of 1 indicates the top of the list, while less than 1 or greater
than the number of graphics objects in the list puts it at the end.
The optional <scene_object_name> is used for the scene_object and must be unique for the scene.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;
	
	ENTER(Scene_add_child_scene);
	if (scene&&child_scene&&name)
	{
		if (!(scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_name, (void *)name, scene->scene_object_list)))
		{
			if (scene_object=create_Scene_object_with_Scene(
				name, child_scene, scene, scene_manager))
			{
				return_code = Scene_add_scene_object(scene, scene_object,position);
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_add_child_scene.  Object with name %s already in scene", name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_add_child_scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_add_child_scene */

int Scene_remove_child_scene(struct Scene *scene,struct Scene *child_scene)
/*******************************************************************************
LAST MODIFIED : 20 November 1998

DESCRIPTION :
Removes <child_scene> from the list of scenes in <scene>.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_remove_child_scene);
	if (scene&&child_scene)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_child_scene,(void *)child_scene,
			scene->scene_object_list))
		{
			Scene_remove_scene_object(scene, scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_remove_child_scene.  child_scene not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_remove_child_scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_remove_child_scene */

int Scene_add_graphical_finite_element(struct Scene *scene,
	struct GROUP(FE_element) *element_group,char *scene_object_name)
/*******************************************************************************
LAST MODIFIED : 28 June 2000

DESCRIPTION :
Adds a graphical <element_group> to the <scene> with some default settings
depending on the scene's graphical_element_mode.
The optional <scene_object_name> allows the name of the scene_object containing
the group to be overridden from the group name; note that this does not allow
the same element group to be added twice.
???RC Could allow multiple renditions for same group in future.
==============================================================================*/
{
	char *element_group_name;
	enum GT_visibility_type visibility;
	int default_value,maximum_value,return_code;
	struct Computed_field *default_coordinate_field,*element_xi_coordinate_field,
		*orientation_scale_field;
	struct Element_discretization element_discretization;
	struct GROUP(FE_node) *data_group,*node_group;
	struct GT_object *glyph;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene_object *scene_object;
	Triple glyph_centre,glyph_size,glyph_scale_factors;

	ENTER(Scene_add_graphical_finite_element);
	if (scene&&element_group)
	{
		if (scene->element_manager)
		{
			if (!Scene_has_graphical_element_group(scene,element_group))
			{
				if (GET_NAME(GROUP(FE_element))(element_group,&element_group_name))
				{
					if (!scene_object_name)
					{
						scene_object_name=element_group_name;
					}
					/* Make the GT_element_group: */
					/* First retrieve node group of the same name as the element group.
						 Retrieve/create data group as necessary */
					node_group=FIND_BY_IDENTIFIER_IN_MANAGER(GROUP(FE_node),name)(
						element_group_name,scene->node_group_manager);
					if (!(data_group=FIND_BY_IDENTIFIER_IN_MANAGER(GROUP(FE_node),name)(
						element_group_name,scene->data_group_manager)))
					{
						if (data_group=CREATE(GROUP(FE_node))(element_group_name))
						{
							if (!ADD_OBJECT_TO_MANAGER(GROUP(FE_node))(data_group,
								scene->data_group_manager))
							{
								DESTROY(GROUP(FE_node))(&data_group);
							}
						}
					}
					if (node_group&&data_group)
					{
						if (gt_element_group=CREATE(GT_element_group)(
							element_group,node_group,data_group,
							scene->element_manager,
							scene->element_group_manager,
							scene->node_manager,
							scene->node_group_manager,
							scene->data_manager,
							scene->data_group_manager,
							scene->computed_field_manager,
							scene->element_point_ranges_selection,
							scene->element_selection,
							scene->node_selection,
							scene->data_selection))
						{
							if (!(scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
								Scene_object_has_name, (void *)scene_object_name,
								scene->scene_object_list)))
							{
								if ((scene_object=create_Scene_object_with_Graphical_element_group(
									scene_object_name, gt_element_group, scene)) && 
									Scene_add_scene_object(scene, scene_object, /*position*/0))
								{
									/* give the new group a default coordinate field */
									/*???RC later get this from region */
									default_coordinate_field=FIND_BY_IDENTIFIER_IN_MANAGER(
										Computed_field,name)("default_coordinate",
											scene->computed_field_manager);
									GT_element_group_set_default_coordinate_field(
										gt_element_group,default_coordinate_field);
									/* set default circle and element discretization in group */
									read_circle_discretization_defaults(&default_value,
										&maximum_value,scene->user_interface);
									GT_element_group_set_circle_discretization(gt_element_group,
										default_value,scene->user_interface);
									read_element_discretization_defaults(&default_value,
										&maximum_value,scene->user_interface);
									element_discretization.number_in_xi1=default_value;
									element_discretization.number_in_xi2=default_value;
									element_discretization.number_in_xi3=default_value;
									GT_element_group_set_element_discretization(gt_element_group,
										&element_discretization,scene->user_interface);
									switch (scene->graphical_element_mode)
									{
										case GRAPHICAL_ELEMENT_INVISIBLE:
										{
											visibility=g_INVISIBLE;
										} break;
										case GRAPHICAL_ELEMENT_EMPTY:
										case GRAPHICAL_ELEMENT_MANUAL:
										{
											visibility=g_VISIBLE;
										} break;
										case GRAPHICAL_ELEMENT_LINES:
										{
											/* add default settings - wireframe (line) rendition */
											if (settings=CREATE(GT_element_settings)(
												GT_ELEMENT_SETTINGS_LINES))
											{
												GT_element_settings_set_material(settings,
													scene->default_material);
												GT_element_settings_set_selected_material(settings,
													FIND_BY_IDENTIFIER_IN_MANAGER(Graphical_material,name)(
														"default_selected",
														scene->graphical_material_manager));
												if (GT_element_group_add_settings(gt_element_group,
													settings,0))
												{
													/* if the group has data, and either the
														 default_coordinate_field defined over them or the
														 element_xi_coordinate, add data_points to the
														 rendition */
													element_xi_coordinate_field=
														FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field,name)(
															"element_xi_coordinate",
															scene->computed_field_manager);
													if (FIRST_OBJECT_IN_GROUP_THAT(FE_node)(
														(GROUP_CONDITIONAL_FUNCTION(FE_node) *)NULL,
														(void *)NULL,data_group)&&
														(FIRST_OBJECT_IN_GROUP_THAT(FE_node)(
															FE_node_has_Computed_field_defined,
															(void *)default_coordinate_field,data_group)||
															(element_xi_coordinate_field&&
																FIRST_OBJECT_IN_GROUP_THAT(FE_node)(
																	FE_node_has_Computed_field_defined,
																	(void *)element_xi_coordinate_field,
																	data_group))))
													{
														if (settings=CREATE(GT_element_settings)(
															GT_ELEMENT_SETTINGS_DATA_POINTS))
														{
															GT_element_settings_set_coordinate_field(settings,
																element_xi_coordinate_field);
															GT_element_settings_set_material(settings,
																scene->default_material);
															GT_element_settings_set_selected_material(settings,
																FIND_BY_IDENTIFIER_IN_MANAGER(Graphical_material,
																	name)("default_selected",
																		scene->graphical_material_manager));
															/* set the glyph to "point" */
															GT_element_settings_get_glyph_parameters(settings,
																&glyph,glyph_centre,glyph_size,
																&orientation_scale_field,glyph_scale_factors);
															glyph=FIND_BY_IDENTIFIER_IN_LIST(GT_object,name)(
																"point",scene->glyph_list);
															GT_element_settings_set_glyph_parameters(settings,
																glyph,glyph_centre,glyph_size,
																orientation_scale_field,glyph_scale_factors);
															if (!GT_element_group_add_settings(gt_element_group,
																settings,0))
															{
																DESTROY(GT_element_settings)(&settings);
															}
														}
													}
													/* build graphics for default rendition */
													GT_element_group_build_graphics_objects(
														gt_element_group,(struct FE_element *)NULL,
														(struct FE_node *)NULL);
												}
												else
												{
													display_message(ERROR_MESSAGE,
														"Scene_add_graphical_finite_element.  "
														"Could not add default settings");
													DESTROY(GT_element_settings)(&settings);
												}
											}
											else
											{
												display_message(ERROR_MESSAGE,
													"Scene_add_graphical_finite_element.  "
													"Could not create default settings");
											}
											visibility=g_VISIBLE;
											scene->display_list_current=0;
										} break;
										default:
										{
											display_message(ERROR_MESSAGE,
												"Scene_add_graphical_finite_element.  "
												"Invalid graphical element mode %s",
												Scene_graphical_element_mode_string(
													scene->graphical_element_mode));
											visibility=g_INVISIBLE;
										} break;
									}
									/* set the visibility of the new GFE */
									if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
										Scene_object_has_element_group,(void *)element_group,
										scene->scene_object_list))
									{
										return_code=
											Scene_object_set_visibility(scene_object,visibility);
									}
									else
									{
										return_code=0;
									}
								}
								else
								{
									return_code=0;
								}
							}
							else
							{
								display_message(WARNING_MESSAGE,
									"Scene_add_graphical_finite_element.  "
									"Object with that name already in scene");
								return_code=1;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"Scene_add_graphical_finite_element.  "
								"Could not make gt_element_group");
							return_code=0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Scene_add_graphical_finite_element.  "
							"Could not get node_group and/or data_group of same name");
						return_code=0;
					}
					DEALLOCATE(element_group_name);
				}
				else
				{
					display_message(ERROR_MESSAGE,"Scene_add_graphical_finite_element.  "
						"Could not get element group name");
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"Scene_add_graphical_finite_element.  "
					"Element group already in scene");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_add_graphical_finite_element.  "
				"Graphical elements not enabled");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_add_graphical_finite_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_add_graphical_finite_element */

int Scene_update_time_behaviour(struct Scene *scene,
	struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 5 October 1998

DESCRIPTION :
If the graphics_object has more than one time, this function ensures that the
Scene_object has a Time_object.
==============================================================================*/
{
	char *graphics_object_name, *time_object_name;
	int return_code;
	struct Scene_object *scene_object;
	struct Time_object *time;
	
	ENTER(Scene_update_time_behaviour);
	/* check arguments */
	if (scene&&graphics_object)
	{
		/* Ensure the Scene object has a time object if the graphics
			object has more than one time */
		if(1 < GT_object_get_number_of_times(graphics_object))
		{
			if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
				Scene_object_has_gt_object,
				(void *)graphics_object,scene->scene_object_list))
			{
				if(!Scene_object_has_time(scene_object))
				{
					if(GET_NAME(GT_object)(graphics_object,&graphics_object_name)
						&& ALLOCATE(time_object_name, char, strlen(graphics_object_name)
						+ strlen(scene->name) + 5))
					{
						sprintf(time_object_name, "%s_in_%s", graphics_object_name,
							scene->name);
						if(time = CREATE(Time_object)(time_object_name))
						{
							Scene_object_set_time_object(scene_object, time);
							Time_object_set_time_keeper(time, scene->default_time_keeper);
							Time_object_add_callback(time, Scene_time_update_callback,
								scene);
						}
						DEALLOCATE(time_object_name);
						DEALLOCATE(graphics_object_name);
					}
				}
				else
				{
					time = Scene_object_get_time_object(scene_object);
				}
				return_code=FOR_EACH_OBJECT_IN_LIST(Scene_object)(
					Scene_object_update_time_behaviour, (void *)time,
					scene->scene_object_list);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_update_time_behaviour.  Unable to find Scene_object for graphics_object");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_update_time_behaviour.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_update_time_behaviour */

int Scene_set_time_behaviour(struct Scene *scene, char *scene_object_name,
	char *time_object_name, struct Time_keeper *time_keeper)
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Creates a Time_object with name <time_object_name> and sets that as the time
object for the scene_object named <scene_object_name>.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;
	struct Time_object *time, *old_time;
	
	ENTER(Scene_update_time_behaviour);
	/* check arguments */
	if (scene&&scene_object_name&&time_object_name)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_name,
			(void *)scene_object_name,scene->scene_object_list))
		{
			if(time = CREATE(Time_object)(time_object_name))
			{
				if(Scene_object_has_time(scene_object))
				{
					old_time = Scene_object_get_time_object(scene_object);
					Time_object_remove_callback(old_time, Scene_time_update_callback,
						scene);
				}
				Scene_object_set_time_object(scene_object, time);
				Time_object_set_time_keeper(time, time_keeper);
				Time_object_add_callback(time, Scene_time_update_callback,
					scene);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_update_time_behaviour.  Unable to create time_object");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_update_time_behaviour.  Unable to find object %s", scene_object_name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_update_time_behaviour.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_update_time_behaviour */

int Scene_get_axis_lengths(struct Scene *scene,Triple axis_lengths)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Returns the length of the axes - iff graphics are enabled.
==============================================================================*/
{
	int return_code;
	struct GT_glyph_set *glyph_set;

	ENTER(Scene_get_axis_lengths);
	if (scene&&scene->axis_object&&axis_lengths)
	{
		if ((glyph_set=GT_OBJECT_GET(GT_glyph_set)(scene->axis_object,/*time*/0.0))
			&&glyph_set->axis1_list)
		{
			axis_lengths[0]=(*(glyph_set->axis1_list))[0];
			axis_lengths[1]=(*(glyph_set->axis2_list))[1];
			axis_lengths[2]=(*(glyph_set->axis3_list))[2];
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_get_axis_lengths.  Invalid glyph set");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_axis_lengths.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_axis_lengths */

int Scene_set_axis_lengths(struct Scene *scene,Triple axis_lengths)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Sets the length of the axes - iff graphics are enabled.
Each axis length can now be set separately.
==============================================================================*/
{
	int return_code;
	struct GT_glyph_set *glyph_set;

	ENTER(Scene_set_axis_lengths);
	if (scene&&scene->axis_object&&axis_lengths&&(0.0<axis_lengths[0])&&
		(0.0<axis_lengths[1])&&(0.0<axis_lengths[2]))
	{
		if ((glyph_set=GT_OBJECT_GET(GT_glyph_set)(scene->axis_object,/*time*/0.0))
			&&glyph_set->axis1_list&&glyph_set->axis2_list&&glyph_set->axis3_list)
		{
			(*(glyph_set->axis1_list))[0]=axis_lengths[0];
			(*(glyph_set->axis2_list))[1]=axis_lengths[1];
			(*(glyph_set->axis3_list))[2]=axis_lengths[2];
			GT_object_changed(scene->axis_object);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_set_axis_lengths.  Invalid axis object");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_axis_lengths.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_axis_lengths */

struct Graphical_material *Scene_get_axis_material(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 21 July 1998

DESCRIPTION :
Returns the material used to draw the axes - iff graphics are enabled.
==============================================================================*/
{
	struct Graphical_material *material;

	ENTER(Scene_get_axis_material);
	if (scene&&scene->axis_object)
	{
		material=scene->axis_object->default_material;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_axis_material.  Invalid argument(s)");
		material=(struct Graphical_material *)NULL;
	}
	LEAVE;

	return (material);
} /* Scene_get_axis_material */

int Scene_set_axis_material(struct Scene *scene,
	struct Graphical_material *material)
/*******************************************************************************
LAST MODIFIED : 21 July 1998

DESCRIPTION :
Sets the material used to draw the axes - iff graphics are enabled.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_set_axis_material);
	if (scene&&scene->axis_object&&material)
	{
		return_code=set_GT_object_default_material(scene->axis_object,material);
		GT_object_changed(scene->axis_object);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_axis_material.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_axis_material */

int Scene_get_axis_origin(struct Scene *scene,Triple axis_origin)
/*******************************************************************************
LAST MODIFIED : 3 August 1998

DESCRIPTION :
Returns the origin of the axes - iff graphics are enabled.
==============================================================================*/
{
	int return_code;
	struct GT_glyph_set *glyph_set;

	ENTER(Scene_get_axis_origin);
	if (scene&&axis_origin&&scene->axis_object)
	{
		if ((glyph_set=GT_OBJECT_GET(GT_glyph_set)(scene->axis_object,/*time*/0.0))
			&&glyph_set->point_list)
		{
			axis_origin[0]=(*(glyph_set->point_list))[0];
			axis_origin[1]=(*(glyph_set->point_list))[1];
			axis_origin[2]=(*(glyph_set->point_list))[2];
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_get_axis_origin.  Invalid glyph set");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_axis_origin.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_axis_origin */

int Scene_set_axis_origin(struct Scene *scene,Triple axis_origin)
/*******************************************************************************
LAST MODIFIED : 3 August 1998

DESCRIPTION :
Sets the length of the axes - iff graphics are enabled.
==============================================================================*/
{
	int return_code;
	struct GT_glyph_set *glyph_set;

	ENTER(Scene_set_axis_origin);
	if (scene&&axis_origin&&scene->axis_object)
	{
		if ((glyph_set=GT_OBJECT_GET(GT_glyph_set)(scene->axis_object,/*time*/0.0))
			&&glyph_set->point_list)
		{
			(*(glyph_set->point_list))[0]=axis_origin[0];
			(*(glyph_set->point_list))[1]=axis_origin[1];
			(*(glyph_set->point_list))[2]=axis_origin[2];

			GT_object_changed(scene->axis_object);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_set_axis_origin.  Invalid axis object");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_axis_origin.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_axis_origin */

enum GT_visibility_type Scene_get_axis_visibility(struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 21 July 1998

DESCRIPTION :
Returns the visibility of the axes - iff graphics are enabled.
==============================================================================*/
{
	enum GT_visibility_type visibility;
	struct Scene_object *scene_object;

	ENTER(Scene_get_axis_visibility);
	if (scene&&scene->axis_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)scene->axis_object,
			scene->scene_object_list))
		{
			visibility=Scene_object_get_visibility(scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_get_axis_visibility.  Invalid axis object");
			visibility=g_INVISIBLE;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_axis_visibility.  Invalid argument(s)");
		visibility=g_INVISIBLE;
	}
	LEAVE;

	return (visibility);
} /* Scene_get_axis_visibility */

int Scene_set_axis_visibility(struct Scene *scene,
	enum GT_visibility_type visibility)
/*******************************************************************************
LAST MODIFIED : 21 July 1998

DESCRIPTION :
Sets the visibility of the axes - iff graphics are enabled.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_axis_visibility);
	if (scene&&scene->axis_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)scene->axis_object,
			scene->scene_object_list))
		{
			if (visibility != Scene_object_get_visibility(scene_object))
			{
				if (return_code=Scene_object_set_visibility(scene_object,visibility))
				{
					scene->display_list_current=0;
				}
			}
			else
			{
				return_code=1;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_set_axis_visibility.  Invalid axis object");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_axis_visibility.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_axis_visibility */

int Scene_get_graphics_range(struct Scene *scene,
	double *centre_x,double *centre_y,double *centre_z,
	double *size_x,double *size_y,double *size_z)
/*******************************************************************************
LAST MODIFIED : 19 June 2000

DESCRIPTION :
Finds the range of all visible graphics objects in scene. If there is nothing
visible, finds the axis origin. If the scene range is a point, adds the axis
lengths.
==============================================================================*/
{
	double max_x,max_y,max_z,min_x,min_y,min_z;
	int return_code;
	struct Graphics_object_range_struct graphics_object_range;
	Triple axis_lengths,axis_origin;

	ENTER(Scene_get_graphics_range);
	if (scene&&centre_x&&centre_y&&centre_z&&size_x&&size_y&&size_z)
	{
		/* get range of visible graphics_objects in scene */
		graphics_object_range.first=1;
		return_code=for_each_Scene_object_in_Scene(scene,
			Scene_object_get_range,(void *)&graphics_object_range);
		if (graphics_object_range.first)
		{
			/* nothing in the scene: get axis origin as centre */
			if (Scene_get_axis_origin(scene,axis_origin))
			{
				*centre_x = axis_origin[0];
				*centre_y = axis_origin[1];
				*centre_z = axis_origin[2];
				*size_x = *size_y = *size_z =0.0;
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			/* get centre and radius of smallest sphere enclosing visible scene */
			max_x=(double)graphics_object_range.maximum[0];
			max_y=(double)graphics_object_range.maximum[1];
			max_z=(double)graphics_object_range.maximum[2];
			min_x=(double)graphics_object_range.minimum[0];
			min_y=(double)graphics_object_range.minimum[1];
			min_z=(double)graphics_object_range.minimum[2];
			*centre_x = 0.5*(max_x+min_x);
			*centre_y = 0.5*(max_y+min_y);
			*centre_z = 0.5*(max_z+min_z);
			*size_x = max_x-min_x;
			*size_y = max_y-min_y;
			*size_z = max_z-min_z;
		}
		if (return_code)
		{
			if ((0.0==(*size_x)) && (0.0==(*size_y)) && (0.0==(*size_z)))
			{
				/* get size from axis lengths */
				Scene_get_axis_lengths(scene,axis_lengths);
				*size_x = (double)axis_lengths[0];
				*size_y = (double)axis_lengths[0];
				*size_z = (double)axis_lengths[0];
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_graphics_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_graphics_range */

int Scene_get_element_group_position(struct Scene *scene,
	struct GROUP(FE_element) *element_group)
/*******************************************************************************
LAST MODIFIED : 28 February 1998

DESCRIPTION :
The order in which objects are drawn is important for OpenGL transparency.
This function returns the position of <element_group> in <scene>, starting
from 1 at the top. A return value of 0 indicates an error - probably saying
that the GFE for element_group is not in the scene.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_get_element_group_position);
	if (scene&&element_group)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			return_code=Scene_object_get_position(scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_get_element_group_position.  "
				"Element_group not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_element_group_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_element_group_position */

int Scene_set_element_group_position(struct Scene *scene,
	struct GROUP(FE_element) *element_group,int position)
/*******************************************************************************
LAST MODIFIED : 26 July 1999

DESCRIPTION :
The order in which objects are drawn is important for OpenGL transparency.
This function sets the position of <element_group> in <scene>, starting
from 1 at the top. A value less than 1 or greater than the number of graphics
objects in the list puts <element_group> at the end.
Scene_object for the group keeps the same name.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_element_group_position);
	if (scene&&element_group)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			/* take it out of the list and add it at the new position */
			ACCESS(Scene_object)(scene_object);
			return_code=(Scene_remove_scene_object(scene,scene_object)&&
				Scene_add_scene_object(scene,scene_object,position));
			DEACCESS(Scene_object)(&scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_set_element_group_position.  "
				"Graphics object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_element_group_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_element_group_position */

int Scene_get_graphics_object_position(struct Scene *scene,
	struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
The order in which objects are drawn is important for OpenGL transparency.
This function returns the position of <graphics_object> in <scene>, starting
from 1 at the top. A return value of 0 indicates an error - probably saying
that the graphics object is not in the scene.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_get_graphics_object_position);
	if (scene&&graphics_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)graphics_object,
			scene->scene_object_list))
		{
			return_code=Scene_object_get_position(scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_get_graphics_object_position.  "
				"Graphics object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_graphics_object_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_get_graphics_object_position */

int Scene_set_graphics_object_position(struct Scene *scene,
	struct GT_object *graphics_object,int position)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
The order in which objects are drawn is important for OpenGL transparency.
This function sets the position of <graphics_object> in <scene>, starting
from 1 at the top. A value less than 1 or greater than the number of graphics
objects in the list puts <graphics_object> at the end.
==============================================================================*/
{
	char *name;
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_graphics_object_position);
	if (scene&&graphics_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)graphics_object,
			scene->scene_object_list))
		{
			/* take it out of the list and add it at the new position */
			ACCESS(Scene_object)(scene_object);
			GET_NAME(Scene_object)(scene_object, &name);
			return_code=(Scene_remove_graphics_object(scene,graphics_object)&&
				Scene_add_graphics_object(scene,graphics_object,position,name));
			DEACCESS(Scene_object)(&scene_object);
			DEALLOCATE(name);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_set_graphics_object_position.  "
				"Graphics object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_graphics_object_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_graphics_object_position */

int Scene_set_scene_object_position(struct Scene *scene,
	struct Scene_object *scene_object,int position)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
The order in which objects are drawn is important for OpenGL transparency.
This function sets the position of <scene_object> in <scene>, starting
from 1 at the top. A value less than 1 or greater than the number of graphics
objects in the list puts <scene_object> at the end.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_set_scene_object_position);
	if (scene&&scene_object)
	{
		/* take it out of the list and add it at the new position */
		ACCESS(Scene_object)(scene_object);
		return_code=(Scene_remove_scene_object(scene,scene_object)&&
			Scene_add_scene_object(scene,scene_object,position));
		DEACCESS(Scene_object)(&scene_object);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_scene_object_position.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_scene_object_position */

enum GT_visibility_type Scene_get_element_group_visibility(
	struct Scene *scene,struct GROUP(FE_element) *element_group)
/*******************************************************************************
LAST MODIFIED : 16 February 1998

DESCRIPTION :
Returns the visibility of the GFE for <element_group> in <scene>.
==============================================================================*/
{
	enum GT_visibility_type visibility;
	struct Scene_object *scene_object;

	ENTER(Scene_get_element_group_visibility);
	if (scene&&element_group)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			visibility=Scene_object_get_visibility(scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_get_element_group_visibility.  "
				"Group not in scene");
			visibility=g_INVISIBLE;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_element_group_visibility.  Invalid argument(s)");
		visibility=g_INVISIBLE;
	}
	LEAVE;

	return (visibility);
} /* Scene_get_element_group_visibility */

int Scene_set_element_group_visibility(struct Scene *scene,
	struct GROUP(FE_element) *element_group,enum GT_visibility_type visibility)
/*******************************************************************************
LAST MODIFIED : 15 June 1998

DESCRIPTION :
Sets the visibility of the GFE for <element_group> in <scene>.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_element_group_visibility);
	if (scene&&element_group)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			return_code=1;
			if (visibility != Scene_object_get_visibility(scene_object))
			{
				return_code=Scene_object_set_visibility(scene_object,visibility);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_set_element_group_visibility.  "
				"Graphics object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_element_group_visibility.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_element_group_visibility */

enum GT_visibility_type Scene_get_graphics_object_visibility(
	struct Scene *scene,struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Returns the visibility of <graphics_object> in <scene>.
==============================================================================*/
{
	enum GT_visibility_type visibility;
	struct Scene_object *scene_object;

	ENTER(Scene_get_graphics_object_visibility);
	if (scene&&graphics_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)graphics_object,
			scene->scene_object_list))
		{
			visibility=Scene_object_get_visibility(scene_object);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_get_graphics_object_visibility.  "
				"Graphics object not in scene");
			visibility=g_INVISIBLE;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_graphics_object_visibility.  Invalid argument(s)");
		visibility=g_INVISIBLE;
	}
	LEAVE;

	return (visibility);
} /* Scene_get_graphics_object_visibility */

int Scene_set_graphics_object_visibility(struct Scene *scene,
	struct GT_object *graphics_object,enum GT_visibility_type visibility)
/*******************************************************************************
LAST MODIFIED : 15 June 1998

DESCRIPTION :
Sets the visibility of <graphics_object> in <scene>.
==============================================================================*/
{
	int return_code;
	struct Scene_object *scene_object;

	ENTER(Scene_set_graphics_object_visibility);
	if (scene&&graphics_object)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object,(void *)graphics_object,
			scene->scene_object_list))
		{
			return_code=1;
			if (visibility != Scene_object_get_visibility(scene_object))
			{
				return_code=Scene_object_set_visibility(scene_object,visibility);
				/* display list of scene needs rebuilding */
				scene->display_list_current=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_set_graphics_object_visibility.  "
				"Graphics object not in scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_set_graphics_object_visibility.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_set_graphics_object_visibility */

int Scene_has_graphics_object(struct Scene *scene,
	struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 8 December 1997

DESCRIPTION :
Returns true if <graphics object> is in the list of objects on <scene>.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_has_graphics_object);
	/* check arguments */
	if (scene&&graphics_object)
	{
		if (FIRST_OBJECT_IN_LIST_THAT(Scene_object)(Scene_object_has_gt_object,
			(void *)graphics_object,scene->scene_object_list))
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_has_graphics_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_has_graphics_object */

int Scene_has_child_scene(struct Scene *scene,struct Scene *child_scene)
/*******************************************************************************
LAST MODIFIED : 20 November 1998

DESCRIPTION :
Returns true if <child_scene> is in the list of scenes in <scene>.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_has_child_scene);
	if (scene&&child_scene)
	{
		if (FIRST_OBJECT_IN_LIST_THAT(Scene_object)(Scene_object_has_child_scene,
			(void *)child_scene,scene->scene_object_list))
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_has_child_scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_has_child_scene */

struct Scene_object *Scene_get_scene_object_by_name(struct Scene *scene,
	char *name)
/*******************************************************************************
LAST MODIFIED : 5 July 1999

DESCRIPTION :
Looks for a scene object that has the given name.
==============================================================================*/
{
	struct Scene_object *return_scene_object;

	ENTER(Scene_get_scene_object_by_name);
	/* check arguments */
	if (scene && name)
	{
		if (!(return_scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_name,(void *)name,
			scene->scene_object_list)))
		{
		display_message(ERROR_MESSAGE,
			"Scene_get_scene_object_by_name.  Object %s not found in scene", name);
			return_scene_object=(struct Scene_object *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_scene_object_by_name.  Invalid argument(s)");
		return_scene_object=(struct Scene_object *)NULL;
	}
	LEAVE;

	return (return_scene_object);
} /* Scene_get_scene_object_by_name */

int Scene_has_graphical_element_group(struct Scene *scene,
	struct GROUP(FE_element) *element_group)
/*******************************************************************************
LAST MODIFIED : 8 December 1997

DESCRIPTION :
Returns true if <element_group> is in the list of objects on <scene>.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_has_graphical_element_group);
	/* check arguments */
	if (scene&&element_group)
	{
		if (FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_has_graphical_element_group.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_has_graphical_element_group */

struct GT_element_group *Scene_get_graphical_element_group(
	struct Scene *scene,struct GROUP(FE_element) *element_group)
/*******************************************************************************
LAST MODIFIED : 8 December 1997

DESCRIPTION :
Returns the graphical element_group for <element_group> in <scene>.
==============================================================================*/
{
	struct GT_element_group *gt_element_group;
	struct Scene_object *scene_object;

	ENTER(Scene_get_graphical_element_group);
	/* check arguments */
	if (scene&&element_group)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_element_group,(void *)element_group,
			scene->scene_object_list))
		{
			gt_element_group=Scene_object_get_graphical_element_group(scene_object);
		}
		else
		{
			gt_element_group=(struct GT_element_group *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_graphical_element_group.  Invalid argument(s)");
		gt_element_group=(struct GT_element_group *)NULL;
	}
	LEAVE;

	return gt_element_group;
} /* Scene_get_graphical_element_group */

#if defined (OLD_CODE)
int toggle_graphics_object_visibility_in_Scene(
	char *graphics_object_name,struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 19 November 1997

DESCRIPTION :
Toggles visibility of graphics object with <graphics_object_name> on <scene>.
Note that invisible objects are the ones that are selected!
==============================================================================*/
{
	enum GT_visibility_type visibility;
	int return_code;
	struct GT_object *graphics_object;
	struct Scene_object *scene_object;

	ENTER(toggle_graphics_object_visibility_in_Scene);
#if defined (OLD_CODE)
	/* check arguments */
	if (graphics_object_name&&scene)
	{
		if (scene_object=FIRST_OBJECT_IN_LIST_THAT(Scene_object)(
			Scene_object_has_gt_object_name,(void *)graphics_object_name,
			scene->scene_object_list))
		{
			if (graphics_object=Scene_object_get_gt_object(scene_object))
			{
				if ((return_code=Scene_object_get_visibility(scene_object,
					&visibility))&&(g_VISIBLE==visibility))
				{
					Scene_object_set_visibility(scene_object,g_INVISIBLE);
#if defined (MOTIF)
					XmListSelectItem(window->selection_list,graphics_object->list_name,0);
#endif /* defined (MOTIF) */
				}
				else
				{
					Scene_object_set_visibility(scene_object,g_VISIBLE);
#if defined (MOTIF)
					XmListDeselectItem(window->selection_list,graphics_object->list_name);
#endif /* defined (MOTIF) */
				}
				/* update the window */
				Scene_changed(window->scene);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"toggle_graphics_object_visibility_in_Scene.  Missing gt_object");
				return_code=0;
			}
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"toggle_graphics_object_visibility_in_Scene.  Invalid argument(s)");
		return_code=0;
	}
#endif /* defined (OLD_CODE) */
	return_code=1;
	LEAVE;

	return (return_code);
} /* toggle_graphics_object_visibility_in_Scene */
#endif /* defined (OLD_CODE) */

int set_Scene(struct Parse_state *state,
	void *scene_address_void,void *scene_manager_void)
/*******************************************************************************
LAST MODIFIED : 22 June 1999

DESCRIPTION :
Modifier function to set the scene from a command.
???RC set_Object routines could become a macro.
==============================================================================*/
{
	char *current_token;
	int return_code;
	struct Scene *scene,**scene_address;
	struct MANAGER(Scene) *scene_manager;

	ENTER(set_Scene);
	if (state)
	{
		if (current_token=state->current_token)
		{
			if (strcmp(PARSER_HELP_STRING,current_token)&&
				strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
			{
				if ((scene_address=(struct Scene **)scene_address_void)&&
					(scene_manager=(struct MANAGER(Scene) *)scene_manager_void))
				{
					if (fuzzy_string_compare(current_token,"NONE"))
					{
						if (*scene_address)
						{
							DEACCESS(Scene)(scene_address);
							*scene_address=(struct Scene *)NULL;
						}
						return_code=1;
					}
					else
					{
						if (scene=FIND_BY_IDENTIFIER_IN_MANAGER(Scene,name)(current_token,
							scene_manager))
						{
							if (*scene_address != scene)
							{
								ACCESS(Scene)(scene);
								if (*scene_address)
								{
									DEACCESS(Scene)(scene_address);
								}
								*scene_address=scene;
							}
							return_code=1;
						}
						else
						{
							display_message(ERROR_MESSAGE,"Unknown scene : %s",
								current_token);
							return_code=0;
						}
					}
					shift_Parse_state(state,1);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"set_Scene.  Invalid argument(s)");
					return_code=0;
				}
			}
			else
			{
				display_message(INFORMATION_MESSAGE," SCENE_NAME|none");
				if (scene_address=(struct Scene **)scene_address_void)
				{
					if (scene= *scene_address)
					{
						display_message(INFORMATION_MESSAGE,"[%s]",scene->name);
					}
					else
					{
						display_message(INFORMATION_MESSAGE,"[none]");
					}
				}
				return_code=1;
			}
		}
		else
		{
			display_message(WARNING_MESSAGE,"Missing scene name");
			display_parse_state_location(state);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"set_Scene.  Missing state");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_Scene */

int set_Scene_including_sub_objects(struct Parse_state *state,
	void *scene_address_void,void *scene_manager_void)
/*******************************************************************************
LAST MODIFIED : 7 July 1999

DESCRIPTION :
Modifier function to set the scene from a command.  This function understands
the use of a period '.' to delimit sub objects and will automatically create
a Scene that wraps a graphics_object from either the scene or a 
GT_element_settings or a whole GT_element_settings so that export commands can
work on these sub_elements.  These created scenes are not added to the manager.
==============================================================================*/
{
	char *current_token, *index, *next_index, *string_copy;
	int return_code;
	gtMatrix transformation;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *gt_element_settings;
	struct Scene *scene,**scene_address;
	struct Scene_object *new_scene_object, *scene_object;
	struct Time_object *time_object;
	struct MANAGER(Scene) *scene_manager;

	ENTER(set_Scene);
	if (state)
	{
		if (current_token=state->current_token)
		{
			if (strcmp(PARSER_HELP_STRING,current_token)&&
				strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
			{
				if ((scene_address=(struct Scene **)scene_address_void)&&
					(scene_manager=(struct MANAGER(Scene) *)scene_manager_void))
				{
					if (fuzzy_string_compare(current_token,"NONE"))
					{
						if (*scene_address)
						{
							DEACCESS(Scene)(scene_address);
							*scene_address=(struct Scene *)NULL;
						}
						return_code=1;
					}
					else
					{
						if (index = strchr(current_token, '.'))
						{
							if (ALLOCATE(string_copy, char, strlen(current_token) + 1))
							{
								strcpy (string_copy, current_token);
								/* Find top level scene */
								index = strchr(string_copy, '.');
								*index = 0;
								if (scene=FIND_BY_IDENTIFIER_IN_MANAGER(Scene,name)(string_copy,
									scene_manager))
								{
									index++;
									/* Find sub object (could be recursive at some point but not
										currently useful) */
									if (next_index = strchr(index, '.'))
									{
										*next_index = 0;
									}
									if (scene_object = Scene_get_scene_object_by_name(scene, index))
									{
										switch (scene_object->type)
										{
											case SCENE_OBJECT_GRAPHICS_OBJECT:
											{
												/* Use the same name so that output using the name is correct */
												if (scene = CREATE(Scene)(scene->name))
												{
													new_scene_object = create_Scene_object_with_Graphics_object(
														index, Scene_object_get_gt_object(scene_object),
														scene);
													if (Scene_object_has_transformation(scene_object))
													{
														Scene_object_get_transformation(scene_object,
															&transformation);
														Scene_object_set_transformation(new_scene_object,
															&transformation);
													}
													if (Scene_object_has_time(scene_object))
													{
														time_object = Scene_object_get_time_object(scene_object);
														Scene_object_set_time_object(new_scene_object, time_object);
													}
													return_code = Scene_add_scene_object(scene, new_scene_object, 0);
												}
												else
												{
													display_message(ERROR_MESSAGE,"set_Scene_including_sub_objects.  "
														"Unable to create temporary scene");
													return_code=0;
												}
											} break;
											case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
											{
												gt_element_group = Scene_object_get_graphical_element_group(scene_object);
												/* Use the same name so that output using the name is correct */
												if (scene = CREATE(Scene)(scene->name))
												{
													return_code = 1;
													if (next_index)
													{
														index = next_index + 1;
														/* Look for a particular settings */
														if (gt_element_settings = first_settings_in_GT_element_group_that(
															gt_element_group, GT_element_settings_has_name, index))
														{
															/* Create a scene object for the graphics object in settings */
															new_scene_object = create_Scene_object_with_Graphics_object(index,
																GT_element_settings_get_graphics_object(gt_element_settings),
																scene);
														}
														else
														{
															display_message(ERROR_MESSAGE,"Unknown settings %s", index);
															return_code=0;
														}
													}
													else
													{
														/* Create a scene object for the whole graphical element */
														new_scene_object = create_Scene_object_with_Graphical_element_group(
															index, gt_element_group, scene);
													}
													if (return_code)
													{
														if (Scene_object_has_transformation(scene_object))
														{
															Scene_object_get_transformation(scene_object,
																&transformation);
															Scene_object_set_transformation(new_scene_object,
																&transformation);
														}
														if (Scene_object_has_time(scene_object))
														{
															time_object = Scene_object_get_time_object(scene_object);
															Scene_object_set_time_object(new_scene_object, time_object);
														}
														return_code = Scene_add_scene_object(scene, new_scene_object, 0);
													}
												}
												else
												{
													display_message(ERROR_MESSAGE,"set_Scene_including_sub_objects.  "
														"Unable to create temporary scene");
													return_code=0;
												}
											} break;
											case SCENE_OBJECT_SCENE:
											{
												scene = Scene_object_get_child_scene(scene_object);
											} break;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,"Unknown sub_object %s of %s",
											index, string_copy);
										return_code=0;
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,"Unknown parent scene : %s",
										string_copy);
									return_code=0;
								}
								if (return_code)
								{
									REACCESS(Scene)(scene_address, scene);
								}
								DEALLOCATE(string_copy);
							}
							else
							{
								display_message(ERROR_MESSAGE,"set_Scene_including_sub_objects.  "
									"Unable to allocate memory");
								return_code=0;
							}
						}
						else
						{
							if (scene=FIND_BY_IDENTIFIER_IN_MANAGER(Scene,name)(current_token,
								scene_manager))
							{
								if (*scene_address != scene)
								{
									REACCESS(Scene)(scene_address, scene);
								}
								return_code=1;
							}
							else
							{
								display_message(ERROR_MESSAGE,"Unknown scene : %s",
									current_token);
								return_code=0;
							}
						}
					}
					shift_Parse_state(state,1);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"set_Scene_including_sub_objects.  Invalid argument(s)");
					return_code=0;
				}
			}
			else
			{
				display_message(INFORMATION_MESSAGE," SCENE_NAME|none");
				if (scene_address=(struct Scene **)scene_address_void)
				{
					if (scene= *scene_address)
					{
						display_message(INFORMATION_MESSAGE,"[%s]",scene->name);
					}
					else
					{
						display_message(INFORMATION_MESSAGE,"[none]");
					}
				}
				return_code=1;
			}
		}
		else
		{
			display_message(WARNING_MESSAGE,"Missing scene name");
			display_parse_state_location(state);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_Scene_including_sub_objects.  Missing state");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_Scene */

int modify_Scene(struct Parse_state *state,void *scene_void,
	void *modify_scene_data_void)
/*******************************************************************************
LAST MODIFIED : 28 March 2000

DESCRIPTION :
Parser commands for modifying scenes - lighting, etc.
==============================================================================*/
{
	char *graphical_element_mode_string,**valid_strings;
	enum Scene_graphical_element_mode graphical_element_mode,
		old_graphical_element_mode;
	int number_of_valid_strings,return_code,scene_changed;
	char *current_token;
	struct Option_table *option_table;
	struct Scene *scene;
	struct Light *light_to_add,*light_to_remove;
	struct Modify_scene_data *modify_scene_data;

	ENTER(modify_Scene);
	if (state)
	{
		if ((modify_scene_data=(struct Modify_scene_data *)
			modify_scene_data_void)&&(modify_scene_data->default_scene))
		{
			if (current_token=state->current_token)
			{
				if (scene=(struct Scene *)scene_void)
				{
					light_to_add=(struct Light *)NULL;
					light_to_remove=(struct Light *)NULL;
					old_graphical_element_mode=Scene_get_graphical_element_mode(scene);

					option_table=CREATE(Option_table)();
					/* add_light */
					Option_table_add_entry(option_table,"add_light",&light_to_add,
						modify_scene_data->light_manager,set_Light);
					/* graphical_element_mode */ 
					graphical_element_mode_string=Scene_graphical_element_mode_string(
						old_graphical_element_mode);
					valid_strings=Scene_graphical_element_mode_get_valid_strings(
						&number_of_valid_strings);
					Option_table_add_enumerator(option_table,number_of_valid_strings,
						valid_strings,&graphical_element_mode_string);
					DEALLOCATE(valid_strings);
					/* remove_light */
					Option_table_add_entry(option_table,"remove_light",&light_to_remove,
						modify_scene_data->light_manager,set_Light);
					if (return_code=Option_table_multi_parse(option_table,state))
					{
						scene_changed=0;
						if (light_to_add)
						{
							Scene_add_light(scene,light_to_add);
							scene_changed=1;
						}
						graphical_element_mode=Scene_graphical_element_mode_from_string(
							graphical_element_mode_string);
						if (graphical_element_mode != old_graphical_element_mode)
						{
							Scene_set_graphical_element_mode(scene,
								graphical_element_mode,
								modify_scene_data->computed_field_manager,
								modify_scene_data->element_manager,
								modify_scene_data->element_group_manager,
								modify_scene_data->fe_field_manager,
								modify_scene_data->node_manager,
								modify_scene_data->node_group_manager,
								modify_scene_data->data_manager,
								modify_scene_data->data_group_manager,
								modify_scene_data->element_point_ranges_selection,
								modify_scene_data->element_selection,
								modify_scene_data->node_selection,
								modify_scene_data->data_selection,
								modify_scene_data->user_interface);
							scene_changed=1;
						}
						if (light_to_remove)
						{
							Scene_remove_light(scene,light_to_remove);
							scene_changed=1;
						}
						if (scene_changed)
						{
							Scene_changed_private(scene);
						}
					}
					DESTROY(Option_table)(&option_table);
				}
				else
				{
					if (strcmp(PARSER_HELP_STRING,current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
					{
						/* see if current_token is a valid scene name */
						if (scene=FIND_BY_IDENTIFIER_IN_MANAGER(Scene,name)(
							current_token,modify_scene_data->scene_manager))
						{
							return_code=shift_Parse_state(state,1);
						}
						else
						{
							scene=modify_scene_data->default_scene;
							return_code=1;
						}
						if (return_code)
						{
							return_code=modify_Scene(state,(void *)scene,
								modify_scene_data_void);
						}
					}
					else
					{
						option_table=CREATE(Option_table)();
						Option_table_add_entry(option_table,"SCENE_NAME",
							modify_scene_data->default_scene,modify_scene_data_void,
							modify_Scene);
						return_code=Option_table_parse(option_table,state);
						DESTROY(Option_table)(&option_table);
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Missing scene modifications");
				display_parse_state_location(state);
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"modify_Scene.  Missing modify_scene_data");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"modify_Scene.  Missing state");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* modify_Scene */

int list_Scene(struct Scene *scene,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 4 February 2000

DESCRIPTION :
Writes the properties of the <scene> to the command window.
==============================================================================*/
{
	int return_code;

	ENTER(list_Scene);
	USE_PARAMETER(dummy_void);
	if (scene)
	{
		display_message(INFORMATION_MESSAGE,"scene : %s\n",scene->name);
		display_message(INFORMATION_MESSAGE,"  graphical element mode: ");
		display_message(INFORMATION_MESSAGE,
			Scene_graphical_element_mode_string(scene->graphical_element_mode));
		display_message(INFORMATION_MESSAGE,"\n");
		if (0<NUMBER_IN_LIST(Scene_object)(scene->scene_object_list))
		{
			display_message(INFORMATION_MESSAGE,"  objects in scene:\n");
			FOR_EACH_OBJECT_IN_LIST(Scene_object)(list_Scene_object,(void *)NULL,
				scene->scene_object_list);
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"  no objects in scene\n");
		}
		display_message(INFORMATION_MESSAGE,"  access count = %d\n",
			scene->access_count);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Scene */

int gfx_modify_g_element_general(struct Parse_state *state,
	void *element_group_void,void *scene_void)
/*******************************************************************************
LAST MODIFIED : 13 December 1999

DESCRIPTION :
Executes a GFX MODIFY G_ELEMENT GENERAL command.
Allows general element_group settings to be changed (eg. discretization) and
updates graphics of settings affected by the changes (probably all).
==============================================================================*/
{
	int circle_discretization,clear_flag,i,return_code;
	struct Computed_field *default_coordinate_field;
	struct FE_field *native_discretization_field;
	struct GROUP(FE_element) *element_group;
	static struct Modifier_entry option_table[]=
	{
		{"circle_discretization",NULL,NULL,set_Circle_discretization},
		{"clear",NULL,NULL,set_char_flag},
		{"default_coordinate",NULL,NULL,set_Computed_field_conditional},
		{"element_discretization",NULL,NULL,set_Element_discretization},
		{"native_discretization",NULL,NULL,set_FE_field},
		{"scene",NULL,NULL,set_Scene},
		{NULL,NULL,NULL,NULL}
	};
	struct Element_discretization element_discretization;
	struct GT_element_group *gt_element_group;
	struct GT_element_settings *settings;
	struct Scene *scene;
	struct Set_Computed_field_conditional_data set_coordinate_field_data;

	ENTER(gfx_modify_g_element_general);
	if (state)
	{
		/* get default scene */
		if (scene=(struct Scene *)scene_void)
		{
			/* if possible, get defaults from element_group on default scene */
			if ((element_group=(struct GROUP(FE_element) *)element_group_void)&&
				(gt_element_group=Scene_get_graphical_element_group(scene,
					element_group)))
			{
				if (default_coordinate_field=
					GT_element_group_get_default_coordinate_field(gt_element_group))
				{
					ACCESS(Computed_field)(default_coordinate_field);
				}
				circle_discretization=GT_element_group_get_circle_discretization(
					gt_element_group);
				GT_element_group_get_element_discretization(gt_element_group,
					&element_discretization);
				if (native_discretization_field=
					GT_element_group_get_native_discretization_field(gt_element_group))
				{
					ACCESS(FE_field)(native_discretization_field);
				}
			}
			else
			{
				circle_discretization=-1;
				element_discretization.number_in_xi1=-1;
				element_discretization.number_in_xi2=-1;
				element_discretization.number_in_xi3=-1;
				default_coordinate_field=(struct Computed_field *)NULL;
				native_discretization_field=(struct FE_field *)NULL;
			}
			/* ACCESS scene for use by set_Scene */
			clear_flag=0;
			ACCESS(Scene)(scene);
			i=0;
			/* circle_discretization */
			(option_table[i]).to_be_modified= &circle_discretization;
			(option_table[i]).user_data= (void *)(scene->user_interface);
			i++;
			/* clear */
			(option_table[i]).to_be_modified= &clear_flag;
			i++;
			/* default_coordinate */
			set_coordinate_field_data.computed_field_manager=
				scene->computed_field_manager;
			set_coordinate_field_data.conditional_function=
				Computed_field_has_up_to_3_numerical_components;
			set_coordinate_field_data.conditional_function_user_data=(void *)NULL;
			(option_table[i]).to_be_modified= &default_coordinate_field;
			(option_table[i]).user_data= &set_coordinate_field_data;
			i++;
			/* element_discretization */
			(option_table[i]).to_be_modified= &element_discretization;
			(option_table[i]).user_data= (void *)(scene->user_interface);
			i++;
			/* native_discretization */
			(option_table[i]).to_be_modified= &native_discretization_field;
			(option_table[i]).user_data=scene->fe_field_manager;
			i++;
			/* scene */
			(option_table[i]).to_be_modified= &scene;
			(option_table[i]).user_data= (void *)(scene->scene_manager);
			i++;
			if (return_code=process_multiple_options(state,option_table))
			{
				/* scene may have changed so get gt_element_group again */
				if (gt_element_group=Scene_get_graphical_element_group(scene,
					element_group))
				{
					if (clear_flag)
					{
						/* remove all settings from group */
						while (settings=
							first_settings_in_GT_element_group_that(gt_element_group,
								(LIST_CONDITIONAL_FUNCTION(GT_element_settings) *)NULL,
								(void *)NULL))
						{
							GT_element_group_remove_settings(gt_element_group,settings);
						}
					}
					GT_element_group_set_circle_discretization(gt_element_group,
						circle_discretization,scene->user_interface);
					GT_element_group_set_element_discretization(gt_element_group,
						&element_discretization,scene->user_interface);
					GT_element_group_set_default_coordinate_field(gt_element_group,
						default_coordinate_field);
					GT_element_group_set_native_discretization_field(gt_element_group,
						native_discretization_field);
					/* regenerate graphics for changed settings */
					return_code=GT_element_group_build_graphics_objects(gt_element_group,
						(struct FE_element *)NULL,(struct FE_node *)NULL);
					return_code=1;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"gfx_modify_g_element_general.  Missing gt_element_group");
					return_code=0;
				}
			} /* parse error, help */
			if (default_coordinate_field)
			{
				DEACCESS(Computed_field)(&default_coordinate_field);
			}
			if (native_discretization_field)
			{
				DEACCESS(FE_field)(&native_discretization_field);
			}
			if (scene)
			{
				DEACCESS(Scene)(&scene);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"gfx_modify_g_element_general.  Missing scene");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"gfx_modify_g_element_general.  "
			"Missing state");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* gfx_modify_g_element_general */

struct Scene_graphics_object_iterator_data
{
	graphics_object_tree_iterator_function iterator_function;
	void *user_data;
};

static int Scene_graphics_objects_in_GT_element_settings_iterator(
	struct GT_element_settings *settings, void *data_void)
/*******************************************************************************
LAST MODIFIED : 29 July 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct GT_object *graphics_object;
	struct Scene_graphics_object_iterator_data *data;
 
	ENTER(Scene_graphics_objects_in_GT_element_settings_iterator);
	if (settings && (data = (struct Scene_graphics_object_iterator_data *)data_void))
	{
		if (GT_element_settings_get_visibility(settings) &&
			(graphics_object = GT_element_settings_get_graphics_object(
			settings)))
		{
			(data->iterator_function)(graphics_object, 0.0, data->user_data);			
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_graphics_objects_in_GT_element_settings_iterator.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_graphics_objects_in_GT_element_settings_iterator */

static int Scene_graphics_objects_in_Scene_object_iterator(
	struct Scene_object *scene_object, void *data_void)
/*******************************************************************************
LAST MODIFIED : 29 July 1998

DESCRIPTION :
==============================================================================*/
{
	double time;
	int return_code;
	struct GT_object *graphics_object;
	struct Scene_graphics_object_iterator_data *data;
 
	ENTER(Scene_graphics_objects_in_Scene_object_iterator);
	if (scene_object && (data = (struct Scene_graphics_object_iterator_data *)data_void))
	{
		if (g_VISIBLE == Scene_object_get_visibility(scene_object))
		{
			switch(scene_object->type)
			{
				case SCENE_OBJECT_GRAPHICS_OBJECT:
				{
					if(Scene_object_has_time(scene_object))
					{
						time = Scene_object_get_time(scene_object);
					}
					else
					{
						time = 0;
					}
					graphics_object = scene_object->gt_object;
					while ( graphics_object )
					{
						(data->iterator_function)(graphics_object, time, data->user_data);
						graphics_object = graphics_object->nextobject;
					}
				} break;
				case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
				{
					for_each_settings_in_GT_element_group(scene_object->gt_element_group,
						Scene_graphics_objects_in_GT_element_settings_iterator, data_void);
				} break;
				case SCENE_OBJECT_SCENE:
				{
					for_each_Scene_object_in_Scene(scene_object->child_scene,
						Scene_graphics_objects_in_Scene_object_iterator, data_void);
				} break;
			}
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_graphics_objects_in_Scene_object_iterator.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;
	
	return (return_code);
} /* Scene_graphics_objects_in_Scene_object_iterator */

int for_each_graphics_object_in_scene(struct Scene *scene,
	graphics_object_tree_iterator_function iterator_function,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 29 July 1998

DESCRIPTION :
This function iterates through every graphics object in the scene
including those in each individual settings of the graphical finite
elements and those chained together with other graphics objects
==============================================================================*/
{
	int return_code;
	struct Scene_graphics_object_iterator_data data;

	ENTER(for_each_graphics_object_in_scene);

	if (scene && iterator_function && user_data)
	{
		data.iterator_function = iterator_function;
		data.user_data = user_data;
		
		for_each_Scene_object_in_Scene(scene,
			Scene_graphics_objects_in_Scene_object_iterator, (void *)&data);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_graphics_object_in_scene.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* for_each_graphics_object_in_scene */

struct Scene_get_data_range_for_spectrum_data
{
	struct Spectrum *spectrum;
	struct Graphics_object_data_range_struct range;
};

static int Scene_get_data_range_for_spectrum_iterator(
	struct GT_object *graphics_object, double time, void *data_void)
/*******************************************************************************
LAST MODIFIED : 29 July 1998

DESCRIPTION :
Expands the range to include the data values of any of the graphics object
in the <scene> which point to this spectrum.
==============================================================================*/
{
	int return_code;
	struct Scene_get_data_range_for_spectrum_data *data;
	
	ENTER(Scene_get_data_range_for_spectrum_iterator);
	USE_PARAMETER(time);
	if (graphics_object &&
		(data = (struct Scene_get_data_range_for_spectrum_data *)data_void))
	{
		if ( graphics_object->spectrum == data->spectrum )
		{
			get_graphics_object_data_range(graphics_object,
				(void *)&(data->range));
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_data_range_for_spectrum_iterator.  Invalid argument(s)");
		return_code=0;
	}

	LEAVE;

	return (return_code);
} /* Scene_get_data_range_for_spectrum_iterator */

int Scene_get_data_range_for_spectrum(struct Scene *scene,
	struct Spectrum *spectrum, float *minimum, float *maximum,
	int *range_set)
/*******************************************************************************
LAST MODIFIED : 29 July 1998

DESCRIPTION :
Expands the range to include the data values of any of the graphics object
in the <scene> which point to this spectrum.
==============================================================================*/
{
	int return_code;
	struct Scene_get_data_range_for_spectrum_data data;

	ENTER(Scene_get_data_range_for_spectrum);

	if ( scene && spectrum )
	{
		data.spectrum = spectrum;
		data.range.first = 1;
		data.range.minimum = 0;
		data.range.maximum = 0;

		for_each_graphics_object_in_scene(scene,
			Scene_get_data_range_for_spectrum_iterator, (void *)&data);

		if ( data.range.first )
		{
			*range_set = 0;
		}
		else
		{
			*range_set = 1;
			*minimum = data.range.minimum;
			*maximum = data.range.maximum;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_get_data_range_for_spectrum.  Invalid argument(s)");
		return_code=0;
	}

	LEAVE;

	return (return_code);
} /* Scene_get_data_range_for_spectrum */
