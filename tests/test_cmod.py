# -*- coding: utf-8 -*-

from __future__ import absolute_import

import cairo
import pytest

try:
    from . import cmod
except ImportError:
    pytest.skip("cmod not built", allow_module_level=True)

def test_foo():
    surface = cmod.create_image_surface()
    assert isinstance(surface, cairo.ImageSurface)
