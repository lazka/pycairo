from __future__ import annotations
import logging
import os
import sys
import re
import shutil
import struct
import tempfile
import zipfile
from pathlib import Path
from urllib.request import urlretrieve as download

CAIRO_VERSION = "1.18.4-v3"


def get_platform() -> str:
    if len(sys.argv) > 1:
        return sys.argv[1]
    if (struct.calcsize("P") * 8) == 32:
        return "x86"
    else:
        return "x64"


logging.basicConfig(format="%(levelname)s - %(message)s", level=logging.DEBUG)

plat = get_platform()
logging.debug(f"Found Platform {plat}")

download_url = (
    "https://github.com/pygobject/cairo-win-build/releases"
    f"/download/{CAIRO_VERSION}/cairo-{CAIRO_VERSION}-{plat}.zip"
)
final_location = Path(__file__).parent.parent / "cairo-prebuild"
download_location = Path(tempfile.mkdtemp())
if final_location.exists():
    logging.info("Final Location already exists clearing it...")
    shutil.rmtree(str(final_location))
final_location.mkdir()
download_file = download_location / "build.zip"
logging.info("Downloading Cairo Binaries for Windows...")
logging.info("Url: %s", download_url)
download(url=download_url, filename=download_file)
logging.info(f"Download complete. Saved to {download_file}.")
logging.info(f"Extracting {download_file} to {download_location}...")
with zipfile.ZipFile(
    download_file, mode="r", compression=zipfile.ZIP_DEFLATED
) as file:  # noqa: E501
    file.extractall(download_location)
os.remove(download_file)
logging.info("Completed Extracting.")
logging.info("Moving Files accordingly.")
plat_location = download_location / f"cairo-{plat}"
for src_file in plat_location.glob("*"):
    logging.debug(f"Moving {src_file} to {final_location}...")
    shutil.move(str(src_file), str(final_location))
logging.info("Moving files Completed")
logging.info("Fixing .pc files")


rex = re.compile("^prefix=(.*)")


def new_place(_: re.Match[str]) -> str:
    return f"prefix={str(final_location.as_posix())}"


pc_files = final_location / "lib" / "pkgconfig"
for i in pc_files.glob("*.pc"):
    logging.info(f"Writing {i}")
    with open(i) as f:
        content = f.read()
        final = rex.sub(new_place, content)
    with open(i, "w") as f:
        f.write(final)

logging.info("Getting pkg-config")
download(
    url="https://github.com/pygobject/cairo-win-build"
    f"/releases/download/{CAIRO_VERSION}/pkgconf.zip",
    filename=download_file,
)
with zipfile.ZipFile(
    download_file, mode="r", compression=zipfile.ZIP_DEFLATED
) as file:  # noqa: E501
    file.extractall(download_location)

os.makedirs(str(final_location / "bin"))
shutil.move(
    str(download_location / "pkgconf" / "bin" / "pkgconf.exe"),
    str(final_location / "bin"),
)

# On MSVC, meson would create static libraries as
# libcairo.a but setuptools doens't know about it.
libreg = re.compile(r"lib(?P<name>\S*)\.a")
libdir = final_location / "lib"
for lib in libdir.glob("lib*.a"):
    name = libreg.match(lib.name).group("name") + ".lib"
    shutil.move(lib, libdir / name)
