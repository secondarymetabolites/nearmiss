# nearmiss
## NEARby MISmatch Search

nearmiss is a fast inexact text matching tool for finding repeats of an area
around a specific anchor string throughout text,
optionally finding matches with substitutions.

It is primarily intended for finding near-match sections of DNA in the vicinity
of specific anchor sequences. The current substitution alphabet is limited to
`ACGT`.

The speed of nearmiss comes from a C extension that uses the SA-IS suffix array
library from Yuta Mori and pointer magic instead.
The search time for anchors is `O(|anchor| log |text|)`.
The search time for repeats is `O(a(sw)^d log t)`, where
- `a` is the number of anchors found
- `s` is the size of the substitution alphabet
- `w` is the size of the matching window
- `d` is the maximum desired number of substitutions to allow in the window
- and `t` is the size of the search text

## Use

```
>>> from nearmiss import Searcher
>>> seq = "TACTANGGnnnTAAAAGnGG"
>>> searcher = Searcher(seq)
>>> searcher.find_anchors("GG")
[6, 18]
>>> searcher.find_anchors("nGG")
[17]
>>> searcher.find_repeat_counts("GG", (-4, -2), max_distance=1)
{18: [1, 0], 6: [1, 0]}
>>> searcher.find_repeat_counts("GG", (-4, -2), max_distance=2)
{18: [1, 0, 1], 6: [1, 0, 1]}
```

For more detailed information, see the source documentation with
`pydoc nearmiss.Searcher` or `help(nearmiss.Searcher)`.

To limit the number of threads used outside the source, set the environment
variable `OMP_NUM_THREADS` to the number of desired threads.

## Installing

### Non-python dependencies

nearmiss uses OpenMP to drastically speed up mismatch searching on many anchors.
To install that on Debian/Ubuntu systems, run `sudo apt-get install libomp5`.

### with pip

`pip install nearmiss`

### from source

`pip install .` in the source directory
