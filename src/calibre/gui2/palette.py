#!/usr/bin/env python2
# vim:fileencoding=utf-8
# License: GPL v3 Copyright: 2019, Kovid Goyal <kovid at kovidgoyal.net>

from __future__ import absolute_import, division, print_function, unicode_literals

from PyQt5.Qt import QColor, QPalette, Qt
from calibre.constants import dark_link_color


dark_link_color = QColor(dark_link_color)


def dark_palette():
    p = QPalette()
    dark_color = QColor(45,45,45)
    disabled_color = QColor(127,127,127)
    p.setColor(p.Window, dark_color)
    p.setColor(p.WindowText, Qt.white)
    p.setColor(p.Base, QColor(18,18,18))
    p.setColor(p.AlternateBase, dark_color)
    p.setColor(p.ToolTipBase, dark_color)
    p.setColor(p.ToolTipText, Qt.white)
    p.setColor(p.Text, Qt.white)
    p.setColor(p.Disabled, p.Text, disabled_color)
    p.setColor(p.Button, dark_color)
    p.setColor(p.ButtonText, Qt.white)
    p.setColor(p.Disabled, p.ButtonText, disabled_color)
    p.setColor(p.BrightText, Qt.red)
    p.setColor(p.Link, dark_link_color)

    p.setColor(p.Highlight, QColor(42, 130, 218))
    p.setColor(p.HighlightedText, Qt.black)
    p.setColor(p.Disabled, p.HighlightedText, disabled_color)
    return p
