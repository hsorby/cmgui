/*******************************************************************************
FILE : graphics_object_highlight.hpp

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
#ifndef GRAPHICS_OBJECT_HIGHLIGHT_HPP
#define GRAPHICS_OBJECT_HIGHLIGHT_HPP

#include "computed_field/computed_field_sub_group_template.hpp"

class GraphicsObjectHighlightBaseFunctor
{
public:
  virtual int call(int identifier)=0;

  virtual ~GraphicsObjectHighlightBaseFunctor()
  {
  };
};

 template <typename ObjectType> class SubGroupHighlightFunctor :
  public GraphicsObjectHighlightBaseFunctor
{
private:
  int(Computed_field_sub_group_object<ObjectType>::*function_pointer)(int);
  Computed_field_sub_group_object<ObjectType> *group;

public:

  SubGroupHighlightFunctor(Computed_field_sub_group_object<ObjectType>* group_in,
      int(Computed_field_sub_group_object<ObjectType>::*function_pointer_in)(int))
  { group = group_in;  function_pointer=function_pointer_in; };

   virtual int call(int identifier)
   {
  	 return (*group.*function_pointer)(identifier);
   };

   ~SubGroupHighlightFunctor()
   {
   };
};

#endif /* GRAPHICS_OBJECT_HIGHLIGHT_HPP */