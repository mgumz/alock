#!/bin/env python
##########################################################
#
# scons - buildsystem for alock
#
##########################################################

import sys
import os
from scons_alock import *

from SCons.Script.SConscript import SConsEnvironment # just do this once
SConsEnvironment.InstallAsMode = alock_installAs

alock_name = 'alock'
alock_version = '1.0'
alock_optfile = [ 'scons.opts', 'user.opts' ]

alock_target = 'src/alock'

alock_meta_files = [ 
    'README', 
    'CHANGELOG', 
    'LICENSE', 
    'alock.txt',
    'alock.html' ]
alock_contrib_files = [ 
    'contrib/README', 
    'contrib/xcursor-fluxbox',
    'contrib/xcursor-pekwm' ]
alock_manpage = 'alock.1'
alock_doc_files = [ 
    'alock.html', 
    'alock.1' ]

Default(alock_target)
SConsignFile('SConsign')

alock_options = Options(alock_optfile)
alock_options.AddOptions(
        BoolOption('debug', 'build debug version', 0),

        BoolOption('passwd', 'support for -auth passwd', 0),
        BoolOption('shadow', 'support for -auth passwd with shadow', 0),
        BoolOption('pam', 'support for -auth pam', 1),
        BoolOption('hash', 'support for -auth <md5|sha1>', 1),

        BoolOption('imlib2', 'support for -bg image via imlib2', 1),
        BoolOption('xrender', 'support for -bg shade via xrender or -cursor image', 1),
        BoolOption('xpm', 'support for reading images via libxpm', 1),

        BoolOption('xcursor', 'support for -bg xcursor:<file>', 1),

        BoolOption('amd5', 'build a little md5-helper', 0),
        BoolOption('asha1', 'build a little sha1-helper', 0),

        PathOption('PREFIX', 'install-path base', '/usr/local'),
        PathOption('DESTDIR', 'install to $DESTDIR/$PREFIX', '/')
)

alock_env = Environment(options = alock_options,
                         TARFLAGS = '-c -z',
                         TARSUFFIX = '.tgz' 
                       )
alock_options.Update(alock_env)
Help(alock_options.GenerateHelpText(alock_env))

###########################################################
#
# installpaths
alock_instdir = os.path.join('$DESTDIR','$PREFIX')
alock_instdir_bin = os.path.join(alock_instdir,'bin')
alock_instdir_data = os.path.join(alock_instdir,'share')
alock_instdir_meta = os.path.join(alock_instdir_data, alock_name + '-' + alock_version)
alock_instdir_meta_contrib = os.path.join(alock_instdir_meta, 'contrib')
alock_instdir_man = os.path.join(alock_instdir, os.path.join('man', 'man1'))

###########################################################
#
# configuring

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

if alock_env['xpm']:
    alock_env.AppendUnique(
            CPPDEFINES = ['HAVE_XPM'],
            LIBS = [ 'Xpm' ])

if alock_env['imlib2']:
    conf = alock_env.Configure()
    print "Checking for Imlib2... ",
    if not conf.env.WhereIs('imlib2-config'):
        print "cant find 'imlib2-config."
        alock_env['imlib2'] = 0
    else:
        imlib2_env = Environment()
        imlib2_env.ParseConfig('imlib2-config --cflags --libs')
        if not imlib2_env.Dictionary()['LIBS']:
            print "missing imlib2, install it."
            alock_env['imlib2'] = 0
        else:
            print "yes"
            alock_env.AppendUnique(
                CPPDEFINES = [ 'HAVE_IMLIB2' ],
                LIBPATH = imlib2_env.Dictionary()['LIBPATH'],
                CPPAPTH = imlib2_env.Dictionary()['CPPPATH'],
                LIBS = imlib2_env.Dictionary()['LIBS']
            )
    conf.Finish()

if alock_env['xrender']:
    conf = alock_env.Configure()
    if conf.CheckLib('Xrender', 'XRenderCreatePicture', 1):
        alock_env.AppendUnique(
            CPPDEFINES = [ 'HAVE_XRENDER' ],
            LIBS = [ 'Xrender' ]
        )
    else:
        print "sorry, no xrender-support found."
        alock_env['xrender'] = 0
    conf.Finish()

        
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


############################################################################
#
# building

alock_program = SConscript( 'src/SConscript', exports = ['alock_env'])
alock_env.Command('alock.1', 'alock.txt', createManPage)
alock_env.Command('alock.html', 'alock.txt', createHtml)

alock_env.AddPostAction(alock_target, Chmod(alock_target, 0755))
alock_env.AddPostAction('alock.txt', Chmod('alock.txt', 0644))
alock_env.AddPostAction('alock.1', Chmod('alock.1', 0644))
alock_env.AddPostAction('alock.html', Chmod('alock.html', 0644))

############################################################################
# 
# installing
#alock_env.InstallAs(prefixCombiner(alock_instdir_bin, ['alock'], os.sep), [alock_target], 0755)
#alock_env.InstallAs(prefixCombiner(alock_meta_files, alock_instdir_meta, os.sep), alock_meta_files, 0644)
#alock_env.InstallAs(prefixCombiner(alock_instdir_man, [alock_manpage], os.sep), [alock_manpage], 0644)
#alock_env.InstallAs(prefixCombiner(alock_instdir_meta_contrib, alock_contrib_files, os.sep), alock_contrib_files, 0645)

# TODO: add a "scons dist" command which builds a propper tarball
#alock_env.Alias('dist', alock_env.Tar(alock_target + '-' + alock_version,
#                                        alock_distfiles))


############################################################################
#
# aliases

alock_env.Alias('docs', alock_doc_files)
#alock_env.Alias('install', alock_instdir)

# vim:ft=python
