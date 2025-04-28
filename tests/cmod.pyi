import cairo


def create_image_surface() -> cairo.ImageSurface:
    ...


def create_context() -> cairo.Context:
    ...


def check_status(status: int) -> None:
    ...


def test_capi() -> None:
    ...
