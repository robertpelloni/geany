#! /usr/bin/env python3

import sys

try:
    import gi
except ImportError:
    sys.exit(77) # skip this test, gi module is not available

gi.require_version('Bobgui', '4.0')

from gi.repository import Bobgui

assert isinstance(Bobgui.INVALID_LIST_POSITION, int), 'Bobgui.INVALID_LIST_POSITION is not an int'
