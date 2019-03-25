#!/usr/bin/env python2
# vim:fileencoding=utf-8
# License: GPL v3 Copyright: 2018, Kovid Goyal <kovid at kovidgoyal.net>

from __future__ import absolute_import, division, print_function, unicode_literals

import sys

is_py3 = sys.version_info.major >= 3


def iterkeys(d):
    return iter(d)


if is_py3:
    def reraise(tp, value, tb=None):
        try:
            if value is None:
                value = tp()
            if value.__traceback__ is not tb:
                raise value.with_traceback(tb)
            raise value
        finally:
            value = None
            tb = None

    import builtins

    zip = builtins.__dict__['zip']
    map = builtins.__dict__['map']
    filter = builtins.__dict__['filter']
    range = builtins.__dict__['range']

    codepoint_to_chr = chr
    unicode_type = str
    string_or_bytes = str, bytes
    long_type = int

    def iteritems(d):
        return iter(d.items())

    def itervalues(d):
        return iter(d.values())

    def environ_item(x):
        if isinstance(x, bytes):
            x = x.decode('utf-8')
        return x

    def exec_path(path, ctx=None):
        ctx = ctx or {}
        with open(path, 'rb') as f:
            code = f.read()
        code = compile(code, f.name, 'exec')
        exec(code, ctx)

else:
    exec("""def reraise(tp, value, tb=None):
    try:
        raise tp, value, tb
    finally:
        tb = None
""")

    from future_builtins import zip, map, filter  # noqa
    range = xrange
    import __builtin__ as builtins

    codepoint_to_chr = unichr
    unicode_type = unicode
    string_or_bytes = unicode, bytes
    long_type = long
    exec_path = execfile

    def iteritems(d):
        return d.iteritems()

    def itervalues(d):
        return d.itervalues()

    def environ_item(x):
        if isinstance(x, unicode_type):
            x = x.encode('utf-8')
        return x
