ALOCK
=====

'alock' is a simple screen lock application, which locks the X server until
the correct password is provided. If the authentication was successful, the X
server is unlocked and the user can continue to work. When 'alock' is started
it just waits for the first keypress. This first keypress is to indicate that
the user now wants to type in the password. Such a behavior might seem to be
annoying at the first glance, however this approach is chosen due to security
reasons.

Note, that 'alock' does not provide any fancy animations like 'xlock' and
'xscreensaver' and probably never will. It's just for locking the current X
session.


Instalation
-----------

	$ autoreconf --install
	$ mkdir build && cd build
	$ ../configure --enable-pam --enable-hash --enable-xrender --enable-imlib2
	$ make && make install


Usage
-----

	alock [-h] [-auth type:opts] [-bg type:opts] [-cursor type:opts] [-input type:opts]

The locking behavior can be customized with four 'alock' modules:
authentication module (`-auth`), background module (`-bg`), cursor module
(`-cursor`) and input module (`-input`). When 'list' value is provided for
any of them, the list of all available (compiled in) types is shown.

When 'alock' is compiled with all modules, then running it without any
arguments is equivalent to:

	$ alock -auth "pam" -input "frame:color=green|yellow|red" -bg "blank:color=black" -cursor "none"

List of authentication module types:

* none - no authentication at all
* pam - authenticate against the users system-password using the 'pam-login' module
* passwd - authenticate against the users system-password
* hash - authenticate using arbitrary hash comparison

List of background module types:

* none - transparent background
* blank - fill the background with color
* shade - dim content of the screen
* image - use image as a background

List of cursor module types:

* none - no change to the current cursor
* glyph - use compiled in glyph
* xcursor - use xcursor file
* image - use image

List of input module types:

* none - no input feedback for user
* frame - display frame on the current display
