bin_PROGRAMS = xfmpc

xfmpc_SOURCES =								\
	main.c								\
	main-ui.h							\
	main-window.c							\
	main-window.h							\
	interface.c							\
	interface.h							\
	extended-interface.c						\
	extended-interface.h						\
	playlist.c							\
	playlist.h							\
	dbbrowser.c							\
	dbbrowser.h							\
	lists.c							\
	lists.h							\
	streams.c							\
	streams.h							\
	streams-dialog.c					\
	streams-dialog.h					\
	songinfo.c							\
	songinfo.h							\
	statusbar.c							\
	statusbar.h							\
	preferences.c							\
	preferences.h							\
	preferences-dialog.c						\
	preferences-dialog.h						\
	cover.c							\
	cover.h							\
	mpdclient.c							\
	mpdclient.h							\
	song-dialog.c							\
	song-dialog.h							\
	menu.c							\
	menu.h							\
	tray.c							\
	tray.h							\
	lastfm.c							\
	lastfm.h							\
	etc.c							\
	etc.h							\
	xfce-arrow-button.c						\
	xfce-arrow-button.h

xfmpc_CFLAGS =								\
	@GTK_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBMPD_CFLAGS@							\
	-lnotify						\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"

xfmpc_LDADD =								\
	@GTK_LIBS@							\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBMPD_LIBS@

INCLUDES =								\
	-I${top_srcdir}							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"

if MAINTAINER_MODE
DISTCLEANFILES =							\
	main-ui.h

BUILT_SOURCES =								\
	main-ui.h

main-ui.h: Makefile $(srcdir)/main-ui.xml
	exo-csource --strip-comments --strip-content --static --name=main_ui $(srcdir)/main-ui.xml > main-ui.h
endif

EXTRA_DIST =								\
	main-ui.xml

