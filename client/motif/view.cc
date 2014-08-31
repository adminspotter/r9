/* view.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 Aug 2014, 16:14:24 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2014  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains the code to generate and manage the main view window
 * for the Revision 9 client program.
 *
 * Changes
 *   18 Jul 2006 TAQ - Created the file.
 *   20 Jul 2006 TAQ - Changed to be a GLwMDrawingArea, which is a GL drawing
 *                     surface.
 *   21 Jul 2006 TAQ - We now draw some stuff in this widget, and double-
 *                     buffering working.  Started working on lighting and
 *                     shading, but it's not doing anything.
 *   24 Jul 2006 TAQ - Still trying to get the lighting/shading to work.
 *   25 Jul 2006 TAQ - Finally got the lighting/shading to work - it appears
 *                     that freeing the XVisualInfo that we got out of the
 *                     widget was actually freeing some internal state of
 *                     the widget, and it then couldn't draw anything
 *                     properly.  I also moved the only call to
 *                     GLwDrawingAreaMakeCurrent to occur right after the call
 *                     to glXCreateContext.  We also no longer have the context
 *                     in a file-global variable; it goes out of scope at the
 *                     end of the init callback, and we never look back.
 *   26 Jul 2006 TAQ - Renamed some stuff.  Added lists of objects (spheres)
 *                     and textures (colors) and functions to manage them.
 *   29 Jul 2006 TAQ - Added frame number to the object struct.
 *   03 Aug 2006 TAQ - Moved geometry management into geometry.c, and texture
 *                     management into texture.c.
 *   10 Aug 2006 TAQ - Added object hash table in here, since object and
 *                     geometry are NOT the same thing.  Added cleanup thread.
 *                     Fleshed out move_object routine.  Fixed draw_objects
 *                     routine, but it's currently horrifically brute-force
 *                     (i.e. we draw EVERYTHING in the hash).
 *   30 Aug 2014 TAQ - Started removing the hash table, since we have a very
 *                     nice new cache object that does that for us.
 *
 * Things to do
 *   - Use our fancy cache object, instead of doing one here by hand.
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *   - Make sure the depth buffer is being created and used.
 *   - Define a set of translations and actions, and create routines to
 *     support each one individually.
 *
 */

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <GL/GLwMDrawA.h>
#include <GL/glut.h>

#include <iostream>

#include "client.h"
#include "../cache.h"

struct object
{
    u_int64_t object_id;
    u_int64_t geometry_id;
    u_int16_t frame_number;
    GLdouble position[3], orientation[3];
};

static void init_callback(Widget, XtPointer, XtPointer);
static void resize_callback(Widget, XtPointer, XtPointer);
static void expose_callback(Widget, XtPointer, XtPointer);
static void draw_objects(void);

static object **objects;
static Widget mainview;
static GLfloat light_position[] = { 5.0, 50.0, -10.0, 1.0 };

Widget create_main_view(Widget parent)
{
    mainview = XtVaCreateManagedWidget("mainview",
                                       glwMDrawingAreaWidgetClass,
                                       parent,
                                       GLwNrgba, True,
                                       GLwNdoublebuffer, True,
                                       GLwNdepthSize, 1,
                                       NULL);
    XtAddCallback(mainview, GLwNginitCallback, init_callback, NULL);
    XtAddCallback(mainview, GLwNresizeCallback, resize_callback, NULL);
    XtAddCallback(mainview, GLwNexposeCallback, expose_callback, NULL);

    /* Set up the object cache */
    /* Start up the cleanup thread */

    return mainview;
}

/* ARGSUSED */
void init_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XVisualInfo *visual_info;
    GLXContext context;
    GLfloat white_light[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat global_ambient[] = { 0.1, 0.1, 0.1, 1.0 };

    /* Create and set the GLXContext */
    XtVaGetValues(w, GLwNvisualInfo, &visual_info, NULL);
    context = glXCreateContext(XtDisplay(w), visual_info, NULL, True);
    GLwDrawingAreaMakeCurrent(w, context);

    /* Depth parameters */
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Lighting model parameters */
    glEnable(GL_LIGHTING);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

    /* Light 0 parameters */
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightfv(GL_LIGHT0, GL_AMBIENT, global_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHT0);

    /* We have to set the viewport and projection matrix before anything
     * will show up on the screen.  The resize function does that.
     */
    resize_callback(w, client_data, call_data);
}

/* ARGSUSED */
void resize_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    GLwDrawingAreaCallbackStruct *cbstruct =
        (GLwDrawingAreaCallbackStruct *)call_data;

    glViewport(0, 0, (GLsizei)cbstruct->width, (GLsizei)cbstruct->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75.0,
                   (GLdouble)cbstruct->width / (GLdouble)cbstruct->height,
                   0.1,
                   100.0);
    glMatrixMode(GL_MODELVIEW);

    /* After we resize, we automatically get exposed, so we don't need to
     * clear the window or explicitly call expose or even do a flush.
     */
}

/* ARGSUSED */
void expose_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Set the view point */
    glLoadIdentity();
    glRotatef(180, 0.0, 0.0, 1.0);
    /* Draw what we're supposed to see */
    draw_objects();
    GLwDrawingAreaSwapBuffers(w);
    glFlush();
}

/*static void draw_objects(void)
{
    object *optr;*/

    /* Brute force, baby - ya just can't beat it */
    /*for (i = 0; i < OBJECT_HASH_BUCKETS; ++i)
    {
        optr = objects[i];
        while (optr != NULL)
        {
            glPushMatrix();
            glTranslated(optr->position[0],
                         optr->position[1],
                         optr->position[2]);
            glRotated(optr->orientation[0], 1.0, 0.0, 0.0);
            glRotated(optr->orientation[1], 0.0, 1.0, 0.0);
            glRotated(optr->orientation[2], 0.0, 0.0, 1.0);
            draw_geometry(optr->geometry_id, optr->frame_number);
            glPopMatrix();
        }
    }
}*/

/*void move_object(u_int64_t objectid, u_int16_t frame,
                 Eigen::Vector3d& newpos,
                 Eigen::Vector3d& neworient)
{
    object *optr = ocache[objectid];*/

    /* Update the object's position */
    /*optr->frame_number = frame;
    optr->position[0] = newpos[0];
    optr->position[1] = newpos[1];
    optr->position[2] = newpos[2];
    optr->orientation[0] = neworient[0];
    optr->orientation[1] = neworient[1];
    optr->orientation[2] = neworient[2];*/

    /* Post an expose for the screen */
    /*expose_callback(mainview, NULL, NULL);
}*/
