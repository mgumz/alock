#!/bin/env python
##########################################################
#
# scons - buildsystem for aklock
#
##########################################################

import sys

aklock_version = '0.5'
aklock_optfile = [ 'scons.opts', 'user.opts' ]

aklock_target = 'src/aklock'

Default(aklock_target)

aklock_options = Options(aklock_optfile)
aklock_options.AddOptions(
        BoolOption('debug', 'build debug version', 0),
        BoolOption('passwd', 'support for classic passwd', 0),
        BoolOption('shadow', 'support for shadowpasswords', 0),
        BoolOption('pam', 'support for pam', 1),
        BoolOption('hash', 'support for hashs(sha1,md5)', 1),

        BoolOption('xcursor', 'support xcursor-themes', 1),

        BoolOption('amd5', 'build a little md5-helper', 0),
        BoolOption('asha1', 'build a little sha1-helper', 0),

        PathOption('prefix', 'install-path base', '/usr/local')
)

aklock_env = Environment(options = aklock_options,
                         TARFLAGS = '-c -z',
                         TARSUFFIX = '.tgz')
aklock_options.Update(aklock_env)
Help(aklock_options.GenerateHelpText(aklock_env))

###########################################################
#
#

if sys.platform == "linux2" or sys.platform == "linux-i386":
    aklock_env.AppendUnique(
        CPPDEFINES = [ 'LINUX' ])

aklock_env.AppendUnique(
        CPPDEFINES = [ 'VERSION=\\"'+aklock_version+'\\"' ],
        CPPFLAGS = [ '-Wall' ],
        CPPPATH = [ '/usr/X11R6/include' ],
        LIBPATH = ['/usr/X11R6/lib'],
        LIBS = [ 'X11' ])

if aklock_env['debug']:
    aklock_env.AppendUnique(
            CPPDEFINES = [ 'DEBUG' ],
            LINKFLAGS = [ '-g' ],
            CPPFLAGS = [ '-g' ])

if aklock_env['passwd']:
    aklock_env.AppendUnique(
            CPPDEFINES = [ 'PASSWD_PWD' ],
            LIBS = [ 'crypt' ])

if aklock_env['pam']:
    aklock_env.AppendUnique(
            CPPDEFINES = [ 'PAM_PWD' ],
            LIBS = [ 'pam', 'crypt' ])

    if sys.platform == 'linux2' or sys.platform == 'linux-i386':
        aklock_env.AppendUnique(LIBS = ['pam_misc'])


if aklock_env['shadow']:
    aklock_env.AppendUnique(
            CPPDEFINES = [ 'SHADOW_PWD' ])

if aklock_env['hash']:
    aklock_env.AppendUnique(
            CPPDEFINES = [ 'HASH_PWD' ])

if aklock_env['xcursor']:
    conf = aklock_env.Configure()
    if conf.CheckLib('Xcursor', 'XcursorSupportsARGB', 1):
        aklock_env.AppendUnique(
                CPPDEFINES = [ 'HAVE_XCURSOR' ],
                LIBS = [ 'Xcursor' ])
    else:
        aklock_env['xcursor'] = 0
        print "sorry, no xcursor-support found."
    conf.Finish()


default_targets = [ aklock_target ]
if aklock_env['amd5']:
    default_targets += [ 'src/amd5' ]

if aklock_env['asha1']:
    default_targets += [ 'src/asha1' ]

Default(default_targets)

aklock_options.Save('scons.opts', aklock_env)

aklock_program = SConscript(
            'src/SConscript',
            exports = ['aklock_env']
        )

aklock_env.Install(
            aklock_env['prefix']+'/bin',
            aklock_program
        )

aklock_env.Alias(
            'install',
            aklock_env['prefix']+'/bin'
        )

# TODO: add a "scons dist" command which builds a propper tarball
#aklock_env.Alias('dist', aklock_env.Tar(aklock_target + '-' + aklock_version,
#                                        aklock_distfiles))

# vim:ft=python
