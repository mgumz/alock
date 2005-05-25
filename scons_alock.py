import os
import shutil

def createManPage(target, source, env):
    """Creates a manpage via asciidoc and xmlto."""
    os.system('asciidoc -d manpage -b docbook -o alock.xml ' + str(source[0]))
    os.system('xmlto man alock.xml')
    os.remove('alock.xml')
    return None

def createHtml(target, source, env):
    """Creates a html-site via asciidoc."""
    os.system('asciidoc -d manpage -b xhtml -o ' + str(target[0]) + ' ' +
            str(source[0]))

def prefixCombiner(prefix, itemlist, glue=''):
    """Returns a list of items where each element is prepend by given
    prefix."""
    result = []
    for item in itemlist:
        result.append(prefix + glue + item)
    return result

# http://scons.tigris.org/servlets/ReadMsg?listName=users&msgNo=2739
# http://scons.tigris.org/servlets/ReadMsg?list=users&msgNo=2783
def alock_installFunc(dest, source, env):
    """Install a source file into a destination by copying it (and its
    permission/mode bits)."""

    owner = env.get('INSTALL_OWNER', None)
    if owner:
        try:
            uid = pwd.getpwnam(owner)[2]
        except TypeError:
            uid = owner
    else:
        uid = -1

    group = env.get('INSTALL_GROUP', None)
    if group:
        try:
            gid = grp.getgrnam(group)[2]
        except TypeError:
            gid = group
    else:
        gid = -1

    mode = env.get('INSTALL_MODE', None)
    if not mode:
        st = os.stat(source)
        mode = (stat.S_IMODE(st[stat.ST_MODE]) | stat.S_IWRITE)
    if isinstance(mode, str):
        mode = int(mode, 8)

    shutil.copy2(source, dest)

    if owner or group:
        os.chown(dest, uid, gid)

    os.chmod(dest, mode)
    return 0
