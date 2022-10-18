#!/usr/bin/env python

import io
import subprocess
import sys
import os
import errno

from setuptools import setup
from distutils.core import Extension, Command, Distribution
from distutils.ccompiler import new_compiler
from distutils.sysconfig import customize_compiler
from distutils.util import change_root
from distutils import log
from distutils import sysconfig


PYCAIRO_VERSION = '1.22.1'
CAIRO_VERSION_REQUIRED = '1.15.10'

PYCAIRO_BUILD_NO_PKGCONFIG = os.environ.get("PYCAIRO_BUILD_NO_PKGCONFIG", False)
PYCAIRO_BUILD_MSVC_STATIC = os.environ.get("PYCAIRO_BUILD_MSVC_STATIC", True)

def get_command_class(name):
    # in case pip loads with setuptools this returns the extended commands
    return Distribution({}).get_command_class(name)


def _check_output(command):
    try:
        return subprocess.check_output(command)
    except OSError as e:
        if e.errno == errno.ENOENT:
            raise SystemExit(
                "%r not found.\nCommand %r" % (command[0], command))
        raise SystemExit(e)
    except subprocess.CalledProcessError as e:
        raise SystemExit(e)


def pkg_config_version_check(pkg, version):
    pkg_config = os.environ.get('PKG_CONFIG', 'pkg-config')
    command = [
        pkg_config,
        "--print-errors",
        "--exists",
        '%s >= %s' % (pkg, version),
    ]

    _check_output(command)


def pkg_config_parse(opt, pkg):
    pkg_config = os.environ.get('PKG_CONFIG', 'pkg-config')
    command = [pkg_config, opt, pkg]
    ret = _check_output(command)
    output = ret.decode()
    opt = opt[-2:]
    return [x.lstrip(opt) for x in output.split()]


def filter_compiler_arguments(compiler, args):
    """Given a compiler instance and a list of compiler warning flags
    returns the list of supported flags.
    """

    if compiler.compiler_type == "msvc":
        # TODO
        return []

    extra = []

    def check_arguments(compiler, args):
        p = subprocess.Popen(
            [compiler.compiler[0]] + args + extra + ["-x", "c", "-E", "-"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        stdout, stderr = p.communicate(b"int i;\n")
        if p.returncode != 0:
            text = stderr.decode("ascii", "replace")
            return False, [a for a in args if a in text]
        else:
            return True, []

    def check_argument(compiler, arg):
        return check_arguments(compiler, [arg])[0]

    # clang doesn't error out for unknown options, force it to
    if check_argument(compiler, '-Werror=unknown-warning-option'):
        extra += ['-Werror=unknown-warning-option']
    if check_argument(compiler, '-Werror=unused-command-line-argument'):
        extra += ['-Werror=unused-command-line-argument']

    # first try to remove all arguments contained in the error message
    supported = list(args)
    while 1:
        ok, maybe_unknown = check_arguments(compiler, supported)
        if ok:
            return supported
        elif not maybe_unknown:
            break
        for unknown in maybe_unknown:
            if not check_argument(compiler, unknown):
                supported.remove(unknown)

    # hm, didn't work, try each argument one by one
    supported = []
    for arg in args:
        if check_argument(compiler, arg):
            supported.append(arg)
    return supported


def add_ext_cflags(ext, compiler):
    args = [
        "-Wall",
        "-Warray-bounds",
        "-Wcast-align",
        "-Wconversion",
        "-Wextra",
        "-Wformat=2",
        "-Wformat-nonliteral",
        "-Wformat-security",
        "-Wimplicit-function-declaration",
        "-Winit-self",
        "-Winline",
        "-Wmissing-format-attribute",
        "-Wmissing-noreturn",
        "-Wnested-externs",
        "-Wold-style-definition",
        "-Wpacked",
        "-Wpointer-arith",
        "-Wreturn-type",
        "-Wshadow",
        "-Wsign-compare",
        "-Wstrict-aliasing",
        "-Wundef",
        "-Wunused-but-set-variable",
        "-Wswitch-default",
    ]

    args += [
        "-Wno-missing-field-initializers",
        "-Wno-unused-parameter",
    ]

    # silence clang for unused gcc CFLAGS added by Debian
    args += [
        "-Wno-unused-command-line-argument",
    ]

    args += [
        "-fno-strict-aliasing",
        "-fvisibility=hidden",
    ]

    args += [
        "-std=c99",
    ]

    ext.extra_compile_args += filter_compiler_arguments(compiler, args)


class build_tests(Command):
    description = "build test libraries and extensions"
    user_options = [
        ("force", "f", "force a rebuild"),
    ]

    def initialize_options(self):
        self.force = False
        self.build_base = None
        self.compiler_type = None

    def finalize_options(self):
        self.set_undefined_options(
            'build',
            ('build_base', 'build_base'))
        self.force = bool(self.force)
        self.compiler_type = new_compiler().compiler_type

    def run(self):
        cmd = self.reinitialize_command("build_ext")
        cmd.inplace = True
        cmd.force = self.force
        cmd.ensure_finalized()
        cmd.run()

        tests_dir = os.path.join("tests", "cmodule")

        ext = Extension(
            name='tests.cmod',
            sources=[
                os.path.join(tests_dir, "cmodule.c"),
                os.path.join(tests_dir, "cmodulelib.c"),
            ],
            include_dirs=[
                tests_dir,
                "cairo",
            ],
            depends=[
                os.path.join(tests_dir, "cmodulelib.h"),
                os.path.join("cairo", "pycairo.h"),
            ],
            define_macros=[("PY_SSIZE_T_CLEAN", None)],
        )

        compiler = new_compiler()
        customize_compiler(compiler)

        add_ext_cflags(ext, compiler)

        if not PYCAIRO_BUILD_NO_PKGCONFIG:
            pkg_config_version_check('cairo', CAIRO_VERSION_REQUIRED)
            ext.include_dirs += pkg_config_parse('--cflags-only-I', 'cairo')
            ext.library_dirs += pkg_config_parse('--libs-only-L', 'cairo')
            ext.libraries += pkg_config_parse('--libs-only-l', 'cairo')
        if self.compiler_type == "msvc" and PYCAIRO_BUILD_MSVC_STATIC:
            ext.libraries += ['user32', 'advapi32', 'ole32']
            ext.define_macros += [('CAIRO_WIN32_STATIC_BUILD', 1)]

        dist = Distribution({"ext_modules": [ext]})

        build_cmd = dist.get_command_obj("build")
        build_cmd.build_base = os.path.join(self.build_base, "pycairo_tests")
        build_cmd.ensure_finalized()

        cmd = dist.get_command_obj("build_ext")
        cmd.inplace = True
        cmd.force = self.force
        cmd.ensure_finalized()
        cmd.run()


class test_cmd(Command):
    description = "run tests"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        import pytest

        # ensure the C extension is build inplace
        cmd = self.reinitialize_command("build_tests")
        cmd.ensure_finalized()
        cmd.run()

        status = pytest.main(["tests"])
        if status != 0:
            raise SystemExit(status)


class install_pkgconfig(Command):
    description = "install .pc file"
    user_options = [
        ('pkgconfigdir=', None, 'pkg-config file install directory'),
    ]

    def initialize_options(self):
        self.root = None
        self.install_base = None
        self.install_data = None
        self.pkgconfigdir = None
        self.compiler_type = None
        self.outfiles = []

    def finalize_options(self):
        self.set_undefined_options(
            'install_lib',
            ('install_base', 'install_base'),
            ('install_data', 'install_data'),
        )

        self.set_undefined_options(
            'install',
            ('root', 'root'),
        )

        self.set_undefined_options(
            'build_ext',
            ('compiler_type', 'compiler_type'),
        )

    def get_outputs(self):
        return self.outfiles

    def get_inputs(self):
        return []

    def run(self):
        # https://github.com/pygobject/pycairo/issues/83
        # The pkg-config file contains absolute paths depending on the
        # prefix. pip uses wheels as cache and when installing with --user
        # and then to a virtualenv, the wheel gets reused containing the
        # wrong paths. So in case bdist_wheel is used, just skip this command.
        cmd = self.distribution.get_command_obj("bdist_wheel", create=False)
        if cmd is not None:
            log.info(
                "Skipping install_pkgconfig, not supported with bdist_wheel")
            return

        # same for bdist_egg
        cmd = self.distribution.get_command_obj("bdist_egg", create=False)
        if cmd is not None:
            log.info(
                "Skipping install_pkgconfig, not supported with bdist_egg")
            return

        if self.compiler_type == "msvc":
            log.info(
                "Skipping install_pkgconfig, not supported with MSVC")
            return

        if self.pkgconfigdir is None:
            python_lib = sysconfig.get_python_lib(True, True,
                                                  self.install_data)
            pkgconfig_dir = os.path.join(os.path.dirname(python_lib),
                                         'pkgconfig')
        else:
            pkgconfig_dir = change_root(self.root, self.pkgconfigdir)
        self.mkpath(pkgconfig_dir)

        pcname = "py3cairo.pc"
        target = os.path.join(pkgconfig_dir, pcname)

        log.info("Writing %s" % target)
        log.info("pkg-config prefix: %s" % self.install_base)
        with open(target, "wb") as h:
            h.write((u"""\
prefix=%(prefix)s

Name: Pycairo
Description: Python %(py_version)d bindings for cairo
Version: %(version)s
Requires: cairo
Cflags: -I${prefix}/include/pycairo
Libs:
""" % {
                "prefix": self.install_base,
                "version": PYCAIRO_VERSION,
                "py_version": sys.version_info[0]}).encode("utf-8"))

        self.outfiles.append(target)


class install_pycairo_header(Command):
    description = "install pycairo header"
    user_options = []

    def initialize_options(self):
        self.install_data = None
        self.install_lib = None
        self.force = None
        self.outfiles = []

    def finalize_options(self):
        self.set_undefined_options(
            'install_lib',
            ('install_data', 'install_data'),
            ('install_lib', 'install_lib'),
        )

        self.set_undefined_options(
            'install',
            ('force', 'force'),
        )

    def get_outputs(self):
        return self.outfiles

    def get_inputs(self):
        return [os.path.join('cairo', 'pycairo.h')]

    def run(self):
        # https://github.com/pygobject/pycairo/issues/92
        # https://github.com/pygobject/pycairo/issues/98
        hname = "py3cairo.h"
        source = self.get_inputs()[0]

        # for things using get_include()
        lib_hdir = os.path.join(self.install_lib, "cairo", "include")
        self.mkpath(lib_hdir)
        lib_header_path = os.path.join(lib_hdir, hname)
        (out, _) = self.copy_file(source, lib_header_path)
        self.outfiles.append(out)

        cmd = self.distribution.get_command_obj("bdist_wheel", create=False)
        if cmd is not None:
            return
        cmd = self.distribution.get_command_obj("bdist_egg", create=False)
        if cmd is not None:
            return

        # for things using pkg-config
        data_hdir = os.path.join(self.install_data, "include", "pycairo")
        self.mkpath(data_hdir)
        header_path = os.path.join(data_hdir, hname)
        (out, _) = self.copy_file(source, header_path)
        self.outfiles.append(out)


def check_setuptools_for_dist():
    if "setuptools" not in sys.modules:
        raise Exception("setuptools not available")


du_sdist = get_command_class("sdist")


class sdist(du_sdist):

    def run(self):
        check_setuptools_for_dist()
        du_sdist.run(self)


du_install_lib = get_command_class("install_lib")


class install_lib(du_install_lib):

    def initialize_options(self):
        self.install_base = None
        self.install_lib = None
        self.install_data = None
        du_install_lib.initialize_options(self)

    def finalize_options(self):
        du_install_lib.finalize_options(self)
        self.set_undefined_options(
            'install',
            ('install_base', 'install_base'),
            ('install_lib', 'install_lib'),
            ('install_data', 'install_data'),
        )

    def run(self):
        du_install_lib.run(self)
        # bdist_egg doesn't run install, so run our commands here instead
        self.run_command("install_pkgconfig")
        self.run_command("install_pycairo_header")


du_build_ext = get_command_class("build_ext")


class build_ext(du_build_ext):

    def initialize_options(self):
        du_build_ext.initialize_options(self)
        self.compiler_type = None

    def finalize_options(self):
        du_build_ext.finalize_options(self)

        self.compiler_type = new_compiler(compiler=self.compiler).compiler_type

    def run(self):
        ext = self.extensions[0]

        if not PYCAIRO_BUILD_NO_PKGCONFIG:
            pkg_config_version_check('cairo', CAIRO_VERSION_REQUIRED)
            ext.include_dirs += pkg_config_parse('--cflags-only-I', 'cairo')
            ext.library_dirs += pkg_config_parse('--libs-only-L', 'cairo')
            ext.libraries += pkg_config_parse('--libs-only-l', 'cairo')
        if not self.compiler_type == "msvc":
            compiler = new_compiler(compiler=self.compiler)
            customize_compiler(compiler)
            add_ext_cflags(ext, compiler)
        elif self.compiler_type == "msvc" and PYCAIRO_BUILD_MSVC_STATIC:
            # these extra libs are needed since we are linking statically
            ext.libraries += ['user32', 'advapi32', 'ole32']
            ext.define_macros += [('CAIRO_WIN32_STATIC_BUILD', 1)]

        du_build_ext.run(self)


def main():

    if sys.version_info[0] < 3:
        raise Exception("Python 2 no longer supported")

    cairo_ext = Extension(
        name='cairo._cairo',
        sources=[
            'cairo/device.c',
            'cairo/bufferproxy.c',
            'cairo/error.c',
            'cairo/cairomodule.c',
            'cairo/context.c',
            'cairo/font.c',
            'cairo/matrix.c',
            'cairo/path.c',
            'cairo/pattern.c',
            'cairo/region.c',
            'cairo/surface.c',
            'cairo/enums.c',
            'cairo/misc.c',
            'cairo/glyph.c',
            'cairo/rectangle.c',
            'cairo/textcluster.c',
            'cairo/textextents.c',
        ],
        depends=[
            'cairo/private.h',
            'cairo/pycairo.h',
        ],
        define_macros=[
            ("PYCAIRO_VERSION_MAJOR", PYCAIRO_VERSION.split('.')[0]),
            ("PYCAIRO_VERSION_MINOR", PYCAIRO_VERSION.split('.')[1]),
            ("PYCAIRO_VERSION_MICRO", PYCAIRO_VERSION.split('.')[2]),
        ],
    )

    with io.open('README.rst', encoding="utf-8") as h:
        long_description = h.read()

    cmdclass = {
        "build_ext": build_ext,
        "install_lib": install_lib,
        "install_pkgconfig": install_pkgconfig,
        "install_pycairo_header": install_pycairo_header,
        "test": test_cmd,
        "build_tests": build_tests,
        "sdist": sdist,
    }
    setup(
        name="pycairo",
        version=PYCAIRO_VERSION,
        url="https://pycairo.readthedocs.io",
        project_urls={
            'Source': 'https://github.com/pygobject/pycairo',
        },
        description="Python interface for cairo",
        long_description=long_description,
        maintainer="Christoph Reiter",
        maintainer_email="reiter.christoph@gmail.com",
        license="LGPL-2.1-only OR MPL-1.1",
        ext_modules=[cairo_ext],
        packages=["cairo"],
        package_data={
            "cairo": [
                "__init__.pyi",
                "py.typed",
                "cairo.dll",
            ],
        },
        classifiers=[
            'Operating System :: OS Independent',
            'Programming Language :: Python :: 3',
            'Programming Language :: Python :: Implementation :: CPython',
            'Programming Language :: Python :: Implementation :: PyPy',
            ('License :: OSI Approved :: '
             'GNU Lesser General Public License v2 (LGPLv2)'),
            'License :: OSI Approved :: Mozilla Public License 1.1 (MPL 1.1)',
        ],
        cmdclass=cmdclass,
        python_requires='>=3.7',
    )


if __name__ == "__main__":
    main()
