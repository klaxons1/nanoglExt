/*
 * Export the native GL spellings as compatibility entry points when NanoGL is
 * built with its usual pgl* name mangling.  Some static-link targets (notably
 * Vita ports) call GL entry points directly instead of using the pgl* aliases.
 */
#include "gl.h"

#if defined( NANOGL_MANGLE_PREPEND ) && !defined( __GNUC__ )

void NANOGL_APIENTRY glDeleteFramebuffers( GLsizei n, const GLuint *framebuffers )
{
	pglDeleteFramebuffers( n, framebuffers );
}

GLenum NANOGL_APIENTRY glCheckFramebufferStatus( GLenum target )
{
	return pglCheckFramebufferStatus( target );
}

GLboolean NANOGL_APIENTRY glIsRenderbuffer( GLuint renderbuffer )
{
	return pglIsRenderbuffer( renderbuffer );
}

void NANOGL_APIENTRY glGenTextures( GLsizei n, GLuint *textures )
{
	pglGenTextures( n, textures );
}

void NANOGL_APIENTRY glPixelStorei( GLenum pname, GLint param )
{
	pglPixelStorei( pname, param );
}

void NANOGL_APIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param )
{
	pglTexParameteri( target, pname, param );
}

void NANOGL_APIENTRY glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data )
{
	pglCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
}

void NANOGL_APIENTRY glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
	pglColor4ub( red, green, blue, alpha );
}

void NANOGL_APIENTRY glDeleteRenderbuffers( GLsizei n, const GLuint *renderbuffers )
{
	pglDeleteRenderbuffers( n, renderbuffers );
}

void NANOGL_APIENTRY glFogf( GLenum pname, GLfloat param )
{
	pglFogf( pname, param );
}

void NANOGL_APIENTRY glFogfv( GLenum pname, const GLfloat *params )
{
	pglFogfv( pname, params );
}

void NANOGL_APIENTRY glFogi( GLenum pname, GLint param )
{
	pglFogi( pname, param );
}

void NANOGL_APIENTRY glFrontFace( GLenum mode )
{
	pglFrontFace( mode );
}

void NANOGL_APIENTRY glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels )
{
	pglGetTexImage( target, level, format, type, pixels );
}

GLboolean NANOGL_APIENTRY glIsEnabled( GLenum cap )
{
	return pglIsEnabled( cap );
}

GLboolean NANOGL_APIENTRY glIsTexture( GLuint texture )
{
	return pglIsTexture( texture );
}

void NANOGL_APIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
	pglVertex4f( x, y, z, w );
}

void NANOGL_APIENTRY glVertex4fv( const GLfloat *v )
{
	pglVertex4fv( v );
}

void NANOGL_APIENTRY glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz )
{
	pglNormal3f( nx, ny, nz );
}

void NANOGL_APIENTRY glTexCoord1f( GLfloat s )
{
	pglTexCoord1f( s );
}

void NANOGL_APIENTRY glTexCoord2f( GLfloat s, GLfloat t )
{
	pglTexCoord2f( s, t );
}

void NANOGL_APIENTRY glTexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
	pglTexCoord3f( s, t, r );
}

void NANOGL_APIENTRY glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	pglTexCoord4f( s, t, r, q );
}

void NANOGL_APIENTRY glTexCoord1fv( const GLfloat *v )
{
	pglTexCoord1fv( v );
}

void NANOGL_APIENTRY glTexCoord2fv( const GLfloat *v )
{
	pglTexCoord2fv( v );
}

void NANOGL_APIENTRY glTexCoord3fv( const GLfloat *v )
{
	pglTexCoord3fv( v );
}

void NANOGL_APIENTRY glTexCoord4fv( const GLfloat *v )
{
	pglTexCoord4fv( v );
}

void NANOGL_APIENTRY glMultiTexCoord1f( GLenum target, GLfloat s )
{
	pglMultiTexCoord1f( target, s );
}

void NANOGL_APIENTRY glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t )
{
	pglMultiTexCoord2f( target, s, t );
}

void NANOGL_APIENTRY glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
	pglMultiTexCoord3f( target, s, t, r );
}

void NANOGL_APIENTRY glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	pglMultiTexCoord4f( target, s, t, r, q );
}

void NANOGL_APIENTRY glMultiTexCoord1fARB( GLenum target, GLfloat s )
{
	pglMultiTexCoord1fARB( target, s );
}

void NANOGL_APIENTRY glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t )
{
	pglMultiTexCoord2fARB( target, s, t );
}

void NANOGL_APIENTRY glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r )
{
	pglMultiTexCoord3fARB( target, s, t, r );
}

void NANOGL_APIENTRY glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
	pglMultiTexCoord4fARB( target, s, t, r, q );
}

#endif /* NANOGL_MANGLE_PREPEND */
