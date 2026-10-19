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

struct texgenstate
{
	GLboolean enabled;
	GLenum mode;
	GLfloat objectPlane[4];
	GLfloat eyePlane[4];
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
static struct texgenstate texgenStates[2][4];
static GLfloat texgenModelView[16];
static GLboolean texgenModelViewValid;

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

enum DisplayListOpcode
{
	DL_BEGIN = 1,
	DL_END,
	DL_VERTEX3,
	DL_VERTEX4,
	DL_COLOR4,
	DL_NORMAL3,
	DL_TEXCOORD4,
	DL_CALL
};

struct DisplayListCommand
{
	GLenum opcode;
	GLuint integer;
	GLfloat value[4];
};

struct DisplayList
{
	GLuint id;
	GLboolean compiled;
	size_t count;
	size_t capacity;
	struct DisplayListCommand *commands;
};

#define DISPLAY_LIST_COUNT 1024
#define DISPLAY_LIST_MAX_DEPTH 64
static struct DisplayList displayLists[DISPLAY_LIST_COUNT];

struct TextureShadow
{
	GLuint texture;
	GLenum target;
	GLint level;
	GLint internalFormat;
	GLsizei width;
	GLsizei height;
	GLenum format;
	GLenum type;
	GLubyte *pixels;
	size_t dataSize;
	GLboolean readable;
	GLboolean compressed;
	GLboolean compressedReadable;
};

static struct TextureShadow *textureShadows;
static size_t textureShadowCount;
static size_t textureShadowCapacity;
static GLint texturePackAlignment = 4;
static GLint textureUnpackAlignment = 4;

struct BufferShadow
{
	GLuint id;
	GLubyte *data;
	size_t size;
	GLenum usage;
	GLboolean mapped;
};

static struct BufferShadow *bufferShadows;
static size_t bufferShadowCount;
static size_t bufferShadowCapacity;
static GLuint boundArrayBuffer;
static GLuint boundElementArrayBuffer;
static struct DisplayList *compilingDisplayList;
static GLenum displayListCompileMode;
static GLboolean executingDisplayList;
static unsigned int displayListDepth;
static GLuint nextDisplayListId = 1;

static struct DisplayList *FindDisplayList( GLuint id )
{
	for( size_t i = 0; i < DISPLAY_LIST_COUNT; ++i )
		if( displayLists[i].id == id )
			return &displayLists[i];
	return NULL;
}

static GLboolean RecordDisplayListCommand( GLenum opcode, GLuint integer, const GLfloat *values )
{
	if( !compilingDisplayList || executingDisplayList )
		return GL_FALSE;
	struct DisplayList *list = compilingDisplayList;
	if( list->count == list->capacity )
	{
		size_t capacity = list->capacity ? list->capacity * 2 : 32;
		if( capacity < list->capacity || capacity > SIZE_MAX / sizeof( *list->commands ))
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return GL_TRUE;
		}
		struct DisplayListCommand *commands = (struct DisplayListCommand *)realloc( list->commands, capacity * sizeof( *commands ));
		if( !commands )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return GL_TRUE;
		}
		list->commands = commands;
		list->capacity = capacity;
	}
	struct DisplayListCommand *command = &list->commands[list->count++];
	command->opcode = opcode;
	command->integer = integer;
	memset( command->value, 0, sizeof( command->value ));
	if( values )
		memcpy( command->value, values, sizeof( command->value ));
	return displayListCompileMode == GL_COMPILE;
}

static void ExecuteDisplayListCommand( const struct DisplayListCommand *command )
{
	switch( command->opcode )
	{
	case DL_BEGIN: GL_MANGLE_NAME( glBegin )( command->integer ); break;
	case DL_END: GL_MANGLE_NAME( glEnd )(); break;
	case DL_VERTEX3: GL_MANGLE_NAME( glVertex3f )( command->value[0], command->value[1], command->value[2] ); break;
	case DL_VERTEX4: GL_MANGLE_NAME( glVertex4f )( command->value[0], command->value[1], command->value[2], command->value[3] ); break;
	case DL_COLOR4: GL_MANGLE_NAME( glColor4f )( command->value[0], command->value[1], command->value[2], command->value[3] ); break;
	case DL_NORMAL3: GL_MANGLE_NAME( glNormal3f )( command->value[0], command->value[1], command->value[2] ); break;
	case DL_TEXCOORD4: GL_MANGLE_NAME( glMultiTexCoord4f )( command->integer, command->value[0], command->value[1], command->value[2], command->value[3] ); break;
	case DL_CALL: GL_MANGLE_NAME( glCallList )( command->integer ); break;
	default: SetWrapperError( GL_INVALID_OPERATION ); break;
	}
}

static GLenum ShadowTextureTarget( GLenum target )
{
	return target == GL_TEXTURE_1D ? GL_TEXTURE_2D : target;
}

static struct TextureShadow *FindTextureShadow( GLuint texture, GLenum target, GLint level, GLboolean create )
{
	target = ShadowTextureTarget( target );
	for( size_t i = 0; i < textureShadowCount; ++i )
		if( textureShadows[i].texture == texture && textureShadows[i].target == target && textureShadows[i].level == level )
			return &textureShadows[i];
	if( !create )
		return NULL;
	if( textureShadowCount == textureShadowCapacity )
	{
		size_t capacity = textureShadowCapacity ? textureShadowCapacity * 2 : 32;
		if( capacity < textureShadowCapacity || capacity > SIZE_MAX / sizeof( *textureShadows ))
			return NULL;
		struct TextureShadow *entries = (struct TextureShadow *)realloc( textureShadows, capacity * sizeof( *entries ));
		if( !entries )
			return NULL;
		textureShadows = entries;
		textureShadowCapacity = capacity;
	}
	struct TextureShadow *entry = &textureShadows[textureShadowCount++];
	memset( entry, 0, sizeof( *entry ));
	entry->texture = texture;
	entry->target = target;
	entry->level = level;
	return entry;
}

static size_t PixelFormatComponents( GLenum format )
{
	switch( format )
	{
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE: return 1;
	case GL_LUMINANCE_ALPHA: return 2;
	case GL_RGB: return 3;
	case GL_RGBA: return 4;
	default: return 0;
	}
}

static size_t AlignedPixelRowSize( size_t size, GLint alignment )
{
	size_t padding = (size_t)alignment - 1;
	if( size > SIZE_MAX - padding )
		return SIZE_MAX;
	return ( size + padding ) & ~padding;
}

static void InvalidateTextureShadow( GLuint texture, GLenum target, GLint level )
{
	struct TextureShadow *entry = FindTextureShadow( texture, target, level, GL_FALSE );
	if( entry )
	{
		entry->readable = GL_FALSE;
		entry->compressedReadable = GL_FALSE;
	}
}

static void DeleteTextureShadow( GLuint texture )
{
	if( tmuState0.boundtexture.value == texture ) tmuState0.boundtexture.value = 0;
	if( tmuState1.boundtexture.value == texture ) tmuState1.boundtexture.value = 0;
	for( size_t i = 0; i < textureShadowCount; )
	{
		if( textureShadows[i].texture == texture )
		{
			free( textureShadows[i].pixels );
			textureShadows[i] = textureShadows[--textureShadowCount];
		}
		else
			++i;
	}
}

static void StoreTextureShadow( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_TRUE );
	if( !entry )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	free( entry->pixels );
	entry->pixels = NULL;
	entry->dataSize = 0;
	entry->compressed = GL_FALSE;
	entry->compressedReadable = GL_FALSE;
	entry->internalFormat = internalFormat;
	entry->width = width;
	entry->height = height;
	entry->format = format;
	entry->type = type;
	entry->readable = GL_FALSE;
	size_t components = PixelFormatComponents( format );
	if( !pixels || width < 0 || height < 0 || type != GL_UNSIGNED_BYTE || !components )
		return;
	if((size_t)width > SIZE_MAX / components )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	size_t rowBytes = (size_t)width * components;
	if((size_t)height && rowBytes > SIZE_MAX / (size_t)height )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	size_t rowStride = AlignedPixelRowSize( rowBytes, textureUnpackAlignment );
	if( rowStride == SIZE_MAX || (size_t)height > SIZE_MAX / (rowStride ? rowStride : 1) )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	size_t totalBytes = rowBytes * (size_t)height;
	entry->pixels = (GLubyte *)malloc( totalBytes ? totalBytes : 1 );
	if( !entry->pixels )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	for( GLsizei y = 0; y < height; ++y )
		memcpy( entry->pixels + (size_t)y * rowBytes, (const GLubyte *)pixels + (size_t)y * rowStride, rowBytes );
	entry->dataSize = totalBytes;
	entry->readable = GL_TRUE;
}

static void StoreCompressedTextureShadow( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei imageSize, const GLvoid *data )
{
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_TRUE );
	if( !entry )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	free( entry->pixels );
	entry->pixels = NULL;
	entry->dataSize = 0;
	entry->readable = GL_FALSE;
	entry->compressedReadable = GL_FALSE;
	entry->compressed = GL_TRUE;
	entry->internalFormat = (GLint)internalFormat;
	entry->width = width;
	entry->height = height;
	entry->format = internalFormat;
	entry->type = GL_UNSIGNED_BYTE;
	if( imageSize < 0 || ( imageSize && !data ))
	{
		SetWrapperError( imageSize < 0 ? GL_INVALID_VALUE : GL_INVALID_OPERATION );
		return;
	}
	if( imageSize )
	{
		entry->pixels = (GLubyte *)malloc((size_t)imageSize);
		if( !entry->pixels )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		memcpy( entry->pixels, data, (size_t)imageSize );
	}
	entry->dataSize = (size_t)imageSize;
	entry->compressedReadable = GL_TRUE;
}

static void UpdateTextureShadow( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_FALSE );
	if( !entry )
		return;
	if( entry->compressed )
	{
		entry->compressedReadable = GL_FALSE;
		return;
	}
	if( !entry->readable || !pixels || xoffset < 0 || yoffset < 0 || width < 0 || height < 0 || type != entry->type || format != entry->format || type != GL_UNSIGNED_BYTE || xoffset > entry->width - width || yoffset > entry->height - height )
	{
		entry->readable = GL_FALSE;
		return;
	}
	size_t components = PixelFormatComponents( format );
	if( !components || (size_t)width > SIZE_MAX / components || (size_t)entry->width > SIZE_MAX / components )
	{
		entry->readable = GL_FALSE;
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	size_t rowBytes = (size_t)width * components;
	size_t srcStride = AlignedPixelRowSize( rowBytes, textureUnpackAlignment );
	size_t dstStride = (size_t)entry->width * components;
	if( srcStride == SIZE_MAX || ((size_t)height && srcStride > SIZE_MAX / (size_t)height ) || ((size_t)entry->height && dstStride > SIZE_MAX / (size_t)entry->height ))
	{
		entry->readable = GL_FALSE;
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	for( GLsizei y = 0; y < height; ++y )
		memcpy( entry->pixels + ((size_t)yoffset + (size_t)y) * dstStride + (size_t)xoffset * components,
			(const GLubyte *)pixels + (size_t)y * srcStride, rowBytes );
}

static void DecodeShadowPixel( const struct TextureShadow *entry, const GLubyte *source, GLubyte rgba[4] )
{
	rgba[0] = rgba[1] = rgba[2] = rgba[3] = 255;
	switch( entry->format )
	{
	case GL_RGBA: memcpy( rgba, source, 4 ); break;
	case GL_RGB: memcpy( rgba, source, 3 ); break;
	case GL_ALPHA: rgba[3] = source[0]; break;
	case GL_LUMINANCE: rgba[0] = rgba[1] = rgba[2] = source[0]; break;
	case GL_LUMINANCE_ALPHA: rgba[0] = rgba[1] = rgba[2] = source[0]; rgba[3] = source[1]; break;
	case GL_RED: rgba[0] = source[0]; break;
	case GL_GREEN: rgba[1] = source[0]; break;
	case GL_BLUE: rgba[2] = source[0]; break;
	default: break;
	}
}

static void ReadTextureShadow( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels )
{
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_FALSE );
	if( !pixels )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	if( !entry || !entry->readable )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	if( type != GL_UNSIGNED_BYTE )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	size_t components = PixelFormatComponents( format );
	if( !components )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	size_t srcComponents = PixelFormatComponents( entry->format );
	if((size_t)entry->width > SIZE_MAX / components || (size_t)entry->width > SIZE_MAX / srcComponents )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	size_t rowBytes = (size_t)entry->width * components;
	size_t dstStride = AlignedPixelRowSize( rowBytes, texturePackAlignment );
	size_t srcStride = (size_t)entry->width * srcComponents;
	if( dstStride == SIZE_MAX || (size_t)entry->height > SIZE_MAX / (dstStride ? dstStride : 1) )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	for( GLsizei y = 0; y < entry->height; ++y )
	{
		GLubyte *dst = (GLubyte *)pixels + (size_t)y * dstStride;
		for( GLsizei x = 0; x < entry->width; ++x )
		{
			GLubyte rgba[4];
			DecodeShadowPixel( entry, entry->pixels + (size_t)y * srcStride + (size_t)x * srcComponents, rgba );
			switch( format )
			{
			case GL_RGBA: memcpy( dst + (size_t)x * 4, rgba, 4 ); break;
			case GL_RGB: memcpy( dst + (size_t)x * 3, rgba, 3 ); break;
			case GL_ALPHA: dst[x] = rgba[3]; break;
			case GL_LUMINANCE: dst[x] = (GLubyte)(( 77u * rgba[0] + 150u * rgba[1] + 29u * rgba[2] + 128u ) >> 8); break;
			case GL_LUMINANCE_ALPHA: dst[(size_t)x * 2] = (GLubyte)(( 77u * rgba[0] + 150u * rgba[1] + 29u * rgba[2] + 128u ) >> 8); dst[(size_t)x * 2 + 1] = rgba[3]; break;
			case GL_RED: dst[x] = rgba[0]; break;
			case GL_GREEN: dst[x] = rgba[1]; break;
			case GL_BLUE: dst[x] = rgba[2]; break;
			default: break;
			}
		}
	}
}

static struct BufferShadow *FindBufferShadow( GLuint id, GLboolean create )
{
	if( !id )
		return NULL;
	for( size_t i = 0; i < bufferShadowCount; ++i )
		if( bufferShadows[i].id == id )
			return &bufferShadows[i];
	if( !create )
		return NULL;
	if( bufferShadowCount == bufferShadowCapacity )
	{
		size_t capacity = bufferShadowCapacity ? bufferShadowCapacity * 2 : 16;
		if( capacity < bufferShadowCapacity || capacity > SIZE_MAX / sizeof( *bufferShadows ))
			return NULL;
		struct BufferShadow *entries = (struct BufferShadow *)realloc( bufferShadows, capacity * sizeof( *entries ));
		if( !entries )
			return NULL;
		bufferShadows = entries;
		bufferShadowCapacity = capacity;
	}
	struct BufferShadow *entry = &bufferShadows[bufferShadowCount++];
	memset( entry, 0, sizeof( *entry ));
	entry->id = id;
	return entry;
}

static GLuint BoundBufferForTarget( GLenum target )
{
	if( target == GL_ARRAY_BUFFER_ARB ) return boundArrayBuffer;
	if( target == GL_ELEMENT_ARRAY_BUFFER_ARB ) return boundElementArrayBuffer;
	return 0;
}

static void DeleteBufferShadow( GLuint id )
{
	for( size_t i = 0; i < bufferShadowCount; )
	{
		if( bufferShadows[i].id == id )
		{
			free( bufferShadows[i].data );
			bufferShadows[i] = bufferShadows[--bufferShadowCount];
		}
		else
			++i;
	}
	if( boundArrayBuffer == id ) boundArrayBuffer = 0;
	if( boundElementArrayBuffer == id ) boundElementArrayBuffer = 0;
}

void InitGLStructs( void)
{
	for( size_t i = 0; i < textureShadowCount; ++i )
		free( textureShadows[i].pixels );
	free( textureShadows );
	textureShadows = NULL;
	textureShadowCount = 0;
	textureShadowCapacity = 0;
	texturePackAlignment = textureUnpackAlignment = 4;
	for( size_t i = 0; i < bufferShadowCount; ++i )
		free( bufferShadows[i].data );
	free( bufferShadows );
	bufferShadows = NULL;
	bufferShadowCount = 0;
	bufferShadowCapacity = 0;
	boundArrayBuffer = boundElementArrayBuffer = 0;
	for( size_t i = 0; i < DISPLAY_LIST_COUNT; ++i )
		free( displayLists[i].commands );
	memset( displayLists, 0, sizeof( displayLists ));
	compilingDisplayList = NULL;
	displayListCompileMode = 0;
	executingDisplayList = GL_FALSE;
	displayListDepth = 0;
	nextDisplayListId = 1;
	memset( texgenStates, 0, sizeof( texgenStates ));
	for( size_t unit = 0; unit < 2; ++unit )
	{
		for( size_t coord = 0; coord < 4; ++coord )
		{
			texgenStates[unit][coord].mode = GL_EYE_LINEAR;
			texgenStates[unit][coord].objectPlane[coord] = 1.0f;
			texgenStates[unit][coord].eyePlane[coord] = 1.0f;
		}
	}
	texgenModelViewValid = GL_FALSE;

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
	if( RecordDisplayListCommand( DL_BEGIN, mode, NULL ))
		return;
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
	texgenModelViewValid = GL_FALSE;
	vertexMark = vertexCount;
	ptrVertexAttribArrayMark = ptrVertexAttribArray;
	indexbase = indexCount;
}

void GL_MANGLE( glEnd )( void )
{
	if( RecordDisplayListCommand( DL_END, 0, NULL ))
		return;
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

static int TexGenCoordIndex( GLenum coord )
{
	switch( coord )
	{
	case GL_TEXTURE_GEN_S: return 0;
	case GL_TEXTURE_GEN_T: return 1;
	case GL_TEXTURE_GEN_R: return 2;
	case GL_TEXTURE_GEN_Q: return 3;
	default: return -1;
	}
}

static int ActiveTextureUnitIndex( void )
{
	if( activetmu == GL_TEXTURE0 ) return 0;
	if( activetmu == GL_TEXTURE1 ) return 1;
	return -1;
}

static GLboolean AnyTexGenEnabled( void )
{
	for( size_t unit = 0; unit < 2; ++unit )
		for( size_t coord = 0; coord < 4; ++coord )
			if( texgenStates[unit][coord].enabled )
				return GL_TRUE;
	return GL_FALSE;
}

void GL_MANGLE( glEnable )( GLenum cap )
{
	int texgenCoord = TexGenCoordIndex( cap );
	if( texgenCoord >= 0 )
	{
		int unit = ActiveTextureUnitIndex();
		if( unit < 0 )
		{
			SetWrapperError( GL_INVALID_ENUM );
			return;
		}
		if( !texgenStates[unit][texgenCoord].enabled )
		{
			FlushOnStateChange();
			texgenStates[unit][texgenCoord].enabled = GL_TRUE;
			texgenModelViewValid = GL_FALSE;
		}
		return;
	}
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
	int texgenCoord = TexGenCoordIndex( cap );
	if( texgenCoord >= 0 )
	{
		int unit = ActiveTextureUnitIndex();
		if( unit < 0 )
		{
			SetWrapperError( GL_INVALID_ENUM );
			return;
		}
		if( texgenStates[unit][texgenCoord].enabled )
		{
			FlushOnStateChange();
			texgenStates[unit][texgenCoord].enabled = GL_FALSE;
			texgenModelViewValid = GL_FALSE;
		}
		return;
	}
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

void GL_MANGLE( glVertex2i )( GLint x, GLint y )
{
	GL_MANGLE_NAME( glVertex2f )((GLfloat)x, (GLfloat)y );
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
	const GLfloat color[4] = { red, green, blue, 1.0f };
	if( RecordDisplayListCommand( DL_COLOR4, 0, color ))
		return;
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

	GLfloat values[4];
	memcpy( values, defaults, sizeof( values ));
	memcpy( values, coords, (size_t)count * sizeof( GLfloat ));
	if( RecordDisplayListCommand( DL_TEXCOORD4, target, values ))
		return;
	memcpy( destination, values, sizeof( values ));

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
	const GLfloat color[4] = { red, green, blue, alpha };
	if( RecordDisplayListCommand( DL_COLOR4, 0, color ))
		return;
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

void GL_MANGLE( glLightModeli )( GLenum pname, GLint param )
{
	FlushOnStateChange();
	glEsImpl->glLightModelf( pname, (GLfloat)param );
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
	if( width < 0 || height < 0 || border != 0 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	GLint requestedInternalFormat = internalformat;
	const GLvoid *data = pixels;
	GLubyte *converted = NULL;
	if( pixels && type == GL_UNSIGNED_BYTE && format == GL_RGBA && ( internalformat == GL_RGB || internalformat == GL_RGB8 || internalformat == GL_RGB5 || internalformat == GL_LUMINANCE || internalformat == GL_LUMINANCE8 || internalformat == GL_LUMINANCE4 ))
	{
		if( width && (size_t)width > SIZE_MAX / 4 )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		size_t rowBytes = (size_t)width * 4;
		if((size_t)height && rowBytes > SIZE_MAX / (size_t)height )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		size_t convertedStride = AlignedPixelRowSize( rowBytes, textureUnpackAlignment );
		if( convertedStride == SIZE_MAX || ((size_t)height && convertedStride > SIZE_MAX / (size_t)height ))
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		size_t convertedSize = convertedStride * (size_t)height;
		converted = (GLubyte *)malloc( convertedSize ? convertedSize : 1 );
		if( !converted )
		{
			SetWrapperError( GL_OUT_OF_MEMORY );
			return;
		}
		size_t rowStride = AlignedPixelRowSize( rowBytes, textureUnpackAlignment );
		for( GLsizei y = 0; y < height; ++y )
		{
			const GLubyte *src = (const GLubyte *)pixels + (size_t)y * rowStride;
			GLubyte *dst = converted + (size_t)y * convertedStride;
			for( GLsizei x = 0; x < width; ++x )
			{
				dst[(size_t)x * 4] = src[(size_t)x * 4];
				dst[(size_t)x * 4 + 1] = src[(size_t)x * 4 + 1];
				dst[(size_t)x * 4 + 2] = src[(size_t)x * 4 + 2];
				dst[(size_t)x * 4 + 3] = 255;
			}
		}
		data = converted;
	}

	internalformat = format;
	FlushOnStateChange();
	glEsImpl->glTexImage2D( target, level, internalformat, width, height, border, format, type, data );
	StoreTextureShadow( target, level, requestedInternalFormat, width, height, format, type, data );
	free( converted );
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

void GL_MANGLE( glTranslate )( GLdouble x, GLdouble y, GLdouble z )
{
	GL_MANGLE_NAME( glTranslatef )((GLfloat)x, (GLfloat)y, (GLfloat)z );
}

void GL_MANGLE( glRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange();
	glEsImpl->glRotatef( angle, x, y, z );
}

void GL_MANGLE( glRotate )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
	GL_MANGLE_NAME( glRotatef )((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z );
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

void GL_MANGLE( glGetBooleanv )( GLenum pname, GLboolean *params )
{
	if( !params )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	FlushOnStateChange();
	glEsImpl->glGetBooleanv( pname, params );
}

void GL_MANGLE( glGetDoublev )( GLenum pname, GLdouble *params )
{
	if( !params )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	size_t count = 1;
	switch( pname )
	{
	case GL_MODELVIEW_MATRIX:
	case GL_PROJECTION_MATRIX:
	case GL_TEXTURE_MATRIX: count = 16; break;
	case GL_CURRENT_COLOR:
	case GL_CURRENT_TEXTURE_COORDS:
	case GL_COLOR_CLEAR_VALUE:
	case GL_VIEWPORT:
	case GL_SCISSOR_BOX: count = 4; break;
	case GL_CURRENT_NORMAL: count = 3; break;
	case GL_DEPTH_RANGE: count = 2; break;
	default: break;
	}
	GLfloat values[16] = { 0 };
	GL_MANGLE_NAME( glGetFloatv )( pname, values );
	for( size_t i = 0; i < count; ++i )
		params[i] = values[i];
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

static GLboolean TexGenNeedsEyeMatrix( void )
{
	for( size_t unit = 0; unit < 2; ++unit )
		for( size_t coord = 0; coord < 4; ++coord )
			if( texgenStates[unit][coord].enabled && texgenStates[unit][coord].mode == GL_EYE_LINEAR )
				return GL_TRUE;
	return GL_FALSE;
}

static void GenerateTextureCoordinates( VertexAttrib *attrib, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	GLfloat object[4] = { x, y, z, w };
	GLfloat eye[4];
	if( TexGenNeedsEyeMatrix() && !texgenModelViewValid )
	{
		glEsImpl->glGetFloatv( GL_MODELVIEW_MATRIX, texgenModelView );
		texgenModelViewValid = GL_TRUE;
	}
	if( TexGenNeedsEyeMatrix() )
	{
		eye[0] = texgenModelView[0] * x + texgenModelView[4] * y + texgenModelView[8] * z + texgenModelView[12] * w;
		eye[1] = texgenModelView[1] * x + texgenModelView[5] * y + texgenModelView[9] * z + texgenModelView[13] * w;
		eye[2] = texgenModelView[2] * x + texgenModelView[6] * y + texgenModelView[10] * z + texgenModelView[14] * w;
		eye[3] = texgenModelView[3] * x + texgenModelView[7] * y + texgenModelView[11] * z + texgenModelView[15] * w;
	}
	for( size_t unit = 0; unit < 2; ++unit )
	{
		GLfloat *coords = unit == 0 ? &attrib->s : &attrib->s_multi;
		for( size_t axis = 0; axis < 4; ++axis )
		{
			const struct texgenstate *state = &texgenStates[unit][axis];
			if( !state->enabled )
				continue;
			const GLfloat *position = state->mode == GL_OBJECT_LINEAR ? object : eye;
			const GLfloat *plane = state->mode == GL_OBJECT_LINEAR ? state->objectPlane : state->eyePlane;
			coords[axis] = plane[0] * position[0] + plane[1] * position[1] + plane[2] * position[2] + plane[3] * position[3];
		}
	}
}

static void StoreVertex( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	CheckQuadsOverflow();
	VertexAttrib vertex = currentVertexAttrib;
	GenerateTextureCoordinates( &vertex, x, y, z, w );

	GLfloat *vert = (GLfloat *)ptrVertexAttribArray++;
	*vert++ = x;
	*vert++ = y;
	*vert++ = z;
	*vert++ = w;
	memcpy( vert, &vertex.nx, sizeof( VertexAttrib ) - 4 * sizeof( GLfloat ));
}

void GL_MANGLE( glVertex3f )( GLfloat x, GLfloat y, GLfloat z )
{
	const GLfloat vertex[4] = { x, y, z, 1.0f };
	if( RecordDisplayListCommand( DL_VERTEX3, 0, vertex ))
		return;
	StoreVertex( x, y, z, 1.0f );
}

void GL_MANGLE( glVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	const GLfloat vertex[4] = { x, y, z, w };
	if( RecordDisplayListCommand( DL_VERTEX4, 0, vertex ))
		return;
	StoreVertex( x, y, z, w );
}

void GL_MANGLE( glVertex4fv )( const GLfloat *v )
{
	GL_MANGLE_NAME( glVertex4f )( v[0], v[1], v[2], v[3] );
}

void GL_MANGLE( glColor4fv )( const GLfloat *v )
{
	GL_MANGLE_NAME( glColor4f )( v[0], v[1], v[2], v[3] );
}

void GL_MANGLE( glColor3ubv )( const GLubyte *v )
{
	GL_MANGLE_NAME( glColor3ub )( v[0], v[1], v[2] );
}

void GL_MANGLE( glColor4ubv )( const GLubyte *v )
{
	GL_MANGLE_NAME( glColor4ub )( v[0], v[1], v[2], v[3] );
}

void GL_MANGLE( glColor3fv )( const GLfloat *v )
{
	GL_MANGLE_NAME( glColor3f )( v[0], v[1], v[2] );
}

// -- nicknekit: xash3d funcs --

void GL_MANGLE( glColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
	const GLfloat color[4] = { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
	if( RecordDisplayListCommand( DL_COLOR4, 0, color ))
		return;
	currentVertexAttrib.red = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue = blue;
	currentVertexAttrib.alpha = alpha;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glColor3ub )( GLubyte red, GLubyte green, GLubyte blue )
{
	const GLfloat color[4] = { red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f };
	if( RecordDisplayListCommand( DL_COLOR4, 0, color ))
		return;
	currentVertexAttrib.red = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue = blue;
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
}

void GL_MANGLE( glNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz )
{
	const GLfloat normal[4] = { nx, ny, nz, 0.0f };
	if( RecordDisplayListCommand( DL_NORMAL3, 0, normal ))
		return;
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
	StoreTextureShadow( target, level, internalformat, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
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

static GLboolean SetTexGenParameter( GLenum coord, GLenum pname, const GLfloat *params )
{
	int index;
	int unit = ActiveTextureUnitIndex();
	if( unit < 0 )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return GL_FALSE;
	}
	switch( coord )
	{
	case GL_S: index = 0; break;
	case GL_T: index = 1; break;
	case GL_R: index = 2; break;
	case GL_Q: index = 3; break;
	default: SetWrapperError( GL_INVALID_ENUM ); return GL_FALSE;
	}
	if( !params )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return GL_FALSE;
	}
	struct texgenstate *state = &texgenStates[unit][index];
	if( pname == GL_TEXTURE_GEN_MODE )
	{
		GLenum mode = (GLenum)params[0];
		if( mode != GL_OBJECT_LINEAR && mode != GL_EYE_LINEAR )
		{
			SetWrapperError( mode == GL_SPHERE_MAP ? GL_INVALID_OPERATION : GL_INVALID_ENUM );
			return GL_FALSE;
		}
		if( state->mode != mode )
		{
			FlushOnStateChange();
			state->mode = mode;
			texgenModelViewValid = GL_FALSE;
		}
		return GL_TRUE;
	}
	if( pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE )
	{
		GLfloat *plane = pname == GL_OBJECT_PLANE ? state->objectPlane : state->eyePlane;
		if( memcmp( plane, params, 4 * sizeof( GLfloat )) != 0 )
		{
			FlushOnStateChange();
			memcpy( plane, params, 4 * sizeof( GLfloat ));
			texgenModelViewValid = GL_FALSE;
		}
		return GL_TRUE;
	}
	SetWrapperError( GL_INVALID_ENUM );
	return GL_FALSE;
}

void GL_MANGLE( glTexGenf )( GLenum coord, GLenum pname, GLfloat param )
{
	(void)SetTexGenParameter( coord, pname, &param );
}

void GL_MANGLE( glTexGeni )( GLenum coord, GLenum pname, GLint param )
{
	GLfloat value = (GLfloat)param;
	(void)SetTexGenParameter( coord, pname, &value );
}

void GL_MANGLE( glTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params )
{
	(void)SetTexGenParameter( coord, pname, params );
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
	GL_MANGLE_NAME( glVertex3f )( v[0], v[1], v[2] );
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
	UpdateTextureShadow( target, level, xoffset, yoffset, width, height, format, type, pixels );
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
	if( textures )
		for( GLsizei i = 0; i < n; ++i )
			DeleteTextureShadow( textures[i] );
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

GLuint GL_MANGLE( glGenLists )( GLsizei range )
{
	if( range < 0 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return 0;
	}
	if( range == 0 || (size_t)range > DISPLAY_LIST_COUNT )
		return 0;
	size_t freeSlots = 0;
	for( size_t i = 0; i < DISPLAY_LIST_COUNT; ++i )
		if( displayLists[i].id == 0 )
			++freeSlots;
	if((size_t)range > freeSlots )
		return 0;

	GLuint candidate = nextDisplayListId ? nextDisplayListId : 1;
	for( size_t tries = 0; tries < DISPLAY_LIST_COUNT; ++tries )
	{
		if( candidate > UINT_MAX - (GLuint)range )
			candidate = 1;
		GLboolean available = GL_TRUE;
		for( GLsizei offset = 0; offset < range; ++offset )
			if( FindDisplayList( candidate + (GLuint)offset ))
			{
				available = GL_FALSE;
				break;
			}
		if( available )
		{
			for( GLsizei offset = 0; offset < range; ++offset )
			{
				for( size_t i = 0; i < DISPLAY_LIST_COUNT; ++i )
					if( displayLists[i].id == 0 )
					{
						displayLists[i].id = candidate + (GLuint)offset;
						break;
					}
			}
			nextDisplayListId = candidate + (GLuint)range;
			return candidate;
		}
		++candidate;
	}
	return 0;
}

void GL_MANGLE( glDeleteLists )( GLuint list, GLsizei range )
{
	if( range < 0 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	if( compilingDisplayList )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	for( GLsizei offset = 0; offset < range && list <= UINT_MAX - (GLuint)offset; ++offset )
	{
		struct DisplayList *entry = FindDisplayList( list + (GLuint)offset );
		if( entry )
		{
			free( entry->commands );
			memset( entry, 0, sizeof( *entry ));
		}
	}
}

GLboolean GL_MANGLE( glIsList )( GLuint list )
{
	struct DisplayList *entry = FindDisplayList( list );
	return entry && entry->compiled;
}

void GL_MANGLE( glNewList )( GLuint list, GLenum mode )
{
	if( compilingDisplayList || executingDisplayList )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	if( mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	struct DisplayList *entry = FindDisplayList( list );
	if( !entry )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	FlushOnStateChange();
	entry->count = 0;
	entry->compiled = GL_FALSE;
	compilingDisplayList = entry;
	displayListCompileMode = mode;
}

void GL_MANGLE( glEndList )( void )
{
	if( !compilingDisplayList )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	compilingDisplayList->compiled = GL_TRUE;
	compilingDisplayList = NULL;
	displayListCompileMode = 0;
}

void GL_MANGLE( glCallList )( GLuint list )
{
	if( RecordDisplayListCommand( DL_CALL, list, NULL ))
		return;
	struct DisplayList *entry = FindDisplayList( list );
	if( !entry || !entry->compiled )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	if( displayListDepth >= DISPLAY_LIST_MAX_DEPTH )
	{
		SetWrapperError( GL_STACK_OVERFLOW );
		return;
	}
	FlushOnStateChange();
	GLboolean wasExecuting = executingDisplayList;
	executingDisplayList = GL_TRUE;
	++displayListDepth;
	for( size_t i = 0; i < entry->count; ++i )
		ExecuteDisplayListCommand( &entry->commands[i] );
	--displayListDepth;
	executingDisplayList = wasExecuting;
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
	if( AnyTexGenEnabled() )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return; // Texgen conversion currently applies to immediate/client-array-element vertices only.
	}
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

void GL_MANGLE( glMultiTexCoord2fvARB )( GLenum target, const GLfloat *v )
{
	if( !v )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	SetCurrentTexCoord( target, v, 2 );
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

void GL_MANGLE( glMultMatrix )( const GLdouble *m )
{
	if( !m )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	GLfloat matrix[16];
	for( size_t i = 0; i < 16; ++i )
		matrix[i] = (GLfloat)m[i];
	GL_MANGLE_NAME( glMultMatrixf )( matrix );
}

void GL_MANGLE( glCompressedTexImage2D )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data )
{
	if( width < 0 || height < 0 || border != 0 || imageSize < 0 || ( imageSize && !data ))
	{
		SetWrapperError( imageSize && !data ? GL_INVALID_OPERATION : GL_INVALID_VALUE );
		return;
	}
	FlushOnStateChange();
	glEsImpl->glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
	StoreCompressedTextureShadow( target, level, internalformat, width, height, imageSize, data );
}

void GL_MANGLE( glCompressedTexImage2DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data )
{
	GL_MANGLE_NAME( glCompressedTexImage2D )( target, level, internalformat, width, height, border, imageSize, data );
}

void GL_MANGLE( glCompressedTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data )
{
	if( xoffset < 0 || yoffset < 0 || width < 0 || height < 0 || imageSize < 0 || ( imageSize && !data ))
	{
		SetWrapperError( imageSize && !data ? GL_INVALID_OPERATION : GL_INVALID_VALUE );
		return;
	}
	FlushOnStateChange();
	glEsImpl->glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
	InvalidateTextureShadow( activetmuState->boundtexture.value, target, level );
}

void GL_MANGLE( glCompressedTexSubImage2DARB )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data )
{
	GL_MANGLE_NAME( glCompressedTexSubImage2D )( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

void GL_MANGLE( glGetCompressedTexImageARB )( GLenum target, GLint level, GLvoid *pixels )
{
	FlushOnStateChange();
	if( !pixels )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_FALSE );
	if( !entry || !entry->compressed || !entry->compressedReadable )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	if( entry->dataSize )
		memcpy( pixels, entry->pixels, entry->dataSize );
}

void GL_MANGLE( glGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels )
{
	FlushOnStateChange();
	ReadTextureShadow( target, level, format, type, pixels );
}

void GL_MANGLE( glGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params )
{
	if( !params )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	struct TextureShadow *entry = FindTextureShadow( activetmuState->boundtexture.value, target, level, GL_FALSE );
	if( !entry )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		*params = 0;
		return;
	}
	switch( pname )
	{
	case GL_TEXTURE_WIDTH: *params = entry->width; break;
	case GL_TEXTURE_HEIGHT: *params = entry->height; break;
	case GL_TEXTURE_BORDER: *params = 0; break;
	case GL_TEXTURE_INTERNAL_FORMAT: *params = entry->internalFormat; break;
	case GL_TEXTURE_COMPRESSED_IMAGE_SIZE: *params = entry->dataSize > INT_MAX ? INT_MAX : (GLint)entry->dataSize; break;
	case GL_TEXTURE_RED_SIZE: *params = entry->format == GL_RED || entry->format == GL_RGB || entry->format == GL_RGBA ? 8 : 0; break;
	case GL_TEXTURE_GREEN_SIZE: *params = entry->format == GL_GREEN || entry->format == GL_RGB || entry->format == GL_RGBA ? 8 : 0; break;
	case GL_TEXTURE_BLUE_SIZE: *params = entry->format == GL_BLUE || entry->format == GL_RGB || entry->format == GL_RGBA ? 8 : 0; break;
	case GL_TEXTURE_ALPHA_SIZE: *params = entry->format == GL_ALPHA || entry->format == GL_LUMINANCE_ALPHA || entry->format == GL_RGBA ? 8 : 0; break;
	default:
		SetWrapperError( GL_INVALID_ENUM );
		*params = 0;
		break;
	}
}

void GL_MANGLE( glPixelStorei )( GLenum pname, GLint param )
{
	if(( pname == GL_PACK_ALIGNMENT || pname == GL_UNPACK_ALIGNMENT ) && param != 1 && param != 2 && param != 4 && param != 8 )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	FlushOnStateChange();
	glEsImpl->glPixelStorei( pname, param );
	if( pname == GL_PACK_ALIGNMENT )
		texturePackAlignment = param;
	else if( pname == GL_UNPACK_ALIGNMENT )
		textureUnpackAlignment = param;
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
	if( AnyTexGenEnabled() )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
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
	InvalidateTextureShadow( activetmuState->boundtexture.value, target, level );
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
	InvalidateTextureShadow( texture, textarget, level );
}

void GL_MANGLE( glRenderbufferStorage )( GLenum target, GLenum internalformat, GLsizei width, GLsizei height )
{
	CHECK_FBO_PROC( glRenderbufferStorage );
	FlushOnStateChange();
	glEsImpl->glRenderbufferStorage( target, internalformat, width, height );
}

#undef CHECK_FBO_PROC

void GL_MANGLE( glBindBufferARB )( GLenum target, GLuint buffer )
{
	if( target != GL_ARRAY_BUFFER_ARB && target != GL_ELEMENT_ARRAY_BUFFER_ARB )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return;
	}
	GLuint previous = BoundBufferForTarget( target );
	if( target == GL_ARRAY_BUFFER_ARB )
	{
		if( buffer && !previous && !skipnanogl )
			FlushOnStateChange();
		glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
		glEsImpl->glColor4f( currentVertexAttrib.red / 255.0f, currentVertexAttrib.green / 255.0f, currentVertexAttrib.blue / 255.0f, currentVertexAttrib.alpha / 255.0f );
		skipnanogl = ( buffer != 0 ) || vboarray;
		boundArrayBuffer = buffer;
		glEsImpl->glBindBuffer( target, buffer );
		if( previous && !buffer )
		{
			arraysValid = GL_FALSE;
			if( !skipnanogl )
			{
				glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
				tmuState0.color_array.changed = GL_TRUE;
				tmuState1.color_array.changed = GL_TRUE;
				tmuState0.texture_coord_array.changed = GL_TRUE;
				tmuState1.texture_coord_array.changed = GL_TRUE;
				ResetNanoState();
			}
		}
	}
	else
	{
		boundElementArrayBuffer = buffer;
		glEsImpl->glBindBuffer( target, buffer );
	}
	if( buffer && !FindBufferShadow( buffer, GL_TRUE ))
		SetWrapperError( GL_OUT_OF_MEMORY );
}

void GL_MANGLE( glGenBuffersARB )( GLsizei count, GLuint *buffers )
{
	if( count < 0 || ( count && !buffers ))
	{
		SetWrapperError( count < 0 ? GL_INVALID_VALUE : GL_INVALID_OPERATION );
		return;
	}
	glEsImpl->glGenBuffers( count, buffers );
	for( GLsizei i = 0; i < count; ++i )
		if( buffers[i] && !FindBufferShadow( buffers[i], GL_TRUE ))
			SetWrapperError( GL_OUT_OF_MEMORY );
}

void GL_MANGLE( glDeleteBuffersARB )( GLsizei count, const GLuint *buffers )
{
	if( count < 0 || ( count && !buffers ))
	{
		SetWrapperError( count < 0 ? GL_INVALID_VALUE : GL_INVALID_OPERATION );
		return;
	}
	glEsImpl->glDeleteBuffers( count, buffers );
	for( GLsizei i = 0; i < count; ++i )
		DeleteBufferShadow( buffers[i] );
	if( !boundArrayBuffer && skipnanogl && !vboarray )
	{
		skipnanogl = GL_FALSE;
		arraysValid = GL_FALSE;
		ResetNanoState();
	}
}

void GL_MANGLE( glBufferDataARB )( GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage )
{
	GLuint id = BoundBufferForTarget( target );
	if( !id )
	{
		SetWrapperError( target != GL_ARRAY_BUFFER_ARB && target != GL_ELEMENT_ARRAY_BUFFER_ARB ? GL_INVALID_ENUM : GL_INVALID_OPERATION );
		return;
	}
	if( size < 0 || size > INT_MAX )
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	struct BufferShadow *entry = FindBufferShadow( id, GL_TRUE );
	if( !entry )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	if( entry->mapped )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	GLubyte *copy = size ? (GLubyte *)malloc((size_t)size) : NULL;
	if( size && !copy )
	{
		SetWrapperError( GL_OUT_OF_MEMORY );
		return;
	}
	if( size )
	{
		if( data ) memcpy( copy, data, (size_t)size );
		else memset( copy, 0, (size_t)size );
	}
	if( usage == GL_STREAM_DRAW_ARB )
		usage = GL_DYNAMIC_DRAW_ARB;
	glEsImpl->glBufferData( target, (GLint)size, data, usage );
	free( entry->data );
	entry->data = copy;
	entry->size = (size_t)size;
	entry->usage = usage;
	entry->mapped = GL_FALSE;
}

void GL_MANGLE( glBufferSubDataARB )( GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data )
{
	GLuint id = BoundBufferForTarget( target );
	struct BufferShadow *entry = FindBufferShadow( id, GL_FALSE );
	if( !id || !entry )
	{
		SetWrapperError( target != GL_ARRAY_BUFFER_ARB && target != GL_ELEMENT_ARRAY_BUFFER_ARB ? GL_INVALID_ENUM : GL_INVALID_OPERATION );
		return;
	}
	if( entry->mapped )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return;
	}
	if( offset < 0 || size < 0 || (uintmax_t)offset + (uintmax_t)size > entry->size || ( size && !data ))
	{
		SetWrapperError( GL_INVALID_VALUE );
		return;
	}
	glEsImpl->glBufferSubData( target, (GLint)offset, (GLint)size, data );
	if( size )
		memcpy( entry->data + (size_t)offset, data, (size_t)size );
}

GLvoid *GL_MANGLE( glMapBufferARB )( GLenum target, GLenum access )
{
	if( target != GL_ARRAY_BUFFER_ARB && target != GL_ELEMENT_ARRAY_BUFFER_ARB )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return NULL;
	}
	if( access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return NULL;
	}
	struct BufferShadow *entry = FindBufferShadow( BoundBufferForTarget( target ), GL_FALSE );
	if( !entry || entry->mapped )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return NULL;
	}
	FlushOnStateChange();
	entry->mapped = GL_TRUE;
	return entry->data;
}

GLboolean GL_MANGLE( glUnmapBufferARB )( GLenum target )
{
	if( target != GL_ARRAY_BUFFER_ARB && target != GL_ELEMENT_ARRAY_BUFFER_ARB )
	{
		SetWrapperError( GL_INVALID_ENUM );
		return GL_FALSE;
	}
	struct BufferShadow *entry = FindBufferShadow( BoundBufferForTarget( target ), GL_FALSE );
	if( !entry || !entry->mapped )
	{
		SetWrapperError( GL_INVALID_OPERATION );
		return GL_FALSE;
	}
	glEsImpl->glBufferData( target, (GLint)entry->size, entry->data, entry->usage );
	entry->mapped = GL_FALSE;
	return GL_TRUE;
}

GLboolean GL_MANGLE( glIsEnabled )( GLenum cap )
{
	int coord = TexGenCoordIndex( cap );
	if( coord >= 0 )
	{
		int unit = ActiveTextureUnitIndex();
		if( unit < 0 )
		{
			SetWrapperError( GL_INVALID_ENUM );
			return GL_FALSE;
		}
		return texgenStates[unit][coord].enabled;
	}
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
NANOGL_ALIAS( glCompressedTexImage2DARB );
NANOGL_ALIAS( glCompressedTexSubImage2D );
NANOGL_ALIAS( glCompressedTexSubImage2DARB );
NANOGL_ALIAS( glCopyTexImage2D );
NANOGL_ALIAS( glCopyTexSubImage2D );
NANOGL_ALIAS( glCullFace );
NANOGL_ALIAS( glDebugMessageCallbackARB );
NANOGL_ALIAS( glDebugMessageControlARB );
NANOGL_ALIAS( glDebugMessageInsertARB );
NANOGL_ALIAS( glDeleteBuffersARB );
NANOGL_ALIAS( glDeleteFramebuffers );
NANOGL_ALIAS( glDeleteLists );
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
NANOGL_ALIAS( glEndList );
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
NANOGL_ALIAS( glGenLists );
NANOGL_ALIAS( glGenRenderbuffers );
NANOGL_ALIAS( glGenTextures );
NANOGL_ALIAS( glGetBooleanv );
NANOGL_ALIAS( glGetCompressedTexImageARB );
NANOGL_ALIAS( glGetDebugMessageLogARB );
NANOGL_ALIAS( glGetDoublev );
NANOGL_ALIAS( glGetError );
NANOGL_ALIAS( glGetFloatv );
NANOGL_ALIAS( glGetIntegerv );
NANOGL_ALIAS( glGetString );
NANOGL_ALIAS( glGetTexImage );
NANOGL_ALIAS( glGetTexLevelParameteriv );
NANOGL_ALIAS( glGetTexParameteriv );
NANOGL_ALIAS( glHint );
NANOGL_ALIAS( glIsEnabled );
NANOGL_ALIAS( glIsList );
NANOGL_ALIAS( glIsRenderbuffer );
NANOGL_ALIAS( glIsTexture );
NANOGL_ALIAS( glLightModelf );
NANOGL_ALIAS( glLightModelfv );
NANOGL_ALIAS( glLightModeli );
NANOGL_ALIAS( glLightf );
NANOGL_ALIAS( glLightfv );
NANOGL_ALIAS( glLineWidth );
NANOGL_ALIAS( glLoadIdentity );
NANOGL_ALIAS( glLoadMatrixf );
NANOGL_ALIAS( glMapBufferARB );
NANOGL_ALIAS( glMaterialf );
NANOGL_ALIAS( glMaterialfv );
NANOGL_ALIAS( glMatrixMode );
NANOGL_ALIAS( glMultMatrix );
NANOGL_ALIAS( glMultMatrixf );
NANOGL_ALIAS( glMultiTexCoord1f );
NANOGL_ALIAS( glMultiTexCoord1fARB );
NANOGL_ALIAS( glMultiTexCoord2f );
NANOGL_ALIAS( glMultiTexCoord2fARB );
NANOGL_ALIAS( glMultiTexCoord2fvARB );
NANOGL_ALIAS( glMultiTexCoord3f );
NANOGL_ALIAS( glMultiTexCoord3fARB );
NANOGL_ALIAS( glMultiTexCoord4f );
NANOGL_ALIAS( glMultiTexCoord4fARB );
NANOGL_ALIAS( glNewList );
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
NANOGL_ALIAS( glRotate );
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
NANOGL_ALIAS( glTexGenf );
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
NANOGL_ALIAS( glTranslate );
NANOGL_ALIAS( glTranslatef );
NANOGL_ALIAS( glUnmapBufferARB );
NANOGL_ALIAS( glVertex2f );
NANOGL_ALIAS( glVertex2i );
NANOGL_ALIAS( glVertex3f );
NANOGL_ALIAS( glVertex3fv );
NANOGL_ALIAS( glVertex4f );
NANOGL_ALIAS( glVertex4fv );
NANOGL_ALIAS( glVertexPointer );
NANOGL_ALIAS( glViewport );
#undef NANOGL_ALIAS
#endif
