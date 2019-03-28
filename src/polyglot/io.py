#!/usr/bin/env python2
# vim:fileencoding=utf-8
# License: GPL v3 Copyright: 2019, Kovid Goyal <kovid at kovidgoyal.net>

from __future__ import absolute_import, division, print_function, unicode_literals

from io import StringIO, BytesIO


class PolyglotStringIO(StringIO):

    def __init__(self, initial_data=None, encoding='utf-8'):
        StringIO.__init__(self)
        self._encoding_for_bytes = encoding
        if initial_data is not None:
            self.write(initial_data)

    def write(self, x):
        if isinstance(x, bytes):
            x = x.decode(self._encoding_for_bytes)
        StringIO.write(self, x)


class PolyglotBytesIO(BytesIO):

    def __init__(self, initial_data=None, encoding='utf-8'):
        BytesIO.__init__(self)
        self._encoding_for_bytes = encoding
        if initial_data is not None:
            self.write(initial_data)

    def write(self, x):
        if not isinstance(x, bytes):
            x = x.encode(self._encoding_for_bytes)
        BytesIO.write(self, x)
