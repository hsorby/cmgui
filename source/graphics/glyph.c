/*******************************************************************************
FILE : glyph.c

LAST MODIFIED : 3 August 1999

DESCRIPTION :
Glyphs are GT_objects which contain simple geometric shapes such as
cylinders, arrows and axes which are (or should) fit into a unit (1x1x1) cube,
with the major axes of the glyph aligned with the x, y and z axes. 
The logical centre of each glyph should be at (0,0,0). This should be
interpreted as follows:
- if the glyph is symmetrical along any axis, its coordinates in that
direction should vary from -0.5 to +0.5;
- if the glyph involves any sort of arrow that is unsymmetric in its direction
(ie. is single-headed), (0,0,0) should be at the base of the arrow.
- axes should therefore be centred at (0,0,0) and extend to 1 in each axis
direction. Axis titles "x", "y" and "z" may be outside the unit cube.

Glyphs are referenced by GT_glyph_set objects. Glyphs themselves should not
reference graphical materials or spectrums.
==============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "general/debug.h"
#include "graphics/glyph.h"
#include "graphics/graphics_object.h"
#include "user_interface/message.h"

/*
Module functions
----------------
*/

static int construct_tube(int number_of_segments_around,float x1,float r1,
	float x2,float r2,float cy,float cz,int primary_axis,Triple *vertex_list,
	Triple *normal_list)
/*******************************************************************************
LAST MODIFIED : 17 July 1998

DESCRIPTION :
Adds vertices and normals for a tube/cone/anulus/disc stretching from position
x1 with radius r1 to position x2 with radius r2. The axis of the cylinder is
parallel with the x axis and its centre is at cy,cz. If <primary_axis> is 1, the
above is the case, if its value is 2, x->y, y->z and z->x, and a further
permutation if <primary_axis> is 3. Values other than 1, 2 or 3 are taken as 1.
The vertices and normals are added to create a single quadrilateral strip
suitable for using with GT_surfaces of type g_SH_DISCONTINUOUS_STRIP.
==============================================================================*/
{
	float longitudinal_normal,normal_angle,radial_normal,theta,y,z;
	int j,ix,iy,iz,return_code;
	Triple *normal,*vertex;

	ENTER(construct_tube);
	if ((2<number_of_segments_around)&&((x1 != x2)||(r1 != r2))&&
		vertex_list&&normal_list)
	{
		return_code=1;
		switch (primary_axis)
		{
			case 2:
			{
				ix=1;
				iy=2;
				iz=0;
			} break;
			case 3:
			{
				ix=2;
				iy=0;
				iz=1;
			} break;
			default:
			{
				ix=0;
				iy=1;
				iz=2;
			} break;
		}
		vertex=vertex_list;
		normal=normal_list;
		/* get radial and longitudinal components of surface normals */
		normal_angle=atan2(r2-r1,x2-x1);
		radial_normal=cos(normal_angle);
		longitudinal_normal=-sin(normal_angle);
		for (j=0;j <= number_of_segments_around;j++)
		{
			theta=2.0*PI*(float)j/(float)number_of_segments_around;
			y = sin(theta);
			z = cos(theta);
			(*vertex)[ix] = x1;
			(*vertex)[iy] = cy+r1*y;
			(*vertex)[iz] = cz+r1*z;
			vertex++;
			(*vertex)[ix] = x2;
			(*vertex)[iy] = cy+r2*y;
			(*vertex)[iz] = cz+r2*z;
			vertex++;
			y *= radial_normal;
			z *= radial_normal;
			(*normal)[ix] = longitudinal_normal;
			(*normal)[iy] = y;
			(*normal)[iz] = z;
			normal++;
			(*normal)[ix] = longitudinal_normal;
			(*normal)[iy] = y;
			(*normal)[iz] = z;
			normal++;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"construct_tube.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* construct_tube */

/*
Global functions
----------------
*/

struct GT_object *make_glyph_arrow_line(char *name,float head_length,
	float half_head_width)
/*******************************************************************************
LAST MODIFIED : 3 August 1999

DESCRIPTION :
Creates a graphics object named <name> consisting of a line from <0,0,0> to
<1,0,0> with 4 arrow head ticks <head_length> long and <half_head_width> out
from the shaft.
==============================================================================*/
{
	int j;
	struct GT_object *glyph;
	struct GT_polyline *polyline;
	Triple *points,*vertex;

	ENTER(make_glyph_arrow_line);
	if (name)
	{
		polyline=(struct GT_polyline *)NULL;
		if (ALLOCATE(points,Triple,10))
		{
			vertex=points;
			/* most coordinates are 0.0, so clear them all to that */
			for (j=0;j<10;j++)
			{
				(*vertex)[0]=0.0;
				(*vertex)[1]=0.0;
				(*vertex)[2]=0.0;
				vertex++;
			}
			/* x-axis */
			points[ 1][0]=1.0;
			points[ 2][0]=1.0;
			points[ 3][0]=1.0-head_length;
			points[ 3][1]=half_head_width;
			points[ 4][0]=1.0;
			points[ 5][0]=1.0-head_length;
			points[ 5][2]=half_head_width;
			points[ 6][0]=1.0;
			points[ 7][0]=1.0-head_length;
			points[ 7][1]=-half_head_width;
			points[ 8][0]=1.0;
			points[ 9][0]=1.0-head_length;
			points[ 9][2]=-half_head_width;
			if (!(polyline=CREATE(GT_polyline)(g_PLAIN_DISCONTINUOUS,5,points,
				/*normalpoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
			}
		}
		if (polyline)
		{
			if (glyph=CREATE(GT_object)(name,g_POLYLINE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_polyline)(glyph,/*time*/0.0,polyline))
				{
					DESTROY(GT_object)(&glyph);
				}
			}
			if (!glyph)
			{
				DESTROY(GT_polyline)(&polyline);
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_arrow_line.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_arrow_line.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_arrow_line */

struct GT_object *make_glyph_arrow_solid(char *name,
	int number_of_segments_around,float shaft_length,float shaft_radius)
/*******************************************************************************
LAST MODIFIED : 3 May 1999

DESCRIPTION :
Creates a graphics object named <name> resembling an arrow made from a cone on
a cylinder. The base of the arrow is at (0,0,0) while its head lies at (1,0,0).
The radius of the cone is 0.5 at its base. The cylinder is <shaft_length> long
with its radius given by <shaft_radius>. The ends of the arrow and the cone
are both closed.
==============================================================================*/
{
	float r1,r2,x1,x2;
	int i;
	struct GT_object *glyph;
	struct GT_surface *surface;
	Triple *points, *normalpoints;

	ENTER(make_glyph_arrow_solid);
	if (name&&(2<number_of_segments_around)&&(0<shaft_radius)&&(1>shaft_radius)&&
		(0<shaft_length)&&(1>shaft_length))
	{
		if (glyph=CREATE(GT_object)(name,g_SURFACE,
			(struct Graphical_material *)NULL))
		{
			for (i=0;(i<4)&&glyph;i++)
			{
				if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
					ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
				{
					switch (i)
					{
						case 0:
						{
							/* base of shaft */
							x1=0.0;
							r1=0.0;
							x2=0.0;
							r2=shaft_radius;
						} break;
						case 1:
						{
							/* shaft */
							x1=0.0;
							r1=shaft_radius;
							x2=shaft_length;
							r2=shaft_radius;
						} break;
						case 2:
						{
							/* base of head */
							x1=shaft_length;
							r1=shaft_radius;
							x2=shaft_length;
							r2=0.5;
						} break;
						case 3:
						{
							/* head */
							x1=shaft_length;
							r1=0.5;
							x2=1.0;
							r2=0.0;
						} break;
					}
					if (!construct_tube(number_of_segments_around,x1,r1,x2,r2,0.0,0.0,1,
						points,normalpoints))
					{
						DEALLOCATE(points);
						DEALLOCATE(normalpoints);
					}
				}
				if (points&&(surface=CREATE(GT_surface)(g_SHADED_TEXMAP,
					g_QUADRILATERAL,2,number_of_segments_around+1,points,
					normalpoints,/*texturepoints*/(Triple *)NULL,g_NO_DATA,
					(GTDATA *)NULL)))
				{
					if (!GT_OBJECT_ADD(GT_surface)(glyph,/*time*/0.0,surface))
					{
						DESTROY(GT_surface)(&surface);
						DESTROY(GT_object)(&glyph);
					}
				}
				else
				{
					DEALLOCATE(points);
					DEALLOCATE(normalpoints);
					DESTROY(GT_object)(&glyph);
				}
			}
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"make_glyph_arrow_solid.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"make_glyph_arrow_solid.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_arrow_solid */

struct GT_object *make_glyph_axes(char *name,float head_length,
	float half_head_width,float label_offset)
/*******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Creates a graphics object named <name> consisting of three axis arrows heading
from <0,0,0> to 1 in each of their directions. The arrows are made up of lines,
with a 4-way arrow head so it looks normal from the other two axes. A further
graphics object containing the axis labels 'x', 'y' and 'z' is attached to it so
that the two objects are displayed and destroyed together. The labels are
located on the respective axes <label_offset> past 1.0.
The length and width of the arrow heads are specified by the final parameters.
==============================================================================*/
{
	char *labels_name,**text;
	int j;
	struct GT_object *glyph,*labels;
	struct GT_pointset *pointset;
	struct GT_polyline *polyline;
	Triple *points,*vertex;

	ENTER(make_glyph_axes);
	if (name)
	{
		polyline=(struct GT_polyline *)NULL;
		if (ALLOCATE(points,Triple,30))
		{
			vertex=points;
			/* most coordinates are 0.0, so clear them all to that */
			for (j=0;j<30;j++)
			{
				(*vertex)[0]=0.0;
				(*vertex)[1]=0.0;
				(*vertex)[2]=0.0;
				vertex++;
			}
			/* x-axis */
			points[ 1][0]=1.0;
			points[ 2][0]=1.0;
			points[ 3][0]=1.0-head_length;
			points[ 3][1]=half_head_width;
			points[ 4][0]=1.0;
			points[ 5][0]=1.0-head_length;
			points[ 5][2]=half_head_width;
			points[ 6][0]=1.0;
			points[ 7][0]=1.0-head_length;
			points[ 7][1]=-half_head_width;
			points[ 8][0]=1.0;
			points[ 9][0]=1.0-head_length;
			points[ 9][2]=-half_head_width;
			/* y-axis */
			points[11][1]=1.0;
			points[12][1]=1.0;
			points[13][1]=1.0-head_length;
			points[13][2]=half_head_width;
			points[14][1]=1.0;
			points[15][1]=1.0-head_length;
			points[15][0]=half_head_width;
			points[16][1]=1.0;
			points[17][1]=1.0-head_length;
			points[17][2]=-half_head_width;
			points[18][1]=1.0;
			points[19][1]=1.0-head_length;
			points[19][0]=-half_head_width;
			/* z-axis */
			points[21][2]=1.0;
			points[22][2]=1.0;
			points[23][2]=1.0-head_length;
			points[23][0]=half_head_width;
			points[24][2]=1.0;
			points[25][2]=1.0-head_length;
			points[25][1]=half_head_width;
			points[26][2]=1.0;
			points[27][2]=1.0-head_length;
			points[27][0]=-half_head_width;
			points[28][2]=1.0;
			points[29][2]=1.0-head_length;
			points[29][1]=-half_head_width;
			if (!(polyline=CREATE(GT_polyline)(g_PLAIN_DISCONTINUOUS,15,points,
				/*normalpoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
			}
		}
		pointset=(struct GT_pointset *)NULL;
		if (ALLOCATE(points,Triple,3)&&
			ALLOCATE(text,char *,3)&&
			ALLOCATE(text[0],char,2)&&
			ALLOCATE(text[1],char,2)&&
			ALLOCATE(text[2],char,2)&&
			ALLOCATE(labels_name,char,strlen(name)+8))
		{
			sprintf(labels_name,"%s_labels",name);
			points[0][0]=1.0+label_offset;
			points[0][1]=0.0;
			points[0][2]=0.0;
			strcpy(text[0],"x");
			points[1][0]=0.0;
			points[1][1]=1.0+label_offset;
			points[1][2]=0.0;
			strcpy(text[1],"y");
			points[2][0]=0.0;
			points[2][1]=0.0;
			points[2][2]=1.0+label_offset;
			strcpy(text[2],"z");
			if (!(pointset=CREATE(GT_pointset)(3,points,text,g_NO_MARKER,0.0,
				g_NO_DATA,(GTDATA *)NULL,(int *)NULL)))
			{
				DEALLOCATE(labels_name);
				DEALLOCATE(text[0]);
				DEALLOCATE(text[1]);
				DEALLOCATE(text[2]);
				DEALLOCATE(text);
				DEALLOCATE(points);
			}
		}
		if (polyline&&pointset)
		{
			if (labels=CREATE(GT_object)(labels_name,g_POINTSET,
				(struct Graphical_material *)NULL))
			{
				GT_OBJECT_ADD(GT_pointset)(labels,/*time*/0.0,pointset);
			}
			if (glyph=CREATE(GT_object)(name,g_POLYLINE,
				(struct Graphical_material *)NULL))
			{
				/* must access labels since destroying glyph will deaccess them - and
					 destroy them as necessary */
				glyph->nextobject=ACCESS(GT_object)(labels);
				GT_OBJECT_ADD(GT_polyline)(glyph,/*time*/0.0,polyline);
			}
			DEALLOCATE(labels_name);
		}
		else
		{
			DESTROY(GT_polyline)(&polyline);
			DESTROY(GT_pointset)(&pointset);
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_axes.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_axes.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_axes */

struct GT_object *make_glyph_cone(char *name,int number_of_segments_around)
/*******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Creates a graphics object named <name> resembling a cone with the given
<number_of_segments_around>. The base of the cone is at <0,0,0> while its head
lies at <1,0,0>. The radius of the cone is 0.5 at its base.
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_surface *surface;
	Triple *points,*normalpoints;

	ENTER(make_glyph_cone);
	if (name&&(2<number_of_segments_around))
	{
		surface=(struct GT_surface *)NULL;
		if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
		{
			construct_tube(number_of_segments_around, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 1,
				points,normalpoints);
			if (!(surface=CREATE(GT_surface)(g_SHADED_TEXMAP,g_QUADRILATERAL,2,
				number_of_segments_around+1,points,normalpoints,
				/*texturepoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
		}
		if (surface)
		{
			if (glyph=CREATE(GT_object)(name,g_SURFACE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_surface)(glyph,/*time*/0.0,surface))
				{
					DESTROY(GT_object)(&glyph);
					DESTROY(GT_surface)(&surface);
				}
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_cone.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_cone.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_cone */

struct GT_object *make_glyph_cross(char *name)
/*******************************************************************************
LAST MODIFIED : 16 July 1999

DESCRIPTION :
Creates a graphics object named <name> consisting of a 3 lines:
from <-0.5,0,0> to <+0.5,0,0>
from <0,-0.5,0> to <0,+0.5,0>
from <0,0,-0.5> to <0,0,+0.5>
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_polyline *polyline;
	Triple *points;

	ENTER(make_glyph_cross);
	if (name)
	{
		polyline=(struct GT_polyline *)NULL;
		if (ALLOCATE(points,Triple,6))
		{
			/* x-line */
			points[0][0]=-0.5;
			points[0][1]=0.0;
			points[0][2]=0.0;
			points[1][0]=+0.5;
			points[1][1]=0.0;
			points[1][2]=0.0;
			/* y-line */
			points[2][0]=0.0;
			points[2][1]=-0.5;
			points[2][2]=0.0;
			points[3][0]=0.0;
			points[3][1]=+0.5;
			points[3][2]=0.0;
			/* z-line */
			points[4][0]=0.0;
			points[4][1]=0.0;
			points[4][2]=-0.5;
			points[5][0]=0.0;
			points[5][1]=0.0;
			points[5][2]=+0.5;
			if (!(polyline=CREATE(GT_polyline)(g_PLAIN_DISCONTINUOUS,3,points,
				/*normalpoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
			}
		}
		if (polyline)
		{
			if (glyph=CREATE(GT_object)(name,g_POLYLINE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_polyline)(glyph,/*time*/0.0,polyline))
				{
					DESTROY(GT_object)(&glyph);
				}
			}
			if (!glyph)
			{
				DESTROY(GT_polyline)(&polyline);
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_cross.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_cross.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_cross */

struct GT_object *make_glyph_cylinder(char *name,int number_of_segments_around)
/*******************************************************************************
LAST MODIFIED : 14 July 1999

DESCRIPTION :
Creates a graphics object named <name> resembling a cylinder with the given
<number_of_segments_around>. The cylinder is centred at (0.5,0,0) and its axis
lies in the direction <1,0,0>. It fits into the unit cube spanning from
(0,-0.5,-0.5) to (0,+0.5,+0.5).
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_surface *surface;
	Triple *points,*normalpoints;

	ENTER(make_glyph_cylinder);
	if (name&&(2<number_of_segments_around))
	{
		surface=(struct GT_surface *)NULL;
		if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
		{
			construct_tube(number_of_segments_around,0.0,0.5,1.0,0.5,0.0,0.0,1,
				points,normalpoints);
			if (!(surface=CREATE(GT_surface)(g_SHADED_TEXMAP,g_QUADRILATERAL,2,
				number_of_segments_around+1,points,normalpoints,
				/*texturepoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
		}
		if (surface)
		{
			if (glyph=CREATE(GT_object)(name,g_SURFACE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_surface)(glyph,/*time*/0.0,surface))
				{
					DESTROY(GT_object)(&glyph);
					DESTROY(GT_surface)(&surface);
				}
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"make_glyph_cylinder.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_cylinder.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_cylinder */

struct GT_object *make_glyph_line(char *name)
/*******************************************************************************
LAST MODIFIED : 14 July 1999

DESCRIPTION :
Creates a graphics object named <name> consisting of a line from <0,0,0> to
<1,0,0>.
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_polyline *polyline;
	Triple *points;

	ENTER(make_glyph_line);
	if (name)
	{
		polyline=(struct GT_polyline *)NULL;
		if (ALLOCATE(points,Triple,2))
		{
			points[0][0]=0.0;
			points[0][1]=0.0;
			points[0][2]=0.0;
			points[1][0]=1.0;
			points[1][1]=0.0;
			points[1][2]=0.0;
			if (!(polyline=CREATE(GT_polyline)(g_PLAIN_DISCONTINUOUS,1,points,
				/*normalpoints*/(Triple *)NULL,g_NO_DATA,(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
			}
		}
		if (polyline)
		{
			if (glyph=CREATE(GT_object)(name,g_POLYLINE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_polyline)(glyph,/*time*/0.0,polyline))
				{
					DESTROY(GT_object)(&glyph);
				}
			}
			if (!glyph)
			{
				DESTROY(GT_polyline)(&polyline);
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_line.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_line.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_line */

struct GT_object *make_glyph_point(char *name,gtMarkerType marker_type,
	float marker_size)
/*******************************************************************************
LAST MODIFIED : 1 December 1998

DESCRIPTION :
Creates a graphics object named <name> consisting of a single point at <0,0,0>.
The point will be drawn with the given <marker_type> and <marker_size>.
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_pointset *pointset;
	Triple *points;

	ENTER(make_glyph_point);
	if (name)
	{
		pointset=(struct GT_pointset *)NULL;
		if (ALLOCATE(points,Triple,1))
		{
			(*points)[0]=0.0;
			(*points)[1]=0.0;
			(*points)[2]=0.0;
			if (!(pointset=CREATE(GT_pointset)(1,points,(char **)NULL,marker_type,
				marker_size,g_NO_DATA,(GTDATA *)NULL,(int *)NULL)))
			{
				DEALLOCATE(points);
			}
		}
		if (pointset)
		{
			if (glyph=CREATE(GT_object)(name,g_POINTSET,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_pointset)(glyph,/*time*/0.0,pointset))
				{
					DESTROY(GT_object)(&glyph);
				}
			}
			if (!glyph)
			{
				DESTROY(GT_pointset)(&pointset);
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"make_glyph_point.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_point.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_point */

struct GT_object *make_glyph_sheet(char *name)
/*******************************************************************************
LAST MODIFIED : 5 May 1999

DESCRIPTION :
Creates a graphics object named <name> resembling a square sheet spanning from
coordinate <-0.5,-0.5,0> to <0.5,0.5,0>.
==============================================================================*/
{
	struct GT_object *glyph;
	struct GT_surface *surface;
	Triple *point,*points,*normalpoints;

	ENTER(make_glyph_sheet);
	if (name)
	{
		surface=(struct GT_surface *)NULL;
		if (ALLOCATE(points,Triple,4)&&
			ALLOCATE(normalpoints,Triple,4))
		{
			point = points;
			/* vertices */
			(*point)[0] = -0.5;
			(*point)[1] = -0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = 0.5;
			(*point)[1] = -0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = 0.5;
			(*point)[1] = 0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = -0.5;
			(*point)[1] = 0.5;
			(*point)[2] = 0.0;
			/* normals */
			point = normalpoints;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			if (!(surface=CREATE(GT_surface)(g_SH_DISCONTINUOUS,g_QUADRILATERAL,1,
				4,points,normalpoints,/*texturepoints*/(Triple *)NULL,g_NO_DATA,
				(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
		}
		if (surface)
		{
			if (glyph=CREATE(GT_object)(name,g_SURFACE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_surface)(glyph,/*time*/0.0,surface))
				{
					DESTROY(GT_object)(&glyph);
					DESTROY(GT_surface)(&surface);
				}
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"make_glyph_sheet.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_sheet.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_sheet */

struct GT_object *make_glyph_sphere(char *name,int number_of_segments_around,
	int number_of_segments_down)
/*******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Creates a graphics object named <name> resembling a sphere with the given
<number_of_segments_around> and <number_of_segments_down> from pole to pole.
The sphere is centred at (0,0,0) and its poles are on the (1,0,0) line. It fits
into the unit cube spanning from -0.5 to +0.5 across all axes. Parameter
<number_of_segments_around> should normally be an even number at least 6 and
twice <number_of_segments_down> look remotely spherical.
==============================================================================*/
{
	float longitudinal_normal,phi,radial_normal,theta,x,y,z;
	int i,j;
	struct GT_object *glyph;
	struct GT_surface *surface;
	Triple *normal,*normalpoints,*points,*vertex;

	ENTER(make_glyph_sphere);
	if (name&&(2<number_of_segments_around)&&(1<number_of_segments_down))
	{
		surface=(struct GT_surface *)NULL;
		if (ALLOCATE(points,Triple,
			(number_of_segments_down+1)*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,(number_of_segments_down+1)*
				(number_of_segments_around+1)))
		{
			/*vertex=points;
				normal=points+(number_of_segments_down+1)*(number_of_segments_around+1);*/
			for (i=0;i <= number_of_segments_down;i++)
			{
				phi=PI*(float)i/(float)number_of_segments_down;
				x=-0.5*cos(phi);
				radial_normal=sin(phi);
				longitudinal_normal=2*x;
				/*printf("x=%g l=%g r=%g\n",x,longitudinal_normal,radial_normal);*/
				vertex=points+i;
				normal=normalpoints+i;
				for (j=0;j <= number_of_segments_around;j++)
				{
					theta=2.0*PI*(float)j/(float)number_of_segments_around;
					y = radial_normal*sin(theta);
					z = radial_normal*cos(theta);
					(*vertex)[0] = x;
					(*vertex)[1] = 0.5*y;
					(*vertex)[2] = 0.5*z;
					vertex += (number_of_segments_down+1);
					(*normal)[0] = longitudinal_normal;
					(*normal)[1] = y;
					(*normal)[2] = z;
					normal += (number_of_segments_down+1);
				}
			}
			if (!(surface=CREATE(GT_surface)(g_SHADED_TEXMAP,g_QUADRILATERAL,
				number_of_segments_down+1,number_of_segments_around+1,
				points,normalpoints,/*texturepoints*/(Triple *)NULL,g_NO_DATA,
				(GTDATA *)NULL)))
			{
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
		}
		if (surface)
		{
			if (glyph=CREATE(GT_object)(name,g_SURFACE,
				(struct Graphical_material *)NULL))
			{
				if (!GT_OBJECT_ADD(GT_surface)(glyph,/*time*/0.0,surface))
				{
					DESTROY(GT_object)(&glyph);
					DESTROY(GT_surface)(&surface);
				}
			}
		}
		else
		{
			glyph=(struct GT_object *)NULL;
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"make_glyph_sphere.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_glyph_sphere.  Invalid argument(s)");
		glyph=(struct GT_object *)NULL;
	}
	LEAVE;

	return (glyph);
} /* make_glyph_sphere */
