/*******************************************************************************
FILE : chooser.c

LAST MODIFIED : 20 January 2000

DESCRIPTION :
Widget for choosing a void pointer identified with a string out of a
hierarchical cascading menu system.
==============================================================================*/
#include <string.h>
#if defined (MOTIF)
#include <Xm/Xm.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeBG.h>
#include <Xm/RowColumn.h>
#endif /* defined (MOTIF) */
#include "general/debug.h"
#include "general/mystring.h"
#include "choose/chooser.h"
#include "user_interface/message.h"
#include "user_interface/user_interface.h"

#define MAX_CHOOSER_ROWS 16

/*
Module variables
----------------
*/
struct Chooser
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Contains information required by the choose_object control dialog.
==============================================================================*/
{
	struct Callback_data destroy_callback,update_callback;
	Widget main_cascade,main_menu,widget,widget_parent;
	void *current_item,*last_updated_item;
}; /* struct Chooser */

/*
Module functions
----------------
*/

static Widget Chooser_get_widget_from_item(Widget menu,void *current_item)
/*******************************************************************************
LAST MODIFIED : 20 January 2000

DESCRIPTION :
Returns the pushbutton widget for the <current_item> in the menus cascading
from <menu>, or NULL if not found. Note this is recursive.
==============================================================================*/
{
	int i,num_children;
	Widget *child_list,item_widget,submenu;
	XtPointer item;
	WidgetClass widget_class;

	ENTER(Chooser_get_widget_from_item);
	item_widget=(Widget)NULL;
	if (menu)
	{
		XtVaGetValues(menu,XmNnumChildren,&num_children,XmNchildren,&child_list,
			NULL);
		XtVaGetValues(menu,XmNentryClass,&widget_class,NULL);
		for (i=0;(i<num_children)&&(!item_widget);i++)
		{
			if (xmPushButtonGadgetClass==widget_class)
			{
				XtVaGetValues(child_list[i],XmNuserData,&item,NULL);
				if (((void *)item==current_item)||(!current_item))
				{
					item_widget=child_list[i];
				}
			}
			else
			{
				/* get submenu attached to cascade button */
				XtVaGetValues(child_list[i],XmNsubMenuId,&submenu,NULL);
				item_widget=Chooser_get_widget_from_item(submenu,current_item);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_get_widget_from_item.  Invalid argument(s)");
	}
	LEAVE;

	return (item_widget);
} /* Chooser_get_widget_from_item */

static int Chooser_update(struct Chooser *chooser)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Tells CMGUI about the current values. Sends a pointer to the current object.
Avoids sending repeated updates if the object address has not changed.
==============================================================================*/
{
	int return_code;

	ENTER(Chooser_update);
	if (chooser)
	{
		if (chooser->current_item != chooser->last_updated_item)
		{
			if (chooser->update_callback.procedure)
			{
				/* now call the procedure with the user data */
				(chooser->update_callback.procedure)(
					chooser->widget,chooser->update_callback.data,
					chooser->current_item);
			}
			chooser->last_updated_item = chooser->current_item;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_update.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Chooser_update */

static void Chooser_destroy_CB(Widget widget,
	XtPointer client_data,XtPointer reason)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Callback for the chooser dialog - tidies up all memory allocation.
==============================================================================*/
{
	struct Chooser *chooser;

	ENTER(Chooser_destroy_CB);
	USE_PARAMETER(widget);
	USE_PARAMETER(reason);
	if (chooser=(struct Chooser *)client_data)
	{
		/* allow parent to destroy itself */
		if (chooser->destroy_callback.procedure)
		{
			/* now call the procedure with the user data */
			(chooser->destroy_callback.procedure)(
				chooser->widget,chooser->destroy_callback.data,(void *)NULL);
		}
		DEALLOCATE(chooser);
	}
	else
	{
		display_message(ERROR_MESSAGE,"Chooser_destroy_CB.  Missing chooser");
	}
	LEAVE;
} /* Chooser_destroy_CB */

static void Chooser_menu_CB(Widget widget,
	XtPointer client_data,XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Callback for the option menu - change of object.
==============================================================================*/
{
	struct Chooser *chooser;
	Widget item_widget;
	XmString new_string;
	XtPointer item;

	ENTER(Chooser_menu_CB);
	if (widget&&(chooser=(struct Chooser *)client_data))
	{
		/* get the widget from the call data */
		if (item_widget=((XmRowColumnCallbackStruct *)call_data)->widget)
		{
			/* Get the object this menu item represents and make it current */
			XtVaGetValues(item_widget,XmNuserData,&item,NULL);
			chooser->current_item=(void *)item;
			/* display name of current_item on main_cascade */
			XtVaGetValues(item_widget,XmNlabelString,(XtPointer)&new_string,NULL);
			XtVaSetValues(chooser->main_cascade,
				XmNlabelString,(XtPointer)new_string,NULL);
			/* inform the client of the change */
			/* always want an update if menu clicked on, so change history: */
			chooser->last_updated_item=(void *)NULL;
			Chooser_update(chooser);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Chooser_menu_CB.  Could not find the activated menu item");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_menu_CB.  Invalid argument(s)");
	}
	LEAVE;
} /* Chooser_menu_CB */

static Widget Chooser_build_menu(Widget parent,int number_of_items,
	void **items,char **item_names,struct Chooser *chooser)
/*******************************************************************************
LAST MODIFIED : 20 January 2000

DESCRIPTION :
Creates a PullDownMenu widget for choosing any of the <number_of_items>
<items>. If there are more than MAX_CHOOSER_ROWS items then this function
builds menus of cascade buttons with their own submenus.
==============================================================================*/
{
	Arg args[3];
	static char *null_item_name="<none>";
	char *first_item_name,*last_item_name,*item_name,**subitem_names;
	int error,first_length,i,max_submenu_items,items_in_menu,items_in_submenu,
		items_remaining,submenu_levels;
	void *item,**subitems;
	Widget button,menu,submenu;
	XmString new_string;

	ENTER(Chooser_build_menu);
	menu=(Widget)NULL;
	if (parent&&((0==number_of_items)||
		((0<number_of_items)&&items&&item_names))&&chooser)
	{
		if (number_of_items<=MAX_CHOOSER_ROWS)
		{
			/* menu of push buttons representing items */
			if (0==number_of_items)
			{
				items_in_menu=1;
				subitems=(void **)NULL;
				subitem_names=(char **)NULL;
			}
			else
			{
				items_in_menu=number_of_items;
				subitems=items;
				subitem_names=item_names;
			}
			XtSetArg(args[0],XmNentryClass,xmPushButtonGadgetClass);
			if (menu=XmCreatePulldownMenu(parent,"menu",args,1))
			{
				/* get callbacks from all the entries in menu */
				XtAddCallback(menu,XmNentryCallback,
					Chooser_menu_CB,(XtPointer)chooser);
				for (i=0;(i<items_in_menu)&&menu;i++)
				{
					if (subitems)
					{
						item = *subitems;
						item_name = *subitem_names;
					}
					else
					{
						item = (void *)NULL;
						item_name = null_item_name;
					}
					XtSetArg(args[0],XmNuserData,(XtPointer)item);
					new_string=XmStringCreateSimple(item_name);
					XtSetArg(args[1],XmNlabelString,(XtPointer)new_string);
					if (button=XmCreatePushButtonGadget(menu,item_name,args,2))
					{
						if ((void *)NULL == chooser->current_item)
						{
							chooser->current_item = item;
						}
						if (item == chooser->current_item)
						{
							/* display current_item on main_cascade */
							XtVaSetValues(chooser->main_cascade,
								XmNlabelString,(XtPointer)new_string,NULL);
						}
						XtManageChild(button);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Chooser_build_menu.  Could not create push button");
						XtDestroyWidget(menu);
					}
					XmStringFree(new_string);
					subitems++;
					subitem_names++;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Chooser_build_menu.  Could not create menu");
			}
		}
		else
		{
			/* menu of cascade buttons with attached submenus */
			/* get the largest number of items we can put in the submenus */
			submenu_levels=1;
			max_submenu_items=MAX_CHOOSER_ROWS;
			while (max_submenu_items*MAX_CHOOSER_ROWS < number_of_items)
			{
				max_submenu_items *= MAX_CHOOSER_ROWS;
				submenu_levels++;
			}
			/* get the minimum number of submenus for this menu */
			items_in_menu=2;
			while (max_submenu_items*items_in_menu < number_of_items)
			{
				items_in_menu++;
			}
			/* get greatest number of items in each submenu, trying to share as
				 evenly as possible between them */
			items_in_submenu=(number_of_items+items_in_menu-1)/items_in_menu;
			XtSetArg(args[0],XmNentryClass,xmCascadeButtonGadgetClass);
			if (menu=XmCreatePulldownMenu(parent,"menu",args,1))
			{
				subitems=items;
				subitem_names=item_names;
				items_remaining=number_of_items;
				for (i=0;(i<items_in_menu)&&menu;i++)
				{
					if (items_remaining == ((items_in_menu-i)*(items_in_submenu-1)))
					{
						items_in_submenu--;
					}
					item_name= (char *)NULL;
					error=0;
					first_item_name= *subitem_names;
					if (append_string(&item_name,first_item_name,&error))
					{
						if (10<strlen(first_item_name))
						{
							item_name[10]='\0';
						}
						append_string(&item_name," ... ",&error);
						first_length = strlen(item_name);
						last_item_name = *(subitem_names+items_in_submenu-1);
						if (append_string(&item_name,last_item_name,&error))
						{
							if (10<strlen(last_item_name))
							{
								item_name[first_length+10]='\0';
							}
						}
					}
					new_string=XmStringCreateSimple(item_name);
					XtSetArg(args[0],XmNlabelString,(XtPointer)new_string);
					if ((submenu=Chooser_build_menu(menu,
						items_in_submenu,subitems,subitem_names,chooser))&&
						(button=XmCreateCascadeButtonGadget(menu,item_name,args,1)))
					{
						/* attach cascade button to new submenu */
						XtVaSetValues(button,
							XmNsubMenuId,(XtPointer)submenu,NULL);
						XtManageChild(button);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Chooser_build_menu.  Could not create cascade button or menu");
						XtDestroyWidget(menu);
					}
					XmStringFree(new_string);
					DEALLOCATE(item_name);
					subitems += items_in_submenu;
					subitem_names += items_in_submenu;
					items_remaining -= items_in_submenu;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Chooser_build_menu.  Could not create menu");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_build_menu.  Invalid argument(s)");
	}
	LEAVE;

	return (menu);
} /* Chooser_build_menu */

/*
Global functions
----------------
*/
Widget CREATE(Chooser)(Widget parent,int number_of_items,void **items,
	char **item_names,void *current_item)
/*******************************************************************************
LAST MODIFIED : 20 January 2000

DESCRIPTION :
Creates a menu from which any of the given <items> with <item_names> may be
chosen.
==============================================================================*/
{
	Arg args[7];
	int num_children;
	struct Chooser *chooser;
	Widget *child_list,return_widget;
	XmFontList fontlist;

	ENTER(CREATE(Chooser));
	return_widget=(Widget)NULL;
	if (parent&&((0==number_of_items)||
		((0<number_of_items)&&items&&item_names)))
	{
		if (ALLOCATE(chooser,struct Chooser,1))
		{
			/* initialise the structure */
			chooser->destroy_callback.procedure=(Callback_procedure *)NULL;
			chooser->destroy_callback.data=(void *)NULL;
			chooser->update_callback.procedure=(Callback_procedure *)NULL;
			chooser->update_callback.data=(void *)NULL;
			chooser->main_cascade=(Widget)NULL;
			chooser->main_menu=(Widget)NULL;
			chooser->widget=parent;
			chooser->widget_parent=parent;
			chooser->current_item=current_item;
			chooser->last_updated_item=current_item;
			XtSetArg(args[0],XmNleftAttachment,XmATTACH_FORM);
			XtSetArg(args[1],XmNrightAttachment,XmATTACH_FORM);
			XtSetArg(args[2],XmNtopAttachment,XmATTACH_FORM);
			XtSetArg(args[3],XmNbottomAttachment,XmATTACH_FORM);
			XtSetArg(args[4],XmNuserData,(XtPointer)chooser);
			XtSetArg(args[5],XmNmarginHeight,0);
			XtSetArg(args[6],XmNmarginWidth,0);
			if (chooser->widget=XmCreateMenuBar(parent,"chooser",args,7))
			{
				/* add destroy callback for top widget */
				XtAddCallback(chooser->widget,XmNdestroyCallback,
					Chooser_destroy_CB,(XtPointer)chooser);
				if (chooser->main_cascade=
					XmCreateCascadeButtonGadget(chooser->widget,"cascade",NULL,0))
				{
					if (Chooser_build_main_menu(chooser->widget,number_of_items,
						items,item_names,current_item))
					{
						/* tricky: steal font from child cascade buttons for main_cascade */
						XtVaGetValues(chooser->main_menu,
							XmNnumChildren,&num_children,XmNchildren,&child_list,NULL);
						if ((0<num_children)&&child_list)
						{
							XtVaGetValues(child_list[0],
								XmNfontList,(XtPointer)&fontlist,NULL);
							XtVaSetValues(chooser->main_cascade,
								XmNfontList,(XtPointer)fontlist,NULL);
						}
						XtManageChild(chooser->main_cascade);
						XtManageChild(chooser->widget);
						return_widget=chooser->widget;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"CREATE(Chooser).  Could not build menu");
						DEALLOCATE(chooser);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"CREATE(Chooser).  Could not make main cascade button");
					DEALLOCATE(chooser);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CREATE(Chooser)(.  Could not make menu bar");
				DEALLOCATE(chooser);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Chooser).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Chooser).  Invalid argument(s)");
	}
	LEAVE;

	return (return_widget);
} /* CREATE(Chooser) */

int Chooser_build_main_menu(Widget chooser_widget,int number_of_items,
	void **items,char **item_names,void *new_item)
/*******************************************************************************
LAST MODIFIED : 20 January 2000

DESCRIPTION :
Makes a cascading menu of the <items> labelled with the given <item_names>.
Clears existing menu and detaches it from the main_cascade if required.
The new menu is attached to the main_cascade button.
==============================================================================*/
{
	int current_item_found,i,return_code;
	struct Chooser *chooser;
	Widget menu;

	ENTER(Chooser_build_main_menu);
	if (chooser_widget&&((0==number_of_items)||
		((0<number_of_items)&&items&&item_names)))
	{
		/* Get the pointer to the data for the chooser dialog */
		XtVaGetValues(chooser_widget,XmNuserData,&chooser,NULL);
		if (chooser&&chooser->main_cascade)
		{
			/* set last_updated_item to avoid update once set */
			chooser->last_updated_item=new_item;
			chooser->current_item=new_item;
			if (chooser->current_item)
			{
				current_item_found=0;
				for (i=0;(!current_item_found)&&(i<number_of_items);i++)
				{
					current_item_found=(items[i]==chooser->current_item);
				}
				if (!current_item_found)
				{
					chooser->current_item=(void *)NULL;
				}
			}
			if (menu=Chooser_build_menu(chooser->widget,number_of_items,items,
				item_names,chooser))
			{
				/* attach cascade button to new menu */
				XtVaSetValues(chooser->main_cascade,XmNsubMenuId,(XtPointer)menu,NULL);
				/* clear existing menu */
				if (chooser->main_menu)
				{
					XtDestroyWidget(chooser->main_menu);
				}
				chooser->main_menu=menu;
				/* allow update in case current_item was changed */
				Chooser_update(chooser);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Chooser_build_main_menu.  Could not create menu");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Chooser_build_main_menu.  Missing chooser");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_build_main_menu.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Chooser_build_main_menu */

int Chooser_set_destroy_callback(Widget chooser_widget,
	struct Callback_data *new_destroy_callback)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Changes the destroy callback item of the chooser_widget.
==============================================================================*/
{
	int return_code;
	struct Chooser *chooser;

	ENTER(Chooser_set_destroy_callback);
	if (chooser_widget&&new_destroy_callback)
	{
		/* Get the pointer to the data for the chooser dialog */
		XtVaGetValues(chooser_widget,XmNuserData,&chooser,NULL);
		if (chooser)
		{
			chooser->destroy_callback.procedure=new_destroy_callback->procedure;
			chooser->destroy_callback.data=new_destroy_callback->data;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Chooser_set_destroy_callback.  Missing chooser");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_set_destroy_callback.  Missing widget");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Chooser_set_destroy_callback */

int Chooser_set_update_callback(Widget chooser_widget,
	struct Callback_data *new_update_callback)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Changes the update callback item of the chooser_widget.
==============================================================================*/
{
	int return_code;
	struct Chooser *chooser;

	ENTER(Chooser_set_update_callback);
	if (chooser_widget&&new_update_callback)
	{
		/* Get the pointer to the data for the chooser dialog */
		XtVaGetValues(chooser_widget,XmNuserData,&chooser,NULL);
		if (chooser)
		{
			chooser->update_callback.procedure=new_update_callback->procedure;
			chooser->update_callback.data=new_update_callback->data;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Chooser_set_update_callback.  Missing chooser");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Chooser_set_update_callback.  Missing widget");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Chooser_set_update_callback */

void *Chooser_get_item(Widget chooser_widget)
/*******************************************************************************
LAST MODIFIED : 19 January 2000

DESCRIPTION :
Returns the currently chosen item in the chooser_widget.
==============================================================================*/
{
	void *return_address;
	struct Chooser *chooser;

	ENTER(Chooser_get_item);
	if (chooser_widget)
	{
		/* Get the pointer to the data for the chooser dialog */
		XtVaGetValues(chooser_widget,XmNuserData,&chooser,NULL);
		if (chooser)
		{
			return_address=chooser->current_item;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Chooser_get_item.  Missing chooser");
			return_address=(void *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Chooser_get_item.  Missing widget");
		return_address=(void *)NULL;
	}
	LEAVE;

	return (return_address);
} /* Chooser_get_item */

int Chooser_set_item(Widget chooser_widget,void *new_item)
/*******************************************************************************
LAST MODIFIED : 20 January 2000

DESCRIPTION :
Changes the chosen item in the chooser_widget.
==============================================================================*/
{
	int return_code;
	struct Chooser *chooser;
	Widget item_widget;
	XmString new_string;
	XtPointer item;

	ENTER(Chooser_set_item);
	if (chooser_widget)
	{
		/* Get the pointer to the data for the chooser dialog */
		XtVaGetValues(chooser_widget,XmNuserData,&chooser,NULL);
		if (chooser)
		{
			/* set last_updated_item to avoid update once set */
			chooser->last_updated_item=new_item;
			chooser->current_item=new_item;
			/* get pushbutton widget for current_item */
			item_widget=
				Chooser_get_widget_from_item(chooser->main_menu,chooser->current_item);
			if (!item_widget)
			{
				chooser->current_item=(void *)NULL;
				item_widget=Chooser_get_widget_from_item(chooser->main_menu,
					chooser->current_item);
				if (item_widget)
				{
					XtVaGetValues(item_widget,XmNuserData,&item,NULL);
					chooser->current_item=(void *)item;
				}
			}
			if (item_widget)
			{
				/* display name of current_item on main_cascade */
				XtVaGetValues(item_widget,XmNlabelString,(XtPointer)&new_string,NULL);
				XtVaSetValues(chooser->main_cascade,
					XmNlabelString,(XtPointer)new_string,NULL);
			}
			/* allow update in case new_item was not valid */
			Chooser_update(chooser);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Chooser_set_item.  Missing chooser");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Chooser_set_item.  Missing widget");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Chooser_set_item */
