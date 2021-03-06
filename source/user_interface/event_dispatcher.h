/*******************************************************************************
FILE : event_dispatcher.h

LAST MODIFIED : 17 January 2006

DESCRIPTION :
Routines for managing the main event loop in cmiss and dispatching events on
registered file descriptors to the correct callbacks.
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (EVENT_DISPATCHER_H)
#define EVENT_DISPATCHER_H

#if defined (BUILD_WITH_CMAKE)
#include "configure/cmgui_configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

#include "general/object.h"
#include "general/list.h"
#if defined (USE_XTAPP_CONTEXT) /* switch (USER_INTERFACE) */
#include <Xm/Xm.h>
#elif defined (WX_USER_INTERFACE) /* switch (USER_INTERFACE) */
#elif defined (WIN32_USER_INTERFACE) /* switch (USER_INTERFACE) */
//#define WINDOWS_LEAN_AND_MEAN
#if !defined (NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#elif defined (CARBON_USER_INTERFACE) /* switch (USER_INTERFACE) */
#elif defined (USE_GTK_MAIN_STEP) /* switch (USER_INTERFACE) */
#elif 1 /* switch (USER_INTERFACE) */
/* This is the default code, it is an event dispatcher designed to run 
	without any particular user interface, I define a preprocess value here to
	make it easy to switch through the code */
#include <general/time.h>
#define USE_GENERIC_EVENT_DISPATCHER
#endif /* switch (USER_INTERFACE) */

#include "user_interface/fd_io.h"

/*
Global types
------------
*/

struct Event_dispatcher;

struct Event_dispatcher_descriptor_callback;

struct Event_dispatcher_timeout_callback;

struct Event_dispatcher_idle_callback;

#if defined (USE_GENERIC_EVENT_DISPATCHER)
struct Event_dispatcher_descriptor_set
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :

==============================================================================*/
{
	fd_set read_set;
	fd_set write_set;
	fd_set error_set;
	int max_timeout_ns;
};
#endif /* defined (USE_GENERIC_EVENT_DISPATCHER) */

#if defined (USE_GENERIC_EVENT_DISPATCHER)
typedef int Event_dispatcher_descriptor_query_function(
	struct Event_dispatcher_descriptor_set *descriptor_set, void *user_data);
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
If <timeout> is set to be non zero then it is included as a maximum sleep time.
==============================================================================*/
#endif /* defined (USE_GENERIC_EVENT_DISPATCHER) */

#if defined (USE_GENERIC_EVENT_DISPATCHER)
typedef int Event_dispatcher_descriptor_check_function(
	struct Event_dispatcher_descriptor_set *descriptor_set,
	void *user_data);
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
==============================================================================*/
#endif /* defined (USE_GENERIC_EVENT_DISPATCHER) */

#if defined (USE_GENERIC_EVENT_DISPATCHER)
typedef int Event_dispatcher_descriptor_dispatch_function(void *user_data);
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
==============================================================================*/
#endif /* defined (USE_GENERIC_EVENT_DISPATCHER) */

typedef int Event_dispatcher_idle_function(void *user_data);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

enum Event_dispatcher_idle_priority
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/
{
	EVENT_DISPATCHER_X_PRIORITY,
	EVENT_DISPATCHER_TRACKING_EDITOR_PRIORITY,
	EVENT_DISPATCHER_IDLE_UPDATE_SCENE_VIEWER_PRIORITY,
	EVENT_DISPATCHER_SYNC_SCENE_VIEWERS_PRIORITY,
	EVENT_DISPATCHER_TUMBLE_SCENE_VIEWER_PRIORITY
};

typedef int Event_dispatcher_timeout_function(void *user_data);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

DECLARE_LIST_TYPES(Fdio);

/*
Global functions
----------------
*/

PROTOTYPE_OBJECT_FUNCTIONS(Fdio);
PROTOTYPE_LIST_FUNCTIONS(Fdio);

#if defined (WX_USER_INTERFACE) && defined (UNIX)  && !defined (DARWIN)
void Event_dispatcher_use_wxCmguiApp_OnAssertFailure(int a);
/*******************************************************************************
LAST MODIFIED : 25 Jul 2007

DESCRIPTION :
==============================================================================*/
#endif /* defined (WX_USER_INTERFACE) */

struct Event_dispatcher *CREATE(Event_dispatcher)(void);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

int DESTROY(Event_dispatcher)(struct Event_dispatcher **event_dispatcher);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
Destroys an Event_dispatcher object
==============================================================================*/

#if defined (USE_GENERIC_EVENT_DISPATCHER)
struct Event_dispatcher_descriptor_callback *Event_dispatcher_add_descriptor_callback(
	struct Event_dispatcher *event_dispatcher,
	Event_dispatcher_descriptor_query_function *query_function,
	Event_dispatcher_descriptor_check_function *check_function,
	Event_dispatcher_descriptor_dispatch_function *dispatch_function,
	void *user_data); 
/*******************************************************************************
LAST MODIFIED : 13 November 2002

DESCRIPTION :
This function is only implemented when using the GENERIC_EVENT_DISPATCHER.
Using this interface to get a read or write callback is deprecated, you should
instead use the fdio API(zinc/fdio.h).
==============================================================================*/
#endif /* defined (USE_GENERIC_EVENT_DISPATCHER) */

int Event_dispatcher_remove_descriptor_callback(
	struct Event_dispatcher *event_dispatcher, 
	struct Event_dispatcher_descriptor_callback *callback_id);
/*******************************************************************************
LAST MODIFIED : 13 November 2002

DESCRIPTION :
Remove the <callback_id> created by Event_dispatcher_add_descriptor_callback
from the <event_dispatcher>.
==============================================================================*/

Fdio_id Event_dispatcher_create_Fdio(struct Event_dispatcher *event_dispatcher,
	cmzn_native_socket_t descriptor);
/*******************************************************************************
LAST MODIFIED : 16 May 2005

DESCRIPTION :
Create a file-descriptor I/O object from an event dispatcher and a descriptor.
Note that cmzn_native_socket_t may be a file-descriptor int if you are on a
*nix platform, or a SOCKET type on Win32.
==============================================================================*/

struct Event_dispatcher_timeout_callback *Event_dispatcher_add_timeout_callback_at_time(
	struct Event_dispatcher *event_dispatcher, unsigned long timeout_s, unsigned long timeout_ns,
	Event_dispatcher_timeout_function *timeout_function, void *user_data);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

struct Event_dispatcher_timeout_callback *Event_dispatcher_add_timeout_callback(
	struct Event_dispatcher *event_dispatcher, unsigned long timeout_s, unsigned long timeout_ns,
	Event_dispatcher_timeout_function *timeout_function, void *user_data);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

int Event_dispatcher_remove_timeout_callback(
	struct Event_dispatcher *event_dispatcher, 
	struct Event_dispatcher_timeout_callback *callback_id);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

struct Event_dispatcher_idle_callback *Event_dispatcher_add_idle_callback(
	struct Event_dispatcher *event_dispatcher, 
	Event_dispatcher_idle_function *idle_function, void *user_data,
	enum Event_dispatcher_idle_priority priority);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

int Event_dispatcher_remove_idle_callback(
	struct Event_dispatcher *event_dispatcher, 
	struct Event_dispatcher_idle_callback *callback_id);
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

struct Event_dispatcher_idle_callback *Event_dispatcher_set_special_idle_callback(
	struct Event_dispatcher *event_dispatcher, 
	Event_dispatcher_idle_function *idle_function, void *user_data,
	enum Event_dispatcher_idle_priority priority);
/*******************************************************************************
LAST MODIFIED : 12 March 2002

DESCRIPTION :
==============================================================================*/

int Event_dispatcher_do_one_event(struct Event_dispatcher *event_dispatcher); 
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

int Event_dispatcher_main_loop(struct Event_dispatcher *event_dispatcher); 
/*******************************************************************************
LAST MODIFIED : 5 March 2002

DESCRIPTION :
==============================================================================*/

int Event_dispatcher_end_main_loop(struct Event_dispatcher *event_dispatcher);
/*******************************************************************************
LAST MODIFIED : 4 March 2002

DESCRIPTION :
==============================================================================*/

#if defined (WX_USER_INTERFACE)
int Event_dispatcher_set_wx_instance(struct Event_dispatcher *event_dispatcher,
	void *user_instance);
#endif

#if defined (USE_XTAPP_CONTEXT)
int Event_dispatcher_set_application_context(struct Event_dispatcher *event_dispatcher,
	XtAppContext application_context);
/*******************************************************************************
LAST MODIFIED : 4 June 2002

DESCRIPTION :
==============================================================================*/
#endif /* defined (USE_XTAPP_CONTEXT) */

int Event_dispatcher_process_idle_event(struct Event_dispatcher *event_dispatcher);

#endif /* !defined (EVENT_DISPATCHER_H) */
