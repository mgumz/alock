ALOCK(1)
========
Mathias Gumz <akira@fluxbox.org>
v1.0, 25 February 2009

NAME
----
alock - locks the local X display until the correct password is entered.

SYNOPSIS
--------
'alock' [-h] [-v] [-bg type:opts] [-cursor type:opts] [-auth type:opts]

DESCRIPTION
-----------
'alock' locks the X server until the user enters the correct password at the
keyboard. If the authentification was successful, the X server is
unlocked and the user can continue to work.

'alock' does not provide fancy animations like 'xlock' and 'xscreensaver'
and never will. It's just for locking the current X session.

When 'alock' is started it just waits for the first keypress. This first
keypress is to indicate that the user now wants to type in the password.
A colored frame is drawn around the screen and the user can now type in
his password. If it was typed in incorrectly, the colored frame turns red and
the user has to wait a certain timeout.

OPTIONS
-------
-h::
    Print a short help
-v::
    Print the version number

-auth type:options::
    Define the type of the authentification, depends strongly on
    how alock was built:
    - list - Displays a list of available types
    - passwd - Tries to authentificate against the users system-password.
               On systems using 'shadow' alock needs the suid-flag set.
    - pam - Tries to authentificate against the users system-password
            using the 'pam-login'-module.
    - md5 - alock creates a md5-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename>  - use content of <filename> as reference
    - sha1 - alock creates a sha1-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename> - use <filename> as reference
    - sha256 - alock creates a sha256-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename> - use <filename> as reference
    - sha384 - alock creates a sha384-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename> - use <filename> as reference
    - sha512 - alock creates a sha512-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename> - use <filename> as reference
    - wpool - alock creates a whirlpool-hash from the entered passphrase and compares it with the hash provided
        * hash=<hash> - use <hash> as reference
        * file=<filename> - use <filename> as reference


-bg type:options::
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

-cursor type:options::
    Define the look-a-like of the cursor/mouse pointer:
    - list - Displays a list of available types
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
    - none - No change to the current cursor

AUTHOR
------
Written by Mathias Gumz <akira at fluxbox dot org>
Based upon xtrlock

MD5
~~~
Code taken from OpenBSD, which took it from public domain.
Original author was Colin Plumb.

SHA1
~~~~
Code based upon public domain code, originally written by
Steve Reid <steve@edmweb.com>

SHA2
~~~~
Code based upon OpenBSD code, originally public domain and
written by Aaron D. Gifford <me@aarongifford.com>.

Whirlpool
~~~~~~~~~
Based upon public domain, originally written by

Paulo S. L. M. Barreto <pbarreto@scopus.com.br>
Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>

RESOURCES
---------

'alock' - Webpages ::
- link:http://darkshed.net[Home]
- link:http://code.google.com/p/alock/issues/list[Issue Tracker]

Algorithms ::
- link:http://en.wikipedia.org/wiki/WHIRLPOOL[whirlpool hash]
- link:http://en.wikipedia.org/wiki/SHA_hash_functions[sha based hashfunctions]
- link:http://en.wikipedia.org/wiki/Md5[md5]


Other Lockers ::
- link:http://www.tux.org/~bagleyd/xlockmore.html[xlockmore]
- link:http://www.jwz.org/xscreensaver/[xscreensaver]
- link:ftp://ftp.debian.org/debian/dists/stable/main/source/x11/[xtrlock]
- link:ftp://ftp.ibiblio.org/pub/Linux/utils/console/[vlock]
- link:ftp://ftp.ibiblio.org/pub/Linux/utils/console/[lockvc]

COPYING
-------
Copyright (C) 2005 - 2009 Mathias Gumz. Free use of this software is
granted under the terms of the MIT. See LICENSE provided in the
distribution.
