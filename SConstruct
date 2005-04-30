#!/bin/env python
##########################################################
#
# scons - buildsystem for aklock
#
##########################################################

import sys

aklock_version = '0.2'
aklock_optfile = 'scons.opts'

aklock_distfiles = ['aklock.c', 'akcursors.c', 'aklock.h',
                    'lock.bitmap', 'mask.bitmap',
                    'SConstruct', 'README', 'CHANGELOG' ]
aklock_sources = [ 'aklock.c', 'akcursors.c' ]
aklock_target = 'aklock'
Default(aklock_target)


aklock_options = Options(aklock_optfile)
aklock_options.AddOptions(
        BoolOption('debug', 'build debug version', 0),
        BoolOption('shadow', 'support for shadowpasswords', 0),
        BoolOption('pam', 'support for pam', 1),
        PathOption('prefix', 'install-path base', '/usr/local')
)

aklock_env = Environment(options = aklock_options, 
                         TARFLAGS = '-c -z',
                         TARSUFFIX = '.tgz')
Help(aklock_options.GenerateHelpText(aklock_env))

###########################################################
#
#
build = aklock_env.Copy()

if sys.platform == "linux2" or sys.platform == "linux-i386":
    build.AppendUnique(
        CPPDEFINES = [ 'LINUX' ])

build.AppendUnique(
        CPPDEFINES = [ 'VERSION=\\"'+aklock_version+'\\"' ],
        CPPFLAGS = [ '-Wall' ],
        CPPPATH = [ '/usr/X11R6/include' ],
        LIBPATH = ['/usr/X11R6/lib'],
        LIBS = [ 'X11', 'crypt' ])

if build['debug']:
    build.AppendUnique(
            CPPDEFINES = [ 'DEBUG' ],
            LINKFLAGS = [ '-g' ],
            CPPFLAGS = [ '-g' ])

if build['pam']:
    build.AppendUnique(
            CPPDEFINES = [ 'PAM_PWD' ],
            LIBS = [ 'pam' ])

    if sys.platform == 'linux2' or sys.platform == 'linux-i386':
        build.AppendUnique(LIBS = ['pam_misc'])
    

if build['shadow']:
    build.AppendUnique(
            CPPDEFINES = [ 'SHADOW_PWD' ])

aklock_program = build.Program(aklock_target, aklock_sources)
build.AddPostAction(aklock_program, Chmod(aklock_target, 0755))

aklock_env.Install(aklock_env['prefix']+'/bin', aklock_program)
aklock_env.Alias('install', aklock_env['prefix']+'/bin')

# TODO: add a "scons dist" command which builds a propper tarball
#aklock_env.Alias('dist', aklock_env.Tar(aklock_target + '-' + aklock_version,
#                                        aklock_distfiles))

# vim:ft=python
