from setuptools import setup, Extension
from distutils.sysconfig import get_config_vars
import os
os.chdir('..')

(opt,) = get_config_vars('OPT')
os.environ['OPT'] = " ".join(
    flag for flag in opt.split() if flag != '-Wstrict-prototypes'
)

setup(
    # Name of this package
    name="ethereum-solidity",

    # Package version
    version='1.8.0',

    description='Solidity compiler python wrapper',
    maintainer='Vitalik Buterin',
    maintainer_email='v@buterin.com',
    license='WTFPL',
    url='http://www.ethereum.org/',

    # Describes how to build the actual extension module from C source files.
    ext_modules=[
        Extension(
            'solidity',         # Python name of the module
            sources=['pysol/pysolidity.cpp'],
            library_dirs=['build/libsolidity/',
                          'build/libdevcrypto/'],
            libraries=['boost_python',
                       'boost_filesystem',
                       'boost_chrono',
                       'boost_thread',
                       'devcrypto',
                       'leveldb',
                       'jsoncpp',
                       'solidity'],
            include_dirs=['/usr/include/boost',
                          '..',
                          '../..',
                          '.'],
            extra_compile_args=['--std=c++11', '-Wno-unknown-pragmas']
        )],
    py_modules=[
    ],
    scripts=[
    ],
    entry_points={
    }
    ),
