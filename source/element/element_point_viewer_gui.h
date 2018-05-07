/*******************************************************************************
FILE : element_pointer_viewer_wx.h

LAST MODIFIED : 6 June 2007

DESCRIPTION :
For wxWidgets only, Dialog for selecting an element point, viewing and editing its fields and
applying changes. Works with Element_point_ranges_selection to display the last
selected element point, or set it if entered in this dialog.
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ELEMENT_POINT_VIEWER_H)
#define ELEMENT_POINT_VIEWER_H

#include "general/callback.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "graphics/element_point_ranges.h"
#include "selection/element_point_ranges_selection.h"
#include "time/time.h"

/*
Global Types
------------
*/

struct Element_Point_viewer;
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Contains all the information carried by the element_point_viewer widget.
The contents of this object are private.
==============================================================================*/

/*
Global Functions
----------------
*/

struct Element_point_viewer *CREATE(Element_point_viewer)(
	struct Element_point_viewer **element_point_viewer_address,
	struct cmzn_region *region,
	struct Element_point_ranges_selection *element_point_ranges_selection,
	struct Computed_field_package *computed_field_package,
	struct Time_object *time_object,
	struct User_interface *user_interface);
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
Creates a dialog for choosing element points and displaying and editing their
fields.
==============================================================================*/

int DESTROY(Element_point_viewer)(
	struct Element_point_viewer **element_point_viewer_address);
/*******************************************************************************
LAST MODIFIED : 24 May 2000

DESCRIPTION:
Destroys the Element_point_viewer. See also Element_point_viewer_close_CB.
==============================================================================*/

int Element_point_viewer_bring_window_to_front(
	struct Element_point_viewer *element_point_viewer);
/*******************************************************************************
LAST MODIFIED : 24 May 2000

DESCRIPTION :
Pops the window for <element_point_viewer> to the front of those visible.
==============================================================================*/

#endif /* !defined (ELEMENT_POINT_VIEWER_H) */
