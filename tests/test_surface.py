import io
import os
import sys
import array
import tempfile
import struct

import cairo
import pytest


def test_context_manager() -> None:
    with cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10) as surface:
        surface.show_page()
    with pytest.raises(cairo.Error) as excinfo:
        surface.show_page()
    assert excinfo.value.status == cairo.Status.SURFACE_FINISHED


def test_surface_cmp_hash() -> None:
    main = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    ctx = cairo.Context(main)
    assert ctx.get_target() == main
    assert not ctx.get_target() != main
    assert main != object()
    assert hash(ctx.get_target()) == hash(main)

    # we implement some hackery to change the underlying object after unmap
    # which would change the hash. Make sure this type is unhashable.
    mapped = main.map_to_image(None)
    with pytest.raises(TypeError):
        hash(mapped)
    assert mapped == mapped
    main.unmap_image(mapped)


def test_surface_map_to_image() -> None:
    main = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    image = main.map_to_image(None)

    with pytest.raises(TypeError):
        type(image)()  # type: ignore

    other = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    with pytest.raises(ValueError):
        other.unmap_image(image)

    with pytest.raises(TypeError):
        main.unmap_image(other)

    with pytest.raises(RuntimeError):
        image.finish()

    main.unmap_image(image)

    with pytest.raises(RuntimeError):
        main.unmap_image(image)

    # from here on everything should fail
    with pytest.raises(cairo.Error):
        cairo.Context(image)

    with pytest.raises(TypeError):
        main.map_to_image(object())  # type: ignore

    with pytest.raises(TypeError):
        main.map_to_image()  # type: ignore

    gced = main.map_to_image(None)
    del gced


def test_surface_map_to_image_context_manager() -> None:
    main = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    with main.map_to_image(None) as image:
        pass

    with pytest.raises(RuntimeError):
        main.unmap_image(image)

    with pytest.raises(cairo.Error) as excinfo:
        image.show_page()
    assert excinfo.value.status == cairo.Status.SURFACE_FINISHED

    with pytest.raises(RuntimeError):
        with image:
            pass


def test_surface_map_to_image_data() -> None:
    main = cairo.ImageSurface(cairo.Format.RGB24, 2, 1)

    main.flush()
    assert bytes(main.get_data()) == b"\x00\x00\x00\x00\x00\x00\x00\x00"
    image = main.map_to_image(None)
    ctx = cairo.Context(image)
    ctx.set_source_rgb(1, 1, 1)
    ctx.paint()
    main.unmap_image(image)
    main.flush()
    assert bytes(main.get_data()) == b"\xff\xff\xff\xff\xff\xff\xff\xff"

    main = cairo.ImageSurface(cairo.Format.RGB24, 2, 1)
    image = main.map_to_image(cairo.RectangleInt(0, 0, 1, 1))
    ctx = cairo.Context(image)
    ctx.set_source_rgb(1, 1, 1)
    ctx.paint()
    main.unmap_image(image)
    main.flush()
    assert bytes(main.get_data()) == b"\xff\xff\xff\xff\x00\x00\x00\x00"


@pytest.mark.skipif(not cairo.HAS_TEE_SURFACE, reason="no tee surface")
def test_tee_surface() -> None:
    main = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    tee = cairo.TeeSurface(main)
    assert isinstance(tee, cairo.TeeSurface)
    with pytest.raises(TypeError):
        cairo.TeeSurface(object())  # type: ignore
    with pytest.raises(TypeError):
        tee.add(object())  # type: ignore
    with pytest.raises(TypeError):
        tee.remove(object())  # type: ignore
    with pytest.raises(TypeError):
        tee.index(object())  # type: ignore
    # the API is horrible, passing a wrong arg sets the surface to an error
    # state instead of returning the status.
    s1 = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    tee.add(s1)
    assert tee.index(0)
    assert tee.index(1)
    tee.remove(s1)


@pytest.mark.skipif(not hasattr(sys, "getrefcount"), reason="PyPy")
def test_image_surface_get_data_refcount() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    refcount = sys.getrefcount(surface)
    d = surface.get_data()
    assert sys.getrefcount(surface) != refcount
    del d
    assert sys.getrefcount(surface) == refcount


def test_image_surface_get_data_crasher() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    try:
        assert 0, surface.get_data()
    except Exception:
        pass


def test_surface_get_content() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    assert surface.get_content() == cairo.Content.COLOR_ALPHA
    assert isinstance(surface.get_content(), cairo.Content)


def test_surface_get_format() -> None:
    surface = cairo.ImageSurface(cairo.Format.ARGB32, 10, 10)
    assert surface.get_format() == cairo.Format.ARGB32
    assert isinstance(surface.get_format(), cairo.Format)


def test_pdf_get_error() -> None:
    cairo.PDFSurface(io.BytesIO(), 10, 10)
    with pytest.raises(TypeError):
        cairo.PDFSurface(object(), 10, 10)  # type: ignore
    with pytest.raises(TypeError):
        cairo.PDFSurface(io.StringIO(), 10, 10)  # type: ignore


def test_pdf_get_versions() -> None:
    versions = cairo.PDFSurface.get_versions()
    assert isinstance(versions, list)
    assert all(isinstance(v, cairo.PDFVersion) for v in versions)


def test_pdf_set_size() -> None:
    fileobj = io.BytesIO()
    surface = cairo.PDFSurface(fileobj, 128, 128)
    surface.set_size(10, 10)
    with pytest.raises(TypeError):
        surface.set_size(10, object())  # type: ignore


@pytest.mark.skipif(not hasattr(cairo.PDFSurface, "set_page_label"),
                    reason="too old cairo")
def test_pdf_set_page_label() -> None:
    fileobj = io.BytesIO()
    with cairo.PDFSurface(fileobj, 128, 128) as surface:
        surface.set_page_label("foo")
        surface.set_page_label("bar")
    with pytest.raises(cairo.Error):
        surface.set_page_label("bar")


@pytest.mark.skipif(not hasattr(cairo.PDFSurface, "set_metadata"),
                    reason="too old cairo")
def test_pdf_set_metadata() -> None:
    fileobj = io.BytesIO()
    with cairo.PDFSurface(fileobj, 128, 128) as surface:
        surface.set_metadata(cairo.PDFMetadata.TITLE, "title")
        surface.set_metadata(cairo.PDFMetadata.TITLE, "title")
        surface.set_metadata(cairo.PDFMetadata.AUTHOR, "author")
    with pytest.raises(cairo.Error):
        surface.set_metadata(cairo.PDFMetadata.AUTHOR, "author")


@pytest.mark.skipif(not hasattr(cairo.PDFSurface, "set_custom_metadata"),
                    reason="too old cairo")
def test_pdf_set_custom_metadata() -> None:
    fileobj = io.BytesIO()
    with cairo.PDFSurface(fileobj, 128, 128) as surface:
        surface.set_custom_metadata("ISBN", "978-0123456789")
        with pytest.raises(cairo.Error):
            surface.set_custom_metadata("Author", "Author isn't allowed")


@pytest.mark.skipif(not hasattr(cairo.PDFSurface, "add_outline"),
                    reason="too old cairo")
def test_pdf_add_outline() -> None:
    fileobj = io.BytesIO()
    with cairo.PDFSurface(fileobj, 128, 128) as surface:
        res = surface.add_outline(
            cairo.PDF_OUTLINE_ROOT, "foo", "page=3 pos=[3.1 6.2]",
            cairo.PDFOutlineFlags.OPEN)
        assert isinstance(res, int)


@pytest.mark.skipif(not hasattr(cairo.PDFSurface, "set_thumbnail_size"),
                    reason="too old cairo")
def test_pdf_set_thumbnail_size() -> None:
    fileobj = io.BytesIO()
    with cairo.PDFSurface(fileobj, 128, 128) as surface:
        surface.set_thumbnail_size(10, 10)
        surface.set_thumbnail_size(0, 0)
        surface.set_thumbnail_size(1, 1)
    with pytest.raises(cairo.Error):
        surface.set_thumbnail_size(0, 0)


def test_pdf_surface() -> None:
    fd, fname = tempfile.mkstemp()
    os.close(fd)
    try:
        cairo.PDFSurface(fname, 10, 10).finish()
    finally:
        os.unlink(fname)

    with pytest.raises(TypeError):
        cairo.PDFSurface()  # type: ignore

    with pytest.raises((ValueError, TypeError)):
        cairo.PDFSurface("\x00")  # type: ignore

    with pytest.raises(TypeError):
        cairo.PDFSurface(object(), 100, 100)  # type: ignore


def test_svg_version_to_string() -> None:
    ver = cairo.SVGSurface.version_to_string(cairo.SVG_VERSION_1_1)
    assert ver and isinstance(ver, str)
    with pytest.raises(ValueError):
        cairo.SVGSurface.version_to_string(-1)  # type: ignore
    with pytest.raises(TypeError):
        cairo.SVGSurface.version_to_string(object())  # type: ignore


@pytest.mark.skipif(not hasattr(cairo.SVGSurface, "get_document_unit"),
                    reason="too old cairo")
def test_svg_surface_get_document_unit() -> None:
    with cairo.SVGSurface(None, 10, 10) as surface:
        # https://gitlab.freedesktop.org/cairo/cairo/-/issues/545
        assert surface.get_document_unit() in [cairo.SVGUnit.PT, cairo.SVGUnit.USER]

    with cairo.SVGSurface(None, 10, 10) as surface:
        surface.set_document_unit(cairo.SVGUnit.PX)
        assert surface.get_document_unit() == cairo.SVGUnit.PX
    with pytest.raises(cairo.Error):
        surface.set_document_unit(cairo.SVGUnit.PX)


def test_svg_surface_restrict_to_version() -> None:
    surface = cairo.SVGSurface(None, 10, 10)
    surface.restrict_to_version(cairo.SVG_VERSION_1_1)
    surface.finish()
    with pytest.raises(cairo.Error):
        surface.restrict_to_version(cairo.SVG_VERSION_1_2)
    with pytest.raises(TypeError):
        surface.restrict_to_version(object())  # type: ignore


def test_pdf_surface_restrict_to_version() -> None:
    surface = cairo.PDFSurface(None, 10, 10)
    surface.restrict_to_version(cairo.PDF_VERSION_1_4)
    surface.finish()
    with pytest.raises(cairo.Error):
        surface.restrict_to_version(cairo.PDF_VERSION_1_5)
    with pytest.raises(TypeError):
        surface.restrict_to_version(object())  # type: ignore


def test_pdf_version_to_string() -> None:
    ver = cairo.PDFSurface.version_to_string(cairo.PDF_VERSION_1_4)
    assert ver and isinstance(ver, str)
    with pytest.raises(ValueError):
        cairo.PDFSurface.version_to_string(-1)  # type: ignore
    with pytest.raises(TypeError):
        cairo.PDFSurface.version_to_string(object())  # type: ignore


def test_ps_surface_error() -> None:
    cairo.PSSurface(io.BytesIO(), 10, 10)
    with pytest.raises(TypeError):
        cairo.PSSurface(object(), 10, 10)  # type: ignore
    with pytest.raises(TypeError):
        cairo.PSSurface(io.StringIO(), 10, 10)  # type: ignore


def test_ps_surface_misc() -> None:
    surface = cairo.PSSurface(None, 10, 10)
    surface.dsc_begin_page_setup()
    surface.dsc_begin_setup()


def test_ps_surface_dsc_comment() -> None:
    surface = cairo.PSSurface(None, 10, 10)
    surface.dsc_comment("%%Title: My excellent document")
    with pytest.raises(cairo.Error):
        surface.dsc_comment("")
    with pytest.raises(TypeError):
        surface.dsc_comment(object())  # type: ignore


def test_ps_get_eps() -> None:
    surface = cairo.PSSurface(None, 10, 10)
    assert isinstance(surface.get_eps(), bool)
    surface.set_eps(True)
    assert surface.get_eps()
    with pytest.raises(TypeError):
        surface.set_eps(object())  # type: ignore


def test_ps_set_size() -> None:
    surface = cairo.PSSurface(None, 10, 10)
    surface.set_size(10, 10)
    with pytest.raises(TypeError):
        surface.set_size(10, object())  # type: ignore


def test_ps_restrict_to_level() -> None:
    surface = cairo.PSSurface(None, 10, 10)
    surface.restrict_to_level(cairo.PSLevel.LEVEL_2)
    with pytest.raises(TypeError):
        surface.restrict_to_level(object())  # type: ignore


def test_ps_surface_level_to_string() -> None:
    level_id = cairo.PSSurface.level_to_string(cairo.PS_LEVEL_2)
    assert isinstance(level_id, str)
    assert cairo.PSSurface.ps_level_to_string(cairo.PS_LEVEL_2) == level_id
    with pytest.raises(ValueError):
        cairo.PSSurface.level_to_string(-1)  # type: ignore
    with pytest.raises(TypeError):
        cairo.PSSurface.level_to_string(object())  # type: ignore


def test_ps_surface_get_levels() -> None:
    levels = cairo.PSSurface.get_levels()
    assert isinstance(levels, list)
    assert all(isinstance(v, cairo.PSLevel) for v in levels)


def test_ps_surface() -> None:
    assert isinstance(cairo.PSSurface(None, 10, 10), cairo.PSSurface)

    fd, fname = tempfile.mkstemp()
    os.close(fd)
    try:
        cairo.PSSurface(fname, 10, 10).finish()
    finally:
        os.unlink(fname)

    with pytest.raises(TypeError):
        cairo.PSSurface()  # type: ignore

    with pytest.raises((ValueError, TypeError)):
        cairo.PSSurface("\x00", 100, 100)

    with pytest.raises(TypeError):
        cairo.PSSurface(object(), 100, 100)  # type: ignore


def test_scg_surface() -> None:
    fd, fname = tempfile.mkstemp()
    os.close(fd)
    try:
        cairo.SVGSurface(fname, 10, 10).finish()
    finally:
        os.unlink(fname)

    with pytest.raises(TypeError):
        cairo.SVGSurface()  # type: ignore

    with pytest.raises((ValueError, TypeError)):
        cairo.SVGSurface("\x00", 10, 10)

    with pytest.raises(TypeError):
        cairo.SVGSurface(object(), 100, 100)  # type: ignore


def test_svg_surface_get_versions() -> None:
    versions = cairo.SVGSurface.get_versions()
    assert isinstance(versions, list)
    assert all(isinstance(v, cairo.SVGVersion) for v in versions)


def test_surface_get_device_scale() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    device_scale = surface.get_device_scale()
    assert all(isinstance(s, float) for s in device_scale)


def test_surface_set_device_scale() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    surface.set_device_scale(5.0, 3.0)

    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 100, 100)
    with pytest.raises(cairo.Error):
        surface.set_device_scale(1, 0)

    with pytest.raises(TypeError):
        surface.set_device_scale(1, object())  # type: ignore

    surface.set_device_scale(sys.float_info.max, -sys.float_info.max)
    assert surface.get_device_scale() == (sys.float_info.max, -sys.float_info.max)

    with pytest.raises(cairo.Error) as excinfo:
        surface.set_device_scale(0, 0)
    assert excinfo.value.status == cairo.Status.INVALID_MATRIX


def test_surface_create_for_rectangle() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 100, 100)
    new = surface.create_for_rectangle(0, 0, 10, 10)
    assert new
    assert isinstance(new, cairo.Surface)

    with pytest.raises(cairo.Error) as excinfo:
        surface.create_for_rectangle(0, 0, 10, -1)
    assert excinfo.value.status == cairo.STATUS_INVALID_SIZE

    with pytest.raises(cairo.Error) as excinfo:
        surface.create_for_rectangle(0, 0, -1, 10)
    assert excinfo.value.status == cairo.STATUS_INVALID_SIZE

    with pytest.raises(TypeError):
        surface.create_for_rectangle(0, 0, 10, object())  # type: ignore

    surface.create_for_rectangle(
        sys.float_info.max, sys.float_info.max,
        sys.float_info.max, sys.float_info.max)

    surface.create_for_rectangle(
        -sys.float_info.max, -sys.float_info.max,
        sys.float_info.min, sys.float_info.min)


def test_surface_create_similar_image() -> None:
    surface = cairo.PDFSurface(None, 1, 1)
    image = surface.create_similar_image(cairo.FORMAT_ARGB32, 24, 42)
    assert image
    assert isinstance(image, cairo.ImageSurface)
    del surface
    assert image.get_width() == 24
    assert image.get_height() == 42
    surface = cairo.PDFSurface(None, 1, 1)
    with pytest.raises(TypeError):
        surface.create_similar_image(cairo.FORMAT_ARGB32, 24, object())  # type: ignore


def test_surface_get_set_mime_data() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_RGB24, 1, 1)
    assert surface.get_mime_data("foo") is None
    assert surface.get_mime_data(cairo.MIME_TYPE_JPEG) is None

    surface.set_mime_data("foo", b"bar")
    assert surface.get_mime_data("foo") == b"bar"
    surface.set_mime_data("foo", None)
    assert surface.get_mime_data("foo") is None

    surface.set_mime_data(cairo.MIME_TYPE_JPEG, b"\x00quux\x00")
    data = surface.get_mime_data(cairo.MIME_TYPE_JPEG)
    assert data is not None
    assert data[:] == b"\x00quux\x00"
    surface.set_mime_data(cairo.MIME_TYPE_JPEG, None)
    assert surface.get_mime_data(cairo.MIME_TYPE_JPEG) is None
    with pytest.raises(TypeError):
        surface.set_mime_data(cairo.MIME_TYPE_JPEG, object())  # type: ignore
    with pytest.raises(TypeError):
        surface.get_mime_data(object())  # type: ignore

    v = memoryview(b"bla")
    surface.set_mime_data("foo", v)
    assert surface.get_mime_data("foo") is v
    surface.set_mime_data("foo", None)
    assert surface.get_mime_data("foo") is None

    vb = bytearray(b"bla")
    surface.set_mime_data("foo", vb)
    assert surface.get_mime_data("foo") is vb
    surface.set_mime_data("foo", None)
    assert surface.get_mime_data("foo") is None


def test_supports_mime_type() -> None:
    surface = cairo.PDFSurface(None, 3, 3)
    assert surface.supports_mime_type(cairo.MIME_TYPE_JPEG)
    assert not surface.supports_mime_type("nope")
    with pytest.raises(TypeError):
        surface.supports_mime_type(object())  # type: ignore


def test_image_surface_create_for_data_array() -> None:
    width, height = 255, 255
    data = array.array('B', [0] * width * height * 4)

    for y in range(height):
        for x in range(width):
            offset = (x + (y * width)) * 4
            alpha = y

            b = int(x * alpha / 255.0)
            g = int(y * alpha / 255.0)
            # cairo.FORMAT_ARGB32 uses pre-multiplied alpha
            data[offset:offset + 4] = array.array(
                "B", struct.pack("=I", b | g << 8 | alpha << 24))

    surface = cairo.ImageSurface.create_for_data(
        data, cairo.FORMAT_ARGB32, width, height)

    # for debugging
    fd, filename = tempfile.mkstemp(prefix='pycairo_', suffix='.png')
    os.close(fd)
    surface.write_to_png(filename)
    os.unlink(filename)


def test_image_surface_write_to_png_filename_and_obj_compare() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 128, 128)
    fd, filename = tempfile.mkstemp(prefix='pycairo_', suffix='.png')
    os.close(fd)
    surface.write_to_png(filename)

    fileobj = io.BytesIO()
    surface.write_to_png(fileobj)

    with open(filename, "rb") as h:
        assert h.read() == fileobj.getvalue()

    os.unlink(filename)


def test_image_surface_png_obj_roundtrip() -> None:
    fileobj = io.BytesIO()
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 128, 128)
    surface.write_to_png(fileobj)
    fileobj.seek(0)
    new_surface = cairo.ImageSurface.create_from_png(fileobj)
    assert surface.get_data() == new_surface.get_data()
    with pytest.raises(TypeError):
        cairo.ImageSurface.create_from_png()  # type: ignore
    with pytest.raises((ValueError, TypeError)):
        cairo.ImageSurface.create_from_png("\x00")
    with pytest.raises(TypeError):
        cairo.ImageSurface.create_from_png(object())  # type: ignore
    with pytest.raises(TypeError):
        cairo.ImageSurface.create_from_png(io.StringIO())  # type: ignore


def test_image_surface_png_file_roundtrip() -> None:
    fd, filename = tempfile.mkstemp(prefix='pycairo_', suffix='.png')
    os.close(fd)

    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 128, 128)
    surface.write_to_png(filename)

    new_surface = cairo.ImageSurface.create_from_png(filename)
    assert surface.get_data() == new_surface.get_data()
    os.unlink(filename)


def test_image_surface_write_to_png_error() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 128, 128)
    with pytest.raises(TypeError):
        surface.write_to_png(42)  # type: ignore
    with pytest.raises((ValueError, TypeError)):
        surface.write_to_png("\x00")


def test_surface_from_stream_closed_before_finished() -> None:
    for _ in [cairo.PDFSurface, cairo.PSSurface, cairo.SVGSurface]:
        fileobj = io.BytesIO()
        surface = cairo.PDFSurface(fileobj, 128, 128)
        fileobj.close()
        with pytest.raises(IOError):
            surface.finish()


def test_script_surface() -> None:
    f = io.BytesIO()
    dev = cairo.ScriptDevice(f)
    surface = cairo.ScriptSurface(dev, cairo.Content.COLOR_ALPHA, 42, 10)
    assert isinstance(surface, cairo.ScriptSurface)
    cairo.Context(surface).paint()
    dev.flush()
    assert b"42" in f.getvalue()
    assert b"paint" in f.getvalue()
    with pytest.raises(TypeError):
        cairo.ScriptSurface()  # type: ignore


def test_script_device_device_ref() -> None:
    f = io.BytesIO()
    dev = cairo.ScriptDevice(f)
    surface = cairo.ScriptSurface(dev, cairo.Content.COLOR_ALPHA, 42, 10)
    del dev
    for _ in range(10):
        surface.get_device()


def test_script_surface_create_for_target() -> None:
    # paint the script proxy
    f = io.BytesIO()
    dev = cairo.ScriptDevice(f)
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    script = cairo.ScriptSurface.create_for_target(dev, surface)
    with pytest.raises(TypeError):
        cairo.ScriptSurface.create_for_target(dev, object())  # type: ignore
    assert isinstance(script, cairo.ScriptSurface)
    ctx = cairo.Context(script)
    ctx.set_source_rgb(0.25, 0.5, 1.0)
    ctx.paint()
    assert b"paint" in f.getvalue()
    surface.flush()
    image_data = bytes(surface.get_data())

    # check if the result is the same
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    ctx2 = cairo.Context(surface)
    ctx2.set_source_rgb(0.25, 0.5, 1.0)
    ctx2.paint()
    surface.flush()
    assert bytes(surface.get_data()) == image_data


def test_misc() -> None:
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
    surface.copy_page()
    surface.mark_dirty()
    surface.show_page()


@pytest.fixture
def surface() -> cairo.Surface:
    return cairo.ImageSurface(cairo.Format.ARGB32, 10, 10)


@pytest.fixture
def image_surface() -> cairo.ImageSurface:
    return cairo.ImageSurface(cairo.Format.ARGB32, 10, 10)


def test_create_similar(surface: cairo.Surface) -> None:
    similar = surface.create_similar(cairo.Content.COLOR, 10, 10)
    assert isinstance(similar, cairo.Surface)
    with pytest.raises(TypeError):
        surface.create_similar()  # type: ignore


def test_get_device_offset(surface: cairo.Surface) -> None:
    surface.set_device_offset(1, 1)
    assert surface.get_device_offset() == (1, 1)
    with pytest.raises(TypeError):
        surface.set_device_offset(1, object())  # type: ignore

    surface.set_device_offset(sys.float_info.max, -sys.float_info.max)
    assert surface.get_device_offset() == (sys.float_info.max, -sys.float_info.max)


def test_get_fallback_resolution(surface: cairo.Surface) -> None:
    surface.set_fallback_resolution(42, 42)
    assert surface.get_fallback_resolution() == (42, 42)
    with pytest.raises(TypeError):
        surface.set_fallback_resolution(42, object())  # type: ignore


def test_mark_dirty_rectangle(surface: cairo.Surface) -> None:
    surface.mark_dirty_rectangle(0, 0, 10, 10)
    with pytest.raises(TypeError):
        surface.mark_dirty_rectangle(0, 0, 10, object())  # type: ignore


def test_write_to_png(image_surface: cairo.ImageSurface) -> None:
    with pytest.raises(TypeError):
        image_surface.write_to_png()  # type: ignore

    with pytest.raises(TypeError):
        image_surface.write_to_png(io.StringIO())  # type: ignore

    with pytest.raises((ValueError, TypeError)) as excinfo:
        image_surface.write_to_png("\x00")
    excinfo.match(r'.* (null|NUL) .*')

    with pytest.raises(TypeError):
        image_surface.write_to_png(object())  # type: ignore


def test_image_surface() -> None:
    with pytest.raises(TypeError):
        cairo.ImageSurface(cairo.FORMAT_ARGB32, 3, object())  # type: ignore


def test_image_surface_create_for_data() -> None:
    format_ = cairo.FORMAT_ARGB32
    surface = cairo.ImageSurface(format_, 3, 3)
    ctx = cairo.Context(surface)
    ctx.paint()
    surface.flush()
    buf = surface.get_data()

    new = cairo.ImageSurface.create_for_data(buf, format_, 3, 3)
    assert new.get_data() == buf

    with pytest.raises(ValueError):
        cairo.ImageSurface.create_for_data(buf, format_, 3, -1)
    with pytest.raises(ValueError):
        cairo.ImageSurface.create_for_data(buf, format_, -1, 3)

    with pytest.raises(cairo.Error) as excinfo:
        cairo.ImageSurface.create_for_data(buf, format_, 3, 3, 3)

    assert excinfo.value.status == cairo.STATUS_INVALID_STRIDE

    with pytest.raises(TypeError):
        cairo.ImageSurface.create_for_data(buf, format_, 3, object())  # type: ignore


@pytest.mark.parametrize("width, height", [(0, 0), (10, 0), (0, 10)])
def test_image_surface_create_for_data_empty(width, height) -> None:
    surface = cairo.ImageSurface.create_for_data(
        bytearray(), cairo.FORMAT_ARGB32, width, height
    )
    assert surface.get_width() == width
    assert surface.get_height() == height
    assert surface.get_stride() == 4 * width


def test_image_surface_get_data_finished() -> None:
    surface = cairo.ImageSurface(cairo.Format.ARGB32, 30, 30)
    surface.finish()
    with pytest.warns(DeprecationWarning):
        surface.get_data()


def test_image_surface_get_data_empty() -> None:
    surface = cairo.ImageSurface(cairo.Format.ARGB32, 0, 0)
    assert isinstance(surface.get_data(), memoryview)
    assert len(surface.get_data()) == 0


def test_image_surface_buffer_get_data_finished() -> None:
    width, height = 6, 4
    buffer = bytearray(width * height * 4)
    surface = cairo.ImageSurface.create_for_data(
        buffer, cairo.FORMAT_ARGB32, width, height, width * 4
    )
    surface.finish()
    with pytest.warns(DeprecationWarning):
        surface.get_data()


def test_image_surface_png_get_data_finished() -> None:
    surface = cairo.ImageSurface(cairo.Format.ARGB32, 30, 30)
    fileobj = io.BytesIO()
    surface.write_to_png(fileobj)
    surface.finish()
    fileobj.seek(0)
    surface = cairo.ImageSurface.create_from_png(fileobj)
    surface.finish()
    with pytest.warns(DeprecationWarning):
        surface.get_data()


def test_image_surface_stride_for_width() -> None:
    v = cairo.ImageSurface.format_stride_for_width(cairo.Format.ARGB32, 10)
    assert v == 40

    with pytest.raises(TypeError):
        cairo.ImageSurface.format_stride_for_width(
            cairo.Format.ARGB32, object())  # type: ignore


def test_image_surface_get_stride(image_surface: cairo.ImageSurface) -> None:
    assert image_surface.get_stride() == 40


def test_recording_surface() -> None:
    with pytest.raises(TypeError):
        cairo.RecordingSurface(cairo.CONTENT_COLOR, object())  # type: ignore

    with pytest.raises(TypeError):
        cairo.RecordingSurface()  # type: ignore

    surface = cairo.RecordingSurface(cairo.CONTENT_COLOR, None)
    assert surface.ink_extents() == (0.0, 0.0, 0.0, 0.0)


@pytest.mark.skipif(not hasattr(cairo.Format, "RGB96F"), reason="too old cairo")
def test_format_rgbf() -> None:
    surface = cairo.ImageSurface(cairo.Format.RGB96F, 3, 3)
    assert surface.get_format() == cairo.Format.RGB96F
    surface = cairo.ImageSurface(cairo.Format.RGBA128F, 3, 3)
    assert surface.get_format() == cairo.Format.RGBA128F


def test_image_surface_release_on_finish() -> None:
    width, height = 6, 4
    buffer = bytearray(width * height * 4)
    mem = memoryview(buffer)
    surface = cairo.ImageSurface.create_for_data(
        mem, cairo.FORMAT_ARGB32, width, height, width * 4
    )
    surface.finish()
    # after the surface is finished, there should be nothing exported from the
    # memoryview anymore
    mem.release()
