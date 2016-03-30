"setup.py"

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import os
import sys

src_root = "./"
install_prefix= "usr/"
lib_dir = os.path.join(install_prefix, 'lib')
include_dir = os.path.join(src_root, install_prefix, "include", "soletta")

if "SOLETTA_SRC_ROOT" in os.environ:
	src_root = os.environ["SOLETTA_SRC_ROOT"]

if "PREFIX" in os.environ:
	install_prefix = os.environ["PREFIX"]

if "LIBDIR" in os.environ:
	lib_dir = os.environ["LIBDIR"]

if "SOL_INCLUDEDIR" in os.environ:
	include_dir = os.environ["SOL_INCLUDEDIR"]

bindings_dir = os.path.join(src_root, "bindings", "python")

srcs = ["mainloop.pyx", "__init__.py"]

soletta_srcs = [os.path.join(bindings_dir, "soletta", src) for src in srcs]

extensions = [
    Extension("soletta", soletta_srcs,
        include_dirs = ['.', include_dir],
        libraries = ["soletta"],
        library_dirs = [lib_dir])
]

packages = ["bindings/python/soletta"]

setup(
    name = "soletta",
    ext_modules = cythonize(extensions)
)
