/*******************************************************************************
FILE : computed_field_adjust_contrast.h

LAST MODIFIED : 25 March 2004

DESCRIPTION :
Implements image contrast adjusting on computed fields.
==============================================================================*/
#if !defined (COMPUTED_FIELD_ADJUST_CONTRAST_H)
#define COMPUTED_FIELD_ADJUST_CONTRAST_H

int Computed_field_register_types_adjust_contrast(
	struct Computed_field_package *computed_field_package,
	struct Cmiss_region *root_region, struct User_interface *user_interface);
/*******************************************************************************
LAST MODIFIED : 25 March 2004

DESCRIPTION :
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_ADJUST_CONTRAST_H) */
