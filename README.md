ALOCK
========

'alock' locks the X server until the user enters the correct password at the keyboard. If the
authentication was successful, the X server is unlocked and the user can continue to work. When
'alock' is started it just waits for the first keypress. This first keypress is to indicate that
the user now wants to type in the password.

Note, that 'alock' does not provide fancy animations like 'xlock' and 'xscreensaver' and never
will. It's just for locking the current X session.


SYNOPSIS
--------

	alock [-h] [-bg type:opts] [-cursor type:opts] [-auth type:opts] [-input type:opts]

	-h
		Print a short help

	-auth type:options
		Define the type of the authentification, depends strongly on
		how alock was built:
		- list - Displays a list of available types
		- none - No authentication at all
		- passwd - Tries to authentificate against the users system-password.
						On systems using 'shadow' alock needs the suid-flag set.
		- pam - Tries to authentificate against the users system-password
						using the 'pam-login'-module.
		- md5 - alock creates a md5-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename>  - use content of <filename> as reference
		- sha1 - alock creates a sha1-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename> - use <filename> as reference
		- sha256 - alock creates a sha256-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename> - use <filename> as reference
		- sha384 - alock creates a sha384-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename> - use <filename> as reference
		- sha512 - alock creates a sha512-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename> - use <filename> as reference
		- wpool - alock creates a whirlpool-hash from the entered passphrase and compares
						it with the hash provided
			* hash=<hash> - use <hash> as reference
			* file=<filename> - use <filename> as reference
	
	-input type:options
		Define the type of the input indicator, which should be used while typing password:
		- list - Displays a list of available types
		- none - No input feedback for user
		- frame - Display frame on the current display
			* width=<int> - Valid from 1 upwards
			* color=<color1>|<color2>|<color3> - Use <color> while typing, checking and upon
							error respectively

	-bg type:options
		Define the type of alock should handle the background:
		- list - Displays a list of available types
		- none - You can see everything like it is
		- blank - Fill the background with color
			* color=<color> - Use <color>
		- shade - Dims content of the screen and recolors it.
			* shade=<perc> - Valid from 1 to 99
			* color=<color> - Use <color>
		- image - Use the image <filename> and puts it as the background
			* file=<filename>
			* center
			* scale
			* tile
			* color=<color> - Use <color>
			* shade=<perc> - Valid from 1 to 99

	-cursor type:options
		Define the look-a-like of the cursor/mouse pointer:
		- list - Displays a list of available types
		- none - No change to the current cursor
		- theme - Use the given internal cursor
			* list - Display all possible themes
			* name=<name>
			* bg=<color> - Use forground-color
			* fg=<color> - Use background-color
		- glyph - Use the given glyph of the "cursor"-font
			* list - Display all possible glyph-names
			* name=<name>
			* bg=<color>
			* fg=<color>
		- xcursor - Use the given <filename> in xcursor-format
			* file=<filename> 
		- image - Use the given <filename>
			* file=<filename>


Running program without any arguments is equivalent to:

	$ alock -auth "pam" -input "frame:color=green|yellow|red" -bg "blank:color=black" -cursor "none"


AUTHORS
-------
Originally written by Mathias Gumz (<akira@fluxbox.org>) based upon xtrlock.

MD5 -
Code taken from OpenBSD, which took it from public domain.
Original author was Colin Plumb,

SHA1 -
Code based upon public domain code, originally written by
Steve Reid (<steve@edmweb.com>),

SHA2 -
Code based upon OpenBSD code, originally public domain and
written by Aaron D. Gifford (<me@aarongifford.com>),

Whirlpool -
Based upon public domain, originally written by
Paulo S. L. M. Barreto (<pbarreto@scopus.com.br>) and
Vincent Rijmen (<vincent.rijmen@esat.kuleuven.ac.be>).
