from setuptools import setup, find_packages
from distutils.core import Extension
import os
from setuptools.command.test import test as TestCommand
import sys

install_requires = [
]

tests_require = [
    'pytest >= 3.3.0',
    'coverage',
]


class PyTest(TestCommand):
    """Allow running tests via python setup.py test."""

    def finalize_options(self):
        """Test command magic."""
        TestCommand.finalize_options(self)
        self.test_args = []
        self.test_suite = True

    def run_tests(self):
        """Run tests."""
        import pytest
        errcode = pytest.main(self.test_args)
        sys.exit(errcode)

with open(os.path.join(os.path.abspath(os.path.dirname(__file__)), 'README.md'), encoding='utf-8') as f:
    long_description = f.read()


setup(
    name="nearmiss",
    python_requires='>=3.5',
    version="0.1.2",
    packages=find_packages(exclude=["tests"]),
    author='Simon Shaw',
    author_email='packaging@secondarymetabolites.org',
    description='Fast inexact text searching with suffix arrays',
    long_description=long_description,
    long_description_content_type='text/markdown',
    install_requires=install_requires,
    tests_require=tests_require,
    entry_points={},
    cmdclass={'test': PyTest},
    license='GPLv3+',
    classifiers=[
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'Topic :: Scientific/Engineering :: Bio-Informatics',
        'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
    ],
    extras_require={
        'testing': tests_require,
    },
    ext_modules=[
        Extension('nearmiss._core',
                  sources=[
                    'source/sais.c',
                    'source/tree.c'
                  ],
                  include_dirs=['source'],
                  extra_compile_args=["-fopenmp"],
                  extra_link_args=["-fopenmp"])
    ]
)
