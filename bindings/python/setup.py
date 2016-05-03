#!/usr/bin/python3

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import os
import sys

using_soletta_build = False

src_root = "./"
install_prefix= "/usr/"
lib_dir = os.path.join(install_prefix, 'lib')
include_dir = os.path.join(install_prefix, "include", "soletta")
install_root = "/"

if "SOLETTA_SRC_ROOT" in os.environ:
    src_root = os.environ["SOLETTA_SRC_ROOT"]
    using_soletta_build = True

if "PREFIX" in os.environ:
    install_prefix = os.environ["PREFIX"]

if "LIBDIR" in os.environ:
    lib_dir = os.environ["LIBDIR"]

if "SOL_INCLUDEDIR" in os.environ:
    include_dir = os.environ["SOL_INCLUDEDIR"]
    using_soletta_build = True

if "SOLETTA_INSTALL_ROOT" in os.environ:
    install_root = os.environ["SOLETTA_INSTALL_ROOT"]

bindings_dir = "./"

if using_soletta_build:
    bindings_dir = os.path.join(src_root, "bindings", "python")

soletta_prefix = ""#os.path.join(install_root, install_prefix)
asyncio_prefix =  os.path.join(bindings_dir, "soletta", "asyncio")
tests_prefix = os.path.join(bindings_dir, "soletta", "tests")

asyncio_srcs = [os.path.join(asyncio_prefix, src) for src in ["_soletta_mainloop.pyx"]]
test_srcs = [os.path.join(tests_prefix, src) for src in ["_soletta_tests.pyx"]]

soletta_extension = os.path.join(soletta_prefix,"soletta", "asyncio", "_soletta_mainloop")
tests_extension = os.path.join(soletta_prefix, "soletta", "tests", "_soletta_tests")

extensions = [
    Extension(soletta_extension, asyncio_srcs,
        include_dirs = ['.', include_dir],
        libraries = ["soletta"],
        library_dirs = [lib_dir]),
    Extension(tests_extension, test_srcs,
        include_dirs = ['.', include_dir],
        libraries = ["soletta"],
        library_dirs = [lib_dir])
]

pkg_dir = {"soletta" : os.path.join(bindings_dir,"soletta")}

asyncio_pkg = "soletta/asyncio"

if "./" in asyncio_pkg:
    asyncio_pkg = asyncio_pkg[2:]

pkg_dir[asyncio_pkg] = os.path.join(bindings_dir, "soletta", "asyncio")

tests_pkg = "soletta/tests"

if "./" in tests_pkg:
    tests_pkg = tests_pkg[2:]

pkg_dir[tests_pkg] = os.path.join(bindings_dir, "soletta", "tests")

packages = ["soletta", asyncio_pkg, tests_pkg]


print("""Soletta Python Build
prefix: {prefix}
src_root: {src_root}
include_dir: {include_dir}
libdir: {libdir}
bindings_dir: {bindings_dir}
soletta_build_dir: {soletta_prefix}
python modules: 
{modules}

install root: {install_root}

""".format(prefix=install_prefix, soletta_prefix=soletta_prefix, src_root=src_root, include_dir=include_dir, libdir=lib_dir, bindings_dir=bindings_dir, install_root = install_root, modules="\n".join([soletta_extension])))

setup(
    name = "soletta",
    package_dir = pkg_dir,
    packages = packages,
    ext_modules = cythonize(extensions)
)

if os.path.isfile("/etc/lsb-release"):
    data = None
    with open("/etc/lsb-release", 'r') as f:
        data = f.read()
    
    if data.find("Ubuntu") != -1:
        "workaround for https://bugs.launchpad.net/ubuntu/+source/python3-defaults/+bug/362570"
        print("implementing workaround for ubuntu...")
        import shutil
        if "./" in install_root:
            install_root = install_root[2:]
            lib_dir = lib_dir[1:]
        base_dir = os.path.join(install_root, lib_dir, "python3.5")
        if os.path.isdir(os.path.join(base_dir, "dist-packages")):
            shutil.rmtree(os.path.join(base_dir, "dist-packages"))
        try:
            os.renames(os.path.join(base_dir, "site-packages"), os.path.join(base_dir, "dist-packages"))
        except:
            print("probably build stage.")