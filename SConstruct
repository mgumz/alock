#!/bin/env python
##########################################################
#
# scons - buildsystem for alock
#
##########################################################

import sys

alock_version = '0.9'
alock_optfile = [ 'scons.opts', 'user.opts' ]

alock_target = 'src/alock'

Default(alock_target)

alock_options = Options(alock_optfile)
alock_options.AddOptions(
        BoolOption('debug', 'build debug version', 0),
        BoolOption('passwd', 'support for classic passwd', 0),
        BoolOption('shadow', 'support for shadowpasswords', 0),
        BoolOption('pam', 'support for pam', 1),
        BoolOption('hash', 'support for hashs(sha1,md5)', 1),

        BoolOption('imlib2', 'support imagereading via imlib2', 1),
        BoolOption('xcursor', 'support xcursor-themes', 1),

        BoolOption('amd5', 'build a little md5-helper', 0),
        BoolOption('asha1', 'build a little sha1-helper', 0),

        PathOption('prefix', 'install-path base', '/usr/local')
)

alock_env = Environment(options = alock_options,
                         TARFLAGS = '-c -z',
                         TARSUFFIX = '.tgz')
alock_options.Update(alock_env)
Help(alock_options.GenerateHelpText(alock_env))

###########################################################
#
#

if sys.platform == "linux2" or sys.platform == "linux-i386":
    alock_env.AppendUnique(
        CPPDEFINES = [ 'LINUX' ])

alock_env.AppendUnique(
        CPPDEFINES = [ 'VERSION=\\"'+alock_version+'\\"' ],
        CPPFLAGS = [ '-Wall' ],
        CPPPATH = [ '/usr/X11R6/include' ],
        LIBPATH = ['/usr/X11R6/lib'],
        LIBS = [ 'X11' ])

conf = alock_env.Configure()
if not conf.CheckLibWithHeader('X11', 'X11/Xlib.h', 'C', 'XOpenDisplay(0);', 1):
    print "sorry, no headers or libs for X11 found, cant build alock."
    Exit(1)
conf.Finish()
    
if alock_env['debug']:
    alock_env.AppendUnique(
            CPPDEFINES = [ 'DEBUG' ],
            LINKFLAGS = [ '-g' ],
            CPPFLAGS = [ '-g' ])

if alock_env['passwd']:
    alock_env.AppendUnique(
            CPPDEFINES = [ 'PASSWD_PWD' ],
            LIBS = [ 'crypt' ])

if alock_env['pam']:
    alock_env.AppendUnique(
            CPPDEFINES = [ 'PAM_PWD' ],
            LIBS = [ 'pam', 'crypt' ])

    if sys.platform == 'linux2' or sys.platform == 'linux-i386':
        alock_env.AppendUnique(LIBS = ['pam_misc'])


if alock_env['shadow']:
    alock_env.AppendUnique(
            CPPDEFINES = [ 'SHADOW_PWD' ])

if alock_env['hash']:
    alock_env.AppendUnique(
            CPPDEFINES = [ 'HASH_PWD' ])

if alock_env['xcursor']:
    conf = alock_env.Configure()
    if conf.CheckLib('Xcursor', 'XcursorSupportsARGB', 1):
        alock_env.AppendUnique(
                CPPDEFINES = [ 'HAVE_XCURSOR' ],
                LIBS = [ 'Xcursor' ])
    else:
        alock_env['xcursor'] = 0
        print "sorry, no xcursor-support found."
    conf.Finish()

if alock_env['imlib2']:
    conf = alock_env.Configure()
    if not conf.env.WhereIs('imlib2-config'):
        print "cant find 'imlib2-config. exit."
        Exit(1)

    imlib2_env = Environment()
    imlib2_env.ParseConfig('imlib2-config --cflags --libs')
    if not imlib2_env.Dictionary()['LIBS']:
        print "missing imlib2, install it."
        Exit(1)

    alock_env.AppendUnique(
        CPPDEFINES = [ 'HAVE_IMLIB2' ],
        LIBPATH = imlib2_env.Dictionary()['LIBPATH'],
	    CPPAPTH = imlib2_env.Dictionary()['CPPPATH'],
        LIBS = imlib2_env.Dictionary()['LIBS']
    )


############################################################################
#
#
    
default_targets = [ alock_target ]
if alock_env['amd5']:
    default_targets += [ 'src/amd5' ]

if alock_env['asha1']:
    default_targets += [ 'src/asha1' ]

Default(default_targets)

alock_options.Save('scons.opts', alock_env)

alock_program = SConscript(
            'src/SConscript',
            exports = ['alock_env']
        )

alock_env.Install(
            alock_env['prefix']+'/bin',
            alock_program
        )

alock_env.Alias(
            'install',
            alock_env['prefix']+'/bin'
        )

# TODO: add a "scons dist" command which builds a propper tarball
#alock_env.Alias('dist', alock_env.Tar(alock_target + '-' + alock_version,
#                                        alock_distfiles))

# vim:ft=python
