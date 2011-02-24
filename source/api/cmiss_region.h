/*******************************************************************************
FILE : cmiss_region.h

LAST MODIFIED : 03 March 2005

DESCRIPTION :
The public interface to the Cmiss_regions.
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
#ifndef __CMISS_REGION_H__
#define __CMISS_REGION_H__

#include "api/cmiss_node.h"
#include "api/cmiss_element.h"
#include "api/cmiss_field.h"
#include "api/cmiss_field_module.h"

/*
Global types
------------
*/

struct Cmiss_region;

#ifndef CMISS_REGION_ID_DEFINED
   typedef struct Cmiss_region * Cmiss_region_id;
   #define CMISS_REGION_ID_DEFINED
#endif /* CMISS_REGION_ID_DEFINED */

/*
Global functions
----------------
*/

/*******************************************************************************
 * Returns a new reference to the region with reference count incremented.
 * Caller is responsible for destroying the new reference.
 * 
 * @param region  The region to obtain a new reference to.
 * @return  New region reference with incremented reference count.
 */
Cmiss_region_id Cmiss_region_access(Cmiss_region_id region);

/*******************************************************************************
 * Destroys this handle to the region, and sets it to NULL.
 * Internally this just decrements the reference count.
 */
int Cmiss_region_destroy(Cmiss_region_id *region);

/***************************************************************************//**
 * Begin caching or increment cache level for this region only. Call this
 * function before making multiple changes to the region or its fields to
 * minimise number of change messages sent to clients.
 * Must call Cmiss_region_end_change after making changes.
 * Important: Do not pair with Cmiss_region_end_hierarchical_change.
 *
 * @param region  The region tree to begin change cache on.
 * @return  1 on success, 0 on failure.
 */
int Cmiss_region_begin_change(Cmiss_region_id region);

/***************************************************************************//**
 * Decrement cache level or end caching of changes for this region only.
 * Call Cmiss_region_begin_change before making multiple field or region changes
 * and call this afterwards. When change level is restored to zero in region,
 * cached change messages are sent out to clients.
 * Important: Do not pair with Cmiss_region_begin_hierarchical_change.
 *
 * @param region  The region tree to end change cache on.
 * @return  1 on success, 0 on failure.
 */
int Cmiss_region_end_change(Cmiss_region_id region);

/***************************************************************************//**
 * Begin caching or increment cache level for all regions in a tree, used to
 * efficiently and safely make hierarchical field changes or modify the tree.
 * Must call Cmiss_region_begin_hierarchical_change after modifications made.
 * Important: Do not pair with non-hierarchical Cmiss_region_end_change.
 *
 * @param region  The root of the region tree to begin change cache on.
 * @return  1 on success, 0 on failure.
 */
int Cmiss_region_begin_hierarchical_change(Cmiss_region_id region);

/***************************************************************************//**
 * Decrement cache level or end caching of changes for all regions in a tree.
 * Call Cmiss_region_begin_hierarchical_change before making hierarchical field
 * changes or modifying the region tree, and call this afterwards. When change
 * level is restored to zero in any region, cached change messages are sent out.
 * Important: Do not pair with non-hierarchical Cmiss_region_begin_change.
 *
 * @param region  The root of the region tree to end change cache on.
 * @return  1 on success, 0 on failure.
 */
int Cmiss_region_end_hierarchical_change(Cmiss_region_id region);

int Cmiss_region_read_file(struct Cmiss_region *region, const char *file_name);
/*******************************************************************************
LAST MODIFIED : 19 August 2002

DESCRIPTION :
==============================================================================*/

/***************************************************************************//**
 * Returns the name of the region. 
 * 
 * @param region  The region whose name is requested.
 * @return  On success: allocated string containing region name.
 */
char *Cmiss_region_get_name(Cmiss_region_id region);

/***************************************************************************//**
 * Sets the name of the region.
 * A valid region name must start with an alphanumeric character, contain only
 * alphanumeric characters, spaces ' ', dots '.', colons ':' or underscores '_',
 * and may not finish with a space.
 * Fails if the new name is already in use by another region in the same parent. 
 * 
 * @param region  The region to be named.
 * @param name  The new name for the region.
 * @return  1 on success, 0 on failure.
 */
int Cmiss_region_set_name(Cmiss_region_id region, const char *name);

/***************************************************************************//**
 * Returns a reference to the parent region of this region.
 * 
 * @param region  The child region.
 * @return  Accessed reference to parent region, or NULL if none.
 */
Cmiss_region_id Cmiss_region_get_parent(Cmiss_region_id region);

/***************************************************************************//**
 * Returns a reference to the first child region of this region.
 * 
 * @param region  The region whose first child is requested.
 * @return  Accessed reference to first child region, or NULL if none.
 */
Cmiss_region_id Cmiss_region_get_first_child(Cmiss_region_id region);

/***************************************************************************//**
 * Returns a reference to this region's next sibling region.
 * 
 * @param region  The region whose next sibling is requested.
 * @return  Accessed reference to next sibling region, or NULL if none.
 */
Cmiss_region_id Cmiss_region_get_next_sibling(Cmiss_region_id region);

/***************************************************************************//**
 * Returns a reference to this region's previous sibling region.
 * 
 * @param region  The region whose previous sibling is requested.
 * @return  Accessed reference to previous sibling region, or NULL if none.
 */
Cmiss_region_id Cmiss_region_get_previous_sibling(Cmiss_region_id region);

/***************************************************************************//**
 * Replaces the region reference with a reference to its next sibling.
 * Convenient for iterating through a child list, equivalent to:
 * {
 *   struct Cmiss_region *temp = Cmiss_region_get_next_sibling(*region_address);
 *   Cmiss_region_destroy(region_address);
 *   *region_address = temp;
 * }
 * 
 * @param region_address  The address of the region reference to replace.
 */
void Cmiss_region_reaccess_next_sibling(Cmiss_region_id *region_address);

/***************************************************************************//**
 * Adds new_child to the end of the list of child regions of this region.
 * If the new_child is already in the region tree, it is first removed. 
 * Fails if new_child contains this region.
 * Fails if new_child is unnamed or the name is already used by another child of
 * this region.
 *
 * @param region  The intended parent region of new_child.
 * @param new_child  The child to add.
 * @return  1 on success, 0 on failure. 
 */
int Cmiss_region_append_child(struct Cmiss_region *region,
	struct Cmiss_region *new_child);

/***************************************************************************//**
 * Inserts new_child before the existing ref_child in the list of child regions
 * of this region. If ref_child is NULL new_child is added at the end of the
 * list. If the new_child is already in the region tree, it is first removed. 
 * Fails if new_child contains this region.
 * Fails if new_child is unnamed or the name is already used by another child of
 * this region.
 *
 * @param region  The intended parent region of new_child.
 * @param new_child  The child to append.
 * @return  1 on success, 0 on failure. 
 */
int Cmiss_region_insert_child_before(Cmiss_region_id region,
	Cmiss_region_id new_child, Cmiss_region_id ref_child);

/***************************************************************************//**
 * Removes old_child from the list of child regions of this region.
 * Fails if old_child is not a child of this region.
 *
 * @param region  The current parent region of old_child.
 * @param old_child  The child to remove.
 * @return  1 on success, 0 on failure. 
 */
int Cmiss_region_remove_child(Cmiss_region_id region,
	Cmiss_region_id old_child);

/***************************************************************************//**
 * Returns true if region is or contains the subregion.
 * 
 * @param region  The region being tested as container.
 * @param subregion  The region being tested for containment.
 * @return  1 if this region is or contains subregion, 0 if not.
 */
int Cmiss_region_contains_subregion(Cmiss_region_id region,
	Cmiss_region_id subregion);

/***************************************************************************//**
 * Returns a reference to the child region with supplied name, if any.
 * 
 * @param region  The region to search.
 * @param name  The name of the child.
 * @return  Accessed reference to the named child, or NULL if no match.
 */
Cmiss_region_id Cmiss_region_find_child_by_name(
	Cmiss_region_id region, const char *name);

/***************************************************************************//**
 * Returns a reference to the subregion at the path relative to this region.
 * The format of the path string is CHILD_NAME/CHILD_NAME/...
 * i.e. forward slash characters '/' are used as parent/child name separators.
 * Single leading and trailing separator characters are ignored.
 * Hence, both name="" and name="/" find the region itself.
 * 
 * @param region  The region to search.
 * @param path  The directory-style path to the subregion.
 * @return  Accessed reference to subregion, or NULL no match.
 */
Cmiss_region_id Cmiss_region_find_subregion_at_path(Cmiss_region_id region,
	const char *path);

/***************************************************************************//**
 * Returns field module container for this region's fields, which must be passed
 * to field factory create methods.
 *
 * @param region  The region from which to obtain the field module.
 * @return  Field module object.
 */
Cmiss_field_module_id Cmiss_region_get_field_module(Cmiss_region_id region);

Cmiss_field_id Cmiss_region_find_field_by_name(Cmiss_region_id region, 
	const char *field_name);
/*******************************************************************************
LAST MODIFIED : 21 April 2008

DESCRIPTION :
Returns the field of <field_name> from the <region> if it is defined,
NULL otherwise.
==============================================================================*/

/***************************************************************************//**
 * Creates and returns a reference to a region compatible with base_region,
 * i.e. able to exist in the same region tree.
 *
 * @see Cmiss_context_create_region
 * @param base_region  An existing region.
 * @return  Accessed reference to the newly created region, or NULL if none.
 */
Cmiss_region_id Cmiss_region_create_region(Cmiss_region_id base_region);

/***************************************************************************//**
 * Create a child region with provided name in parent region.
 * Fails if a child of that name exists already.
 *
 * @see Cmiss_region_set_name
 * @param parent_region  The parent region for the new region.
 * @param name  The name for the newly created region
 * @return  Accessed reference to the new child region, or NULL if failed.
 */
Cmiss_region_id Cmiss_region_create_child(Cmiss_region_id parent_region, 
	const char *name);

/***************************************************************************//**
 * Create a region at the specified relative path, creating any intermediary
 * regions if required.
 * Fails if a subregion exists at that path already.
 * 
 * @param top_region  The region the path is relative to.
 * @param path  Region path, a series of valid region names separated by a
 * forward slash "/". Leading and trailing separator slashes are optional.
 * @return  Accessed reference to the new subregion, or NULL if failed.
 */
Cmiss_region_id Cmiss_region_create_subregion(Cmiss_region_id top_region, 
	const char *path);

/***************************************************************************//**
 * Reads region data from a memory buffer into the specified region if compatible.
 *
 * @param region  The region into which to read the new data.
 * @param memory_buffer  The memory block containing the region data.
 * @param memory_buffer_size  The length of the memory block.
 * @return  1 if data successfully read and merged into specified region, 0 otherwise.
 */
int Cmiss_region_read_from_memory(struct Cmiss_region *region, const void *memory_buffer,
	const unsigned int memory_buffer_size);

#endif /* __CMISS_REGION_H__ */
