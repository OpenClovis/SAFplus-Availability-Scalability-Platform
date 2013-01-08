#!/usr/bin/env python

# bootstrap setuptools if necessary
from ez_setup import use_setuptools
use_setuptools()

import os

execfile(os.path.join("kid", "release.py"))

from setuptools import setup, find_packages

setup(
    name="kid",
    version=version,
    description= "A simple and pythonic XML template language",
    author=author,
    author_email=email,
    license=license,
    long_description=long_description,
    keywords = "xml template html web",
    url = "http://www.kid-templating.org",
    download_url = "http://www.kid-templating.org/dist/%s/kid-%s.tar.gz" % \
                   (version, version),
    entry_points = {
        'console_scripts': [
          'kid = kid.run:main',
          'kidc = kid.compile:main',
        ],
    },
    py_modules=[],
    packages=["kid", 'kid.test'],
    install_requires=[],
    classifiers = [
            'Development Status :: 4 - Beta',
            'Environment :: Console',
            'Environment :: Web Environment',
            'Intended Audience :: Developers',
            'License :: OSI Approved :: MIT License',
            'Operating System :: OS Independent',
            'Programming Language :: Python',
            'Topic :: Internet :: WWW/HTTP',
            'Topic :: Internet :: WWW/HTTP :: Dynamic Content',
            'Topic :: Software Development :: Libraries :: Python Modules',
            'Topic :: Text Processing :: Markup :: XML'
        ])
