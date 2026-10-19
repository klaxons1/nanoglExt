#include "gl.h"
#include "glesinterface.h"
#include "nanogl.h"

#include <stdio.h>
#include <string.h>

#define SVG_WIDTH 640
#define SVG_HEIGHT 480
#define VERTEX_STRIDE 64

typedef struct RenderState
{
	const unsigned char *vertices;
	const unsigned char *colors;
	const unsigned short *indices;
	GLsizei vertexStride;
	GLsizei colorStride;
	GLenum clientTexture;
	FILE *out;
	unsigned int drawCalls;
} RenderState;

static RenderState state;

static const GLubyte *APIENTRY DemoGetString( GLenum name )
{
	static const GLubyte extensions[] = "GL_OES_framebuffer_object";
	(void)name;
	return extensions;
}

static void APIENTRY DemoClientActiveTexture( GLenum texture )
{
	state.clientTexture = texture;
}

static void APIENTRY DemoVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)size;
	(void)type;
	state.vertexStride = stride;
	state.vertices = (const unsigned char *)pointer;
}

static void APIENTRY DemoColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)size;
	(void)type;
	state.colorStride = stride;
	state.colors = (const unsigned char *)pointer;
}

static void APIENTRY DemoTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)size;
	(void)type;
	(void)stride;
	(void)pointer;
}

static void APIENTRY DemoNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	(void)type;
	(void)stride;
	(void)pointer;
}

static void APIENTRY DemoClientState( GLenum stateName )
{
	(void)stateName;
}

static void WriteVertex( unsigned short index )
{
	const GLfloat *vertex = (const GLfloat *)( state.vertices + (size_t)index * state.vertexStride );
	const unsigned char *color = state.colors + (size_t)index * state.colorStride;
	int x = (int)(( vertex[0] * 0.5f + 0.5f ) * SVG_WIDTH);
	int y = (int)(( 0.5f - vertex[1] * 0.5f ) * SVG_HEIGHT);

	fprintf( state.out, "%d,%d", x, y );
	(void)color;
}

static void WriteTriangle( unsigned short i0, unsigned short i1, unsigned short i2 )
{
	const unsigned char *color = state.colors + (size_t)i0 * state.colorStride;
	fprintf( state.out,
		"<polygon points=\"" );
	WriteVertex( i0 );
	fputc( ' ', state.out );
	WriteVertex( i1 );
	fputc( ' ', state.out );
	WriteVertex( i2 );
	fprintf( state.out,
		"\" fill=\"rgb(%u,%u,%u)\" fill-opacity=\"%.3f\" stroke=\"#111827\" stroke-width=\"2\"/>\n",
		color[0], color[1], color[2], color[3] / 255.0 );
}

static void APIENTRY DemoDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	const unsigned short *ix = (const unsigned short *)indices;
	(void)type;
	state.drawCalls++;

	if( mode == GL_TRIANGLES )
	{
		for( GLsizei i = 0; i + 2 < count; i += 3 )
			WriteTriangle( ix[i], ix[i + 1], ix[i + 2] );
	}
	else if( mode == GL_LINES )
	{
		for( GLsizei i = 0; i + 1 < count; i += 2 )
		{
			const GLfloat *a = (const GLfloat *)( state.vertices + (size_t)ix[i] * state.vertexStride );
			const GLfloat *b = (const GLfloat *)( state.vertices + (size_t)ix[i + 1] * state.vertexStride );
			fprintf( state.out, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"#e5e7eb\" stroke-width=\"2\"/>\n",
				( a[0] * 0.5f + 0.5f ) * SVG_WIDTH, ( 0.5f - a[1] * 0.5f ) * SVG_HEIGHT,
				( b[0] * 0.5f + 0.5f ) * SVG_WIDTH, ( 0.5f - b[1] * 0.5f ) * SVG_HEIGHT );
		}
	}
}

static void APIENTRY DemoNop( void )
{
}

void *GL_GetProcAddress( const char *name )
{
	if( !strcmp( name, "glGetString" )) return (void *)DemoGetString;
	if( !strcmp( name, "glClientActiveTexture" )) return (void *)DemoClientActiveTexture;
	if( !strcmp( name, "glVertexPointer" )) return (void *)DemoVertexPointer;
	if( !strcmp( name, "glColorPointer" )) return (void *)DemoColorPointer;
	if( !strcmp( name, "glTexCoordPointer" )) return (void *)DemoTexCoordPointer;
	if( !strcmp( name, "glNormalPointer" )) return (void *)DemoNormalPointer;
	if( !strcmp( name, "glEnableClientState" ) || !strcmp( name, "glDisableClientState" )) return (void *)DemoClientState;
	if( !strcmp( name, "glDrawElements" )) return (void *)DemoDrawElements;
	if( strstr( name, "Framebuffer" ) || strstr( name, "Renderbuffer" )) return NULL;
	return (void *)DemoNop;
}

static void DrawArrayTriangle( void )
{
	static const GLfloat vertices[3][3] = { { -0.92f, -0.62f, 0.0f }, { -0.64f, -0.62f, 0.0f }, { -0.78f, -0.32f, 0.0f } };
	static const GLfloat normals[3][3] = { { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
	static const GLfloat texcoords[3][2] = { { 0, 0 }, { 1, 0 }, { 0.5f, 1 } };
	static const GLubyte colors[3][4] = { { 255, 190, 64, 255 }, { 255, 120, 40, 255 }, { 255, 230, 100, 255 } };

	pglVertexPointer( 3, GL_FLOAT, 0, vertices );
	pglNormalPointer( GL_FLOAT, 0, &normals[0][0] );
	pglColorPointer( 4, GL_UNSIGNED_BYTE, 0, &colors[0][0] );
	pglTexCoordPointer( 2, GL_FLOAT, 0, &texcoords[0][0] );
	pglEnableClientState( GL_VERTEX_ARRAY );
	pglEnableClientState( GL_NORMAL_ARRAY );
	pglEnableClientState( GL_COLOR_ARRAY );
	pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	pglBegin( GL_TRIANGLES );
	pglArrayElement( 0 );
	pglArrayElement( 1 );
	pglArrayElement( 2 );
	pglEnd();
	pglDisableClientState( GL_VERTEX_ARRAY );
	pglDisableClientState( GL_NORMAL_ARRAY );
	pglDisableClientState( GL_COLOR_ARRAY );
	pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

static void DrawFace( const GLfloat color[4], const GLfloat normal[3], const GLfloat coords[4][2] )
{
	pglColor4f( color[0], color[1], color[2], color[3] );
	pglNormal3f( normal[0], normal[1], normal[2] );
	pglBegin( GL_QUADS );
	for( int i = 0; i < 4; ++i )
	{
		pglTexCoord4f( coords[i][0], coords[i][1], 0.0f, 1.0f );
		pglVertex3f( coords[i][0], coords[i][1], 0.0f );
	}
	pglEnd();
}

int main( int argc, char **argv )
{
	const char *filename = argc > 1 ? argv[1] : "demos/compat_result.svg";
	static const GLfloat top[4][2] = { { -0.58f, 0.28f }, { 0.0f, 0.58f }, { 0.58f, 0.28f }, { 0.0f, -0.02f } };
	static const GLfloat left[4][2] = { { -0.58f, 0.28f }, { 0.0f, -0.02f }, { 0.0f, -0.66f }, { -0.58f, -0.36f } };
	static const GLfloat right[4][2] = { { 0.0f, -0.02f }, { 0.58f, 0.28f }, { 0.58f, -0.36f }, { 0.0f, -0.66f } };
	static const GLfloat topColor[4] = { 0.28f, 0.72f, 0.96f, 1.0f };
	static const GLfloat leftColor[4] = { 0.12f, 0.42f, 0.78f, 1.0f };
	static const GLfloat rightColor[4] = { 0.18f, 0.58f, 0.42f, 1.0f };
	static const GLfloat normals[3] = { 0.0f, 0.0f, 1.0f };

	state.clientTexture = GL_TEXTURE0;
	state.out = fopen( filename, "w" );
	if( !state.out )
	{
		perror( filename );
		return 1;
	}
	if( !nanoGL_Init())
	{
		fclose( state.out );
		fprintf( stderr, "NanoGL initialization failed\n" );
		return 1;
	}

	fprintf( state.out, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", SVG_WIDTH, SVG_HEIGHT, SVG_WIDTH, SVG_HEIGHT );
	fprintf( state.out, "<rect width=\"100%%\" height=\"100%%\" fill=\"#101827\"/>\n" );
	fprintf( state.out, "<text x=\"32\" y=\"42\" fill=\"#f3f4f6\" font-family=\"sans-serif\" font-size=\"20\">NanoGL legacy immediate-mode demo</text>\n" );

	DrawFace( topColor, normals, top );
	DrawFace( leftColor, normals, left );
	DrawFace( rightColor, normals, right );
	DrawArrayTriangle();
	pglFlush();

	fprintf( state.out, "<text x=\"32\" y=\"450\" fill=\"#9ca3af\" font-family=\"monospace\" font-size=\"13\">Captured GLES draws: %u | GL_QUADS, glArrayElement, normals, RGBA, 4D texcoords</text>\n", state.drawCalls );
	fprintf( state.out, "</svg>\n" );
	fclose( state.out );
	nanoGL_Destroy();
	return 0;
}
