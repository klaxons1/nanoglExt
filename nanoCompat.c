/*
 * Export the native GL spellings as compatibility entry points when NanoGL is
 * built with its usual pgl* name mangling.  Some static-link targets (notably
 * Vita ports) call GL entry points directly instead of using the pgl* aliases.
 */
#include "gl.h"

#ifdef NANOGL_MANGLE_PREPEND

void NANOGL_APIENTRY glDeleteFramebuffers( GLsizei n, const GLuint *framebuffers )
{
	pglDeleteFramebuffers( n, framebuffers );
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

#endif /* NANOGL_MANGLE_PREPEND */
