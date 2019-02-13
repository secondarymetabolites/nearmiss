# pylint: disable=missing-docstring,no-self-use

from unittest import TestCase

from nearmiss import Searcher
from nearmiss.core import _Tree, check_area


class TestCheckArea(TestCase):
    def test_wrong_type(self):
        with self.assertRaisesRegex(TypeError, "expected tuple"):
            check_area([0, 0])
        with self.assertRaisesRegex(ValueError, "expected tuple"):
            check_area((0, 0, 0))

    def test_inverted(self):
        with self.assertRaisesRegex(ValueError, "start after end"):
            check_area((-1, -5))

    def test_overlap(self):
        with self.assertRaisesRegex(ValueError, "overlapping"):
            check_area((-1, 1))

    def test_good(self):
        check_area((-10, -5))


class TestConstructionC(TestCase):
    def test_good_args(self):
        assert _Tree("stuff")

    def test_bad_args(self):
        with self.assertRaisesRegex(TypeError, "not a string"):
            _Tree(0)
        with self.assertRaisesRegex(TypeError, "expected 1 arguments, got 0"):
            _Tree()


class TestFind(TestCase):
    def test_single(self):
        assert Searcher("abc").find_anchors("b") == [1]

    def test_multiple(self):
        assert set(Searcher("abcabc").find_anchors("ab")) == {0, 3}

    def test_none(self):
        assert Searcher("abc").find_anchors("e") == []

    def test_emptystring(self):
        assert Searcher("ab").find_anchors("") == []

    def test_long(self):
        seq = ("A"*17 + "NGG") * 2
        searcher = Searcher(seq)
        assert searcher.find_anchors("A"*18) == []
        assert set(searcher.find_anchors("A"*17)) == {0, 20}
        assert len(searcher.find_anchors("A"*13)) == 10  # 5 places in each half


class TestGetMismatches(TestCase):
    def find(self, text, anchor_text="GG", before=(-14, -1), distance=0, other_text=None):
        searcher = Searcher(text)
        results = searcher.find_repeat_counts(anchor_text, before_window=before,
                                              max_distance=distance,
                                              other_text=other_text)
        assert sorted(searcher.find_anchors(anchor_text)) == sorted(list(results))
        return results

    def test_self_hit(self):
        seq = ("T"*4 + "A"*13 + "NGG") + ("T"*4 + "C"*13 + "NGG")
        searcher = _Tree(seq)
        pams = searcher.find_anchors("GG")
        assert set(pams) == {18, 38}
        assert self.find(seq) == {18: [1], 38: [1]}

    def test_exact_hit(self):
        seq = ("T"*4 + "A"*13 + "NGG") * 2
        assert self.find(seq, distance=0) == {18: [2], 38: [2]}
        assert self.find(seq, distance=0, other_text="X"*len(seq)) == {18: [0], 38: [0]}

    def test_distance_one(self):
        seq = ("T"*4 + "A"*13 + "NGG") * 2
        seq = seq[:4] + "C" + seq[5:]
        assert self.find(seq, distance=1) == {18: [1, 1], 38: [1, 1]}

    def test_distance_two(self):
        seq = ("T"*4 + "A"*13 + "NGG") * 2
        seq = seq[:4] + "CC" + seq[6:]
        assert self.find(seq, distance=2) == {18: [1, 0, 1], 38: [1, 0, 1]}

    def test_overlarge_max(self):
        with self.assertRaisesRegex(ValueError, "max distance is larger than search window size"):
            self.find("A", distance=4, before=(-10, -7))

    def test_alternate_window_size(self):
        seq = "xAB.xExx?BxxE"
        # the ? should change to A for the anchor at the last E
        # but the first anchor window can't mutate to have an ?
        # the . tests that the trailing edge of the window is respected
        res = self.find(seq, anchor_text="E", distance=1, before=(-4, -2))
        assert res == {
            5: [1, 0],  # self hit and no way to generate a ? for distance 1
            12: [1, 1],  # self hit and ? changed to A which matched
        }
