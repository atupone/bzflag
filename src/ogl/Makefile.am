if CLIENT_INCLUDED
noinst_LTLIBRARIES = libGLKit.la
endif

AM_CPPFLAGS = \
	$(SDL_CFLAGS) \
	$(CONF_CPPFLAGS) \
	-I$(top_srcdir)/include
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

MAINTAINERCLEANFILES = \
	Makefile.in

libGLKit_la_LDFLAGS = -static
libGLKit_la_SOURCES =			\
	Element_Chunk.cxx		\
	OpenGLFramebuffer.cxx		\
	OpenGLGState.cxx		\
	OpenGLLight.cxx			\
	OpenGLMaterial.cxx		\
	OpenGLTexture.cxx		\
	Vertex_Chunk.cxx                \
	VBO_Element.h			\
	VBO_Element.cxx                 \
	VBO_Handler.h			\
	VBO_Handler.cxx                 \
	VBO_Vertex.h                    \
	VBO_Vertex.cxx			\
	OpenGLUtils.cxx			\
	RenderNode.cxx

EXTRA_DIST = \
	README
