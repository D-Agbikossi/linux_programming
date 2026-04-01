/*
 * Python C extension: vibration — statistical functions on sequences of doubles.
 *
 * Math:
 *   peak_to_peak: max(x) - min(x)
 *   rms: sqrt( (1/n) * sum(x_i^2) )
 *   std_dev: sample std: sqrt( sum((x_i - mean)^2) / (n-1) ), n>=2; n==1 -> 0.0
 *   above_threshold: count of x_i > threshold (strict)
 *   summary: count, mean, min, max in one pass (min/max/mean); mean uses Kahan? simple sum for n small — double sum is OK for coursework
 *
 * Python <-> C: PyList_Check / PyTuple_Check; PySequence_GetItem + PyFloat_AsDouble;
 *               PyErr_SetString on bad types; Py_BuildValue for dict.
 *
 * Memory: no extra heap for data — we iterate the sequence with PySequence_GetItem
 *         (temporary PyObject* per element, DECREF immediately). No malloc for arrays.
 *
 * Time: O(n) per function; summary is O(n) single pass.
 *
 * Numerical notes: std_dev uses two-pass algorithm (mean then variance) for clarity
 * and stability vs naive one-pass E[X^2]-E[X]^2. Welford optional for online variance.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

static int
parse_double_seq(PyObject *seq, Py_ssize_t *pn)
{
	if (!PyList_Check(seq) && !PyTuple_Check(seq)) {
		PyErr_SetString(PyExc_TypeError, "expected list or tuple of floats");
		return -1;
	}
	*pn = PySequence_Size(seq);
	if (*pn < 0)
		return -1;
	return 0;
}

static int
get_double_at(PyObject *seq, Py_ssize_t i, double *out)
{
	PyObject *item = PySequence_GetItem(seq, i);
	if (item == NULL)
		return -1;
	*out = PyFloat_AsDouble(item);
	Py_DECREF(item);
	if (PyErr_Occurred())
		return -1;
	return 0;
}

static PyObject *
peak_to_peak(PyObject *self, PyObject *args)
{
	PyObject *seq;
	Py_ssize_t n, i;
	double mn, mx, x;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;
	if (parse_double_seq(seq, &n) < 0)
		return NULL;
	if (n == 0)
		return PyFloat_FromDouble(0.0);
	if (get_double_at(seq, 0, &mn) < 0)
		return NULL;
	mx = mn;
	for (i = 1; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		if (x < mn)
			mn = x;
		if (x > mx)
			mx = x;
	}
	return PyFloat_FromDouble(mx - mn);
}

static PyObject *
rms(PyObject *self, PyObject *args)
{
	PyObject *seq;
	Py_ssize_t n, i;
	double sumsq = 0.0, x;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;
	if (parse_double_seq(seq, &n) < 0)
		return NULL;
	if (n == 0)
		return PyFloat_FromDouble(0.0);
	for (i = 0; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		sumsq += x * x;
	}
	return PyFloat_FromDouble(sqrt(sumsq / (double)n));
}

static PyObject *
std_dev(PyObject *self, PyObject *args)
{
	PyObject *seq;
	Py_ssize_t n, i;
	double mean = 0.0, varsum = 0.0, x;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;
	if (parse_double_seq(seq, &n) < 0)
		return NULL;
	if (n == 0)
		return PyFloat_FromDouble(0.0);
	if (n == 1) {
		if (get_double_at(seq, 0, &x) < 0)
			return NULL;
		(void)x;
		return PyFloat_FromDouble(0.0);
	}
	for (i = 0; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		mean += x;
	}
	mean /= (double)n;
	for (i = 0; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		double d = x - mean;
		varsum += d * d;
	}
	return PyFloat_FromDouble(sqrt(varsum / (double)(n - 1)));
}

static PyObject *
above_threshold(PyObject *self, PyObject *args)
{
	PyObject *seq;
	double thr, x;
	Py_ssize_t n, i;
	Py_ssize_t cnt = 0;

	if (!PyArg_ParseTuple(args, "Od", &seq, &thr))
		return NULL;
	if (parse_double_seq(seq, &n) < 0)
		return NULL;
	for (i = 0; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		if (x > thr)
			cnt++;
	}
	return PyLong_FromSsize_t(cnt);
}

static PyObject *
summary(PyObject *self, PyObject *args)
{
	PyObject *seq;
	Py_ssize_t n, i;
	double mn, mx, x, sum = 0.0;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;
	if (parse_double_seq(seq, &n) < 0)
		return NULL;
	if (n == 0)
		return Py_BuildValue("{s:i,s:d,s:d,s:d}",
		    "count", 0, "mean", 0.0, "min", 0.0, "max", 0.0);
	if (get_double_at(seq, 0, &x) < 0)
		return NULL;
	mn = mx = x;
	sum = x;
	for (i = 1; i < n; i++) {
		if (get_double_at(seq, i, &x) < 0)
			return NULL;
		sum += x;
		if (x < mn)
			mn = x;
		if (x > mx)
			mx = x;
	}
	return Py_BuildValue("{s:n,s:d,s:d,s:d}",
	    "count", n,
	    "mean", sum / (double)n,
	    "min", mn,
	    "max", mx);
}

static PyMethodDef VibrationMethods[] = {
	{"peak_to_peak", peak_to_peak, METH_VARARGS, "max - min"},
	{"rms", rms, METH_VARARGS, "root mean square"},
	{"std_dev", std_dev, METH_VARARGS, "sample standard deviation"},
	{"above_threshold", above_threshold, METH_VARARGS, "count strictly above threshold"},
	{"summary", summary, METH_VARARGS, "dict count, mean, min, max"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef vibrationmodule = {
	PyModuleDef_HEAD_INIT,
	"vibration",
	"Vibration statistics (double arrays)",
	-1,
	VibrationMethods
};

PyMODINIT_FUNC
PyInit_vibration(void)
{
	return PyModule_Create(&vibrationmodule);
}
