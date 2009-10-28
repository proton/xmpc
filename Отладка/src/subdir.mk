################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cover.c \
../src/dbbrowser.c \
../src/etc.c \
../src/extended-interface.c \
../src/interface.c \
../src/lastfm.c \
../src/lists.c \
../src/main-window.c \
../src/main.c \
../src/menu.c \
../src/mpdclient.c \
../src/playlist.c \
../src/preferences-dialog.c \
../src/preferences.c \
../src/song-dialog.c \
../src/songinfo.c \
../src/statusbar.c \
../src/streams-dialog.c \
../src/streams.c \
../src/tray.c \
../src/xfce-arrow-button.c 

OBJS += \
./src/cover.o \
./src/dbbrowser.o \
./src/etc.o \
./src/extended-interface.o \
./src/interface.o \
./src/lastfm.o \
./src/lists.o \
./src/main-window.o \
./src/main.o \
./src/menu.o \
./src/mpdclient.o \
./src/playlist.o \
./src/preferences-dialog.o \
./src/preferences.o \
./src/song-dialog.o \
./src/songinfo.o \
./src/statusbar.o \
./src/streams-dialog.o \
./src/streams.o \
./src/tray.o \
./src/xfce-arrow-button.o 

C_DEPS += \
./src/cover.d \
./src/dbbrowser.d \
./src/etc.d \
./src/extended-interface.d \
./src/interface.d \
./src/lastfm.d \
./src/lists.d \
./src/main-window.d \
./src/main.d \
./src/menu.d \
./src/mpdclient.d \
./src/playlist.d \
./src/preferences-dialog.d \
./src/preferences.d \
./src/song-dialog.d \
./src/songinfo.d \
./src/statusbar.d \
./src/streams-dialog.d \
./src/streams.d \
./src/tray.d \
./src/xfce-arrow-button.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-2.0 -I/usr/include/libmpd-1.0/ -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include -I/usr/include/freetype2 -I/usr/include/libpng12 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


