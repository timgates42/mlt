/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <string.h>
#include <framework/mlt.h>
#include <gdk/gdk.h>

#ifdef USE_PIXBUF
extern mlt_producer producer_pixbuf_init( char *filename );
extern mlt_filter filter_rescale_init( mlt_profile profile, char *arg );
#endif

#ifdef USE_GTK2
extern mlt_consumer consumer_gtk2_preview_init( mlt_profile profile, void *widget );
#endif

#ifdef USE_PANGO
extern mlt_producer producer_pango_init( const char *filename );
#endif

static void initialise( )
{
	static int init = 0;
	if ( init == 0 )
	{
		init = 1;
		g_type_init( );
	}
}

void *mlt_create_producer( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	initialise( );

#ifdef USE_PIXBUF
	if ( !strcmp( id, "pixbuf" ) )
		return producer_pixbuf_init( arg );
#endif

#ifdef USE_PANGO
	if ( !strcmp( id, "pango" ) )
		return producer_pango_init( arg );
#endif

	return NULL;
}

void *mlt_create_filter( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	initialise( );

#ifdef USE_PIXBUF
	if ( !strcmp( id, "gtkrescale" ) )
		return filter_rescale_init( profile, arg );
#endif

	return NULL;
}

void *mlt_create_transition( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	return NULL;
}

void *mlt_create_consumer( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	initialise( );

#ifdef USE_GTK2
	if ( !strcmp( id, "gtk2_preview" ) )
		return consumer_gtk2_preview_init( profile, arg );
#endif

	return NULL;
}

