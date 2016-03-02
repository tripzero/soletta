"setup.py"

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import os

src_root = "./"
install_prefix= "usr/"
lib_prefix = "{}lib".format(install_prefix)

if "SOLETTA_SRC_ROOT" in os.environ:
	src_root = os.environ["SOLETTA_SRC_ROOT"]

if "INSTALL_ROOT" in os.environ:
	install_prefix = os.environ["INSTALL_ROOT"]

if "SOLETTA_LIBRARY_PATH" in os.environ:
	lib_prefix = os.environ["SOLETTA_LIBRARY_PATH"]

extensions = [
    Extension("soletta", ["{}bindings/python/mainloop.pyx".format(src_root)],
        include_dirs = ["{}build/soletta_sysroot/{}/include/soletta".format(src_root, install_prefix)],
        libraries = ["soletta"],
        library_dirs = ["{}build/soletta_sysroot/{}/".format(src_root, lib_prefix)])
]

setup(
    name = "Python Soletta",
    ext_modules = cythonize(extensions)
)
