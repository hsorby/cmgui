/*******************************************************************************
FILE : choose_object_list_private.h

LAST MODIFIED : 20 January 2000

DESCRIPTION :
???RC Version of choose_object using lists instead of managers.
Macros for implementing an option menu dialog control for choosing an object
from a list (subject to an optional conditional function).
Calls the client-specified callback routine if a different object is chosen.
==============================================================================*/
#if !defined (CHOOSE_OBJECT_LIST_PRIVATE_H)
#define CHOOSE_OBJECT_LIST_PRIVATE_H

#include <stdio.h>
#include "general/debug.h"
#include "choose/choose_object_list.h"
#include "choose/chooser.h"
#include "user_interface/message.h"
#include "user_interface/user_interface.h"

/*
Module variables
----------------
*/
#define FULL_DECLARE_CHOOSE_OBJECT_LIST_STRUCT_TYPE( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_STRUCT_TYPE( object_type ) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Contains information required by the choose_object_list control dialog. \
============================================================================*/ \
{ \
	struct LIST(object_type) *object_list; \
	LIST_CONDITIONAL_FUNCTION(object_type) *conditional_function; \
	Widget chooser_widget,parent; \
	struct Callback_data update_callback; \
} /* struct CHOOSE_OBJECT_LIST_STRUCT(object_type) */

/*
Module functions
----------------
*/

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_UPDATE_( object_type ) \
	choose_object_list_update_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_UPDATE_( object_type ) colu ## object_type
#endif
#define CHOOSE_OBJECT_LIST_UPDATE( object_type ) \
	CHOOSE_OBJECT_LIST_UPDATE_(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_UPDATE_FUNCTION( object_type ) \
static int CHOOSE_OBJECT_LIST_UPDATE(object_type)( \
	struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *choose_object_list) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Tells CMGUI about the current values. Sends a pointer to the current object. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHOOSE_OBJECT_LIST_UPDATE(object_type)); \
	if (choose_object_list) \
	{ \
		if (choose_object_list->update_callback.procedure) \
		{ \
			/* now call the procedure with the user data */ \
			(choose_object_list->update_callback.procedure)( \
				(Widget)NULL,choose_object_list->update_callback.data, \
				(struct object_type *) \
				Chooser_get_item(choose_object_list->chooser_widget)); \
		} \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_UPDATE(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_UPDATE(object_type) */

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_DESTROY_CB_( object_type ) \
	choose_object_list_destroy_cb_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_DESTROY_CB_( object_type ) coldc ## object_type
#endif
#define CHOOSE_OBJECT_LIST_DESTROY_CB( object_type ) \
	CHOOSE_OBJECT_LIST_DESTROY_CB_(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_DESTROY_CB_FUNCTION( object_type ) \
static void CHOOSE_OBJECT_LIST_DESTROY_CB(object_type)(Widget widget, \
	void *choose_object_list_void,void *dummy_void) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Callback for the choose_object_list dialog - tidies up all memory allocation. \
============================================================================*/ \
{ \
	struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *choose_object_list; \
\
	ENTER(CHOOSE_OBJECT_LIST_DESTROY_CB(object_type)); \
	USE_PARAMETER(widget); \
	USE_PARAMETER(dummy_void); \
	if (choose_object_list= \
		(struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *)choose_object_list_void) \
	{ \
		DEALLOCATE(choose_object_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_DESTROY_CB(" #object_type ").  Invalid argument"); \
	} \
	LEAVE; \
} /* CHOOSE_OBJECT_LIST_DESTROY_CB(object_type) */

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_UPDATE_CB_( object_type ) \
	choose_object_list_update_cb_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_UPDATE_CB_( object_type ) coluc ## object_type
#endif
#define CHOOSE_OBJECT_LIST_UPDATE_CB( object_type ) \
	CHOOSE_OBJECT_LIST_UPDATE_CB_(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_UPDATE_CB_FUNCTION( object_type ) \
static void CHOOSE_OBJECT_LIST_UPDATE_CB(object_type)(Widget widget, \
	void *choose_object_list_void,void *dummy_void) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Callback for the choose_object_list dialog - tidies up all memory allocation. \
============================================================================*/ \
{ \
	struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *choose_object_list; \
\
	ENTER(CHOOSE_OBJECT_LIST_UPDATE_CB(object_type)); \
	USE_PARAMETER(widget); \
	USE_PARAMETER(dummy_void); \
	if (choose_object_list= \
		(struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *)choose_object_list_void) \
	{ \
		CHOOSE_OBJECT_LIST_UPDATE(object_type)(choose_object_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_UPDATE_CB(" #object_type ").  Invalid argument"); \
	} \
	LEAVE; \
} /* CHOOSE_OBJECT_LIST_UPDATE_CB(object_type) */

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT_( object_type ) \
	choose_object_list_add_to_list_struct_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT_( object_type ) \
	colatls ## object_type
#endif
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT( object_type ) \
	CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT_(object_type)

#define DECLARE_CHOOSE_OBJECT_ADD_TO_LIST_STRUCT_TYPE( object_type ) \
struct CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Data for adding objects to an allocated list. Handles conditional function. \
============================================================================*/ \
{ \
  char **item_names; \
	int number_of_items; \
	LIST_CONDITIONAL_FUNCTION(object_type) *conditional_function; \
  void **items; \
} /* struct CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT(object_type) */

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST_( object_type ) \
	choose_object_add_to_list_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST_( object_type ) coatl ## object_type
#endif
#define CHOOSE_OBJECT_LIST_ADD_TO_LIST( object_type ) \
	CHOOSE_OBJECT_LIST_ADD_TO_LIST_(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_ADD_TO_LIST_FUNCTION( object_type ) \
static int CHOOSE_OBJECT_LIST_ADD_TO_LIST(object_type)( \
	struct object_type *object,void *add_data_void) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Puts the <object> at the array position pointed to by <list_position>. \
============================================================================*/ \
{ \
	int return_code; \
	struct CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT(object_type) *add_data; \
 \
	ENTER(CHOOSE_OBJECT_LIST_ADD_TO_LIST(object_type)); \
	if (object&&(add_data=(struct CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT( \
		object_type) *)add_data_void)&&add_data->items&&add_data->item_names) \
	{ \
		return_code=1; \
		if (!(add_data->conditional_function)|| \
			(add_data->conditional_function)(object,(void *)NULL)) \
		{ \
			if (GET_NAME(object_type)(object,add_data->item_names + \
				add_data->number_of_items)) \
			{ \
				add_data->items[add_data->number_of_items] = (void *)object; \
				(add_data->number_of_items++); \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE,"CHOOSE_OBJECT_LIST_ADD_TO_LIST(" \
					#object_type ").  Could not get name of object"); \
				return_code=0; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"CHOOSE_OBJECT_LIST_ADD_TO_LIST(" \
			#object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
 \
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_ADD_TO_LIST(object_type) */

#if defined (FULL_NAMES)
#define CHOOSE_OBJECT_LIST_GET_ITEMS_( object_type ) \
	choose_object_list_get_items_ ## object_type
#else
#define CHOOSE_OBJECT_LIST_GET_ITEMS_( object_type ) colgi ## object_type
#endif
#define CHOOSE_OBJECT_LIST_GET_ITEMS( object_type ) \
	CHOOSE_OBJECT_LIST_GET_ITEMS_(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_GET_ITEMS_FUNCTION( object_type ) \
static int CHOOSE_OBJECT_LIST_GET_ITEMS(object_type)( \
	struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *choose_object_list, \
	int *number_of_items,void ***items_address,char ***item_names_address) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Allocates and fills an array of all the choosable objects and their names. \
============================================================================*/ \
{ \
	int i,max_number_of_objects,return_code; \
	struct CHOOSE_OBJECT_LIST_ADD_TO_LIST_STRUCT(object_type) add_to_list_data; \
\
	ENTER(CHOOSE_OBJECT_LIST_GET_ITEMS(object_type)); \
	return_code=0; \
	if (choose_object_list&&number_of_items&&items_address&&item_names_address) \
	{ \
		max_number_of_objects= \
			NUMBER_IN_LIST(object_type)(choose_object_list->object_list); \
		add_to_list_data.conditional_function= \
			choose_object_list->conditional_function; \
		add_to_list_data.number_of_items=0; \
		add_to_list_data.items=(void **)NULL; \
		add_to_list_data.item_names=(char **)NULL; \
		if (((0==max_number_of_objects) || \
			ALLOCATE(add_to_list_data.items,void *,max_number_of_objects) && \
			ALLOCATE(add_to_list_data.item_names,char *,max_number_of_objects))&& \
			FOR_EACH_OBJECT_IN_LIST(object_type)( \
				CHOOSE_OBJECT_LIST_ADD_TO_LIST(object_type),(void *)&add_to_list_data, \
				choose_object_list->object_list)) \
		{ \
			*number_of_items = add_to_list_data.number_of_items; \
			*items_address = add_to_list_data.items; \
			*item_names_address = add_to_list_data.item_names; \
			return_code=1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"CHOOSE_OBJECT_LIST_GET_ITEMS(" #object_type ").  Failed"); \
			if (add_to_list_data.items) \
			{ \
				DEALLOCATE(add_to_list_data.items); \
			} \
			if (add_to_list_data.item_names) \
			{  \
				for (i=0;i<add_to_list_data.number_of_items;i++) \
				{ \
					DEALLOCATE(add_to_list_data.item_names[i]); \
				} \
				DEALLOCATE(add_to_list_data.item_names); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_GET_ITEMS(" #object_type ").  Invalid argument(s)"); \
	} \
	LEAVE; \
 \
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_GET_ITEMS(object_type) */

/*
Global functions
----------------
*/

#define DECLARE_CREATE_CHOOSE_OBJECT_LIST_WIDGET_FUNCTION( object_type ) \
PROTOTYPE_CREATE_CHOOSE_OBJECT_LIST_WIDGET_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Creates an option menu from which an object from the list may be chosen. \
============================================================================*/ \
{ \
	char **item_names; \
	int i,number_of_items; \
	struct Callback_data callback; \
	struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *choose_object_list; \
	void **items; \
\
	ENTER(CREATE_CHOOSE_OBJECT_LIST_WIDGET(object_type)); \
	if (object_list&&parent) \
	{ \
		if (ALLOCATE(choose_object_list, \
			struct CHOOSE_OBJECT_LIST_STRUCT(object_type),1)) \
		{ \
			/* initialise the structure */ \
			choose_object_list->chooser_widget=(Widget)NULL; \
			choose_object_list->parent=parent; \
			choose_object_list->update_callback.procedure= \
				(Callback_procedure *)NULL; \
			choose_object_list->update_callback.data=(void *)NULL; \
			choose_object_list->object_list=object_list; \
			choose_object_list->conditional_function=conditional_function; \
		  if (CHOOSE_OBJECT_LIST_GET_ITEMS(object_type)(choose_object_list, \
				&number_of_items,&items,&item_names)) \
			{ \
				if (choose_object_list->chooser_widget= \
					CREATE(Chooser)(parent,number_of_items,items,item_names, \
					(void *)current_object)) \
				{ \
					callback.data=(void *)choose_object_list; \
					callback.procedure=CHOOSE_OBJECT_LIST_UPDATE_CB(object_type); \
					Chooser_set_update_callback(choose_object_list->chooser_widget, \
						&callback); \
					/* get destroy callback from chooser so parent can destroy itself */ \
					callback.procedure=CHOOSE_OBJECT_LIST_DESTROY_CB(object_type); \
					Chooser_set_destroy_callback(choose_object_list->chooser_widget, \
						&callback); \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE, \
						"CREATE_CHOOSE_OBJECT_LIST_WIDGET(" #object_type \
						").  Could not create chooser"); \
					DEALLOCATE(choose_object_list); \
				} \
				if (items) \
				{ \
					DEALLOCATE(items); \
				} \
				if (item_names) \
				{  \
					for (i=0;i<number_of_items;i++) \
					{ \
						DEALLOCATE(item_names[i]); \
					} \
					DEALLOCATE(item_names); \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"CREATE_CHOOSE_OBJECT_LIST_WIDGET(" #object_type \
					").  Could not get items"); \
				DEALLOCATE(choose_object_list); \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"CREATE_CHOOSE_OBJECT_LIST_WIDGET(" #object_type \
				").  Not enough memory"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CREATE_CHOOSE_OBJECT_LIST_WIDGET(" #object_type \
			").  Invalid argument(s)"); \
		choose_object_list=(struct CHOOSE_OBJECT_LIST_STRUCT(object_type) *)NULL; \
	} \
	LEAVE; \
\
	return (choose_object_list); \
} /* CREATE_CHOOSE_OBJECT_LIST_WIDGET(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_GET_CALLBACK_FUNCTION( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_GET_CALLBACK_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Returns a pointer to the callback item of the choose_object_list widget. \
============================================================================*/ \
{ \
	struct Callback_data *callback; \
\
	ENTER(CHOOSE_OBJECT_LIST_GET_CALLBACK(object_type)); \
	if (choose_object_list) \
	{ \
		callback=&(choose_object_list->update_callback); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_GET_CALLBACK(" #object_type ").  Invalid argument"); \
		callback=(struct Callback_data *)NULL; \
	} \
	LEAVE; \
\
	return (callback); \
} /* CHOOSE_OBJECT_LIST_GET_CALLBACK(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_SET_CALLBACK_FUNCTION( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_SET_CALLBACK_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Changes the callback item of the choose_object_list widget. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHOOSE_OBJECT_LIST_SET_CALLBACK(object_type)); \
	if (choose_object_list&&new_callback) \
	{ \
		choose_object_list->update_callback.procedure=new_callback->procedure; \
		choose_object_list->update_callback.data=new_callback->data; \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_SET_CALLBACK(" #object_type \
			").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_SET_CALLBACK(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_GET_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_GET_OBJECT_FUNCTION( object_type ) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Returns the currently chosen object in the choose_object_list widget. \
============================================================================*/ \
{ \
	struct object_type *object; \
\
	ENTER(CHOOSE_OBJECT_LIST_GET_OBJECT(object_type)); \
	if (choose_object_list) \
	{ \
		object=(struct object_type *)Chooser_get_item( \
			choose_object_list->chooser_widget); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_GET_OBJECT(" #object_type ").  Invalid argument"); \
		object=(struct object_type *)NULL; \
	} \
	LEAVE; \
\
	return (object); \
} /* CHOOSE_OBJECT_LIST_GET_OBJECT(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_SET_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_SET_OBJECT_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Changes the chosen object in the choose_object_list widget. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHOOSE_OBJECT_LIST_SET_OBJECT(object_type)); \
	if (choose_object_list) \
	{ \
		return_code= \
			Chooser_set_item(choose_object_list->chooser_widget,(void *)new_object); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_SET_OBJECT(" #object_type \
			").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_SET_OBJECT(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_REFRESH_FUNCTION( object_type ) \
PROTOTYPE_CHOOSE_OBJECT_LIST_REFRESH_FUNCTION( object_type ) \
/***************************************************************************** \
LAST MODIFIED : 20 January 2000 \
\
DESCRIPTION : \
Tells the choose_object_list widget that the list has changed. \
============================================================================*/ \
{ \
	char **item_names; \
	int i,number_of_items,return_code; \
	void **items; \
\
	ENTER(CHOOSE_OBJECT_LIST_REFRESH(object_type)); \
	return_code=0; \
	if (choose_object_list) \
	{ \
		if (CHOOSE_OBJECT_LIST_GET_ITEMS(object_type)(choose_object_list, \
			&number_of_items,&items,&item_names)) \
		{ \
			return_code=Chooser_build_main_menu( choose_object_list->chooser_widget, \
				number_of_items,items,item_names, \
				Chooser_get_item(choose_object_list->chooser_widget)); \
			if (items) \
			{ \
				DEALLOCATE(items); \
			} \
			if (item_names) \
			{  \
				for (i=0;i<number_of_items;i++) \
				{ \
					DEALLOCATE(item_names[i]); \
				} \
				DEALLOCATE(item_names); \
			} \
		} \
		if (!return_code) \
		{ \
			display_message(ERROR_MESSAGE, \
				"CHOOSE_OBJECT_LIST_REFRESH(" #object_type ").  Failed"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHOOSE_OBJECT_LIST_REFRESH(" #object_type ").  Invalid argument"); \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHOOSE_OBJECT_LIST_REFRESH(object_type) */

#define DECLARE_CHOOSE_OBJECT_LIST_MODULE_FUNCTIONS( object_type ) \
DECLARE_CHOOSE_OBJECT_LIST_UPDATE_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_DESTROY_CB_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_UPDATE_CB_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_ADD_TO_LIST_STRUCT_TYPE(object_type); \
DECLARE_CHOOSE_OBJECT_LIST_ADD_TO_LIST_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_GET_ITEMS_FUNCTION(object_type)

#define DECLARE_CHOOSE_OBJECT_LIST_GLOBAL_FUNCTIONS( object_type ) \
DECLARE_CREATE_CHOOSE_OBJECT_LIST_WIDGET_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_SET_CALLBACK_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_SET_OBJECT_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_GET_CALLBACK_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_GET_OBJECT_FUNCTION(object_type) \
DECLARE_CHOOSE_OBJECT_LIST_REFRESH_FUNCTION(object_type)

#endif /* !defined (CHOOSE_OBJECT_LIST_PRIVATE_H) */
