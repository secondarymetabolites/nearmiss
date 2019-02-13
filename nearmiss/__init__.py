# License: GNU General Public License v3 or later
# A copy of GNU GPL v3 should have been included in this software package in LICENSE.txt.

""" nearmiss (or NEARby MISmatch Search).
Provides a C extension for fast searching of large amounts of text via suffix
arrays. Inexact searching is supported.

Due to the datastructures in the C level needing to persist for performance,
the bulk of the extension is methods of Searcher, built from the initial query
text.
"""

from .core import (
    Searcher,
)
