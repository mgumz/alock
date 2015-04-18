ALOCK
=====

'Alock' is a simple screen lock application, which locks the X server until
the correct password is provided. If the authentication was successful, the X
server is unlocked and the user can continue to work. When 'alock' is started
it just waits for the first keypress. This first keypress is to indicate that
the user now wants to type in the password. Such a behavior might seem to be
annoying at the first glance, however this approach is chosen due to security
reasons.

Note, that 'alock' does not provide any fancy animations like 'xlock' and
'xscreensaver' and probably never will. It's just for locking the current X
session.


Installation
------------

	$ autoreconf --install
	$ mkdir build && cd build
	$ ../configure --enable-pam --enable-hash --enable-xrender --enable-imlib2
	$ make && make install


Usage
-----

The locking behavior can be customized with four 'alock' modules:
authentication (`-auth`), background (`-bg`), cursor (`-cursor`) and
input module (`-input`). In order to obtain the list of all available
modules (compiled in), issue the command as follows:

	$ alock --modules

When 'alock' is compiled with all modules, then running it without any
arguments is equivalent to:

	$ alock -auth pam -bg blank:color=black -cursor none \
	        -input frame:input=green,check=yellow,error=red

It is also possible to modify visual behavior of background, cursor and
input modules via the X Resources. All available resource names can be
found in the 'alock' manual file. The exemplary configuration might be as
follows:

	ALock*background*option:  center
	ALock*background*shade:   50
	ALock*cursor*name:        plus
	ALock*input.frame*input:  white
	ALock*input.frame*check:  blue


Available modules
-----------------

List of authentication modules:

* none - no authentication at all (not recommended)
* pam - authenticate against the users system-password using the 'pam-login' module
* passwd - authenticate against the users system-password
* hash - authenticate using arbitrary hash comparison

List of background modules:

* none - transparent background
* blank - fill the background with color
* shade - dim content of the screen
* image - use image as a background

List of cursor modules:

* none - no change to the current cursor
* glyph - use compiled in glyph
* xcursor - use xcursor file
* image - use image

List of input modules:

* none - no input feedback for user
* frame - display frame on the current display
