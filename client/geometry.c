/* geometry.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Sep 2013, 14:21:10 trinity
 *
 * Revision IX game client
 * Copyright (C) 2006  Trinity Annabelle Quirk
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
 * This file contains the geometry management routines for the Revision IX
 * client program.
 *
 * We will save the geometries and textures in directory structures under
 * the user's config directory ($HOME/.revision9/).  Since we will possibly
 * have a HUGE number of geometries and textures, we'll split things up
 * based on the last digit or two of the ID.  To start, we'll do the last
 * two numbers, and then inside each of those directories, there'll be a
 * directory for each object ID, which will contain all the frames for that
 * object.
 *
 * File names should probably be the concatenation of the object ID and the
 * frame number.
 *
 * Once we load or retrieve the geometry, we'll make a display list out of
 * it, and only retain the display list - we don't need to keep all that
 * geometry stuff around in main memory, when we already have it encoded in
 * the display list.
 *
 * It's probably necessary, for the sake of not exhausting OpenGL's supply
 * of display lists, that we keep the geometries separate from the objects,
 * since many objects can use the same geometry, and it'd be a waste to
 * have a bunch of display lists which contain the same thing.
 *
 * We'll keep the prefixes around so we don't have to keep recreating them.
 * The geometry_prefix is the system-wide geometry repository (in
 * /usr/share/revision9/geometry), where the geometry_cache is the user's
 * personal repository (in $HOME/.revision9/geometry).  First we'll look in
 * the system store, then if we don't find what we need, we'll look in the
 * cache.  If we *still* don't find it, we'll send out a server request.
 *
 * Changes
 *   03 Aug 2006 TAQ - Created the file.
 *   04 Aug 2006 TAQ - Wrote the hash table - man, those things are really
 *                     simple to implement.  Wrote geometry file reading
 *                     and writing.
 *   05 Aug 2006 TAQ - Added the timestamp, which is set during the
 *                     find_geometry routine.  Makes some amount of sense
 *                     that if we're finding a geometry, we're going to
 *                     use it.  Or something like that.  Added the pruning
 *                     thread.
 *   09 Aug 2006 TAQ - Completed update_geometry.  Now notify users of
 *                     entries removed from hash table.  We no longer save
 *                     geometry files here, since they're no longer part of
 *                     the protocol.  Removed the data element in the
 *                     geometry structure.
 *
 * Things to do
 *   - One thing I'd really like to do is allow people to do a little bit
 *   of customization of their avatars or ships or what have you.  Probably
 *   just color changes, or addition of a logo, or simple stuff like that.
 *   Figure out the simplest way to do it, based on the method we're trying
 *   to do here.
 *
 * $Id: geometry.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <GL/glut.h>

#include "client.h"

#define GEOMETRY_HASH_BUCKETS  91

typedef struct geometry_tag
{
    u_int64_t geometry_id;
    unsigned short frame;
    struct timeval lastused;
    GLuint disp_list;
    struct geometry_tag *next;
}
geometry;

static void parse_geometry_file(FILE *, u_int64_t, u_int16_t);
static int hash_func(u_int64_t, u_int16_t);
static void insert_geometry(u_int64_t, u_int16_t, geometry *);
static geometry *find_geometry(u_int64_t, u_int16_t);
static void *geometry_prune_worker(void *);

static geometry **geometries = NULL;
static char *geometry_prefix = GEOMETRY_PREFIX, *geometry_cache = NULL;
static pthread_t prune_thread;

void setup_geometry(void)
{
    char fname[PATH_MAX], errstr[256];
    int ret;

    /* Do some error checking here */
    strncpy(fname, getenv("HOME"), sizeof(fname) - 1);
    strncat(fname, "/.revision9/geometry", sizeof(fname) - strlen(fname) - 1);
    geometry_cache = strdup(fname);

    /* Create the hash table */
    if ((geometries = (geometry **)malloc(sizeof(geometry *)
					  * GEOMETRY_HASH_BUCKETS)) == NULL)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't allocate geometry hash table");
	main_post_message(errstr);
	return;
    }
    memset(geometries, 0, sizeof(geometry *) * GEOMETRY_HASH_BUCKETS);

    /* We might consider doing a simple file mtime comparison between
     * everything in the cache and everything in the store, and deleting
     * stuff from the cache which is newer in the store.
     */

    /* Start the thread for cleaning up the hash.  We'll wake up once a
     * minute, check what hasn't been used in ten minutes (configurable
     * length?) and delete it from the hash, to save some memory and
     * display list space.
     */
    if ((ret = pthread_create(&prune_thread,
			      NULL,
			      geometry_prune_worker,
			      NULL)) != 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't start geometry cleanup thread: %s", strerror(ret));
	main_post_message(errstr);
    }
}

void cleanup_geometry(void)
{
    int i;
    geometry *gptr, *prev;

    /* Evaporate the hash table */
    if (geometries != NULL)
    {
	for (i = 0; i < GEOMETRY_HASH_BUCKETS; ++i)
	    if ((gptr = geometries[i]) != NULL)
	    {
		do
		{
		    prev = gptr;
		    gptr = gptr->next;
		    if (glIsList(prev->disp_list))
			glDeleteLists(prev->disp_list, 1);
		    free(prev);
		}
		while (gptr != NULL);
		geometries[i] = NULL;
	    }
	free(geometries);
	geometries = NULL;
    }

    /* Clean up the geometry cache directory string */
    if (geometry_cache)
    {
	free(geometry_cache);
	geometry_cache = NULL;
    }
}

void load_geometry(u_int64_t objectid, u_int16_t frame)
{
    FILE *f;
    char fname[PATH_MAX];

    /* In case somebody's slacking on calling setup routines in the right
     * order.
     */
    if (geometry_cache == NULL)
	setup_geometry();

    /* First, bail out if it's already loaded */
    if (find_geometry(objectid, frame))
	return;

    /* Look in the user cache, since it could possibly be updated more
     * recently than the system store.
     */
    snprintf(fname, sizeof(fname),
	     "%s/%02llx/%lld/%lld-%d.txt",
	     geometry_cache, objectid & 0xFF, objectid, objectid, frame);
    if ((f = fopen(fname, "r")) != NULL)
    {
	parse_geometry_file(f, objectid, frame);
	fclose(f);
	return;
    }

    /* Didn't find it in the user cache; now look in the system store */
    snprintf(fname, sizeof(fname),
	     "%s/%02llx/%lld/%lld-%d.txt",
	     geometry_prefix, objectid & 0xFF, objectid, objectid, frame);
    if ((f = fopen(fname, "r")) != NULL)
    {
	parse_geometry_file(f, objectid, frame);
	fclose(f);
	return;
    }

    /* If we can't find the geometry and have to request it, we should
     * have some sort of fallback geometry to draw in the meantime
     * before we receive the new stuff.
     */
    /*send_geometry_request(objectid, frame);*/
}

static void parse_geometry_file(FILE *f, u_int64_t objectid, u_int16_t frame)
{
    char str[PATH_MAX], *head;
    geometry *g;

    if ((g = (geometry *)malloc(sizeof(geometry))) == NULL)
	return;

    g->geometry_id = objectid;
    g->frame = frame;
    /* We'll have to figure out what the first open list number is */
    glNewList(0, GL_COMPILE);
    while (fgets(str, sizeof(str), f) != NULL)
    {
	head = str + strspn(str, " \t");
	if (*head != '#')
	{
	    if (!strncmp(head, "polygon", 7))
	    {
		GLfloat pt1[3], pt2[3], pt3[3], n1[3], n2[3], n3[3];
		u_int64_t tid;

		sscanf(head, "polygon "
		       "%f, %f, %f, %f, %f, %f, "
		       "%f, %f, %f, %f, %f, %f, "
		       "%f, %f, %f, %f, %f, %f, "
		       "%lld",
		       &pt1[0], &pt1[1], &pt1[2], &n1[0], &n1[1], &n1[2],
		       &pt2[0], &pt2[1], &pt2[2], &n2[0], &n2[1], &n2[2],
		       &pt3[0], &pt3[1], &pt3[2], &n3[0], &n3[1], &n3[2],
		       &tid);
		glBegin(GL_POLYGON);
		draw_texture(tid);
		glNormal3fv(n1);
		glVertex3fv(pt1);
		glNormal3fv(n2);
		glVertex3fv(pt2);
		glNormal3fv(n3);
		glVertex3fv(pt3);
		glEnd();
	    }
	    else if (!strncmp(head, "sphere", 5))
	    {
		GLfloat pt1[3], r;
		u_int64_t tid;

		sscanf(head, "sphere %f, %f, %f, %f, %lld",
		       &pt1[0], &pt1[1], &pt1[2], &r, &tid);
		if (pt1[0] != 0.0 || pt1[1] != 0.0 || pt1[2] != 0.0)
		{
		    glPushMatrix();
		    glTranslatef(pt1[0], pt1[1], pt1[2]);
		}
		draw_texture(tid);
		glutSolidSphere(r, 4, 4);
		if (pt1[0] != 0.0 || pt1[1] != 0.0 || pt1[2] != 0.0)
		    glPopMatrix();
	    }
	    /* We don't recognize the line; ignore it */
	}
    }
    glEndList();
    g->disp_list = 0;
    insert_geometry(objectid, frame, g);
}

void draw_geometry(u_int64_t objectid, u_int16_t frame)
{
    geometry *gptr = find_geometry(objectid, frame);

    if (gptr != NULL)
	glCallList(gptr->disp_list);
}

void update_geometry(u_int64_t objectid, u_int16_t frame)
{
    load_geometry(objectid, frame);
}

static int hash_func(u_int64_t objectid, u_int16_t frame)
{
    return (objectid + frame % GEOMETRY_HASH_BUCKETS);
}

/* Put a geometry into the hash table */
static void insert_geometry(u_int64_t objectid, u_int16_t frame, geometry *g)
{
    int bucket = hash_func(objectid, frame);
    geometry *gptr = geometries[bucket];

    if (geometries[bucket] == NULL)
    {
	geometries[bucket] = g;
	g->next = NULL;
    }
    else
    {
	/* Hashing collision; chain the entries */
	while (gptr->next != NULL)
	    gptr = gptr->next;
	gptr->next = g;
	g->next = NULL;
    }
}

/* Find a geometry in the hash table */
geometry *find_geometry(u_int64_t objectid, u_int16_t frame)
{
    geometry *gptr = geometries[hash_func(objectid, frame)];

    while (gptr != NULL
	   && gptr->geometry_id != objectid
	   && gptr->frame != frame)
	gptr = gptr->next;
    if (gptr != NULL)
	gettimeofday(&gptr->lastused, NULL);
    return gptr;
}

/* ARGSUSED */
static void *geometry_prune_worker(void *notused)
{
    struct timeval tv;
    char errstr[256];
    int i, count;
    geometry *gptr, *prev;

    for (;;)
    {
	sleep(60);
	if (!gettimeofday(&tv, NULL))
	{
	    snprintf(errstr, sizeof(errstr),
		     "Geometry thread couldn't get time of day: %s",
		     strerror(errno));
	    main_post_message(errstr);
	    continue;
	}
	count = 0;
	for (i = 0; i < GEOMETRY_HASH_BUCKETS; ++i)
	{
	    gptr = prev = geometries[i];
	    while (gptr != NULL)
	    {
		if (gptr->lastused.tv_sec + 600 <= tv.tv_sec)
		{
		    if (gptr == prev)
		    {
			/* The head of this bucket is getting the boot */
			geometries[i] = gptr->next;
			if (glIsList(gptr->disp_list))
			    glDeleteLists(gptr->disp_list, 1);
			free(gptr);
			gptr = prev = geometries[i];
		    }
		    else
		    {
			/* The victim is in the middle of the chain */
			prev->next = gptr->next;
			if (glIsList(gptr->disp_list))
			    glDeleteLists(gptr->disp_list, 1);
			free(gptr);
			gptr = prev->next;
		    }
		    ++count;
		}
		else
		{
		    prev = gptr;
		    gptr = gptr->next;
		}
	    }
	}
	if (count > 0)
	{
	    snprintf(errstr, sizeof(errstr),
		     "Removed %d entities from geometry cache", count);
	    main_post_message(errstr);
	}
    }
}
