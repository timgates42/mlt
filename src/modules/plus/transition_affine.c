/*
 * transition_affine.c -- affine transformations
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

#include <framework/mlt_transition.h>
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/** Calculate real geometry.
*/

static void geometry_calculate( mlt_transition this, char *store, struct mlt_geometry_item_s *output, float position )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	mlt_geometry geometry = mlt_properties_get_data( properties, store, NULL );
	int mirror_off = mlt_properties_get_int( properties, "mirror_off" );
	int repeat_off = mlt_properties_get_int( properties, "repeat_off" );
	int length = mlt_geometry_get_length( geometry );

	// Allow wrapping
	if ( !repeat_off && position >= length && length != 0 )
	{
		int section = position / length;
		position -= section * length;
		if ( !mirror_off && section % 2 == 1 )
			position = length - position;
	}

	// Fetch the key for the position
	mlt_geometry_fetch( geometry, output, position );
}


static mlt_geometry transition_parse_keys( mlt_transition this, char *name, char *store, int normalised_width, int normalised_height )
{
	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	// Try to fetch it first
	mlt_geometry geometry = mlt_properties_get_data( properties, store, NULL );

	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Determine length and obtain cycle
	int length = out - in + 1;
	double cycle = mlt_properties_get_double( properties, "cycle" );

	// Allow a geometry repeat cycle
	if ( cycle >= 1 )
		length = cycle;
	else if ( cycle > 0 )
		length *= cycle;

	if ( geometry == NULL )
	{
		// Get the new style geometry string
		char *property = mlt_properties_get( properties, name );

		// Create an empty geometries object
		geometry = mlt_geometry_init( );

		// Parse the geometry if we have one
		mlt_geometry_parse( geometry, property, length, normalised_width, normalised_height );

		// Store it
		mlt_properties_set_data( properties, store, geometry, 0, ( mlt_destructor )mlt_geometry_close, NULL );
	}
	else
	{
		// Check for updates and refresh if necessary
		mlt_geometry_refresh( geometry, mlt_properties_get( properties, name ), length, normalised_width, normalised_height );
	}

	return geometry;
}

static mlt_geometry composite_calculate( mlt_transition this, struct mlt_geometry_item_s *result, int nw, int nh, float position )
{
	// Structures for geometry
	mlt_geometry start = transition_parse_keys( this, "geometry", "geometries", nw, nh );

	// Do the calculation
	geometry_calculate( this, "geometries", result, position );

	return start;
}

static inline float composite_calculate_key( mlt_transition this, char *name, char *store, int norm, float position )
{
	// Struct for the result
	struct mlt_geometry_item_s result;

	// Structures for geometry
	transition_parse_keys( this, name, store, norm, 0 );

	// Do the calculation
	geometry_calculate( this, store, &result, position );

	return result.x;
}

typedef struct 
{
	float matrix[3][3];
}
affine_t;

static void affine_init( float this[3][3] )
{
	this[0][0] = 1;
	this[0][1] = 0;
	this[0][2] = 0;
	this[1][0] = 0;
	this[1][1] = 1;
	this[1][2] = 0;
	this[2][0] = 0;
	this[2][1] = 0;
	this[2][2] = 1;
}

// Multiply two this affine transform with that
static void affine_multiply( float this[3][3], float that[3][3] )
{
	float output[3][3];
	int i;
	int j;

	for ( i = 0; i < 3; i ++ )
		for ( j = 0; j < 3; j ++ )
			output[i][j] = this[i][0] * that[j][0] + this[i][1] * that[j][1] + this[i][2] * that[j][2];

	this[0][0] = output[0][0];
	this[0][1] = output[0][1];
	this[0][2] = output[0][2];
	this[1][0] = output[1][0];
	this[1][1] = output[1][1];
	this[1][2] = output[1][2];
	this[2][0] = output[2][0];
	this[2][1] = output[2][1];
	this[2][2] = output[2][2];
}

// Rotate by a given angle
static void affine_rotate_x( float this[3][3], float angle )
{
	float affine[3][3];
	affine[0][0] = cos( angle * M_PI / 180 );
	affine[0][1] = 0 - sin( angle * M_PI / 180 );
	affine[0][2] = 0;
	affine[1][0] = sin( angle * M_PI / 180 );
	affine[1][1] = cos( angle * M_PI / 180 );
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

static void affine_rotate_y( float this[3][3], float angle )
{
	float affine[3][3];
	affine[0][0] = cos( angle * M_PI / 180 );
	affine[0][1] = 0;
	affine[0][2] = 0 - sin( angle * M_PI / 180 );
	affine[1][0] = 0;
	affine[1][1] = 1;
	affine[1][2] = 0;
	affine[2][0] = sin( angle * M_PI / 180 );
	affine[2][1] = 0;
	affine[2][2] = cos( angle * M_PI / 180 );
	affine_multiply( this, affine );
}

static void affine_rotate_z( float this[3][3], float angle )
{
	float affine[3][3];
	affine[0][0] = 1;
	affine[0][1] = 0;
	affine[0][2] = 0;
	affine[1][0] = 0;
	affine[1][1] = cos( angle * M_PI / 180 );
	affine[1][2] = sin( angle * M_PI / 180 );
	affine[2][0] = 0;
	affine[2][1] = - sin( angle * M_PI / 180 );
	affine[2][2] = cos( angle * M_PI / 180 );
	affine_multiply( this, affine );
}

static void affine_scale( float this[3][3], float sx, float sy )
{
	float affine[3][3];
	affine[0][0] = sx;
	affine[0][1] = 0;
	affine[0][2] = 0;
	affine[1][0] = 0;
	affine[1][1] = sy;
	affine[1][2] = 0;
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

// Shear by a given value
static void affine_shear( float this[3][3], float shear_x, float shear_y, float shear_z )
{
	float affine[3][3];
	affine[0][0] = 1;
	affine[0][1] = tan( shear_x * M_PI / 180 );
	affine[0][2] = 0;
	affine[1][0] = tan( shear_y * M_PI / 180 );
	affine[1][1] = 1;
	affine[1][2] = tan( shear_z * M_PI / 180 );
	affine[2][0] = 0;
	affine[2][1] = 0;
	affine[2][2] = 1;
	affine_multiply( this, affine );
}

static void affine_offset( float this[3][3], int x, int y )
{
	this[0][2] += x;
	this[1][2] += y;
}

// Obtain the mapped x coordinate of the input
static inline double MapX( float this[3][3], int x, int y )
{
	return this[0][0] * x + this[0][1] * y + this[0][2];
}

// Obtain the mapped y coordinate of the input
static inline double MapY( float this[3][3], int x, int y )
{
	return this[1][0] * x + this[1][1] * y + this[1][2];
}

static inline double MapZ( float this[3][3], int x, int y )
{
	return this[2][0] * x + this[2][1] * y + this[2][2];
}

#define MAX( x, y ) x > y ? x : y
#define MIN( x, y ) x < y ? x : y

static void affine_max_output( float this[3][3], float *w, float *h, float dz )
{
	int tlx = MapX( this, -720, 576 ) / dz;
	int tly = MapY( this, -720, 576 ) / dz;
	int trx = MapX( this, 720, 576 ) / dz;
	int try = MapY( this, 720, 576 ) / dz;
	int blx = MapX( this, -720, -576 ) / dz;
	int bly = MapY( this, -720, -576 ) / dz;
	int brx = MapX( this, 720, -576 ) / dz;
	int bry = MapY( this, 720, -576 ) / dz;

	int max_x;
	int max_y;
	int min_x;
	int min_y;

	max_x = MAX( tlx, trx );
	max_x = MAX( max_x, blx );
	max_x = MAX( max_x, brx );

	min_x = MIN( tlx, trx );
	min_x = MIN( min_x, blx );
	min_x = MIN( min_x, brx );

	max_y = MAX( tly, try );
	max_y = MAX( max_y, bly );
	max_y = MAX( max_y, bry );

	min_y = MIN( tly, try );
	min_y = MIN( min_y, bly );
	min_y = MIN( min_y, bry );

	*w = ( float )( max_x - min_x + 1 ) / 1440.0;
	*h = ( float )( max_y - min_y + 1 ) / 1152.0;
}

#define IN_RANGE( v, r )	( v >= - r / 2 && v < r / 2 )

static inline void get_affine( affine_t *affine, mlt_transition this, float position )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	int keyed = mlt_properties_get_int( properties, "keyed" );
	affine_init( affine->matrix );

	if ( keyed == 0 )
	{
		float fix_rotate_x = mlt_properties_get_double( properties, "fix_rotate_x" );
		float fix_rotate_y = mlt_properties_get_double( properties, "fix_rotate_y" );
		float fix_rotate_z = mlt_properties_get_double( properties, "fix_rotate_z" );
		float rotate_x = mlt_properties_get_double( properties, "rotate_x" );
		float rotate_y = mlt_properties_get_double( properties, "rotate_y" );
		float rotate_z = mlt_properties_get_double( properties, "rotate_z" );
		float fix_shear_x = mlt_properties_get_double( properties, "fix_shear_x" );
		float fix_shear_y = mlt_properties_get_double( properties, "fix_shear_y" );
		float fix_shear_z = mlt_properties_get_double( properties, "fix_shear_z" );
		float shear_x = mlt_properties_get_double( properties, "shear_x" );
		float shear_y = mlt_properties_get_double( properties, "shear_y" );
		float shear_z = mlt_properties_get_double( properties, "shear_z" );
		float ox = mlt_properties_get_double( properties, "ox" );
		float oy = mlt_properties_get_double( properties, "oy" );

		affine_rotate_x( affine->matrix, fix_rotate_x + rotate_x * position );
		affine_rotate_y( affine->matrix, fix_rotate_y + rotate_y * position );
		affine_rotate_z( affine->matrix, fix_rotate_z + rotate_z * position );
		affine_shear( affine->matrix, 
					  fix_shear_x + shear_x * position, 
					  fix_shear_y + shear_y * position,
					  fix_shear_z + shear_z * position );
		affine_offset( affine->matrix, ox, oy );
	}
	else
	{
		float rotate_x = composite_calculate_key( this, "rotate_x", "rotate_x_info", 360, position );
		float rotate_y = composite_calculate_key( this, "rotate_y", "rotate_y_info", 360, position );
		float rotate_z = composite_calculate_key( this, "rotate_z", "rotate_z_info", 360, position );
		float shear_x = composite_calculate_key( this, "shear_x", "shear_x_info", 360, position );
		float shear_y = composite_calculate_key( this, "shear_y", "shear_y_info", 360, position );
		float shear_z = composite_calculate_key( this, "shear_z", "shear_z_info", 360, position );

		affine_rotate_x( affine->matrix, rotate_x );
		affine_rotate_y( affine->matrix, rotate_y );
		affine_rotate_z( affine->matrix, rotate_z );
		affine_shear( affine->matrix, shear_x, shear_y, shear_z );
	}
}

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the transition object
	mlt_transition this = mlt_frame_pop_service( a_frame );

	// Get the properties of the transition
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// Image, format, width, height and image for the b frame
	uint8_t *b_image = NULL;
	mlt_image_format b_format = mlt_image_yuv422;
	int b_width;
	int b_height;

	// Get the unique name to retrieve the frame position
	char *name = mlt_properties_get( properties, "_unique_id" );

	// Assign the current position to the name
	mlt_position position =  mlt_properties_get_position( a_props, name );
	mlt_position in = mlt_properties_get_position( properties, "in" );
	mlt_position out = mlt_properties_get_position( properties, "out" );
	int mirror = mlt_properties_get_position( properties, "mirror" );
	int length = out - in + 1;

	// Obtain the normalised width and height from the a_frame
	int normalised_width = mlt_properties_get_int( a_props, "normalised_width" );
	int normalised_height = mlt_properties_get_int( a_props, "normalised_height" );

	double consumer_ar = mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) ;

	// Structures for geometry
	struct mlt_geometry_item_s result;

	if ( mirror && position > length / 2 )
		position = abs( position - length );

	// Fetch the a frame image
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	// Calculate the region now
	composite_calculate( this, &result, normalised_width, normalised_height, ( float )position );

	// Fetch the b frame image
	result.w = ( int )( result.w * *width / normalised_width );
	result.h = ( int )( result.h * *height / normalised_height );
	result.x = ( int )( result.x * *width / normalised_width );
	result.y = ( int )( result.y * *height / normalised_height );
	//result.w -= ( int )abs( result.w ) % 2;
	//result.x -= ( int )abs( result.x ) % 2;
	b_width = result.w;
	b_height = result.h;

	if ( mlt_properties_get_double( b_props, "aspect_ratio" ) == 0.0 )
		mlt_properties_set_double( b_props, "aspect_ratio", consumer_ar );

	if ( !strcmp( mlt_properties_get( a_props, "rescale.interp" ), "none" ) )
	{
		mlt_properties_set( b_props, "rescale.interp", "nearest" );
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", consumer_ar );
	}
	else
	{
		mlt_properties_set( b_props, "rescale.interp", mlt_properties_get( a_props, "rescale.interp" ) );
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", consumer_ar );
	}

	mlt_properties_set_int( b_props, "distort", mlt_properties_get_int( properties, "distort" ) );
	mlt_frame_get_image( b_frame, &b_image, &b_format, &b_width, &b_height, 0 );
	result.w = b_width;
	result.h = b_height;

	// Check that both images are of the correct format and process
	if ( *format == mlt_image_yuv422 && b_format == mlt_image_yuv422 )
	{
		register int x, y;
		register int dx, dy;
		double dz;
		float sw, sh;

		// Get values from the transition
		float scale_x = mlt_properties_get_double( properties, "scale_x" );
		float scale_y = mlt_properties_get_double( properties, "scale_y" );
		int scale = mlt_properties_get_int( properties, "scale" );

		uint8_t *p = *image;
		uint8_t *q = *image;

		int cx = result.x + ( b_width >> 1 );
		int cy = result.y + ( b_height >> 1 );
	
		int lower_x = 0 - cx;
		int upper_x = *width - cx;
		int lower_y = 0 - cy;
		int upper_y = *height - cy;

		int b_stride = b_width << 1;
		int a_stride = *width << 1;
		int x_offset = ( int )result.w >> 1;
		int y_offset = ( int )result.h >> 1;

		uint8_t *alpha = mlt_frame_get_alpha_mask( b_frame );
		uint8_t *mask = mlt_frame_get_alpha_mask( a_frame );
		uint8_t *pmask = mask;
		float mix;

		affine_t affine;

		get_affine( &affine, this, ( float )position );

		q = *image;

		dz = MapZ( affine.matrix, 0, 0 );

		if ( mask == NULL )
		{
			mask = mlt_pool_alloc( *width * *height );
			pmask = mask;
			memset( mask, 255, *width * *height );
		}

		if ( ( int )abs( dz * 1000 ) < 25 )
			goto getout;

		if ( scale )
		{
			affine_max_output( affine.matrix, &sw, &sh, dz );
			affine_scale( affine.matrix, sw, sh );
		}
		else if ( scale_x != 0 && scale_y != 0 )
		{
			affine_scale( affine.matrix, scale_x, scale_y );
		}

		if ( alpha == NULL )
		{
			for ( y = lower_y; y < upper_y; y ++ )
			{
				p = q;

				for ( x = lower_x; x < upper_x; x ++ )
				{
					dx = MapX( affine.matrix, x, y ) / dz + x_offset;
					dy = MapY( affine.matrix, x, y ) / dz + y_offset;

					if ( dx >= 0 && dx < b_width && dy >=0 && dy < b_height )
					{
						pmask ++;
						dx -= dx & 1;
						*p ++ = *( b_image + dy * b_stride + ( dx << 1 ) );
						*p ++ = *( b_image + dy * b_stride + ( dx << 1 ) + ( ( x & 1 ) << 1 ) + 1 );
					}
					else
					{
						p += 2;
						pmask ++;
					}
				}

				q += a_stride;
			}
		}
		else
		{
			for ( y = lower_y; y < upper_y; y ++ )
			{
				p = q;

				for ( x = lower_x; x < upper_x; x ++ )
				{
					dx = MapX( affine.matrix, x, y ) / dz + x_offset;
					dy = MapY( affine.matrix, x, y ) / dz + y_offset;

					if ( dx >= 0 && dx < b_width && dy >=0 && dy < b_height )
					{
						*pmask ++ = *( alpha + dy * b_width + dx );
						mix = ( float )*( alpha + dy * b_width + dx ) / 255.0;
						dx -= dx & 1;
						*p = *p * ( 1 - mix ) + mix * *( b_image + dy * b_stride + ( dx << 1 ) );
						p ++;
						*p = *p * ( 1 - mix ) + mix * *( b_image + dy * b_stride + ( dx << 1 ) + ( ( x & 1 ) << 1 ) + 1 );
						p ++;
					}
					else
					{
						p += 2;
						pmask ++;
					}
				}

				q += a_stride;
			}
		}

getout:
		a_frame->get_alpha_mask = NULL;
		mlt_properties_set_data( a_props, "alpha", mask, 0, mlt_pool_release, NULL );
	}

	return 0;
}

/** Affine transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	// Get a unique name to store the frame position
	char *name = mlt_properties_get( MLT_TRANSITION_PROPERTIES( transition ), "_unique_id" );

	// Assign the current position to the name
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	mlt_properties_set_position( a_props, name, mlt_frame_get_position( a_frame ) );

	// Push the transition on to the frame
	mlt_frame_push_service( a_frame, transition );

	// Push the b_frame on to the stack
	mlt_frame_push_frame( a_frame, b_frame );

	// Push the transition method
	mlt_frame_push_get_image( a_frame, transition_get_image );

	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_affine_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "sx", 1 );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "sy", 1 );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "distort", 0 );
		mlt_properties_set( MLT_TRANSITION_PROPERTIES( transition ), "geometry", "0,0:100%x100%" );
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );
		transition->process = transition_process;
	}
	return transition;
}
