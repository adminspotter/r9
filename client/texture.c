/* texture.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 10 Aug 2006, 11:13:28 trinity
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
 * This file contains the texture management routines for the Revision IX
 * client program.
 *
 * We will save the geometries and textures in directory structures under
 * the user's config directory ($HOME/.revision9/).  Since we will possibly
 * have a HUGE number of geometries and textures, we'll split things up
 * based on the last digit or two of the ID.  To start, we'll do the last
 * two numbers, and then inside each of those directories will be the texture
 * files.  Each texture can have two files:  the texture data itself (text),
 * and a possible texture map (a graphic file of some format, probably XPM).
 *
 * Let's keep the prefixes around so we don't have to keep recreating them.
 * The texture_prefix is the system-wide texture repository (in
 * /usr/share/revision9/texture), where the texture_cache is the user's
 * personal repository (in $HOME/.revision9/texture).  First we'll look in
 * the system store, then if we don't find what we need, we'll look in the
 * cache.  If we *still* don't find it, we'll send out a server request.
 *
 * Changes
 *   03 Aug 2006 TAQ - Created the file.
 *   04 Aug 2006 TAQ - Wrote the hash table.
 *   05 Aug 2006 TAQ - Added time lastused timestamp to the struct, which is
 *                     set during the call to find_texture.  Started work on
 *                     the hash pruning thread.  Removed the delete_texture
 *                     routine, since it was redundant with the pruning thread.
 *   09 Aug 2006 TAQ - Fallback texture is now 18% gray.  Now notify user
 *                     of entries removed from the hash table.
 *
 * Things to do
 *
 * $Id: texture.c 10 2013-01-25 22:13:00Z trinity $
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

#include "client.h"

#define TEXTURE_HASH_BUCKETS  91

typedef struct texture_tag
{
    u_int64_t texture_id;
    struct timeval lastused;
    GLfloat diffuse[4], specular[4], shininess;
    /* Some sort of texture map in here too */
    struct texture_tag *next;
}
texture;

static void parse_texture_file(FILE *, u_int64_t);
static int hash_func(u_int64_t);
static void insert_texture(u_int64_t, texture *);
static texture *find_texture(u_int64_t);
static void *texture_prune_worker(void *);

static texture **textures = NULL;
static char *texture_prefix = TEXTURE_PREFIX, *texture_cache = NULL;
static pthread_t prune_thread;

void setup_texture(void)
{
    char fname[PATH_MAX], errstr[256];
    int ret;

    /* Do some error checking here */
    strncpy(fname, getenv("HOME"), sizeof(fname));
    strncat(fname, "/.revision9/texture", sizeof(fname) - strlen(fname) - 1);
    texture_cache = strdup(fname);

    /* Create the hash table */
    if ((textures = (texture **)malloc(sizeof(texture *)
				       * TEXTURE_HASH_BUCKETS)) == NULL)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't allocate texture hash table");
	main_post_message(errstr);
	return;
    }
    memset(textures, 0, sizeof(texture *) * TEXTURE_HASH_BUCKETS);

    /* We might consider doing a simple file mtime comparison between
     * everything in the cache and everything in the store, and deleting
     * stuff from the cache which is newer in the store.
     */

    /* Start the thread for cleaning up the hash.  We'll wake up once
     * a minute, check what hasn't been used in 10 minutes (configurable
     * length?) and delete it from the hash, to save some memory.
     */
    if ((ret = pthread_create(&prune_thread, NULL, texture_prune_worker, NULL))
	!= 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't start texture cleanup thread: %s", strerror(ret));
	main_post_message(errstr);
    }
}

void cleanup_texture(void)
{
    int i;
    texture *tptr, *prev;

    /* Evaporate the hash table */
    if (textures != NULL)
    {
	for (i = 0; i < TEXTURE_HASH_BUCKETS; ++i)
	    if ((tptr = textures[i]) != NULL)
	    {
		do
		{
		    prev = tptr;
		    tptr = tptr->next;
		    free(prev);
		}
		while (tptr != NULL);
		textures[i] = NULL;
	    }
	free(textures);
	textures = NULL;
    }

    /* Clean up the texture cache directory string */
    if (texture_cache)
    {
	free(texture_cache);
	texture_cache = NULL;
    }
}

void load_texture(u_int64_t textureid)
{
    FILE *f;
    char fname[PATH_MAX];

    /* In case somebody's slacking on calling setup routines in the right
     * order.
     */
    if (texture_cache == NULL)
	setup_texture();

    /* First, bail out if it's already loaded */
    if (find_texture(textureid))
	return;

    /* Look in the user cache, since it could possibly be updated more
     * recently than the system store.
     */
    snprintf(fname, sizeof(fname),
	     "%s/%02llx/%lld.txt",
	     texture_cache, textureid & 0xFF, textureid);
    if ((f = fopen(fname, "r")) != NULL)
    {
	parse_texture_file(f, textureid);
	fclose(f);
	return;
    }

    /* Didn't find it in the user cache; now look in the system store */
    snprintf(fname, sizeof(fname),
	     "%s/%02llx/%lld.txt",
	     texture_prefix, textureid & 0xFF, textureid);
    if ((f = fopen(fname, "r")) != NULL)
    {
	parse_texture_file(f, textureid);
	fclose(f);
	return;
    }

    /* If we can't find the texture and have to request it, we should
     * have some sort of fallback texture to use in the meantime
     * before we receive the new stuff.
     */
    /*send_texture_request(textureid);*/
}

static void parse_texture_file(FILE *f, u_int64_t textureid)
{
    char str[PATH_MAX], *head;
    texture *t;

    if ((t = (texture *)malloc(sizeof(texture))) == NULL)
	return;

    t->texture_id = textureid;
    while (fgets(str, sizeof(str), f) != NULL)
    {
	head = str + strspn(str, " \t");
	if (*head != '#')
	{
	    if (!strncmp(head, "diffuse", 7))
	    {
		sscanf(head, "diffuse %f, %f, %f, %f",
		       &t->diffuse[0], &t->diffuse[1],
		       &t->diffuse[2], &t->diffuse[3]);
	    }
	    else if (!strncmp(head, "specular", 8))
	    {
		sscanf(head, "specular %f, %f, %f, %f",
		       &t->specular[0], &t->specular[1],
		       &t->specular[2], &t->specular[3]);
	    }
	    else if (!strncmp(head, "shininess", 9))
	    {
		sscanf(head, "shininess %f", &t->shininess);
	    }
	    else if (!strncmp(head, "map", 3))
	    {
		/* This will be a filename for a mapped texture */
	    }
	    /* We don't recognize the line; ignore it */
	}
    }
    gettimeofday(&t->lastused, NULL);
    insert_texture(textureid, t);
}

void update_texture(u_int64_t textureid)
{
    load_texture(textureid);
}

void draw_texture(u_int64_t textureid)
{
    texture *t = find_texture(textureid);

    if (t != NULL)
    {
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, t->diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, t->specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, &t->shininess);
    }
    else
    {
	/* Fallback texture */
	GLfloat material_diff[] = { 0.18, 0.18, 0.18, 1.0 };
	GLfloat material_shin[] = { 0.0 };

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material_diff);
	glMaterialfv(GL_FRONT, GL_SHININESS, material_shin);
    }
}

static int hash_func(u_int64_t textureid)
{
    return (textureid % TEXTURE_HASH_BUCKETS);
}

/* Put a texture into the hash table */
static void insert_texture(u_int64_t textureid, texture *t)
{
    int bucket = hash_func(textureid);
    texture *tptr = textures[bucket];

    if (textures[bucket] == NULL)
    {
	textures[bucket] = t;
	t->next = NULL;
    }
    else
    {
	/* Hashing collision; chain the entries */
	while (tptr->next != NULL)
	    tptr = tptr->next;
	tptr->next = t;
	t->next = NULL;
    }
}

/* Find a texture in the hash table */
static texture *find_texture(u_int64_t textureid)
{
    texture *tptr = textures[hash_func(textureid)];

    while (tptr != NULL && tptr->texture_id != textureid)
	tptr = tptr->next;
    if (tptr != NULL)
	gettimeofday(&tptr->lastused, NULL);
    return tptr;
}

/* This routine will clean out any textures from the hash that haven't been
 * used in 10 minutes.  We should eventually make that into a configurable
 * option, because it's the right thing to do.
 */
/* ARGSUSED */
static void *texture_prune_worker(void *notused)
{
    struct timeval tv;
    char errstr[256];
    int i, count;
    texture *tptr, *prev;

    for (;;)
    {
	sleep(60);
	if (!gettimeofday(&tv, NULL))
	{
	    snprintf(errstr, sizeof(errstr),
		     "Texture thread couldn't get time of day: %s",
		     strerror(errno));
	    main_post_message(errstr);
	    continue;
	}
	count = 0;
	for (i = 0; i < TEXTURE_HASH_BUCKETS; ++i)
	{
	    tptr = prev = textures[i];
	    while (tptr != NULL)
	    {
		if (tptr->lastused.tv_sec + 600 <= tv.tv_sec)
		{
		    if (tptr == prev)
		    {
			/* The head of this bucket is getting the boot */
			textures[i] = tptr->next;
			free(tptr);
			tptr = prev = textures[i];
		    }
		    else
		    {
			/* The victim is in the middle of the chain */
			prev->next = tptr->next;
			free(tptr);
			tptr = prev->next;
		    }
		    ++count;
		}
		else
		{
		    prev = tptr;
		    tptr = tptr->next;
		}
	    }
	}
	if (count > 0)
	{
	    snprintf(errstr, sizeof(errstr),
		     "Removed %d entities from texture cache", count);
	    main_post_message(errstr);
	}
    }
}
