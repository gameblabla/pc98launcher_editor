PC98Launcher editor by gameblabla
===================

![Screenshot of PC98Launcher editor app](/SCREENSHOT.png?raw=true "Screenshot of PC98Launcher editor app")

This is a simple Win32 app for Windows XP+ (can be compiled for lower) that allows you to edit/create LAUNCH.DAT files, 
download screenshots from MobyGames using the name specified for the purpose of usinbg it with PC98Launcher.

Using the "Download screenshots" button, this will also prefill the Genre, Year (for the PC-98 platform port), and Developer name from mobygames.
Screenshots also get converted to BMP (with a palette of 240 colors) once done to allow it to be used together with PC98Launcher.
Prefilling "Series" is currently not supported as the Mobygames api currently does not allow people to search for a specific game id that belongs to a group.

You can also select the executable or batch file to run as default for "Start" field.
Normally it expects !start.bat but you can map it to AUTOEXEC.BAT etc...

TODO
====

- Allow creating !start.bat file if it does not exist with notepad and then refresh.
- Show screenshots ?
- Improve conversion for RGB/RGBA PNG files, it's quite rough right now but good enough for thumbnails.
- Eventually make a Qt5/Qt6 or GTK3 version of it. It does work on Wine however.
- Support the Sharp X68000 as well. This would need changes to the downscaler i believe. (Some games target 512x512 or higher)
