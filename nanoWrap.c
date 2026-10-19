/*
Copyright (C) 2007-2009 Olli Hinkka

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
#include <e32def.h>
#include <e32std.h>
*/

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl.h"
#include "glesinterface.h"
#include "nanogl.h"
#include "nanogl_private.h"

#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1

struct nanoState
{
	GLboolean alpha_test;
	GLboolean blend;
	GLboolean clip_planei;
	GLboolean color_logic_op;
	GLboolean color_material;
	GLboolean cull_face;
	GLboolean depth_test;
	GLboolean dither;
	GLboolean fog;
	GLboolean light0;
	GLboolean light1;
	GLboolean light2;
	GLboolean light3;
	GLboolean light4;
	GLboolean light5;
	GLboolean light6;
	GLboolean light7;
	GLboolean lighting;
	GLboolean line_smooth;
	GLboolean matrix_palette_oes;
	GLboolean multisample;
	GLboolean normalize;
	GLboolean point_smooth;
	GLboolean point_sprite_oes;
	GLboolean polygon_offset_fill;
	GLboolean rescale_normal;
	GLboolean sample_alpha_to_coverage;
	GLboolean sample_alpha_to_one;
	GLboolean sample_coverage;
	GLboolean scissor_test;
	GLboolean stencil_test;
	GLboolean depthmask;
	GLclampd depth_range_near;
	GLclampd depth_range_far;
	GLenum depth_func;
	GLenum cullface;
	GLenum shademodel;
	GLenum sfactor;
	GLenum dfactor;
	GLenum matrixmode;
	GLenum alphafunc;
	GLclampf alpharef;
	GLint viewport[4];
	GLint scissorbox[4];
};

static struct nanoState nanoglState;

static struct nanoState nanoglInitState = {
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_FALSE,
	GL_TRUE,
	0.0f,
	1.0f,
	GL_LESS,
	GL_BACK,
	GL_SMOOTH,
	GL_ONE,
	GL_ZERO,
	GL_MODELVIEW,
	GL_ALWAYS,
	0.0f,
	{ -1, -1, -1, -1 },
	{ -1, -1, -1, -1 },
};

struct booleanstate
{
	GLboolean value;
	GLboolean changed;
};

struct floatstate
{
	GLfloat value;
	GLboolean changed;
};

struct uintstate
{
	GLuint value;
	GLboolean changed;
};

struct ptrstate
{
	GLint size;
	GLenum type;
	GLsizei stride;
	GLvoid *ptr;
	GLboolean changed;
	GLboolean enabled;
};

struct nanotmuState
{
	struct booleanstate texture_2d;
	struct floatstate texture_env_mode;
	struct uintstate boundtexture;
	struct ptrstate vertex_array;
	struct ptrstate color_array;
	struct ptrstate texture_coord_array;
	struct ptrstate normal_array;
};

static struct nanotmuState tmuState0;
static struct nanotmuState tmuState1;

static struct nanotmuState tmuInitState = {
	{ GL_FALSE, GL_FALSE },
	{ GL_MODULATE, GL_FALSE },
	{ 0x7fffffff, GL_FALSE },
	{ 4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE },
	{ 4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE },
	{ 4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE },
	{ 3, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE },
};

static struct nanotmuState *activetmuState = &tmuState0;

extern GlESInterface *glEsImpl;

static GLenum wrapperPrimitiveMode = GL_QUADS;
static GLenum batchedPrimitiveMode = GL_TRIANGLES;
GLboolean useTexCoordArray = GL_FALSE;
static GLenum activetmu = GL_TEXTURE0;
static GLenum clientactivetmu = GL_TEXTURE0;

static GLboolean useMultiTexCoordArray = GL_FALSE;

static GLboolean delayedttmuchange = GL_FALSE;
static GLenum delayedtmutarget = GL_TEXTURE0;

typedef struct VertexAttrib
{
	float x;
	float y;
	float z;
	float w;
	float nx;
	float ny;
	float nz;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;

	float s;
	float t;
	float r;
	float q;
	float s_multi;
	float t_multi;
	float r_multi;
	float q_multi;
} VertexAttrib;

static VertexAttrib vertexattribs[60000];

static GLushort indexArray[50000];

static GLuint vertexCount = 0;
static GLuint indexCount = 0;
static GLuint vertexMark = 0;
static int indexbase = 0;

static VertexAttrib *ptrVertexAttribArray = NULL;
static VertexAttrib *ptrVertexAttribArrayMark = NULL;

static VertexAttrib currentVertexAttrib;
static VertexAttrib currentVertexAttribInit = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 255, 255, 255, 255, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static GLushort *ptrIndexArray = NULL;

static GLboolean arraysValid = GL_FALSE;

static GLboolean skipnanogl;
static GLenum nanoWrapperError = GL_NO_ERROR;

static void SetWrapperError( GLenum error )
{
	if( nanoWrapperError == GL_NO_ERROR )
		nanoWrapperError = error;
}

void InitGLStructs( void )
{
	ptrVertexAttribArray = vertexattribs;
	ptrVertexAttribArrayMark = ptrVertexAttribArray;
	ptrIndexArray = indexArray;

	memcpy( &nanoglState, &nanoglInitState, sizeof( struct nanoState ));
	memcpy( &tmuState0, &tmuInitState, sizeof( struct nanotmuState ));
	memcpy( &tmuState1, &tmuInitState, sizeof( struct nanotmuState ));
	memcpy( &currentVertexAttrib, &currentVertexAttribInit, sizeof( struct VertexAttrib ));

	activetmuState = &tmuState0;
	wrapperPrimitiveMode = GL_QUADS;
	batchedPrimitiveMode = GL_TRIANGLES;
	useTexCoordArray = GL_FALSE;
	activetmu = GL_TEXTURE0;
	clientactivetmu = GL_TEXTURE0;
	delayedttmuchange = GL_FALSE;
	delayedtmutarget = GL_TEXTURE0;
	vertexCount = 0;
	indexCount = 0;
	vertexMark = 0;
	indexbase = 0;
	arraysValid = GL_FALSE;
	skipnanogl = GL_FALSE;
	nanoWrapperError = GL_NO_ERROR;
}

static void ResetNanoState( void )
{
	if( tmuState0.color_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
	}

	if( tmuState0.vertex_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
	}

	if( tmuState0.texture_coord_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if( tmuState0.normal_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
	}
	glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
		tmuState0.vertex_array.type,
		tmuState0.vertex_array.stride,
		tmuState0.vertex_array.ptr );

	glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
		tmuState0.texture_coord_array.type,
		tmuState0.texture_coord_array.stride,
		tmuState0.texture_coord_array.ptr );

	glEsImpl->glColorPointer( tmuState0.color_array.size,
		tmuState0.color_array.type,
		tmuState0.color_array.stride,
		tmuState0.color_array.ptr );

	glEsImpl->glNormalPointer(
		tmuState0.normal_array.type,
		tmuState0.normal_array.stride,
		tmuState0.normal_array.ptr );

	glEsImpl->glMatrixMode( nanoglState.matrixmode );

	glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );

	glEsImpl->glBlendFunc( nanoglState.sfactor, nanoglState.dfactor );

	// glEsImpl->glBindTexture(GL_TEXTURE_2D, stackTextureState);

	glEsImpl->glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, activetmuState->texture_env_mode.value );

	arraysValid = GL_FALSE;
	skipnanogl = GL_FALSE;
}

static void FlushOnStateChange( void )
{
	if( skipnanogl )
		return;
	if( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
#ifndef USE_CORE_PROFILE
		glEsImpl->glActiveTexture( delayedtmutarget );
#endif
	}

	if( !vertexCount )
		return;

	GLboolean multitex = tmuState1.texture_2d.value;

	if( !arraysValid )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
		glEsImpl->glVertexPointer( 4, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].x );
		glEsImpl->glNormalPointer( GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].nx );
		glEsImpl->glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( VertexAttrib ), &vertexattribs[0].red );
		glEsImpl->glTexCoordPointer( 4, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].s );
		glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
		glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if( multitex )
		{
			glEsImpl->glTexCoordPointer( 4, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].s_multi );
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
		useMultiTexCoordArray = multitex;
		arraysValid = GL_TRUE;
	}
	else if( multitex != useMultiTexCoordArray )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if( multitex )
		{
			glEsImpl->glTexCoordPointer( 4, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].s_multi );
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
		useMultiTexCoordArray = multitex;
	}

	glEsImpl->glDrawElements( batchedPrimitiveMode, (GLsizei)( ptrIndexArray - indexArray ), GL_UNSIGNED_SHORT, indexArray );

	vertexMark = vertexCount = 0;
	indexbase = indexCount = 0;
	ptrVertexAttribArrayMark = ptrVertexAttribArray = vertexattribs;

	ptrIndexArray = indexArray;

	useTexCoordArray = GL_FALSE;
}

void nanoGL_Flush( void )
{
	FlushOnStateChange();
}

void nanoGL_Reset( void )
{
	ResetNanoState();
}

void GL_MANGLE( glBegin )( GLenum mode )
{
	GLenum batchmode;

	if( mode == GL_POINTS )
		batchmode = GL_POINTS;
	else if( mode == GL_LINES || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP )
		batchmode = GL_LINES;
	else
		batchmode = GL_TRIANGLES;

	if( batchmode != batchedPrimitiveMode )
	{
		FlushOnStateChange();
		batchedPrimitiveMode = batchmode;
	}

	wrapperPrimitiveMode = mode;
	vertexMark = vertexCount;
	ptrVertexAttribArrayMark = ptrVertexAttribArray;
	indexbase = indexCount;
}

void GL_MANGLE( glEnd )( void )
{
	GLuint blockverts = ((unsigned char *)ptrVertexAttribArray - (unsigned char *)ptrVertexAttribArrayMark ) / sizeof( VertexAttrib );
	GLuint minverts;

	if( batchedPrimitiveMode == GL_POINTS )
		minverts = 1;
	else if( batchedPrimitiveMode == GL_LINES )
		minverts = 2;
	else
		minverts = 3;

	vertexCount += blockverts;
	if( blockverts < minverts )
	{
		vertexCount = vertexMark;
		ptrVertexAttribArray = ptrVertexAttribArrayMark;
		return;
	}
	switch( wrapperPrimitiveMode )
	{
	case GL_QUADS:
	{
		int qcount = ( vertexCount - vertexMark ) / 4;

		for( int count = 0; count < qcount; count++ )
		{
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 1;
			*ptrIndexArray++ = indexCount + 2;
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 2;
			*ptrIndexArray++ = indexCount + 3;
			indexCount += 4;
			vertexCount += 2;
		}
	}
	break;
	case GL_QUAD_STRIP:
	{
		if(( vertexCount - vertexMark ) & 1 )
		{
			vertexCount--;
			ptrVertexAttribArray--;
		}
		int quadverts = vertexCount - vertexMark;
		int qcount = quadverts / 2 - 1;
		for( int count = 0; count < qcount; ++count )
		{
			GLuint base = indexCount + count * 2;
			*ptrIndexArray++ = (GLushort)base;
			*ptrIndexArray++ = (GLushort)( base + 1 );
			*ptrIndexArray++ = (GLushort)( base + 3 );
			*ptrIndexArray++ = (GLushort)base;
			*ptrIndexArray++ = (GLushort)( base + 3 );
			*ptrIndexArray++ = (GLushort)( base + 2 );
		}
		indexCount += quadverts;
		vertexCount += qcount * 2;
	}
	break;
	case GL_TRIANGLES:
	{
		int vcount = ( vertexCount - vertexMark ) / 3;
		for( int count = 0; count < vcount; count++ )
		{
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 1;
			*ptrIndexArray++ = indexCount + 2;
			indexCount += 3;
		}
	}
	break;
	case GL_TRIANGLE_STRIP:
	{
		int vcount = (( vertexCount - vertexMark ) - 3 );

		*ptrIndexArray++ = indexCount;
		*ptrIndexArray++ = indexCount + 1;
		*ptrIndexArray++ = indexCount + 2;
		indexCount += 3;

		for( int count = 0; count < vcount; count++ )
		{
			if( count & 1 )
			{
				*ptrIndexArray++ = indexCount - 2;
				*ptrIndexArray++ = indexCount - 1;
			}
			else
			{
				*ptrIndexArray++ = indexCount - 1;
				*ptrIndexArray++ = indexCount - 2;
			}
			*ptrIndexArray++ = indexCount;
			indexCount++;
		}
		vertexCount += ( vertexCount - vertexMark - 3 ) * 2;
	}
	break;
	case GL_POLYGON:
	case GL_TRIANGLE_FAN:
	{
		*ptrIndexArray++ = indexCount++;
		*ptrIndexArray++ = indexCount++;
		*ptrIndexArray++ = indexCount++;
		int vcount = (( vertexCount - vertexMark ) - 3 );
		for( int count = 0; count < vcount; count++ )
		{
			*ptrIndexArray++ = indexbase;
			*ptrIndexArray++ = indexCount - 1;
			*ptrIndexArray++ = indexCount++;
			vertexCount += 2;
		}
	}
	break;
	case GL_POINTS:
	{
		int vcount = ( vertexCount - vertexMark );

		for( int count = 0; count < vcount; count++ )
			*ptrIndexArray++ = indexCount++;
	}
	break;
	case GL_LINES:
	{
		int vcount = ( vertexCount - vertexMark ) & ~1;

		for( int count = 0; count < vcount; count++ )
			*ptrIndexArray++ = indexCount++;

		if(( vertexCount - vertexMark ) & 1 )
		{
			vertexCount--;
			ptrVertexAttribArray--;
		}
	}
	break;
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	{
		int vcount = (( vertexCount - vertexMark ) - 1 );

		for( int count = 0; count < vcount; count++ )
		{
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 1;
			indexCount++;
		}
		indexCount++;

		if( wrapperPrimitiveMode == GL_LINE_LOOP )
		{
			*ptrIndexArray++ = indexCount - 1;
			*ptrIndexArray++ = indexbase;
			vertexCount += vcount + 1;
		}
		else
		{
			vertexCount += vcount - 1;
		}
	}
	break;

	default:
		vertexCount = vertexMark;
		ptrVertexAttribArray = ptrVertexAttribArrayMark;
		break;
	}
	if((size_t)( ptrVertexAttribArray - vertexattribs ) > ( sizeof( vertexattribs ) / sizeof( vertexattribs[0] )) - 4096 || (size_t)( ptrIndexArray - indexArray ) > ( sizeof( indexArray ) / sizeof( indexArray[0] )) - 4096 )
		FlushOnStateChange();
}

void GL_MANGLE( glEnable )( GLenum cap )
{
	if( skipnanogl )
	{
		glEsImpl->glEnable( cap );
		return;
	}
	GLboolean statechanged = GL_FALSE;
	switch( cap )
	{
	case GL_ALPHA_TEST:
	{
		if( !nanoglState.alpha_test )
		{
			nanoglState.alpha_test = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_BLEND:
	{
		if( !nanoglState.blend )
		{
			nanoglState.blend = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	// case GL_CLIP_PLANEi
	case GL_COLOR_LOGIC_OP:
	{
		if( !nanoglState.color_logic_op )
		{
			nanoglState.color_logic_op = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_COLOR_MATERIAL:
	{
		if( !nanoglState.color_material )
		{
			nanoglState.color_material = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_CULL_FACE:
	{
		if( !nanoglState.cull_face )
		{
			nanoglState.cull_face = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_DEPTH_TEST:
	{
		if( !nanoglState.depth_test )
		{
			nanoglState.depth_test = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_DITHER:
	{
		if( !nanoglState.dither )
		{
			nanoglState.dither = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_FOG:
	{
		if( !nanoglState.fog )
		{
			nanoglState.fog = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT0:
	{
		if( !nanoglState.light0 )
		{
			nanoglState.light0 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT1:
	{
		if( !nanoglState.light1 )
		{
			nanoglState.light1 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT2:
	{
		if( !nanoglState.light2 )
		{
			nanoglState.light2 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT3:
	{
		if( !nanoglState.light3 )
		{
			nanoglState.light3 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT4:
	{
		if( !nanoglState.light4 )
		{
			nanoglState.light4 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT5:
	{
		if( !nanoglState.light5 )
		{
			nanoglState.light5 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT6:
	{
		if( !nanoglState.light6 )
		{
			nanoglState.light6 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT7:
	{
		if( !nanoglState.light7 )
		{
			nanoglState.light7 = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHTING:
	{
		if( !nanoglState.lighting )
		{
			nanoglState.lighting = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LINE_SMOOTH:
	{
		if( !nanoglState.line_smooth )
		{
			nanoglState.line_smooth = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	/*        case GL_MATRIX_PALETTE_OES:
		{
		if (!nanoglState.matrix_palette_oes)
		    {
		    nanoglState.matrix_palette_oes = GL_TRUE;
		    statechanged = GL_TRUE;
		    }
		break;
		}*/
	case GL_MULTISAMPLE:
	{
		if( !nanoglState.multisample )
		{
			nanoglState.multisample = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_NORMALIZE:
	{
		if( !nanoglState.normalize )
		{
			nanoglState.normalize = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	/*        case GL_POINT_SPRITE_OES:
		{
		if (!nanoglState.point_sprite_oes)
		    {
		    nanoglState.point_sprite_oes = GL_TRUE;
		    statechanged = GL_TRUE;
		    }
		break;
		}*/
	case GL_POLYGON_OFFSET_FILL:
	{
		if( !nanoglState.polygon_offset_fill )
		{
			nanoglState.polygon_offset_fill = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_RESCALE_NORMAL:
	{
		if( !nanoglState.rescale_normal )
		{
			nanoglState.rescale_normal = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		if( !nanoglState.sample_alpha_to_coverage )
		{
			nanoglState.sample_alpha_to_coverage = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		if( !nanoglState.sample_alpha_to_one )
		{
			nanoglState.sample_alpha_to_one = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_COVERAGE:
	{
		if( !nanoglState.sample_coverage )
		{
			nanoglState.sample_coverage = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SCISSOR_TEST:
	{
		if( !nanoglState.scissor_test )
		{
			nanoglState.scissor_test = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_STENCIL_TEST:
	{
		if( !nanoglState.stencil_test )
		{
			nanoglState.stencil_test = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_TEXTURE_2D:
	{
		if( !activetmuState->texture_2d.value )
		{
			FlushOnStateChange();
			glEsImpl->glEnable( cap );
			activetmuState->texture_2d.value = GL_TRUE;
			return;
		}
		break;
	}
#if 0 // todo: implement cubemap texgen
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
	{
		FlushOnStateChange( );
		nanoglState.texgen = true;
		return;
	}
#endif
	default:
		break;
	}

	if( statechanged )
	{
		FlushOnStateChange();
		glEsImpl->glEnable( cap );
	}
}

void GL_MANGLE( glDisable )( GLenum cap )
{
	if( skipnanogl )
	{
		glEsImpl->glDisable( cap );
		return;
	}
	GLboolean statechanged = GL_FALSE;
	switch( cap )
	{
	case GL_ALPHA_TEST:
	{
		if( nanoglState.alpha_test )
		{
			nanoglState.alpha_test = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_BLEND:
	{
		if( nanoglState.blend )
		{
			nanoglState.blend = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	// case GL_CLIP_PLANEi
	case GL_COLOR_LOGIC_OP:
	{
		if( nanoglState.color_logic_op )
		{
			nanoglState.color_logic_op = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_COLOR_MATERIAL:
	{
		if( nanoglState.color_material )
		{
			nanoglState.color_material = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_CULL_FACE:
	{
		if( nanoglState.cull_face )
		{
			nanoglState.cull_face = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_DEPTH_TEST:
	{
		if( nanoglState.depth_test )
		{
			nanoglState.depth_test = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_DITHER:
	{
		if( nanoglState.dither )
		{
			nanoglState.dither = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_FOG:
	{
		if( nanoglState.fog )
		{
			nanoglState.fog = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT0:
	{
		if( !nanoglState.light0 )
		{
			nanoglState.light0 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT1:
	{
		if( !nanoglState.light1 )
		{
			nanoglState.light1 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT2:
	{
		if( !nanoglState.light2 )
		{
			nanoglState.light2 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT3:
	{
		if( !nanoglState.light3 )
		{
			nanoglState.light3 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT4:
	{
		if( !nanoglState.light4 )
		{
			nanoglState.light4 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT5:
	{
		if( !nanoglState.light5 )
		{
			nanoglState.light5 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT6:
	{
		if( !nanoglState.light6 )
		{
			nanoglState.light6 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT7:
	{
		if( !nanoglState.light7 )
		{
			nanoglState.light7 = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LIGHTING:
	{
		if( nanoglState.lighting )
		{
			nanoglState.lighting = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_LINE_SMOOTH:
	{
		if( nanoglState.line_smooth )
		{
			nanoglState.line_smooth = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	/*        case GL_MATRIX_PALETTE_OES:
		{
		if (nanoglState.matrix_palette_oes)
		    {
		    nanoglState.matrix_palette_oes = GL_FALSE;
		    statechanged = GL_TRUE;
		    }
		break;
		}*/
	case GL_MULTISAMPLE:
	{
		if( nanoglState.multisample )
		{
			nanoglState.multisample = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_NORMALIZE:
	{
		if( nanoglState.normalize )
		{
			nanoglState.normalize = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	/*        case GL_POINT_SPRITE_OES:
		{
		if (nanoglState.point_sprite_oes)
		    {
		    nanoglState.point_sprite_oes = GL_FALSE;
		    statechanged = GL_TRUE;
		    }
		break;
		}*/
	case GL_POLYGON_OFFSET_FILL:
	{
		if( nanoglState.polygon_offset_fill )
		{
			nanoglState.polygon_offset_fill = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_RESCALE_NORMAL:
	{
		if( nanoglState.rescale_normal )
		{
			nanoglState.rescale_normal = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		if( nanoglState.sample_alpha_to_coverage )
		{
			nanoglState.sample_alpha_to_coverage = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		if( nanoglState.sample_alpha_to_one )
		{
			nanoglState.sample_alpha_to_one = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_COVERAGE:
	{
		if( nanoglState.sample_coverage )
		{
			nanoglState.sample_coverage = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_SCISSOR_TEST:
	{
		if( nanoglState.scissor_test )
		{
			nanoglState.scissor_test = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_STENCIL_TEST:
	{
		if( nanoglState.stencil_test )
		{
			nanoglState.stencil_test = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_TEXTURE_2D:
	{
		if( activetmuState->texture_2d.value )
		{
			FlushOnStateChange();
			glEsImpl->glDisable( cap );
			activetmuState->texture_2d.value = GL_FALSE;
			return;
		}
		break;
	}
#if 0
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
	{
		FlushOnStateChange( );
		nanoglState.texgen = false;
		return;
	}
#endif
	default:
		break;
	}

	if( statechanged )
	{
		FlushOnStateChange();
		glEsImpl->glDisable( cap );
	}
}

void GL_MANGLE( glVertex2f )( GLfloat x, GLfloat y )
{
	GL_MANGLE_NAME( glVertex3f )( x, y, 0.0f );
}

static inline unsigned int ClampTo255( float value )
{
	if( value <= 0.0f )
		return 0;
	if( value >= 255.0f )
		return 255;
	return (unsigned int)value;
}

void GL_MANGLE( glColor3f )( GLfloat red, GLfloat green, GLfloat blue )
{
	currentVertexAttrib.red = (unsigned char)ClampTo255( red * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( green * 255.0f );
	currentVertexAttrib.blue = (unsigned char)ClampTo255( blue * 255.0f );
	currentVertexAttrib.alpha = 255;
}

static void SetCurrentTexCoord( GLenum target, const GLfloat *coords, GLint count )
{
	GLfloat *destination;
	const GLfloat defaults[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	if( target == GL_TEXTURE0 )
		destination = &currentVertexAttrib.s;
	else if( target == GL_TEXTURE1 )
		destination = &currentVertexAttrib.s_multi;
	else
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}

	memcpy( destination, defaults, sizeof( defaults ));
	memcpy( destination, coords, (size_t)count * sizeof( GLfloat ));

	if( skipnanogl )
		glEsImpl->glMultiTexCoord4f( target, destination[0], destination[1], destination[2], destination[3] );
}

void GL_MANGLE( glTexCoord1f )( GLfloat s )
{
	SetCurrentTexCoord( activetmu, &s, 1 );
}

void GL_MANGLE( glTexCoord2f )( GLfloat s, GLfloat t )
{
	const GLfloat coords[2] = { s, t };
	SetCurrentTexCoord( activetmu, coords, 2 );
}

void GL_MANGLE( glTexCoord3f )( GLfloat s, GLfloat t, GLfloat r )
{
	const GLfloat coords[3] = { s, t, r };
	SetCurrentTexCoord( activetmu, coords, 3 );
}

void GL_MANGLE( glTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	const GLfloat coords[4] = { s, t, r, q };
	SetCurrentTexCoord( activetmu, coords, 4 );
}

void GL_MANGLE( glTexCoord1fv )( const GLfloat *v )
{
	SetCurrentTexCoord( activetmu, v, 1 );
}

void GL_MANGLE( glTexCoord2fv )( const GLfloat *v )
{
	SetCurrentTexCoord( activetmu, v, 2 );
}

void GL_MANGLE( glTexCoord3fv )( const GLfloat *v )
{
	SetCurrentTexCoord( activetmu, v, 3 );
}

void GL_MANGLE( glTexCoord4fv )( const GLfloat *v )
{
	SetCurrentTexCoord( activetmu, v, 4 );
}

void GL_MANGLE( glViewport )( GLint x, GLint y, GLsizei width, GLsizei height )
{
	if( !skipnanogl && nanoglState.viewport[0] == x && nanoglState.viewport[1] == y && nanoglState.viewport[2] == width && nanoglState.viewport[3] == height )
	{
		return;
	}
	nanoglState.viewport[0] = x;
	nanoglState.viewport[1] = y;
	nanoglState.viewport[2] = width;
	nanoglState.viewport[3] = height;
	FlushOnStateChange();
	glEsImpl->glViewport( x, y, width, height );
}

void GL_MANGLE( glLoadIdentity )( void )
{
	FlushOnStateChange();
	glEsImpl->glLoadIdentity();
}

void GL_MANGLE( glColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
	currentVertexAttrib.red = (unsigned char)ClampTo255( red * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( green * 255.0f );
	currentVertexAttrib.blue = (unsigned char)ClampTo255( blue * 255.0f );
	currentVertexAttrib.alpha = (unsigned char)ClampTo255( alpha * 255.0f );
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	FlushOnStateChange();
#ifdef USE_CORE_PROFILE
	glEsImpl->glOrtho( left, right, bottom, top, zNear, zFar );
#else
	glEsImpl->glOrthof( left, right, bottom, top, zNear, zFar );
#endif
}

// Rikku2000: Light
void GL_MANGLE( glLightf )( GLenum light, GLenum pname, GLfloat param )
{
	FlushOnStateChange();

	glEsImpl->glLightf( light, pname, param );
}

void GL_MANGLE( glLightfv )( GLenum light, GLenum pname, const GLfloat *params )
{
	FlushOnStateChange();

	glEsImpl->glLightfv( light, pname, params );
}

void GL_MANGLE( glLightModelf )( GLenum pname, GLfloat param )
{
	FlushOnStateChange();

	glEsImpl->glLightModelf( pname, param );
}

void GL_MANGLE( glLightModelfv )( GLenum pname, const GLfloat *params )
{
	FlushOnStateChange();

	glEsImpl->glLightModelfv( pname, params );
}

void GL_MANGLE( glMaterialf )( GLenum face, GLenum pname, GLfloat param )
{
	FlushOnStateChange();

	glEsImpl->glMaterialf( face, pname, param );
}

void GL_MANGLE( glMaterialfv )( GLenum face, GLenum pname, const GLfloat *params )
{
	FlushOnStateChange();

	glEsImpl->glMaterialfv( face, pname, params );
}

void GL_MANGLE( glColorMaterial )( GLenum face, GLenum mode )
{
	// GLES 1.1 fixes color material tracking to front-and-back ambient+diffuse.
	// Enabling GL_COLOR_MATERIAL already provides this behavior; there is no
	// corresponding GLES glColorMaterial entry point.
	if( face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	if( mode != GL_AMBIENT && mode != GL_DIFFUSE && mode != GL_SPECULAR && mode != GL_EMISSION && mode != GL_AMBIENT_AND_DIFFUSE )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	if( face != GL_FRONT_AND_BACK || mode != GL_AMBIENT_AND_DIFFUSE )
		SetWrapperError( GL_INVALID_OPERATION );
}

void GL_MANGLE( glMatrixMode )( GLenum mode )
{
	if( nanoglState.matrixmode == mode )
	{
		return;
	}
	nanoglState.matrixmode = mode;
	FlushOnStateChange();
	glEsImpl->glMatrixMode( mode );
}

void GL_MANGLE( glTexParameterf )( GLenum target, GLenum pname, GLfloat param )
{
	if( pname == GL_TEXTURE_BORDER_COLOR )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return; // desktop OpenGL border color is not supported by GLES
	}
	if(( pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ) && param == GL_CLAMP )
	{
		param = 0x812F;
	}

	FlushOnStateChange();
	glEsImpl->glTexParameterf( target, pname, param );
}

void GL_MANGLE( glTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params )
{
	GL_MANGLE_NAME( glTexParameterf )( target, pname, params[0] );
}

void GL_MANGLE( glTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	unsigned char *data = (unsigned char *)pixels;

	if( pixels && format == GL_RGBA && ( internalformat == GL_RGB || internalformat == GL_RGB8 || internalformat == GL_RGB5 || internalformat == GL_LUMINANCE || internalformat == GL_LUMINANCE8 || internalformat == GL_LUMINANCE4 )) // strip alpha from texture
	{
		const union
		{
			unsigned char b[4];
			unsigned int u;
		} alphamask = { { 0, 0, 0, 255 } };

		const unsigned int *restrict in = (const unsigned int *)pixels;
		unsigned int *restrict out;
		int size = width * height;

		data = (unsigned char *)malloc( size * 4 );
		out = (unsigned int *)data;

		// help compiler to vectorize the loop...
		for( int i = 0; i < size; i++ )
			out[i] = in[i] | alphamask.u;
	}

	internalformat = format;
	glEsImpl->glTexImage2D( target, level, internalformat, width, height, border, format, type, data );

	if( data != pixels )
		free( data );
}

void GL_MANGLE( glDrawBuffer )( GLenum mode )
{
	// GLES has one draw buffer; FRONT/BACK select the same platform target.
	if( mode != GL_FRONT && mode != GL_BACK )
		SetWrapperError( GL_INVALID_ENUM );
}

void GL_MANGLE( glTranslatef )( GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange();
	glEsImpl->glTranslatef( x, y, z );
}

void GL_MANGLE( glRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange();
	glEsImpl->glRotatef( angle, x, y, z );
}

void GL_MANGLE( glScalef )( GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange();
	glEsImpl->glScalef( x, y, z );
}

void GL_MANGLE( glDepthRange )( GLclampd zNear, GLclampd zFar )
{
	if(( nanoglState.depth_range_near == zNear ) && ( nanoglState.depth_range_far == zFar ))
	{
		return;
	}
	else
	{
		nanoglState.depth_range_near = zNear;
		nanoglState.depth_range_far = zFar;
	}
	FlushOnStateChange();
#ifdef USE_CORE_PROFILE
	glEsImpl->glDepthRange( zNear, zFar );
#else
	glEsImpl->glDepthRangef( zNear, zFar );
#endif
}

void GL_MANGLE( glDepthFunc )( GLenum func )
{
	if( nanoglState.depth_func == func )
	{
		return;
	}
	else
	{
		nanoglState.depth_func = func;
	}
	FlushOnStateChange();
	glEsImpl->glDepthFunc( func );
}

void GL_MANGLE( glFinish )( void )
{
	FlushOnStateChange();
	glEsImpl->glFinish();
}

void GL_MANGLE( glGetFloatv )( GLenum pname, GLfloat *params )
{
	FlushOnStateChange();
	glEsImpl->glGetFloatv( pname, params );
}

void GL_MANGLE( glCullFace )( GLenum mode )
{
	if( nanoglState.cullface == mode )
	{
		return;
	}
	else
	{
		nanoglState.cullface = mode;
	}
	FlushOnStateChange();
	glEsImpl->glCullFace( mode );
}

void GL_MANGLE( glFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	FlushOnStateChange();
	glEsImpl->glFrustumf( left, right, bottom, top, zNear, zFar );
}

void GL_MANGLE( glClear )( GLbitfield mask )
{
	FlushOnStateChange();
	glEsImpl->glClear( mask );
}

// a1ba: xash3d-fwgs emits large glBegin/glEnd block in net_graph 2, overflowing the
// batch buffer. quads are one of the most used primitives and each quad is standalone
// so we can catch quad boundary and do an implicit flush and continue.
// we can in theory do that for GL_TRIANGLES but as far as I remember, nothing seriously uses GL_TRIANGLES
static void CheckQuadsOverflow( void )
{
	if( wrapperPrimitiveMode == GL_QUADS && (( ptrVertexAttribArray - ptrVertexAttribArrayMark ) % 4 ) == 0 )
	{
		// indices aren't written until glEnd, so try to calculate the final indices size
		size_t indices = ( ptrIndexArray - indexArray ) + ( ptrVertexAttribArray - ptrVertexAttribArrayMark ) / 4 * 6;

		// 4096 just gives a small headroom in case of Begin/End blocks larger than usual...
		if((size_t)( ptrVertexAttribArray - vertexattribs ) > ( sizeof( vertexattribs ) / sizeof( vertexattribs[0] )) - 4096 || indices > ( sizeof( indexArray ) / sizeof( indexArray[0] )) - 4096 )
		{
			GL_MANGLE_NAME( glEnd )();
			FlushOnStateChange();
			GL_MANGLE_NAME( glBegin )( GL_QUADS );
		}
	}
}

static void StoreVertex( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	CheckQuadsOverflow();

	GLfloat *vert = (GLfloat *)ptrVertexAttribArray++;
	*vert++ = x;
	*vert++ = y;
	*vert++ = z;
	*vert++ = w;
	memcpy( vert, &currentVertexAttrib.nx, sizeof( VertexAttrib ) - 4 * sizeof( GLfloat ));
}

void GL_MANGLE( glVertex3f )( GLfloat x, GLfloat y, GLfloat z )
{
	StoreVertex( x, y, z, 1.0f );
}

void GL_MANGLE( glVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	StoreVertex( x, y, z, w );
}

void GL_MANGLE( glVertex4fv )( const GLfloat *v )
{
	StoreVertex( v[0], v[1], v[2], v[3] );
}

void GL_MANGLE( glColor4fv )( const GLfloat *v )
{
	currentVertexAttrib.red = (unsigned char)ClampTo255( v[0] * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( v[1] * 255.0f );
	currentVertexAttrib.blue = (unsigned char)ClampTo255( v[2] * 255.0f );
	currentVertexAttrib.alpha = (unsigned char)ClampTo255( v[3] * 255.0f );
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glColor3ubv )( const GLubyte *v )
{
	currentVertexAttrib.red = v[0];
	currentVertexAttrib.green = v[1];
	currentVertexAttrib.blue = v[2];
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glColor4ubv )( const GLubyte *v )
{
	// *((unsigned int*)(&currentVertexAttrib.red)) = *((unsigned int*)(v));
	currentVertexAttrib.red = v[0];
	currentVertexAttrib.green = v[1];
	currentVertexAttrib.blue = v[2];
	currentVertexAttrib.alpha = v[3];
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glColor3fv )( const GLfloat *v )
{
	currentVertexAttrib.red = (unsigned char)ClampTo255( v[0] * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( v[1] * 255.0f );
	currentVertexAttrib.blue = (unsigned char)ClampTo255( v[2] * 255.0f );
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

// -- nicknekit: xash3d funcs --

void GL_MANGLE( glColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
	currentVertexAttrib.red = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue = blue;
	currentVertexAttrib.alpha = alpha;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glColor3ub )( GLubyte red, GLubyte green, GLubyte blue )
{
	currentVertexAttrib.red = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue = blue;
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz )
{
	currentVertexAttrib.nx = nx;
	currentVertexAttrib.ny = ny;
	currentVertexAttrib.nz = nz;
	if( skipnanogl )
		glEsImpl->glNormal3f( nx, ny, nz );
}

void GL_MANGLE( glNormal3fv )( const GLfloat *v )
{
	GL_MANGLE_NAME( glNormal3f )( v[0], v[1], v[2] );
}

void GL_MANGLE( glCopyTexImage2D )( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
	FlushOnStateChange();
	glEsImpl->glCopyTexImage2D( target, level, internalformat, x, y, width, height, border );
}

void GL_MANGLE( glTexImage1D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	(void)target;
	GL_MANGLE_NAME( glTexImage2D )( GL_TEXTURE_2D, level, internalformat, width, 1, border, format, type, pixels );
}

void GL_MANGLE( glTexImage3D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	(void)target;
	if( depth != 1 )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	GL_MANGLE_NAME( glTexImage2D )( GL_TEXTURE_2D, level, internalformat, width, height, border, format, type, pixels );
}

void GL_MANGLE( glTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels )
{
	GL_MANGLE_NAME( glTexSubImage2D )( target, level, xoffset, 0, width, 1, format, type, pixels );
}

void GL_MANGLE( glTexSubImage3D )( GLenum target, GLint level,
	GLint xoffset, GLint yoffset,
	GLint zoffset, GLsizei width,
	GLsizei height, GLsizei depth,
	GLenum format,
	GLenum type, const GLvoid *pixels )
{
	if( zoffset != 0 || depth != 1 )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	GL_MANGLE_NAME( glTexSubImage2D )( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

GLboolean GL_MANGLE( glIsTexture )( GLuint texture )
{
	FlushOnStateChange();
	return glEsImpl->glIsTexture( texture );
}

// TODO: add native normal/reflection map texgen support

void GL_MANGLE( glTexGeni )( GLenum coord, GLenum pname, GLint param )
{
	(void)coord;
	(void)pname;
	(void)param;
	SetWrapperError( GL_INVALID_OPERATION );
}

void GL_MANGLE( glTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params )
{
	(void)coord;
	(void)pname;
	(void)params;
	SetWrapperError( GL_INVALID_OPERATION );
}

// -- --//

void GL_MANGLE( glHint )( GLenum target, GLenum mode )
{
	FlushOnStateChange();
	glEsImpl->glHint( target, mode );
}

void GL_MANGLE( glBlendFunc )( GLenum sfactor, GLenum dfactor )
{
	if( skipnanogl )
	{
		glEsImpl->glBlendFunc( sfactor, dfactor );
		return;
	}
	if(( nanoglState.sfactor == sfactor ) && ( nanoglState.dfactor == dfactor ))
	{
		return;
	}

	nanoglState.sfactor = sfactor;
	nanoglState.dfactor = dfactor;
	FlushOnStateChange();
	glEsImpl->glBlendFunc( sfactor, dfactor );
}

void GL_MANGLE( glPopMatrix )( void )
{
	FlushOnStateChange();
	glEsImpl->glPopMatrix();
}

void GL_MANGLE( glShadeModel )( GLenum mode )
{
	if( nanoglState.shademodel == mode )
	{
		return;
	}
	nanoglState.shademodel = mode;
	FlushOnStateChange();
	glEsImpl->glShadeModel( mode );
}

void GL_MANGLE( glPushMatrix )( void )
{
	FlushOnStateChange();
	glEsImpl->glPushMatrix();
}

void GL_MANGLE( glTexEnvf )( GLenum target, GLenum pname, GLfloat param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvf( target, pname, param );
		return;
	}
	if( target == GL_TEXTURE_ENV )
	{
		if( pname == GL_TEXTURE_ENV_MODE )
		{
			if( param == activetmuState->texture_env_mode.value )
			{
				return;
			}
			else
			{
				FlushOnStateChange();
				glEsImpl->glTexEnvf( target, pname, param );
				activetmuState->texture_env_mode.value = param;
				return;
			}
		}
	}
	FlushOnStateChange();
	glEsImpl->glTexEnvf( target, pname, param );
}

void GL_MANGLE( glVertex3fv )( const GLfloat *v )
{
	StoreVertex( v[0], v[1], v[2], 1.0f );
}

void GL_MANGLE( glDepthMask )( GLboolean flag )
{
	if( !skipnanogl )
	{
		if( nanoglState.depthmask == flag )
		{
			return;
		}
		nanoglState.depthmask = flag;
		FlushOnStateChange();
	}
	glEsImpl->glDepthMask( flag );
}

void GL_MANGLE( glBindTexture )( GLenum target, GLuint texture )
{
	if( skipnanogl )
	{
		glEsImpl->glBindTexture( target, texture );
		activetmuState->boundtexture.value = texture;
		return;
	}
	if( activetmuState->boundtexture.value == texture )
	{
		return;
	}
	FlushOnStateChange();
	activetmuState->boundtexture.value = texture;
	glEsImpl->glBindTexture( target, texture );
}

void GL_MANGLE( glGetIntegerv )( GLenum pname, GLint *params )
{
	FlushOnStateChange();
	glEsImpl->glGetIntegerv( pname, params );
}

const GLubyte *GL_MANGLE( glGetString )( GLenum name )
{
	if( name == GL_EXTENSIONS && nano_extensions_string )
		return (const GLubyte *)nano_extensions_string;
	return glEsImpl->glGetString( name );
}

void GL_MANGLE( glAlphaFunc )( GLenum func, GLclampf ref )
{
	if( !skipnanogl && nanoglState.alphafunc == func && nanoglState.alpharef == ref )
	{
		return;
	}
	nanoglState.alphafunc = func;
	nanoglState.alpharef = ref;
	FlushOnStateChange();
	glEsImpl->glAlphaFunc( func, ref );
}

void GL_MANGLE( glFlush )( void )
{
	FlushOnStateChange();
	glEsImpl->glFlush();
}

void GL_MANGLE( glReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels )
{
	if( format == GL_DEPTH_COMPONENT )
	{
		// GLES 1.1 has no depth readback command. Never write a guessed fixed
		// byte count into the caller's buffer.
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	FlushOnStateChange();
	glEsImpl->glReadPixels( x, y, width, height, format, type, pixels );
}

void GL_MANGLE( glReadBuffer )( GLenum mode )
{
	// GLES has one read buffer; FRONT/BACK select the same platform target.
	if( mode != GL_FRONT && mode != GL_BACK )
		SetWrapperError( GL_INVALID_ENUM );
}

void GL_MANGLE( glLoadMatrixf )( const GLfloat *m )
{
	FlushOnStateChange();
	glEsImpl->glLoadMatrixf( m );
}

void GL_MANGLE( glTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
	FlushOnStateChange();
	glEsImpl->glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

void GL_MANGLE( glClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
	FlushOnStateChange();
	glEsImpl->glClearColor( red, green, blue, alpha );
}

GLenum GL_MANGLE( glGetError )( void )
{
	if( nanoWrapperError != GL_NO_ERROR )
	{
		GLenum error = nanoWrapperError;
		nanoWrapperError = GL_NO_ERROR;
		return error;
	}
	return glEsImpl->glGetError();
}

void GL_MANGLE( glActiveTexture )( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glActiveTexture( texture );
		return;
	}
	if( activetmu == texture )
	{
		return;
	}
	if( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
	}
	else
	{
		delayedttmuchange = GL_TRUE;
		delayedtmutarget = texture;
	}
	if( texture == GL_TEXTURE0 )
	{
		activetmuState = &tmuState0;
	}
	else
	{
		activetmuState = &tmuState1;
	}
	activetmu = texture;
}

void GL_MANGLE( glActiveTextureARB )( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glActiveTexture( texture );
		return;
	}
	if( activetmu == texture )
	{
		return;
	}
	if( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
	}
	else
	{
		delayedttmuchange = GL_TRUE;
		delayedtmutarget = texture;
	}
	if( texture == GL_TEXTURE0 )
	{
		activetmuState = &tmuState0;
	}
	else
	{
		activetmuState = &tmuState1;
	}
	activetmu = texture;
}

void GL_MANGLE( glClientActiveTexture )( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glClientActiveTexture( texture );
		return;
	}
	clientactivetmu = texture;
}

void GL_MANGLE( glClientActiveTextureARB )( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glClientActiveTexture( texture );
		return;
	}
	clientactivetmu = texture;
}

void GL_MANGLE( glPolygonMode )( GLenum face, GLenum mode )
{
	if( face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	if( mode != GL_POINT && mode != GL_LINE && mode != GL_FILL )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	if( mode != GL_FILL )
		SetWrapperError( GL_INVALID_OPERATION );
}

void GL_MANGLE( glDeleteTextures )( GLsizei n, const GLuint *textures )
{
	FlushOnStateChange();
	glEsImpl->glDeleteTextures( n, textures );
}

void GL_MANGLE( glClearDepth )( GLclampd depth )
{
	FlushOnStateChange();
	glEsImpl->glClearDepthf( depth );
}

void GL_MANGLE( glClipPlane )( GLenum plane, const GLdouble *equation )
{
	FlushOnStateChange();
	float array[4];
	array[0] = (GLfloat)( equation[0] );
	array[1] = (GLfloat)( equation[1] );
	array[2] = (GLfloat)( equation[2] );
	array[3] = (GLfloat)( equation[3] );
	glEsImpl->glClipPlanef( plane, array );
}

void GL_MANGLE( glScissor )( GLint x, GLint y, GLsizei width, GLsizei height )
{
	if( !skipnanogl && nanoglState.scissorbox[0] == x && nanoglState.scissorbox[1] == y && nanoglState.scissorbox[2] == width && nanoglState.scissorbox[3] == height )
	{
		return;
	}
	nanoglState.scissorbox[0] = x;
	nanoglState.scissorbox[1] = y;
	nanoglState.scissorbox[2] = width;
	nanoglState.scissorbox[3] = height;
	FlushOnStateChange();
	glEsImpl->glScissor( x, y, width, height );
}

void GL_MANGLE( glPointSize )( GLfloat size )
{
	FlushOnStateChange();
	glEsImpl->glPointSize( size );
}

static size_t ArrayElementTypeSize( GLenum type )
{
	switch( type )
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		return 1;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
		return 2;
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
	case GL_FIXED:
		return 4;
	case GL_DOUBLE:
		return 8;
	default:
		return 0;
	}
}

static const GLubyte *ArrayElementAddress( const struct ptrstate *array, GLint index, GLint components )
{
	if( index < 0 || array->stride < 0 || !array->ptr )
	{
		SetWrapperError( index < 0 || array->stride < 0 ? GL_INVALID_VALUE : GL_INVALID_OPERATION );
		return NULL;
	}

	size_t typeSize = ArrayElementTypeSize( array->type );
	if( !typeSize )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return NULL;
	}

	size_t stride = array->stride ? (size_t)array->stride : (size_t)components * typeSize;
	if( stride && (size_t)index > ( SIZE_MAX - (uintptr_t)array->ptr ) / stride )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return NULL;
	}
	return (const GLubyte *)array->ptr + (size_t)index * stride;
}

static GLfloat ReadArrayComponent( const GLubyte *data, GLenum type, GLboolean normalize )
{
	switch( type )
	{
	case GL_BYTE:
	{
		GLbyte value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? ( value == -128 ? -1.0f : value / 127.0f ) : (GLfloat)value;
	}
	case GL_UNSIGNED_BYTE:
	{
		GLubyte value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? value / 255.0f : (GLfloat)value;
	}
	case GL_SHORT:
	{
		GLshort value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? ( value == -32768 ? -1.0f : value / 32767.0f ) : (GLfloat)value;
	}
	case GL_UNSIGNED_SHORT:
	{
		GLushort value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? value / 65535.0f : (GLfloat)value;
	}
	case GL_INT:
	{
		GLint value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? ( value == INT_MIN ? -1.0f : (GLfloat)((double)value / INT_MAX) ) : (GLfloat)value;
	}
	case GL_UNSIGNED_INT:
	{
		GLuint value;
		memcpy( &value, data, sizeof( value ));
		return normalize ? (GLfloat)((double)value / UINT_MAX) : (GLfloat)value;
	}
	case GL_FLOAT:
	{
		GLfloat value;
		memcpy( &value, data, sizeof( value ));
		return value;
	}
	case GL_DOUBLE:
	{
		GLdouble value;
		memcpy( &value, data, sizeof( value ));
		return (GLfloat)value;
	}
	case GL_FIXED:
	{
		GLfixed value;
		memcpy( &value, data, sizeof( value ));
		return value / 65536.0f;
	}
	default:
		return 0.0f;
	}
}

void GL_MANGLE( glArrayElement )( GLint i )
{
	if( skipnanogl )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}

	struct nanotmuState *texState = clientactivetmu == GL_TEXTURE0 ? &tmuState0 :
		clientactivetmu == GL_TEXTURE1 ? &tmuState1 : NULL;
	if( !texState )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}

	const GLubyte *data;
	if( tmuState0.color_array.enabled )
	{
		struct ptrstate *array = &tmuState0.color_array;
		GLint components = array->size;
		if( components < 3 || components > 4 || !( data = ArrayElementAddress( array, i, components )))
		{
			if( components < 3 || components > 4 )
				SetWrapperError( GL_INVALID_VALUE );
			return;
		}
		size_t typeSize = ArrayElementTypeSize( array->type );
		GLfloat color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for( GLint c = 0; c < components; ++c )
			color[c] = ReadArrayComponent( data + (size_t)c * typeSize, array->type, GL_TRUE );
		GL_MANGLE_NAME( glColor4f )( color[0], color[1], color[2], color[3] );
	}

	if( tmuState0.normal_array.enabled )
	{
		struct ptrstate *array = &tmuState0.normal_array;
		if( !( data = ArrayElementAddress( array, i, 3 )))
			return;
		size_t typeSize = ArrayElementTypeSize( array->type );
		GL_MANGLE_NAME( glNormal3f )(
			ReadArrayComponent( data, array->type, GL_FALSE ),
			ReadArrayComponent( data + typeSize, array->type, GL_FALSE ),
			ReadArrayComponent( data + 2 * typeSize, array->type, GL_FALSE ));
	}

	if( texState->texture_coord_array.enabled )
	{
		struct ptrstate *array = &texState->texture_coord_array;
		GLint components = array->size;
		if( components < 1 || components > 4 || !( data = ArrayElementAddress( array, i, components )))
		{
			if( components < 1 || components > 4 )
				SetWrapperError( GL_INVALID_VALUE );
			return;
		}
		size_t typeSize = ArrayElementTypeSize( array->type );
		GLfloat coords[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for( GLint c = 0; c < components; ++c )
			coords[c] = ReadArrayComponent( data + (size_t)c * typeSize, array->type, GL_FALSE );
		SetCurrentTexCoord( clientactivetmu, coords, 4 );
	}

	if( tmuState0.vertex_array.enabled )
	{
		struct ptrstate *array = &tmuState0.vertex_array;
		GLint components = array->size;
		if( components < 2 || components > 4 || !( data = ArrayElementAddress( array, i, components )))
		{
			if( components < 2 || components > 4 )
				SetWrapperError( GL_INVALID_VALUE );
			return;
		}
		size_t typeSize = ArrayElementTypeSize( array->type );
		GLfloat vertex[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for( GLint c = 0; c < components; ++c )
			vertex[c] = ReadArrayComponent( data + (size_t)c * typeSize, array->type, GL_FALSE );
		GL_MANGLE_NAME( glVertex4f )( vertex[0], vertex[1], vertex[2], vertex[3] );
	}
}

void GL_MANGLE( glLineWidth )( GLfloat width )
{
	FlushOnStateChange();
	glEsImpl->glLineWidth( width );
}

void GL_MANGLE( glCallList )( GLuint list )
{
	(void)list;
	SetWrapperError( GL_INVALID_OPERATION );
}

void GL_MANGLE( glColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
	FlushOnStateChange();
	glEsImpl->glColorMask( red, green, blue, alpha );
}

void GL_MANGLE( glStencilFunc )( GLenum func, GLint ref, GLuint mask )
{
	FlushOnStateChange();
	glEsImpl->glStencilFunc( func, ref, mask );
}

void GL_MANGLE( glStencilOp )( GLenum fail, GLenum zfail, GLenum zpass )
{
	FlushOnStateChange();
	glEsImpl->glStencilOp( fail, zfail, zpass );
}

struct ptrstate vertex_array;
struct ptrstate color_array;
struct ptrstate texture_coord_array;

static GLuint ReadDrawIndex( const GLvoid *indices, GLenum type, GLsizei index )
{
	switch( type )
	{
	case GL_UNSIGNED_BYTE:
		return ((const GLubyte *)indices)[index];
	case GL_UNSIGNED_SHORT:
	{
		GLushort value;
		memcpy( &value, (const GLubyte *)indices + (size_t)index * sizeof( value ), sizeof( value ));
		return value;
	}
	case GL_UNSIGNED_INT:
	{
		GLuint value;
		memcpy( &value, (const GLubyte *)indices + (size_t)index * sizeof( value ), sizeof( value ));
		return value;
	}
	default:
		return UINT_MAX;
	}
}

static GLushort *ExpandQuadIndices( GLenum *mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei *expandedCount )
{
	if( *mode != GL_QUADS && *mode != GL_QUAD_STRIP )
		return NULL;
	if( count < 0 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return NULL;
	}
	if( type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return NULL;
	}
	GLsizei quads = *mode == GL_QUADS ? count / 4 : ( count / 2 > 0 ? count / 2 - 1 : 0 );
	if( quads > INT_MAX / 6 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return NULL;
	}
	GLsizei outputCount = quads * 6;
	GLushort *output = outputCount ? (GLushort *)malloc((size_t)outputCount * sizeof( *output )) : NULL;
	if( outputCount && !output )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return NULL;
	}
	GLsizei out = 0;
	for( GLsizei q = 0; q < quads; ++q )
	{
		GLsizei base = *mode == GL_QUADS ? q * 4 : q * 2;
		GLuint a = ReadDrawIndex( indices, type, base );
		GLuint b = ReadDrawIndex( indices, type, base + 1 );
		GLuint c = *mode == GL_QUADS ? ReadDrawIndex( indices, type, base + 2 ) : ReadDrawIndex( indices, type, base + 3 );
		GLuint d = *mode == GL_QUADS ? ReadDrawIndex( indices, type, base + 3 ) : ReadDrawIndex( indices, type, base + 2 );
		if( a > USHRT_MAX || b > USHRT_MAX || c > USHRT_MAX || d > USHRT_MAX )
		{
			free( output );
			SetWrapperError( GL_INVALID_OPERATION );
			return NULL;
		}
		output[out++] = (GLushort)a;
		output[out++] = (GLushort)b;
		output[out++] = (GLushort)c;
		output[out++] = (GLushort)a;
		output[out++] = (GLushort)c;
		output[out++] = (GLushort)d;
	}
	*mode = GL_TRIANGLES;
	*expandedCount = outputCount;
	return output;
}

void GL_MANGLE( glDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	GLsizei drawCount = count;
	GLenum drawMode = mode;
	GLenum drawType = type;
	const GLvoid *drawIndices = indices;
	GLushort *expandedIndices = NULL;
	if( mode == GL_QUADS || mode == GL_QUAD_STRIP )
	{
		if( skipnanogl )
		{
			SetWrapperError( GL_INVALID_OPERATION );
			return; // GLES cannot expand quad modes when indices are in a VBO.
		}
		if( count > 0 && !indices )
		{
			SetWrapperError( GL_INVALID_OPERATION );
			return;
		}
		expandedIndices = ExpandQuadIndices( &drawMode, count, type, indices, &drawCount );
		if( drawMode != GL_TRIANGLES )
			return; // expansion rejected an invalid count/type or could not allocate.
		drawType = GL_UNSIGNED_SHORT;
		drawIndices = expandedIndices;
	}
	else if( mode == GL_POLYGON )
	{
		drawMode = GL_TRIANGLE_FAN;
	}
	if( skipnanogl )
	{
		glEsImpl->glDrawElements( drawMode, drawCount, drawType, drawIndices );
		free( expandedIndices );
		return;
	}
	// ensure that all primitives specified between glBegin/glEnd pairs
	// are rendered first, and that we have correct tmu in use..
	FlushOnStateChange();
	// setup correct vertex/color/texcoord pointers
	if( arraysValid || tmuState0.vertex_array.changed || tmuState0.color_array.changed || tmuState0.texture_coord_array.changed || tmuState0.normal_array.changed )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
	}
	if( arraysValid || tmuState0.vertex_array.changed )
	{
		if( tmuState0.vertex_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
		}
		glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
			tmuState0.vertex_array.type,
			tmuState0.vertex_array.stride,
			tmuState0.vertex_array.ptr );
		tmuState0.vertex_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.color_array.changed )
	{
		if( tmuState0.color_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
			glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
		}
		glEsImpl->glColorPointer( tmuState0.color_array.size,
			tmuState0.color_array.type,
			tmuState0.color_array.stride,
			tmuState0.color_array.ptr );
		tmuState0.color_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.normal_array.changed )
	{
		if( tmuState0.normal_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
		}
		glEsImpl->glNormalPointer( tmuState0.normal_array.type,
			tmuState0.normal_array.stride,
			tmuState0.normal_array.ptr );
		tmuState0.normal_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.texture_coord_array.changed )
	{
		tmuState0.texture_coord_array.changed = GL_FALSE;
		if( tmuState0.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
			tmuState0.texture_coord_array.type,
			tmuState0.texture_coord_array.stride,
			tmuState0.texture_coord_array.ptr );
	}

	if( arraysValid || tmuState1.texture_coord_array.changed )
	{
		tmuState1.texture_coord_array.changed = GL_FALSE;
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if( tmuState1.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState1.texture_coord_array.size,
			tmuState1.texture_coord_array.type,
			tmuState1.texture_coord_array.stride,
			tmuState1.texture_coord_array.ptr );
	}

	arraysValid = GL_FALSE;
	glEsImpl->glDrawElements( drawMode, drawCount, drawType, drawIndices );
	free( expandedIndices );
}

bool vboarray;

void GL_MANGLE( glEnableClientState )( GLenum array )
{
	if( skipnanogl )
	{
		glEsImpl->glEnableClientState( array );
		if( array == GL_VERTEX_ARRAY )
			vboarray = true;
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	else
	{
		return;
	}
	switch( array )
	{
	case GL_VERTEX_ARRAY:
		if( clientstate->vertex_array.enabled )
		{
			return;
		}
		clientstate->vertex_array.enabled = GL_TRUE;
		clientstate->vertex_array.changed = GL_TRUE;
		break;
	case GL_COLOR_ARRAY:
		if( clientstate->color_array.enabled )
		{
			return;
		}
		clientstate->color_array.enabled = GL_TRUE;
		clientstate->color_array.changed = GL_TRUE;

		break;
	case GL_NORMAL_ARRAY:
		if( clientstate->normal_array.enabled )
		{
			return;
		}
		clientstate->normal_array.enabled = GL_TRUE;
		clientstate->normal_array.changed = GL_TRUE;

		break;
	case GL_TEXTURE_COORD_ARRAY:
		if( clientstate->texture_coord_array.enabled )
		{
			return;
		}
		clientstate->texture_coord_array.enabled = GL_TRUE;
		clientstate->texture_coord_array.changed = GL_TRUE;
		break;
	default:
		break;
	}
}

void GL_MANGLE( glDisableClientState )( GLenum array )
{
	if( skipnanogl )
	{
		glEsImpl->glDisableClientState( array );
		if( array == GL_VERTEX_ARRAY )
			vboarray = false;
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	else
	{
		return;
	}
	switch( array )
	{
	case GL_VERTEX_ARRAY:
		if( !clientstate->vertex_array.enabled )
		{
			return;
		}
		clientstate->vertex_array.enabled = GL_FALSE;
		clientstate->vertex_array.changed = GL_TRUE;
		break;
	case GL_COLOR_ARRAY:
		if( !clientstate->color_array.enabled )
		{
			return;
		}
		clientstate->color_array.enabled = GL_FALSE;
		clientstate->color_array.changed = GL_TRUE;

		break;
	case GL_NORMAL_ARRAY:
		if( !clientstate->normal_array.enabled )
		{
			return;
		}
		clientstate->normal_array.enabled = GL_FALSE;
		clientstate->normal_array.changed = GL_TRUE;

		break;
	case GL_TEXTURE_COORD_ARRAY:
		if( !clientstate->texture_coord_array.enabled )
		{
			return;
		}
		clientstate->texture_coord_array.enabled = GL_FALSE;
		clientstate->texture_coord_array.changed = GL_TRUE;
		break;
	default:
		break;
	}
}

void GL_MANGLE( glVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	if( skipnanogl )
	{
		glEsImpl->glVertexPointer( size, type, stride, pointer );
		return;
	}
	if( tmuState0.vertex_array.size == size && tmuState0.vertex_array.stride == stride && tmuState0.vertex_array.type == type && tmuState0.vertex_array.ptr == pointer )
	{
		return;
	}
	tmuState0.vertex_array.size = size;
	tmuState0.vertex_array.stride = stride;
	tmuState0.vertex_array.type = type;
	tmuState0.vertex_array.ptr = (GLvoid *)pointer;
	tmuState0.vertex_array.changed = GL_TRUE;
}

void GL_MANGLE( glTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	if( skipnanogl )
	{
		glEsImpl->glTexCoordPointer( size, type, stride, pointer );
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	if( clientstate->texture_coord_array.size == size && clientstate->texture_coord_array.stride == stride && clientstate->texture_coord_array.type == type && clientstate->texture_coord_array.ptr == pointer )
	{
		return;
	}
	clientstate->texture_coord_array.size = size;
	clientstate->texture_coord_array.stride = stride;
	clientstate->texture_coord_array.type = type;
	clientstate->texture_coord_array.ptr = (GLvoid *)pointer;
	clientstate->texture_coord_array.changed = GL_TRUE;
}

void GL_MANGLE( glColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	if( tmuState0.color_array.size == size && tmuState0.color_array.stride == stride && tmuState0.color_array.type == type && tmuState0.color_array.ptr == pointer )
	{
		return;
	}
	tmuState0.color_array.size = size;
	tmuState0.color_array.stride = stride;
	tmuState0.color_array.type = type;
	tmuState0.color_array.ptr = (GLvoid *)pointer;
	tmuState0.color_array.changed = GL_TRUE;
}

void GL_MANGLE( glNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	int size = 0;
	if( tmuState0.normal_array.size == size && tmuState0.normal_array.stride == stride && tmuState0.normal_array.type == type && tmuState0.normal_array.ptr == pointer )
	{
		return;
	}
	tmuState0.normal_array.size = size;
	tmuState0.normal_array.stride = stride;
	tmuState0.normal_array.type = type;
	tmuState0.normal_array.ptr = (GLvoid *)pointer;
	tmuState0.normal_array.changed = GL_TRUE;
}

void GL_MANGLE( glPolygonOffset )( GLfloat factor, GLfloat units )
{
	FlushOnStateChange();
	glEsImpl->glPolygonOffset( factor, units );
}

void GL_MANGLE( glStencilMask )( GLuint mask )
{
	FlushOnStateChange();
	glEsImpl->glStencilMask( mask );
}

void GL_MANGLE( glClearStencil )( GLint s )
{
	FlushOnStateChange();
	glEsImpl->glClearStencil( s );
}

void GL_MANGLE( glMultiTexCoord1fARB )( GLenum target, GLfloat s )
{
	SetCurrentTexCoord( target, &s, 1 );
}

void GL_MANGLE( glMultiTexCoord2fARB )( GLenum target, GLfloat s, GLfloat t )
{
	const GLfloat coords[2] = { s, t };
	SetCurrentTexCoord( target, coords, 2 );
}

void GL_MANGLE( glMultiTexCoord3fARB )( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
	const GLfloat coords[3] = { s, t, r };
	SetCurrentTexCoord( target, coords, 3 );
}

void GL_MANGLE( glMultiTexCoord4fARB )( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	const GLfloat coords[4] = { s, t, r, q };
	SetCurrentTexCoord( target, coords, 4 );
}

void GL_MANGLE( glMultiTexCoord1f )( GLenum target, GLfloat s )
{
	GL_MANGLE_NAME( glMultiTexCoord1fARB )( target, s );
}

void GL_MANGLE( glMultiTexCoord2f )( GLenum target, GLfloat s, GLfloat t )
{
	GL_MANGLE_NAME( glMultiTexCoord2fARB )( target, s, t );
}

void GL_MANGLE( glMultiTexCoord3f )( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
	GL_MANGLE_NAME( glMultiTexCoord3fARB )( target, s, t, r );
}

void GL_MANGLE( glMultiTexCoord4f )( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	GL_MANGLE_NAME( glMultiTexCoord4fARB )( target, s, t, r, q );
}

/* Vladimir  */
/*void GL_MANGLE(glDrawArrays)( GLenum mode, int first, int count)
{
    FlushOnStateChange();
    glEsImpl->glDrawArrays(mode, first , count);
}*/
void GL_MANGLE( glMultMatrixf )( const GLfloat *m )
{
	FlushOnStateChange();
	glEsImpl->glMultMatrixf( m );
}

void GL_MANGLE( glCompressedTexImage2D )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data )
{
	FlushOnStateChange();
	glEsImpl->glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
}

void GL_MANGLE( glGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels )
{
	// glGetTexImage is a desktop OpenGL command, not part of GLES 1.1.
	// Keep the compatibility symbol for linkers, but fail explicitly rather
	// than attempting to resolve a non-existent GLES entry point.
	(void)target;
	(void)level;
	(void)format;
	(void)type;
	(void)pixels;
	SetWrapperError( GL_INVALID_OPERATION );
}

void GL_MANGLE( glPixelStorei )( GLenum pname, GLint param )
{
	FlushOnStateChange();
	glEsImpl->glPixelStorei( pname, param );
}

void GL_MANGLE( glFogi )( GLenum pname, GLint param )
{
	FlushOnStateChange();
	glEsImpl->glFogf( pname, param );
}

void GL_MANGLE( glFogf )( GLenum pname, GLfloat param )
{
	FlushOnStateChange();
	glEsImpl->glFogf( pname, param );
}

void GL_MANGLE( glFogfv )( GLenum pname, const GLfloat *params )
{
	FlushOnStateChange();
	glEsImpl->glFogfv( pname, params );
}

void GL_MANGLE( glGetTexParameteriv )( GLenum target, GLenum pname, GLint *params )
{
	FlushOnStateChange();
	glEsImpl->glGetTexParameteriv( target, pname, params );
}

// This gives: called unimplemented OpenGL ES API (Android)
void GL_MANGLE( glTexParameteri )( GLenum target, GLenum pname, GLint param )
{
	if( pname == GL_TEXTURE_BORDER_COLOR )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return; // desktop OpenGL border color is not supported by GLES
	}
	if(( pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ) && param == GL_CLAMP )
	{
		param = 0x812F;
	}

	FlushOnStateChange();
	glEsImpl->glTexParameteri( target, pname, param );
}

void GL_MANGLE( glTexParameterx )( GLenum target, GLenum pname, GLfixed param )
{
	if( pname == GL_TEXTURE_BORDER_COLOR )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return; // desktop OpenGL border color is not supported by GLES
	}
	if(( pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ) && param == GL_CLAMP )
	{
		param = 0x812F;
	}
	FlushOnStateChange();
	glEsImpl->glTexParameterx( target, pname, param );
}

void GL_MANGLE( glGenTextures )( GLsizei n, GLuint *textures )
{
	FlushOnStateChange();
	glEsImpl->glGenTextures( n, textures );
}

void GL_MANGLE( glFrontFace )( GLenum mode )
{
	FlushOnStateChange();
	glEsImpl->glFrontFace( mode );
}

// End Vladimir

void GL_MANGLE( glTexEnvi )( GLenum target, GLenum pname, GLint param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvi( target, pname, param );
		return;
	}
	if( target == GL_TEXTURE_ENV )
	{
		if( pname == GL_TEXTURE_ENV_MODE )
		{
			if( param == activetmuState->texture_env_mode.value )
			{
				return;
			}
			else
			{
				FlushOnStateChange();
				glEsImpl->glTexEnvi( target, pname, param );
				activetmuState->texture_env_mode.value = param;
				return;
			}
		}
	}
	FlushOnStateChange();
	glEsImpl->glTexEnvi( target, pname, param );
}

void GL_MANGLE( glTexEnvfv )( GLenum target, GLenum pname, const GLfloat *param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvfv( target, pname, param );
		return;
	}

	FlushOnStateChange();
	glEsImpl->glTexEnvfv( target, pname, param );
}

void GL_MANGLE( glDrawArrays )( GLenum mode, GLint first, GLsizei count )
{
	if( mode == GL_QUADS || mode == GL_QUAD_STRIP )
	{
		if( skipnanogl )
		{
			SetWrapperError( GL_INVALID_OPERATION );
			return; // VBO-backed arrays cannot be expanded without reading their data.
		}
		if( first < 0 || count < 0 )
		{
			SetWrapperError( GL_INVALID_VALUE );
			return;
		}
		if((size_t)first + (size_t)count > (size_t)USHRT_MAX + 1 )
		{
			SetWrapperError( GL_INVALID_OPERATION );
			return; // GLES 1.x element indices are limited to 16 bits.
		}
		GLushort *sequence = count ? (GLushort *)malloc((size_t)count * sizeof( *sequence )) : NULL;
		if( count && !sequence )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		for( GLsizei i = 0; i < count; ++i )
			sequence[i] = (GLushort)( first + i );
		GL_MANGLE_NAME( glDrawElements )( mode, count, GL_UNSIGNED_SHORT, sequence );
		free( sequence );
		return;
	}
	if( mode == GL_POLYGON )
		mode = GL_TRIANGLE_FAN;
	if( skipnanogl )
	{
		glEsImpl->glDrawArrays( mode, first, count );
		return;
	}
	// ensure that all primitives specified between glBegin/glEnd pairs
	// are rendered first, and that we have correct tmu in use..
	FlushOnStateChange();
	// setup correct vertex/color/texcoord pointers
	if( arraysValid || tmuState0.vertex_array.changed || tmuState0.color_array.changed || tmuState0.texture_coord_array.changed || tmuState0.normal_array.changed )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
	}
	if( arraysValid || tmuState0.vertex_array.changed )
	{
		if( tmuState0.vertex_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
		}
		glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
			tmuState0.vertex_array.type,
			tmuState0.vertex_array.stride,
			tmuState0.vertex_array.ptr );
		tmuState0.vertex_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.color_array.changed )
	{
		if( tmuState0.color_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
			glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
		}
		glEsImpl->glColorPointer( tmuState0.color_array.size,
			tmuState0.color_array.type,
			tmuState0.color_array.stride,
			tmuState0.color_array.ptr );
		tmuState0.color_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.normal_array.changed )
	{
		if( tmuState0.normal_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
		}
		glEsImpl->glNormalPointer( tmuState0.normal_array.type,
			tmuState0.normal_array.stride,
			tmuState0.normal_array.ptr );
		tmuState0.normal_array.changed = GL_FALSE;
	}
	if( arraysValid || tmuState0.texture_coord_array.changed )
	{
		tmuState0.texture_coord_array.changed = GL_FALSE;
		if( tmuState0.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
			tmuState0.texture_coord_array.type,
			tmuState0.texture_coord_array.stride,
			tmuState0.texture_coord_array.ptr );
	}

	if( arraysValid || tmuState1.texture_coord_array.changed )
	{
		tmuState1.texture_coord_array.changed = GL_FALSE;
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if( tmuState1.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState1.texture_coord_array.size,
			tmuState1.texture_coord_array.type,
			tmuState1.texture_coord_array.stride,
			tmuState1.texture_coord_array.ptr );
	}

	arraysValid = GL_FALSE;
	glEsImpl->glDrawArrays( mode, first, count );
}

/*void GL_MANGLE(glNormalPointer)(GLenum type, GLsizei stride, const void *ptr)
{
    glEsImpl->glNormalPointer( type, stride, ptr );
}*/

void GL_MANGLE( glCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
	FlushOnStateChange();
	glEsImpl->glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

#define CHECK_FBO_PROC( proc ) do { if( !glEsImpl->proc ) { SetWrapperError( GL_INVALID_OPERATION ); return; } } while( 0 )

void GL_MANGLE( glGenFramebuffers )( GLsizei n, GLuint *framebuffers )
{
	CHECK_FBO_PROC( glGenFramebuffers );
	FlushOnStateChange();
	glEsImpl->glGenFramebuffers( n, framebuffers );
}

void GL_MANGLE( glGenRenderbuffers )( GLsizei n, GLuint *renderbuffers )
{
	CHECK_FBO_PROC( glGenRenderbuffers );
	FlushOnStateChange();
	glEsImpl->glGenRenderbuffers( n, renderbuffers );
}

void GL_MANGLE( glBindRenderbuffer )( GLenum target, GLuint renderbuffer )
{
	CHECK_FBO_PROC( glBindRenderbuffer );
	FlushOnStateChange();
	glEsImpl->glBindRenderbuffer( target, renderbuffer );
}

void GL_MANGLE( glBindFramebuffer )( GLenum target, GLuint framebuffer )
{
	CHECK_FBO_PROC( glBindFramebuffer );
	FlushOnStateChange();
	glEsImpl->glBindFramebuffer( target, framebuffer );
}

void GL_MANGLE( glFramebufferRenderbuffer )( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
	CHECK_FBO_PROC( glFramebufferRenderbuffer );
	FlushOnStateChange();
	glEsImpl->glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

GLboolean GL_MANGLE( glIsRenderbuffer )( GLuint renderbuffer )
{
	if( !glEsImpl->glIsRenderbuffer )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return GL_FALSE;
	}
	FlushOnStateChange();
	return glEsImpl->glIsRenderbuffer( renderbuffer );
}

GLenum GL_MANGLE( glCheckFramebufferStatus )( GLenum target )
{
	if( !glEsImpl->glCheckFramebufferStatus )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return 0;
	}
	FlushOnStateChange();
	return glEsImpl->glCheckFramebufferStatus( target );
}

void GL_MANGLE( glDeleteFramebuffers )( GLsizei n, const GLuint *framebuffers )
{
	CHECK_FBO_PROC( glDeleteFramebuffers );
	FlushOnStateChange();
	glEsImpl->glDeleteFramebuffers( n, framebuffers );
}

void GL_MANGLE( glDeleteRenderbuffers )( GLsizei n, const GLuint *renderbuffers )
{
	CHECK_FBO_PROC( glDeleteRenderbuffers );
	FlushOnStateChange();
	glEsImpl->glDeleteRenderbuffers( n, renderbuffers );
}

void GL_MANGLE( glFramebufferTexture2D )( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
	CHECK_FBO_PROC( glFramebufferTexture2D );
	FlushOnStateChange();
	glEsImpl->glFramebufferTexture2D( target, attachment, textarget, texture, level );
}

void GL_MANGLE( glRenderbufferStorage )( GLenum target, GLenum internalformat, GLsizei width, GLsizei height )
{
	CHECK_FBO_PROC( glRenderbufferStorage );
	FlushOnStateChange();
	glEsImpl->glRenderbufferStorage( target, internalformat, width, height );
}

#undef CHECK_FBO_PROC

void GL_MANGLE( glBindBufferARB )( GLuint target, GLuint index )
{
	static int sindex;

	if( index && !sindex && !skipnanogl )
		FlushOnStateChange();
	glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
	glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );

	if( target == 0x8892 )
	{
		skipnanogl = ( !!index ) || vboarray;
	}
	glEsImpl->glBindBuffer( target, index );
	if( sindex && !index )
	{
		arraysValid = GL_FALSE;
		if( !skipnanogl )
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
			tmuState0.color_array.changed = 1;
			tmuState1.color_array.changed = 1;
			tmuState0.texture_coord_array.changed = 1;
			tmuState1.texture_coord_array.changed = 1;
			arraysValid = 0;
			ResetNanoState();
		}
	}
	sindex = index;
}

void GL_MANGLE( glGenBuffersARB )( GLuint count, GLuint *indexes )
{
	glEsImpl->glGenBuffers( count, indexes );
}

void GL_MANGLE( glDeleteBuffersARB )( GLuint count, GLuint *indexes )
{
	glEsImpl->glDeleteBuffers( count, indexes );
}

void GL_MANGLE( glBufferDataARB )( GLuint target, GLuint size, void *buffer, GLuint type )
{
	if( type == 0x88E0 )
		type = 0x88E8;
	glEsImpl->glBufferData( target, size, buffer, type );
}

void GL_MANGLE( glBufferSubDataARB )( GLuint target, GLsizei offset, GLsizei size, void *buffer )
{
	glEsImpl->glBufferSubData( target, offset, size, buffer );
}

GLboolean GL_MANGLE( glIsEnabled )( GLenum cap )
{
	return glEsImpl->glIsEnabled( cap );
}

void GL_MANGLE( glDebugMessageControlARB )( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled )
{
	if( glEsImpl->glDebugMessageControlKHR )
		glEsImpl->glDebugMessageControlKHR( source, type, severity, count, ids, enabled );
}

void GL_MANGLE( glDebugMessageInsertARB )( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *buf )
{
	if( glEsImpl->glDebugMessageInsertKHR )
		glEsImpl->glDebugMessageInsertKHR( source, type, id, severity, length, buf );
}

void GL_MANGLE( glDebugMessageCallbackARB )( GL_DEBUG_PROC_ARB callback, void *userParam )
{
	if( glEsImpl->glDebugMessageCallbackKHR )
		glEsImpl->glDebugMessageCallbackKHR( callback, userParam );
}

GLuint GL_MANGLE( glGetDebugMessageLogARB )( GLuint count, GLsizei bufsize, GLenum *sources, GLenum *types, GLuint *ids, GLuint *severities, GLsizei *lengths, char *messageLog )
{
	if( glEsImpl->glGetDebugMessageLogKHR )
		return glEsImpl->glGetDebugMessageLogKHR( count, bufsize, sources, types, ids, severities, lengths, messageLog );
	return 0;
}

/* Export both the pgl* API and its conventional GL spelling in mangled builds.
 * The aliases are declared in this translation unit so GCC/Clang can bind each
 * symbol directly to the exact wrapper implementation and ABI. */
#if defined( NANOGL_MANGLE_PREPEND ) && defined( __GNUC__ )
#define NANOGL_ALIAS( name ) extern __typeof__( p ## name ) name __attribute__(( alias( "p" #name )))
NANOGL_ALIAS( glActiveTexture );
NANOGL_ALIAS( glActiveTextureARB );
NANOGL_ALIAS( glAlphaFunc );
NANOGL_ALIAS( glArrayElement );
NANOGL_ALIAS( glBegin );
NANOGL_ALIAS( glBindBufferARB );
NANOGL_ALIAS( glBindFramebuffer );
NANOGL_ALIAS( glBindRenderbuffer );
NANOGL_ALIAS( glBindTexture );
NANOGL_ALIAS( glBlendFunc );
NANOGL_ALIAS( glBufferDataARB );
NANOGL_ALIAS( glBufferSubDataARB );
NANOGL_ALIAS( glCallList );
NANOGL_ALIAS( glCheckFramebufferStatus );
NANOGL_ALIAS( glClear );
NANOGL_ALIAS( glClearColor );
NANOGL_ALIAS( glClearDepth );
NANOGL_ALIAS( glClearStencil );
NANOGL_ALIAS( glClientActiveTexture );
NANOGL_ALIAS( glClientActiveTextureARB );
NANOGL_ALIAS( glClipPlane );
NANOGL_ALIAS( glColor3f );
NANOGL_ALIAS( glColor3fv );
NANOGL_ALIAS( glColor3ub );
NANOGL_ALIAS( glColor3ubv );
NANOGL_ALIAS( glColor4f );
NANOGL_ALIAS( glColor4fv );
NANOGL_ALIAS( glColor4ub );
NANOGL_ALIAS( glColor4ubv );
NANOGL_ALIAS( glColorMask );
NANOGL_ALIAS( glColorMaterial );
NANOGL_ALIAS( glColorPointer );
NANOGL_ALIAS( glCompressedTexImage2D );
NANOGL_ALIAS( glCopyTexImage2D );
NANOGL_ALIAS( glCopyTexSubImage2D );
NANOGL_ALIAS( glCullFace );
NANOGL_ALIAS( glDebugMessageCallbackARB );
NANOGL_ALIAS( glDebugMessageControlARB );
NANOGL_ALIAS( glDebugMessageInsertARB );
NANOGL_ALIAS( glDeleteBuffersARB );
NANOGL_ALIAS( glDeleteFramebuffers );
NANOGL_ALIAS( glDeleteRenderbuffers );
NANOGL_ALIAS( glDeleteTextures );
NANOGL_ALIAS( glDepthFunc );
NANOGL_ALIAS( glDepthMask );
NANOGL_ALIAS( glDepthRange );
NANOGL_ALIAS( glDisable );
NANOGL_ALIAS( glDisableClientState );
NANOGL_ALIAS( glDrawArrays );
NANOGL_ALIAS( glDrawBuffer );
NANOGL_ALIAS( glDrawElements );
NANOGL_ALIAS( glEnable );
NANOGL_ALIAS( glEnableClientState );
NANOGL_ALIAS( glEnd );
NANOGL_ALIAS( glFinish );
NANOGL_ALIAS( glFlush );
NANOGL_ALIAS( glFogf );
NANOGL_ALIAS( glFogfv );
NANOGL_ALIAS( glFogi );
NANOGL_ALIAS( glFramebufferRenderbuffer );
NANOGL_ALIAS( glFramebufferTexture2D );
NANOGL_ALIAS( glFrontFace );
NANOGL_ALIAS( glFrustum );
NANOGL_ALIAS( glGenBuffersARB );
NANOGL_ALIAS( glGenFramebuffers );
NANOGL_ALIAS( glGenRenderbuffers );
NANOGL_ALIAS( glGenTextures );
NANOGL_ALIAS( glGetDebugMessageLogARB );
NANOGL_ALIAS( glGetError );
NANOGL_ALIAS( glGetFloatv );
NANOGL_ALIAS( glGetIntegerv );
NANOGL_ALIAS( glGetString );
NANOGL_ALIAS( glGetTexImage );
NANOGL_ALIAS( glGetTexParameteriv );
NANOGL_ALIAS( glHint );
NANOGL_ALIAS( glIsEnabled );
NANOGL_ALIAS( glIsRenderbuffer );
NANOGL_ALIAS( glIsTexture );
NANOGL_ALIAS( glLightModelf );
NANOGL_ALIAS( glLightModelfv );
NANOGL_ALIAS( glLightf );
NANOGL_ALIAS( glLightfv );
NANOGL_ALIAS( glLineWidth );
NANOGL_ALIAS( glLoadIdentity );
NANOGL_ALIAS( glLoadMatrixf );
NANOGL_ALIAS( glMaterialf );
NANOGL_ALIAS( glMaterialfv );
NANOGL_ALIAS( glMatrixMode );
NANOGL_ALIAS( glMultMatrixf );
NANOGL_ALIAS( glMultiTexCoord1f );
NANOGL_ALIAS( glMultiTexCoord1fARB );
NANOGL_ALIAS( glMultiTexCoord2f );
NANOGL_ALIAS( glMultiTexCoord2fARB );
NANOGL_ALIAS( glMultiTexCoord3f );
NANOGL_ALIAS( glMultiTexCoord3fARB );
NANOGL_ALIAS( glMultiTexCoord4f );
NANOGL_ALIAS( glMultiTexCoord4fARB );
NANOGL_ALIAS( glNormal3f );
NANOGL_ALIAS( glNormal3fv );
NANOGL_ALIAS( glNormalPointer );
NANOGL_ALIAS( glOrtho );
NANOGL_ALIAS( glPixelStorei );
NANOGL_ALIAS( glPointSize );
NANOGL_ALIAS( glPolygonMode );
NANOGL_ALIAS( glPolygonOffset );
NANOGL_ALIAS( glPopMatrix );
NANOGL_ALIAS( glPushMatrix );
NANOGL_ALIAS( glReadBuffer );
NANOGL_ALIAS( glReadPixels );
NANOGL_ALIAS( glRenderbufferStorage );
NANOGL_ALIAS( glRotatef );
NANOGL_ALIAS( glScalef );
NANOGL_ALIAS( glScissor );
NANOGL_ALIAS( glShadeModel );
NANOGL_ALIAS( glStencilFunc );
NANOGL_ALIAS( glStencilMask );
NANOGL_ALIAS( glStencilOp );
NANOGL_ALIAS( glTexCoord1f );
NANOGL_ALIAS( glTexCoord1fv );
NANOGL_ALIAS( glTexCoord2f );
NANOGL_ALIAS( glTexCoord2fv );
NANOGL_ALIAS( glTexCoord3f );
NANOGL_ALIAS( glTexCoord3fv );
NANOGL_ALIAS( glTexCoord4f );
NANOGL_ALIAS( glTexCoord4fv );
NANOGL_ALIAS( glTexCoordPointer );
NANOGL_ALIAS( glTexEnvf );
NANOGL_ALIAS( glTexEnvfv );
NANOGL_ALIAS( glTexEnvi );
NANOGL_ALIAS( glTexGenfv );
NANOGL_ALIAS( glTexGeni );
NANOGL_ALIAS( glTexImage1D );
NANOGL_ALIAS( glTexImage2D );
NANOGL_ALIAS( glTexImage3D );
NANOGL_ALIAS( glTexParameterf );
NANOGL_ALIAS( glTexParameterfv );
NANOGL_ALIAS( glTexParameteri );
NANOGL_ALIAS( glTexParameterx );
NANOGL_ALIAS( glTexSubImage1D );
NANOGL_ALIAS( glTexSubImage2D );
NANOGL_ALIAS( glTexSubImage3D );
NANOGL_ALIAS( glTranslatef );
NANOGL_ALIAS( glVertex2f );
NANOGL_ALIAS( glVertex3f );
NANOGL_ALIAS( glVertex3fv );
NANOGL_ALIAS( glVertex4f );
NANOGL_ALIAS( glVertex4fv );
NANOGL_ALIAS( glVertexPointer );
NANOGL_ALIAS( glViewport );
#undef NANOGL_ALIAS
#endif
