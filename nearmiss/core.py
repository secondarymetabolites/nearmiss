# License: GNU General Public License v3 or later
# A copy of GNU GPL v3 should have been included in this software package in LICENSE.txt.

"""
The core parts of nearmiss, mostly wrapping the C extension (_core).
"""

from typing import Dict, List, Tuple

# pylint doesn't seem to handle C extensions nicely
try:
    from ._core import Tree as _Tree  # pylint: disable=no-name-in-module
except ImportError:
    raise ImportError("cannot import _core, rebuild or install libgomp")


def check_area(area: Tuple[int, int]) -> bool:
    """ Ensures that a tuple of ints indicating a search area is valid.

        Arguments:
            area: the area coordinates

        Returns:
            True if valid, otherwise an error is raised with specific details
    """
    if not isinstance(area, tuple):
        raise TypeError("expected tuple, not %s" % type(area))
    if len(area) != 2 or not all(isinstance(i, int) for i in area):
        raise ValueError("expected tuple of two ints, not {!r}".format(area))
    start, end = area
    if start > end:
        raise ValueError("start after end in window before anchor")
    if end > 0:
        raise ValueError("window before anchor overlapping anchor ")
    return True


class Searcher(_Tree):
    """ A class that builds a suffix tree of text and can search for specific
        text along with inexact matches in the same or other text.

        Args:
            text: the text to be used for finding the initial anchor points of
                  interest
    """
    # a thin helper wrapper around the C version for simplicity of use
    def __init__(self, text: str) -> None:
        super().__init__(text)
        self._text = text

    def find_anchors(self, target: str) -> List[int]:  # pylint: disable=useless-super-delegation
        """ Finds start positions in the initial text of the given target text.

            Args:
                target: the string to match

            Returns:
                a list of start positions (0-indexed) within the text the class
                was constructed with
        """
        # it seems useless, but this override allows for static type checking
        # and better documentation via help()
        return super().find_anchors(target)

    def find_repeat_counts(self, target: str, before_window: Tuple[int, int],
                           max_distance: int = 2, other_text: str = None,
                          ) -> Dict[int, List[int]]:
        """ Finds matches and inexact (but both case sensitive) matches of the
            text in the specified window around anchors.

            Substitutions made will use the DNA alphabet (i.e. GCAT).

            Args:
                target: the string to match for anchors
                max_distance: the maximum number of substitutions to check for
                              with inexact matches (default: 2)
                before_window: a tuple of ints specifying the area before the
                               anchor in which matching should take place
                               e.g. (-10, -5)
                other_text: optional other text to search for mismatches in,
                            instead of the construction text

            Returns:
                a dictionary mapping
                    the start position (0-indexed) within the
                    text the class was constructed with that matched the target
                to
                    a list of matches at each distance (the first of which is
                    exact matches)
        """

        if other_text is None:
            other_text = self._text
        check_area(before_window)

        if before_window[1] - before_window[0] < max_distance:
            raise ValueError("max distance is larger than search window size")

        anchors = self.find_anchors(target)
        hits = super().find_repeat_counts(anchors, target, max_distance,
                                          before_window[0], before_window[1],
                                          other_text)
        return dict(hits)
