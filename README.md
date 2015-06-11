# qt-mpwidget

A Qt widget (QWidget based, not Qt Quick / QML) that implements a simple videoplayer based on mplayer's slave mode. Comes with example application "singleplayer" which plays one video file.

This is a fork of jgehring's qmpwidget project. I'm using it for a home media center app under Fedora 19+, qt 4.8+ and 5+, mplayer SVN-r36171-4.8.1.

Features:

* before playing a movie, try loading the first MB of the moviefile. We do this asynchronuously (in a child process), so if the HD stalls or NFS is blocked we can return "file not accessible" without any delay
* capslock checking
* detect movie crop and adjust mplayer viewport for best display (actual detection via external script, crop size can also be set programmatically)
* inhibit screensavers during play, un-inhibit screensavers when movie is paused. Supports various DBUS interfaces and X11 native calls to communicate with the screensaver
* communication between the mplayer slave mode and the widget is buffered / queued. Otherwise mplayer will lose commands
* robust multiple seek calls, i.e. 3 seeks forward and 1 seek backward result in the same (or similar) position as 2 seeks forward
* a robust detection of "current location" (N minutes and M seconds into the movie) in file
* lots of error checking to see if mplayer has crashed or is blocked
* automatic selection of appropriate "-vo" mplayer parameter (vdpau, radeon, ...)
* hide/unhide mouse cursor as appropriate
* code is compatible with QT_NO_CAST_FROM_ASCII and QT_NO_CAST_FROM_BYTEARRAY
* set/unset deinterlace

UI

* help screen
* a simple seek bar at the bottom, and various keyboard seek
* speed up, slow down
* mute, unmute
* fullscreen or windowed
* cycle through available languages for audio and subtitles
* automatic preferred language selection
* forbidden languages (cycle will skip them)

This code has only ever been used by me. It has been tested extensively, but only on my systems. You might have luck integrating it into your own project or you might get random errors. I'm happy to answer questions though and add changes to make this code more portable and usable.
