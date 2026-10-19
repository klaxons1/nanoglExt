#include "gl.h"
#include "glesinterface.h"
#include "nanogl.h"
#include <stdio.h>
#include <string.h>

#define MAX_DRAWS   8
#define MAX_INDICES 64

static struct
{
	GLenum mode;
	GLsizei count;
	GLushort indices[MAX_INDICES];
} draws[MAX_DRAWS];

static int numdraws;
static int nonGlesProcLookups;
static GLenum clientTextureUnit = GL_TEXTURE0;
static GLint observedTexCoordSize;
static GLsizei observedTexCoordStride;
static const void *observedTexCoordPointer;
static const void *observedNormalPointer;
static const void *observedVertexPointer;
static const void *observedColorPointer;
static GLint observedVertexSize;

static void APIENTRY StubNop( void )
{
}

static const GLubyte *APIENTRY StubGetString( GLenum name )
{
	(void)name;
	return (const GLubyte *)"GL_IMG_ray_tracing"; // PowerVR Wizard might've died but it still lives in our hearts
}

static void APIENTRY StubGetFloatv( GLenum pname, GLfloat *params )
{
	if( pname == GL_MODELVIEW_MATRIX )
	{
		memset( params, 0, 16 * sizeof( *params ));
		params[0] = params[5] = params[10] = params[15] = 1.0f;
	}
}

static void APIENTRY StubClientActiveTexture( GLenum texture )
{
	clientTextureUnit = texture;
}

static void APIENTRY StubTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)type;
	if( clientTextureUnit == GL_TEXTURE0 )
	{
		observedTexCoordSize = size;
		observedTexCoordStride = stride;
		observedTexCoordPointer = pointer;
	}
}

static void APIENTRY StubNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)type;
	(void)stride;
	observedNormalPointer = pointer;
}

static void APIENTRY StubColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)size;
	(void)type;
	(void)stride;
	observedColorPointer = pointer;
}

static void APIENTRY StubVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)type;
	(void)stride;
	observedVertexSize = size;
	observedVertexPointer = pointer;
}

static void APIENTRY StubDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	if( numdraws >= MAX_DRAWS || count > MAX_INDICES || type != GL_UNSIGNED_SHORT )
	{
		numdraws = MAX_DRAWS + 1;
		return;
	}
	draws[numdraws].mode = mode;
	draws[numdraws].count = count;
	memcpy( draws[numdraws].indices, indices, count * sizeof( GLushort ));
	numdraws++;
}

void *GL_GetProcAddress( const char *name )
{
	if( strstr( name, "Framebuffer" ) || strstr( name, "Renderbuffer" ))
		return NULL;
	if( !strcmp( name, "glGetTexImage" ) || !strcmp( name, "glColorMaterial" ))
		nonGlesProcLookups++;
	if( !strcmp( name, "glDrawElements" ))
		return (void *)StubDrawElements;
	if( !strcmp( name, "glClientActiveTexture" ))
		return (void *)StubClientActiveTexture;
	if( !strcmp( name, "glTexCoordPointer" ))
		return (void *)StubTexCoordPointer;
	if( !strcmp( name, "glNormalPointer" ))
		return (void *)StubNormalPointer;
	if( !strcmp( name, "glColorPointer" ))
		return (void *)StubColorPointer;
	if( !strcmp( name, "glVertexPointer" ))
		return (void *)StubVertexPointer;
	if( !strcmp( name, "glGetString" ))
		return (void *)StubGetString;
	if( !strcmp( name, "glGetFloatv" ))
		return (void *)StubGetFloatv;
	return (void *)StubNop;
}

static int CheckDraw( int draw, GLenum mode, GLsizei count, const GLushort *expect )
{
	if( draw >= numdraws )
		return 1;
	if( draws[draw].mode != mode || draws[draw].count != count )
		return 1;
	if( memcmp( draws[draw].indices, expect, count * sizeof( GLushort )))
		return 1;
	return 0;
}

static void EmitVertices( int n )
{
	for( int i = 0; i < n; i++ )
		pglVertex3f((GLfloat)i, 0.0f, 0.0f );
}

static int Test_ExtensionsString( void )
{
	const char *ext = (const char *)pglGetString( GL_EXTENSIONS );

	if( !strstr( ext, "GL_IMG_ray_tracing" ) || !strstr( ext, "GL_ARB_multitexture" ))
		return 1;
	return 0;
}

static int Test_NoDesktopOnlyGlesLookups( void )
{
	return nonGlesProcLookups != 0;
}

static int Test_GetTexImageRejectedByGles( void )
{
	GLubyte pixels[4] = { 0, 0, 0, 0 };
	pglGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	return pglGetError() != GL_INVALID_OPERATION;
}

static int Test_MissingFramebufferExtensionIsSafe( void )
{
	return pglCheckFramebufferStatus( 0x8D40 ) != 0 || pglGetError() != GL_INVALID_OPERATION;
}

static int Test_UnsupportedDesktopOperationsReportErrors( void )
{
	GLubyte depth[16];
	memset( depth, 0xA5, sizeof( depth ));

	pglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	if( pglGetError() != GL_INVALID_OPERATION )
		return 1;

	pglCallList( 1 );
	if( pglGetError() != GL_INVALID_VALUE )
		return 1;

	pglTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
	if( pglGetError() != GL_INVALID_OPERATION )
		return 1;

	pglReadPixels( 0, 0, 2, 2, GL_DEPTH_COMPONENT, GL_FLOAT, depth );
	if( pglGetError() != GL_INVALID_ENUM )
		return 1;
	for( size_t i = 0; i < sizeof( depth ); ++i )
		if( depth[i] != 0xA5 )
			return 1;
	return 0;
}

static int Test_ObjectLinearTexGen( void )
{
	const GLfloat plane[4] = { 2.0f, 3.0f, 4.0f, 5.0f };
	GLfloat actual[4];
	pglTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
	pglTexGenfv( GL_S, GL_OBJECT_PLANE, plane );
	pglEnable( GL_TEXTURE_GEN_S );
	pglDrawArrays( GL_TRIANGLES, 0, 3 );
	if( pglGetError() != GL_INVALID_OPERATION )
		return 1;
	numdraws = 0;
	pglBegin( GL_TRIANGLES );
	pglVertex3f( 1.0f, 2.0f, 3.0f );
	pglVertex3f( 0.0f, 0.0f, 0.0f );
	pglVertex3f( 1.0f, 0.0f, 0.0f );
	pglEnd();
	nanoGL_Flush();
	pglDisable( GL_TEXTURE_GEN_S );
	if( numdraws != 1 || !observedTexCoordPointer )
		return 1;
	memcpy( actual, observedTexCoordPointer, sizeof( actual ));
	if( actual[0] != 25.0f || actual[1] != 0.5f || actual[2] != 0.75f || actual[3] != 1.0f )
		return 1;

	const GLfloat eyePlane[4] = { 1.0f, 0.0f, 0.0f, 2.0f };
	pglTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
	pglTexGenfv( GL_S, GL_EYE_PLANE, eyePlane );
	pglEnable( GL_TEXTURE_GEN_S );
	if( !pglIsEnabled( GL_TEXTURE_GEN_S ))
		return 1;
	numdraws = 0;
	pglBegin( GL_TRIANGLES );
	pglVertex3f( 1.0f, 2.0f, 3.0f );
	pglVertex3f( 0.0f, 0.0f, 0.0f );
	pglVertex3f( 1.0f, 0.0f, 0.0f );
	pglEnd();
	nanoGL_Flush();
	pglDisable( GL_TEXTURE_GEN_S );
	memcpy( actual, observedTexCoordPointer, sizeof( actual ));
	return numdraws != 1 || actual[0] != 3.0f;
}

static int Test_FourComponentAttributes( void )
{
	const GLfloat expectedTex[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
	const GLfloat expectedNormal[3] = { 1.0f, 2.0f, 3.0f };
	const GLfloat expectedVertex[4] = { 0.0f, 0.0f, 0.0f, 0.5f };
	GLfloat actualTex[4], actualNormal[3], actualVertex[4];

	observedTexCoordSize = observedVertexSize = 0;
	observedTexCoordStride = 0;
	observedTexCoordPointer = observedNormalPointer = observedVertexPointer = NULL;
	pglBegin( GL_TRIANGLES );
	pglTexCoord4f( expectedTex[0], expectedTex[1], expectedTex[2], expectedTex[3] );
	pglNormal3f( expectedNormal[0], expectedNormal[1], expectedNormal[2] );
	pglVertex4f( expectedVertex[0], expectedVertex[1], expectedVertex[2], expectedVertex[3] );
	EmitVertices( 2 );
	pglEnd();
	nanoGL_Flush();

	if( observedTexCoordSize != 4 || observedVertexSize != 4 || observedTexCoordStride < 64 || !observedTexCoordPointer || !observedNormalPointer || !observedVertexPointer )
		return 1;
	memcpy( actualTex, observedTexCoordPointer, sizeof( actualTex ));
	memcpy( actualNormal, observedNormalPointer, sizeof( actualNormal ));
	memcpy( actualVertex, observedVertexPointer, sizeof( actualVertex ));
	return memcmp( actualTex, expectedTex, sizeof( actualTex )) ||
		memcmp( actualNormal, expectedNormal, sizeof( actualNormal )) ||
		memcmp( actualVertex, expectedVertex, sizeof( actualVertex ));
}

static int Test_ArrayElementCapturesClientArrays( void )
{
	static const GLfloat vertices[] = { -1, -1, 0, 1, 0, -1, 0, 1, 0, 1, 0, 1 };
	static const GLfloat texcoords[] = { 0, 0, 1, 0, 0.5f, 1 };
	static const GLfloat normals[] = { 0, 0, 1, 0, 1, 0, 1, 0, 0 };
	static const GLubyte colors[] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };
	GLfloat actual[4];
	GLubyte actualColor[4];

	pglVertexPointer( 3, GL_FLOAT, 0, vertices );
	pglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colors );
	pglNormalPointer( GL_FLOAT, 0, normals );
	pglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
	pglEnableClientState( GL_VERTEX_ARRAY );
	pglEnableClientState( GL_COLOR_ARRAY );
	pglEnableClientState( GL_NORMAL_ARRAY );
	pglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	pglBegin( GL_TRIANGLES );
	pglArrayElement( 2 );
	pglArrayElement( 0 );
	pglArrayElement( 1 );
	pglEnd();
	nanoGL_Flush();

	if( !observedVertexPointer || !observedNormalPointer || !observedTexCoordPointer || !observedColorPointer )
		return 1;
	memcpy( actual, observedVertexPointer, sizeof( actual ));
	if( actual[0] != 0.0f || actual[1] != 1.0f || actual[2] != 0.0f )
		return 1;
	memcpy( actualColor, observedColorPointer, sizeof( actualColor ));
	if( actualColor[0] != 0 || actualColor[1] != 0 || actualColor[2] != 255 || actualColor[3] != 255 )
		return 1;
	memcpy( actual, observedTexCoordPointer, sizeof( actual ));
	if( actual[0] != 0.5f || actual[1] != 1.0f || actual[2] != 0.0f || actual[3] != 1.0f )
		return 1;
	memcpy( actual, observedNormalPointer, 3 * sizeof( GLfloat ));
	return actual[0] != 1.0f || actual[1] != 0.0f || actual[2] != 0.0f;
}

static int Test_TwoQuads( void )
{
	static const GLushort expect[] = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };

	numdraws = 0;
	pglBegin( GL_QUADS );
	EmitVertices( 8 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_TRIANGLES, 12, expect );
}

static int Test_Lines( void )
{
	static const GLushort expect[] = { 0, 1, 2, 3 };

	numdraws = 0;
	pglBegin( GL_LINES );
	EmitVertices( 4 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_LINES, 4, expect );
}

static int Test_LineStrip( void )
{
	static const GLushort expect[] = { 0, 1, 1, 2, 2, 3 };

	numdraws = 0;
	pglBegin( GL_LINE_STRIP );
	EmitVertices( 4 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_LINES, 6, expect );
}

static int Test_LineLoop( void )
{
	static const GLushort expect[] = { 0, 1, 1, 2, 2, 0 };

	numdraws = 0;
	pglBegin( GL_LINE_LOOP );
	EmitVertices( 3 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_LINES, 6, expect );
}

static int Test_ClassChangeFlush( void )
{
	static const GLushort expectquad[] = { 0, 1, 2, 0, 2, 3 };
	static const GLushort expectline[] = { 0, 1 };

	numdraws = 0;
	pglBegin( GL_QUADS );
	EmitVertices( 4 );
	pglEnd();
	pglBegin( GL_LINES );
	EmitVertices( 2 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 2 )
		return 1;
	return CheckDraw( 0, GL_TRIANGLES, 6, expectquad ) | CheckDraw( 1, GL_LINES, 2, expectline );
}

static int Test_OddLinesVertexDropped( void )
{
	static const GLushort expect[] = { 0, 1, 2, 3 };

	numdraws = 0;
	pglBegin( GL_LINES );
	EmitVertices( 3 );
	pglEnd();
	pglBegin( GL_LINES );
	EmitVertices( 2 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_LINES, 4, expect );
}

static int Test_Points( void )
{
	static const GLushort expect[] = { 0, 1, 2 };

	numdraws = 0;
	pglBegin( GL_POINTS );
	EmitVertices( 3 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_POINTS, 3, expect );
}

static int Test_QuadsAndQuadStrip( void )
{
	static const GLushort expect[] = {
		0, 1, 2, 0, 2, 3,
		4, 5, 7, 4, 7, 6,
		8, 9, 10, 8, 10, 11
	};

	numdraws = 0;
	pglBegin( GL_QUADS );
	EmitVertices( 4 );
	pglEnd();
	pglBegin( GL_QUAD_STRIP );
	EmitVertices( 4 );
	pglEnd();
	pglBegin( GL_QUADS );
	EmitVertices( 4 );
	pglEnd();
	nanoGL_Flush();

	if( numdraws != 1 )
		return 1;
	return CheckDraw( 0, GL_TRIANGLES, 18, expect );
}

static int Test_CompressedTextureReadback( void )
{
	static const GLubyte block[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	GLubyte result[8] = { 0 };
	GLint imageSize = 0;
	pglBindTexture( GL_TEXTURE_2D, 78 );
	pglCompressedTexImage2D( GL_TEXTURE_2D, 0, 0x8D64, 4, 4, 0, sizeof( block ), block );
	pglGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &imageSize );
	if( imageSize != (GLint)sizeof( block ))
		return 1;
	pglGetCompressedTexImageARB( GL_TEXTURE_2D, 0, result );
	return memcmp( result, block, sizeof( block )) != 0;
}

static int Test_MappedBufferEmulation( void )
{
	static const GLubyte initial[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	GLuint buffer = 42;
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, buffer );
	pglBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof( initial ), initial, GL_DYNAMIC_DRAW_ARB );
	GLubyte *mapped = (GLubyte *)pglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_READ_WRITE );
	if( !mapped || memcmp( mapped, initial, sizeof( initial )))
		return 1;
	mapped[3] = 99;
	if( !pglUnmapBufferARB( GL_ARRAY_BUFFER_ARB ))
		return 1;
	mapped = (GLubyte *)pglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_READ_ONLY );
	if( !mapped || mapped[3] != 99 )
		return 1;
	if( !pglUnmapBufferARB( GL_ARRAY_BUFFER_ARB ))
		return 1;
	pglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	pglDeleteBuffersARB( 1, &buffer );
	return 0;
}

static int Test_TextureShadowReadback( void )
{
	static const GLubyte source[16] = {
		255, 0, 0, 255, 0, 255, 0, 128,
		0, 0, 255, 64, 32, 64, 96, 255
	};
	static const GLubyte replacement[4] = { 7, 8, 9, 10 };
	GLubyte result[16];
	GLubyte alpha[8];
	GLint width = 0, height = 0;
	pglBindTexture( GL_TEXTURE_2D, 77 );
	pglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, source );
	pglGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
	pglGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );
	if( width != 2 || height != 2 )
		return 1;
	memset( result, 0, sizeof( result ));
	pglGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, result );
	if( memcmp( result, source, sizeof( source )))
		return 1;
	memset( alpha, 0, sizeof( alpha ));
	pglGetTexImage( GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alpha );
	if( alpha[0] != 255 || alpha[1] != 128 || alpha[4] != 64 || alpha[5] != 255 )
		return 1;
	pglTexSubImage2D( GL_TEXTURE_2D, 0, 1, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, replacement );
	pglGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, result );
	if( memcmp( result + 4, replacement, sizeof( replacement )))
		return 1;
	pglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, 1 );
	memset( result, 0xA5, sizeof( result ));
	pglGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, result );
	if( pglGetError() != GL_INVALID_OPERATION )
		return 1;
	for( size_t i = 0; i < sizeof( result ); ++i )
		if( result[i] != 0xA5 )
			return 1;
	return 0;
}

static int Test_DisplayListImmediateGeometry( void )
{
	static const GLushort expect[] = { 0, 1, 2 };
	GLuint list = pglGenLists( 1 );
	if( !list || pglIsList( list ))
		return 1;

	numdraws = 0;
	pglNewList( list, GL_COMPILE );
	pglBegin( GL_TRIANGLES );
	pglVertex3f( -1.0f, 0.0f, 0.0f );
	pglVertex3f( 0.0f, 1.0f, 0.0f );
	pglVertex3f( 1.0f, 0.0f, 0.0f );
	pglEnd();
	pglEndList();
	if( numdraws || !pglIsList( list ))
		return 1;

	pglCallList( list );
	nanoGL_Flush();
	if( numdraws != 1 || CheckDraw( 0, GL_TRIANGLES, 3, expect ))
		return 1;

	GLuint nested = pglGenLists( 1 );
	pglNewList( nested, GL_COMPILE );
	pglCallList( list );
	pglEndList();
	numdraws = 0;
	pglCallList( nested );
	nanoGL_Flush();
	if( numdraws != 1 || CheckDraw( 0, GL_TRIANGLES, 3, expect ))
		return 1;
	pglDeleteLists( list, 2 );
	return pglIsList( list ) || pglIsList( nested );
}

static int Test_DrawElementsQuadConversion( void )
{
	static const GLfloat vertices[8][3] = { { 0 } };
	static const GLubyte quadIndices[] = { 0, 1, 2, 3 };
	static const GLushort stripIndices[] = { 4, 5, 6, 7 };
	static const GLushort quadExpected[] = { 0, 1, 2, 0, 2, 3 };
	static const GLushort stripExpected[] = { 4, 5, 7, 4, 7, 6 };
	static const GLushort arraysExpected[] = { 8, 9, 10, 8, 10, 11 };

	pglVertexPointer( 3, GL_FLOAT, 0, vertices );
	pglEnableClientState( GL_VERTEX_ARRAY );
	numdraws = 0;
	pglDrawElements( GL_QUADS, 4, GL_UNSIGNED_BYTE, quadIndices );
	pglDrawElements( GL_QUAD_STRIP, 4, GL_UNSIGNED_SHORT, stripIndices );
	pglDrawArrays( GL_QUADS, 8, 4 );
	if( numdraws != 3 )
		return 1;
	return CheckDraw( 0, GL_TRIANGLES, 6, quadExpected ) |
		CheckDraw( 1, GL_TRIANGLES, 6, stripExpected ) |
		CheckDraw( 2, GL_TRIANGLES, 6, arraysExpected );
}

#define RUN_TEST( fn )                   \
	do                                   \
	{                                    \
		if( fn())                       \
		{                                \
			printf( "FAIL: " #fn "\n" ); \
			ret++;                       \
		}                                \
	}                                    \
	while( 0 )

int main( void )
{
	int ret = 0;

	if( !nanoGL_Init())
	{
		printf( "FAIL: nanoGL_Init\n" );
		return 1;
	}

	RUN_TEST( Test_ExtensionsString );
	RUN_TEST( Test_NoDesktopOnlyGlesLookups );
	RUN_TEST( Test_GetTexImageRejectedByGles );
	RUN_TEST( Test_MissingFramebufferExtensionIsSafe );
	RUN_TEST( Test_UnsupportedDesktopOperationsReportErrors );
	RUN_TEST( Test_FourComponentAttributes );
	RUN_TEST( Test_ObjectLinearTexGen );
	RUN_TEST( Test_ArrayElementCapturesClientArrays );
	RUN_TEST( Test_TwoQuads );
	RUN_TEST( Test_Lines );
	RUN_TEST( Test_LineStrip );
	RUN_TEST( Test_LineLoop );
	RUN_TEST( Test_ClassChangeFlush );
	RUN_TEST( Test_OddLinesVertexDropped );
	RUN_TEST( Test_Points );
	RUN_TEST( Test_QuadsAndQuadStrip );
	RUN_TEST( Test_DrawElementsQuadConversion );
	RUN_TEST( Test_TextureShadowReadback );
	RUN_TEST( Test_MappedBufferEmulation );
	RUN_TEST( Test_CompressedTextureReadback );
	RUN_TEST( Test_DisplayListImmediateGeometry );

	nanoGL_Destroy();

	return ret;
}
