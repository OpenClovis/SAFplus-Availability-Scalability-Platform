# -*- coding: utf-8 -*-

"""Utility stuff for tests."""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Ryan Tomayko (rtomayko@gmail.com)"
__copyright__ = "Copyright 2004-2005, Ryan Tomayko"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import sys
import os
import traceback
try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

import kid.test

class stdold:
    """Original sys.stderr and sys.stdout."""
    out = sys.stdout
    err = sys.stderr

def raises(ExpectedException, *args, **kwargs):
    """Raise AssertionError if code does not raise expected exception."""
    assert args
    if isinstance(args[0], str):
        (expr,) = args
        assert isinstance(expr, str)
        frame = sys._getframe(1)
        loc = frame.f_locals.copy()
        loc.update(kwargs)
        try:
            exec expr in frame.f_globals, loc
        except ExpectedException, e:
            return e
        except Exception, e:
            pass
        else:
            e = None
    else:
        func, args = args[0], args[1:]
        assert callable(func)
        try:
            func(*args, **kwargs)
        except ExpectedException, e:
            return e
        except Exception, e:
            pass
        else:
            e = None
        expr = ["%r" % x for x in args]
        expr.extend(["%s=%r" % x for x in kwargs.items()])
        expr = '%s(%s)' % (func.__name__, ', '.join(expr))
    if e:
        e = 'raised %s instead of' % e.__class__
    else:
        e = 'did not raise'
    raise AssertionError('%s %s %s' % (expr, e, ExpectedException))

def dot():
    stdold.err.write('.')

def skip():
    stdold.err.write('s')

def come_on_guido_this_is_just_wrong(name):
    mod = __import__(name)
    components = name.split('.')
    for comp in components[1:]:
        mod = getattr(mod, comp)
    return mod

def get_funcs(mod):
    """Return a list of test functions for the given module object."""
    funcs = []
    for name in dir(mod):
        if name[:4] == 'test':
            attr = getattr(mod, name)
            if callable(attr):
                funcs.append(attr)
    return funcs

def run_suite(tests, stop_first=True):
    """Run tests given a list of modules that export __test__ variables."""
    try:
        os.mkdir(kid.test.output_dir)
    except OSError:
        e = sys.exc_info()[1]
        if int(e.errno) != 17:
            raise
    bad = []
    kid.test.basic_tests = 1
    test_cnt = skip_cnt = bad_cnt = 0
    from time import time
    start = time()
    # run over modules...
    for module_name in tests:
        try:
            mod = come_on_guido_this_is_just_wrong(module_name)
        except ImportError, e:
            if 'No module named py' not in str(e):
                raise
            skip_cnt += 1
            skip()
            continue # you don't have pylib - so i won't run these tests
        #if not hasattr(mod, '__tests__'):
        #    raise '%r does not export a __tests__ variable.' % module_name
        if hasattr(mod, 'setup_module'):
            mod.setup_module(mod)
        try:
            # run each test...
            for test in get_funcs(mod):
                test_cnt += 1
                sys.stdout, sys.stderr = StringIO(), StringIO()
                try:
                    test()
                except:
                    bad_cnt += 1
                    asserr = isinstance(sys.exc_info()[0], AssertionError)
                    ftype = asserr and 'F' or 'E'
                    buf = StringIO()
                    traceback.print_exc(file=buf)
                    stdold.err.write(ftype)
                    bad.append((test, ftype, sys.exc_info(), \
                        (sys.stdout.getvalue(), sys.stderr.getvalue())))
                    if stop_first:
                        sys.stdout, sys.stderr = stdold.out, stdold.err
                        sys.stderr.write(
                            '*\n\bBailing after %d tests\n\n' % test_cnt)
                        out, err = bad[-1][3]
                        if out:
                            sys.stderr.write(
                                '-- sys.stdout:\n%s\n' % out.strip())
                        if err:
                            sys.stderr.write(
                                '-- sys.stderr:\n%s\n' % err.strip())
                        raise
                else:
                    dot()
            sys.stdout, sys.stderr = stdold.out, stdold.err
        finally:
            if hasattr(mod, 'teardown_module'):
                mod.teardown_module(mod)
    done = time()
    sys.stderr.write('\n')
    for test, ftype, exc_info, (out, err) in bad:
        sys.stderr.write('\n%s: %s\n' %
            ({'F': 'Failure', 'E': 'Error'}.get(ftype, 'Bad'),
                test.__doc__ or test.__name__))
        if out:
            sys.stderr.write(
                '-- sys.stdout:\n%s\n' % out.strip())
        if err:
            sys.stderr.write(
                '-- sys.stderr:\n%s\n' % err.strip())
        traceback.print_exception(
            exc_info[0], exc_info[1], exc_info[2], 15, sys.stderr)
    sys.stderr.write('\nTests: %d (+%d extended) OK (%g seconds)\n'
        % (test_cnt, kid.test.additional_tests, done - start))
    if skip_cnt:
        sys.stderr.write('Skipped tests (need py lib): %d\n' % skip_cnt)
    if bad_cnt:
        sys.stderr.write('Bad tests: %d\n' % bad_cnt)
