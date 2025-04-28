import cairo
import pytest

try:
    from . import cmod
except ImportError:
    pytest.skip("cmod not built", allow_module_level=True)


def test_create_image_surface() -> None:
    surface = cmod.create_image_surface()
    assert isinstance(surface, cairo.ImageSurface)


def test_create_context() -> None:
    context = cmod.create_context()
    assert isinstance(context, cairo.Context)
    assert isinstance(context.get_target(), cairo.ImageSurface)


def test_check_status() -> None:
    cmod.check_status(cairo.STATUS_SUCCESS)

    with pytest.raises(cairo.Error) as excinfo:
        cmod.check_status(cairo.STATUS_NO_MEMORY)
    assert excinfo.value.status == cairo.STATUS_NO_MEMORY

    with pytest.raises(cairo.Error) as excinfo:
        cmod.check_status(9999)
    assert excinfo.value.status == 9999


def test_capi() -> None:
    cmod.test_capi()
