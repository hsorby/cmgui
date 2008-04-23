/*******************************************************************************
FILE : computed_field_conditional.h

LAST MODIFIED : 27 July 2007

DESCRIPTION :
Implements computed fields which conditionally calculate their inputs.
==============================================================================*/
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
#if !defined (COMPUTED_FIELD_CONDITIONAL_H)
#define COMPUTED_FIELD_CONDITIONAL_H

#include "general/value.h"
#include "api/cmiss_computed_field.h"

#define Computed_field_set_type_if Cmiss_computed_field_set_type_if

int Computed_field_set_type_if(struct Computed_field *field,
	struct Computed_field *source_field_one,
	struct Computed_field *source_field_two,
	struct Computed_field *source_field_three);
/*******************************************************************************
LAST MODIFIED : 27 July 2007

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_IF with the supplied
fields, <source_field_one>, <source_field_two> and <source_field_three>.
Sets the number of components equal to the source_fields.
For each component, if the value of source_field_one is TRUE (non-zero) then
the result will be the value of source_field_two, otherwise the result will
be source_field_three.
==============================================================================*/

int Computed_field_register_types_conditional(
	struct Computed_field_package *computed_field_package);
/*******************************************************************************
LAST MODIFIED : 27 July 2007

DESCRIPTION :
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_CONDITIONAL_H) */
