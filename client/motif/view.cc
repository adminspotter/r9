/* view.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Nov 2015, 20:36:39 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2015  Trinity Annabelle Quirk
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
 * Things to do
 *   - Use our fancy cache object, instead of doing one here by hand.
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *   - Make sure the depth buffer is being created and used.
 *   - Define a set of translations and actions, and create routines to
 *     support each one individually.
 *
 */

#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <GL/GLwMDrawA.h>
#include <GL/glut.h>

#include <iostream>

#include "../client_core.h"
#include "client.h"

static void init_callback(Widget, XtPointer, XtPointer);
static void resize_callback(Widget, XtPointer, XtPointer);
static void expose_callback(Widget, XtPointer, XtPointer);
extern void draw_objects(void);

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

void expose_main_view(void)
{
    expose_callback(mainview, NULL, NULL);
}
