/* License: GNU General Public License v3 or later
   A copy of GNU GPL v3 should have been included in this software package in LICENSE.txt.
*/

#include <Python.h>
#include <omp.h>

#include "sais.h"

typedef struct {
    const char *_text;
    int *_suffixArray;
    int _length;
} SuffixArray;

typedef struct {
    PyObject_HEAD
    SuffixArray data;
} Tree;

static int
Tree_init(Tree *self, PyObject *args) {
    char *text = NULL;
    int argCount = PyObject_Length(args);
    if (argCount != 1) {
        PyErr_Format(PyExc_TypeError, "expected 1 arguments, got %d", argCount);
        return -1;
    }
    if (!PyArg_ParseTuple(args, "s", &text)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 is not a string");
        return -1;
    }
    self->data._length = strlen(text);
    self->data._text = strdup(text);
    self->data._suffixArray = (int *) malloc((size_t) self->data._length * sizeof(int));
    sais((const unsigned char *) self->data._text, self->data._suffixArray, self->data._length);

    return 0;
}


static void
Tree_dealloc(Tree* self)
{
    if (self->data._text != NULL) {
        free((char *) self->data._text);
    }
    if (self->data._suffixArray != NULL) {
        free(self->data._suffixArray);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}


void
find(SuffixArray const* tree, char* query, int queryLength, PyObject* list)
{
    int left = 0;
    int right = tree->_length - 1;

    while (left <= right) {
        int center = left + (right - left) / 2;
        int comparison = strncmp(query, (char *) &(tree->_text[tree->_suffixArray[center]]), queryLength);
        if (comparison < 0) {
            right = center - 1;
        } else if (comparison == 0) {
            PyList_Append(list, PyLong_FromLong((long) tree->_suffixArray[center]));

            left = center - 1;
            while (left >= 0 && strncmp(query, (char *) &(tree->_text[tree->_suffixArray[left]]), queryLength) == 0) {
                PyList_Append(list, PyLong_FromLong((long) tree->_suffixArray[left]));
                left--;
            }

            right = center + 1;
            while (right < tree->_length - 1 && strncmp(query, (char *) &(tree->_text[tree->_suffixArray[right]]), queryLength) == 0) {
                PyList_Append(list, PyLong_FromLong((long) tree->_suffixArray[right]));
                right++;
            }

            break;
        } else { /* comparison > 0 */
            left = center + 1;
        }
    }
}


static PyObject*
Tree_find_anchors(Tree *self, PyObject *args)
{
    char *query = NULL;
    PyObject *list = NULL;
    int argCount = PyObject_Length(args);

    if (argCount != 1) {
        PyErr_Format(PyExc_TypeError, "expected 1 arguments, got %d", argCount);
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "s", &query)) {
        PyErr_Format(PyExc_TypeError, "expected a string, not '%s'", Py_TYPE(PyObject_GetItem(args, 0)));
        return NULL;
    }

    list = PyList_New(0);

    if (query[0]) {
        find(&(self->data), query, strlen(query), list);
    }

    return list;
}

#define TREE_SHIFT(_treename, _suffix) ((char *) (_treename)->_text + (_treename)->_suffixArray[(_suffix)])

int
count(SuffixArray const* tree, char* query, char* anchor, int downstreamStart)
{
    int queryLength = strlen(query);
    int anchorLength = strlen(anchor);
    int left = 0;
    int right = tree->_length - queryLength;
    int count = 0;

    while (left <= right) {
        int center = left + (right - left) / 2;
        char* window = TREE_SHIFT(tree, center);
        int comparison = strncmp(query, window, queryLength);
        if (comparison < 0) {
            right = center - 1;
        } else if (comparison > 0) {
            left = center + 1;
        } else {
            if (strncmp(anchor, window - downstreamStart, anchorLength) == 0) {
                count += 1;
            }

            for (left = center -1; left >= 0 && strncmp(query, TREE_SHIFT(tree, left), queryLength) == 0; left--) {
                window = TREE_SHIFT(tree, left);
                if (strncmp(anchor, window - downstreamStart, anchorLength) == 0) {
                    count += 1;
                }
            }

            for (right = center + 1; right < tree->_length && strncmp(query, TREE_SHIFT(tree, right), queryLength) == 0; right++) {
                window = TREE_SHIFT(tree, right);
                if (strncmp(anchor, window - downstreamStart, anchorLength) == 0) {
                    count += 1;
                }
            }

            break;
        }
    }
    return count;
}

void
find_inexact(SuffixArray const* tree, int anchorStart, int downstreamStart, int downstreamEnd,
                  int maxNumChanges, int depth, int *counts, char *query, int changeStart,
                  char* anchor)
{
    const char* options = "ACGT";
    int i = 0;
    int position = downstreamStart + changeStart;
    int hits = count(tree, query, anchor, downstreamStart);
    counts[depth] += hits;

    if (depth == maxNumChanges) {
        return;
    }

    for (i = changeStart; position < downstreamEnd; position++, i++) {
        char original = query[i];
        int option = 0;
        for (option = 0; option < 4; option++) {
            if (options[option] == original) {
                continue;
            }
            query[i] = options[option];
            find_inexact(tree, anchorStart, downstreamStart, downstreamEnd, maxNumChanges, depth + 1, counts, query, i + 1, anchor);
        }
        query[i] = original;
    }
}

PyObject*
find_mismatches(Tree const* tree, PyObject *anchors, char *anchorText, int maxDistance,
        int downstreamStart, int downstreamEnd, SuffixArray *other, int threads)
{

    /* for locking around PyObject functions that (de)allocate memory */
    omp_lock_t gilReplacement;
    PyObject *list = PyList_New(0);
    const size_t size = sizeof(int) * (maxDistance + 1);
    int i = 0;

    omp_init_lock(&gilReplacement);
    if (threads > 0) {
        omp_set_num_threads(threads);
    }

#pragma omp parallel for shared(anchors, list)
    for (i = 0; i < PyObject_Length(anchors); i++) {
        long anchorStart = PyLong_AsLong(PyList_GetItem(anchors, i));
        int *counts = NULL;
        char *query = NULL;
        int distance = 0;
        PyObject* anchorResult = NULL;
        /* skip any anchor that would read off the end of the text */
        if (anchorStart < -downstreamStart) {
            continue;
        }
        counts = malloc(size);
        memset(counts, 0, size);
        query = strndup(tree->data._text + (anchorStart + downstreamStart), downstreamEnd - downstreamStart);

        omp_set_lock(&gilReplacement);
        anchorResult = PyList_New(maxDistance + 1);
        omp_unset_lock(&gilReplacement);

        find_inexact(other, anchorStart, downstreamStart, downstreamEnd, maxDistance, 0, counts, query, 0, anchorText);

        omp_set_lock(&gilReplacement);
        for (distance = 0; distance < maxDistance + 1; distance++) {
            PyList_SetItem(anchorResult, distance, PyLong_FromLong((long) counts[distance]));
        }
        /* NN => build a tuple of two objects:
            the first being the anchor object
            the second being the list of hit counts
         */
        PyList_Append(list, Py_BuildValue("NN", PyList_GetItem(anchors, i), anchorResult));
        omp_unset_lock(&gilReplacement);
        free(query);
        free(counts);
    }
    omp_destroy_lock(&gilReplacement);
    return list;
}

static PyObject*
Tree_find_repeat_counts(Tree* tree, PyObject* args)
{
    int argCount = PyObject_Length(args);
    PyObject* anchors = NULL;
    char *anchorText = NULL;
    int downstreamStart = 0;
    int downstreamEnd = 0;
    int maxNumChanges = 0;
    char *text = NULL;
    int threads = 0;
    SuffixArray other;

    if (argCount != 7) {  /* anchors, anchor string, max_distance, downstream_start, downsteam_end, text */
        PyErr_Format(PyExc_TypeError, "expected 7 arguments, got %d", argCount);
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "Osiiisi", &anchors, &anchorText, &maxNumChanges, &downstreamStart, &downstreamEnd, &text, &threads)) {
        PyErr_SetString(PyExc_TypeError, "expected other types");
        return NULL;
    }

    if (downstreamStart >= downstreamEnd) {
        PyErr_SetString(PyExc_ValueError, "downstream_start must be less than downstream_end");
        Py_DECREF(anchors);
        return NULL;
    }

    if (downstreamStart >= 0 || downstreamEnd > 0) {
        PyErr_SetString(PyExc_ValueError, "downstream coordinates must be relative (i.e. negative or zero)");
        return NULL;
    }

    other._text = text;
    other._length = strlen(text);
    other._suffixArray = (int *) malloc((size_t) other._length * sizeof(int));
    sais((const unsigned char *) other._text, other._suffixArray, other._length);

    return find_mismatches(tree, anchors, anchorText, maxNumChanges, downstreamStart, downstreamEnd, &other, threads);
}


static PyMethodDef
Tree_methods[] = {
    {"__init__", Tree_init, METH_VARARGS, "Creates a new suffix tree with the given text"},
    {"find_anchors", Tree_find_anchors, METH_VARARGS, "Returns a list of all start positions of the anchor"},
    {"find_repeat_counts", Tree_find_repeat_counts, METH_VARARGS, "Returns a list of tuples [(anchor index, list of matches with n substitutions),...]"},
    {NULL}
};

static PyTypeObject
anchor_search_TreeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "anchor_search._core.Tree",       /* tp_name */
    sizeof(Tree),              /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)Tree_dealloc,  /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT
    | Py_TPFLAGS_BASETYPE,     /* tp_flags */
    "Tree objects",            /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Tree_methods,              /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Tree_init,       /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyModuleDef
anchor_search_core = {
    PyModuleDef_HEAD_INIT,
    "nearmiss._core",
    "Adds suffix tree structures implemented in C for speed",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit__core(void)
{
    PyObject* m = NULL;

    anchor_search_TreeType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&anchor_search_TreeType) < 0)
        return NULL;

    m = PyModule_Create(&anchor_search_core);
    if (m == NULL)
        return NULL;

    Py_INCREF(&anchor_search_TreeType);
    PyModule_AddObject(m, "Tree", (PyObject *)&anchor_search_TreeType);
    return m;
}
