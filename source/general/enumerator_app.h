/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ENUMERATOR_APP_H_
#define ENUMERATOR_APP_H_

#define OPTION_TABLE_ADD_ENUMERATOR( enumerator_type ) \
	Option_table_add_enumerator_ ## enumerator_type

#define PROTOTYPE_OPTION_TABLE_ADD_ENUMERATOR_FUNCTION( enumerator_type ) \
	int OPTION_TABLE_ADD_ENUMERATOR(enumerator_type)(struct Option_table *option_table, \
   enum enumerator_type *enumerator)

#endif /* ENUMERATOR_APP_H_ */
