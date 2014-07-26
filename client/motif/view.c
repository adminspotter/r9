/* view.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 10 Aug 2006, 14:27:32 trinity
 *
 * Revision IX game client
 * Copyright (C) 2004  Trinity Annabelle Quirk
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
 *
 * Things to do
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *   - Make sure the depth buffer is being created and used.
 *   - Define a set of translations and actions, and create routines to
 *   support each one individually.
 *
 * $Id: view.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <GL/GLwMDrawA.h>
#include <GL/glut.h>

#include "client.h"

#ifndef OBJECT_HASH_BUCKETS
#define OBJECT_HASH_BUCKETS  91
#endif

typedef struct object_tag
{
    u_int64_t object_id;
    struct timeval lastused;
    u_int64_t geometry_id;
    u_int16_t frame_number;
    GLdouble position[3], orientation[3];
    struct object_tag *next;
}
object;

static void init_callback(Widget, XtPointer, XtPointer);
static void resize_callback(Widget, XtPointer, XtPointer);
static void expose_callback(Widget, XtPointer, XtPointer);
static void draw_objects(void);
static int hash_func(u_int64_t);
static void insert_object(u_int64_t, object *);
static object *find_object(u_int64_t);
static void *object_prune_worker(void *);

static object **objects;
static Widget mainview;
static GLfloat light_position[] = { 5.0, 50.0, -10.0, 1.0 };
static pthread_t prune_thread;

Widget create_main_view(Widget parent)
{
    char errstr[256];
    int ret;

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

    /* Set up the hash table */
    if ((objects = (object **)malloc(sizeof(object *)
                                     * OBJECT_HASH_BUCKETS)) == NULL)
    {
        snprintf(errstr, sizeof(errstr),
                 "Couldn't allocate object hash table");
        main_post_message(errstr);
    }
    else
        memset(objects, 0, sizeof(object *) * OBJECT_HASH_BUCKETS);

    /* Start up the cleanup thread */
    if ((ret = pthread_create(&prune_thread, NULL, object_prune_worker, NULL))
        != 0)
    {
        snprintf(errstr, sizeof(errstr),
                 "Couldn't start texture cleanup thread: %s", strerror(ret));
        main_post_message(errstr);
    }

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

static void draw_objects(void)
{
    int i;
    object *optr;

    /* Brute force, baby - ya just can't beat it */
    for (i = 0; i < OBJECT_HASH_BUCKETS; ++i)
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
            optr = optr->next;
        }
    }
}

void move_object(u_int64_t objectid, u_int16_t frame,
                 double newx, double newy, double newz,
                 double newox, double newoy, double newoz)
{
    object *optr = find_object(objectid);

    if (optr == NULL)
    {
        /* We're getting notifications for a new object; add it to the hash */
        if ((optr = (object *)malloc(sizeof(object))) == NULL)
            return;
        optr->object_id = objectid;
        gettimeofday(&optr->lastused, NULL);
        load_geometry(0, 0);
        optr->geometry_id = 0;
        optr->frame_number = frame;
        optr->next = NULL;
        insert_object(objectid, optr);
    }

    /* Update the object's position */
    optr->frame_number = frame;
    optr->position[0] = newx;
    optr->position[1] = newy;
    optr->position[2] = newz;
    optr->orientation[0] = newox;
    optr->orientation[1] = newoy;
    optr->orientation[2] = newoz;

    /* Post an expose for the screen */
    expose_callback(mainview, NULL, NULL);
}

static int hash_func(u_int64_t objectid)
{
    return (objectid % OBJECT_HASH_BUCKETS);
}

static void insert_object(u_int64_t objectid, object *o)
{
    int bucket = hash_func(objectid);
    object *optr = objects[bucket];

    if (objects[bucket] == NULL)
    {
        objects[bucket] = o;
        o->next = NULL;
    }
    else
    {
        /* Hashing collision; chain the entries */
        while (optr->next != NULL)
            optr = optr->next;
        optr->next = o;
        o->next = NULL;
    }
}

static object *find_object(u_int64_t objectid)
{
    object *optr = objects[hash_func(objectid)];

    while (optr != NULL && optr->object_id != objectid)
        optr = optr->next;
    if (optr != NULL)
        gettimeofday(&optr->lastused, NULL);
    return optr;
}

/* This routine will clean out any objects from the hash that haven't been
 * used in 10 minutes.  We should eventually make that into a configurable
 * option, because it's the right thing to do.
 */
/* ARGSUSED */
static void *object_prune_worker(void *notused)
{
    struct timeval tv;
    char errstr[256];
    int i, count;
    object *optr, *prev;

    for (;;)
    {
        sleep(60);
        if (!gettimeofday(&tv, NULL))
        {
            snprintf(errstr, sizeof(errstr),
                     "Object thread couldn't get time of day: %s",
                     strerror(errno));
            main_post_message(errstr);
            continue;
        }
        count = 0;
        for (i = 0; i < OBJECT_HASH_BUCKETS; ++i)
        {
            optr = prev = objects[i];
            while (optr != NULL)
            {
                if (optr->lastused.tv_sec + 600 <= tv.tv_sec)
                {
                    if (optr == prev)
                    {
                        /* The head of this bucket is getting the boot */
                        objects[i] = optr->next;
                        free(optr);
                        optr = prev = objects[i];
                    }
                    else
                    {
                        /* The victim is in the middle of the chain */
                        prev->next = optr->next;
                        free(optr);
                        optr = prev->next;
                    }
                    ++count;
                }
                else
                {
                    prev = optr;
                    optr = optr->next;
                }
            }
        }
        if (count > 0)
        {
            snprintf(errstr, sizeof(errstr),
                     "Removed %d entities from object cache", count);
            main_post_message(errstr);
        }
    }
}
