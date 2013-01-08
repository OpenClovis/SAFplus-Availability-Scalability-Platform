"""Unit tests for the kid.TemplatePath class.

The testing directory structure looks something like this:

    tmpdir1/
           /index.kid
           /members/
                   /index.kid
                   /stuff.kid
                   /master.kid
           /nonmembers/
                      /index.kid
                      /garbage.kid
                      /master.kid
           /shared/
                  /message.kid
                  /error.kid
                  /errors/
                         /error1.kid
                         /error2.kid
                         /error3.kid
    tmpdir2/
           /index.kid
           /indexz.kid
           /master.kid
           /members/
                   /master.kid
                   /stuff.kid
    tmpdir3/
           /test.kid
           /base.kid

"""

from os import mkdir
from os.path import join as joinpath, normpath
from tempfile import mkdtemp
from shutil import rmtree

import kid
kfind = kid.path.find

def setup_module(module):
    '''Create a testing directory structure.'''
    global files, tmpdir1, tmpdir2, tmpdir3

    def _create(dir, subdir, files):
        '''Create files.'''
        if subdir:
            subdir = joinpath(dir, subdir)
            mkdir(subdir)
        else:
            subdir = dir
        for file in files.split():
            open(joinpath(subdir, file), 'w').write('nothing')
        return subdir

    # create the directory structure
    tmpdir1 = mkdtemp(prefix='kid_test_templatepath1_')
    _create(tmpdir1, None, 'index.kid')
    _create(tmpdir1, 'members', 'index.kid stuff.kid master.kid')
    _create(tmpdir1, 'nonmembers', 'index.kid garbage.kid master.kid')
    _create(_create(tmpdir1, 'shared', 'message.kid error.kid'),
        'errors', 'error1.kid error2.kid error3.kid')
    tmpdir2 = mkdtemp(prefix='kid_test_templatepath2_')
    _create(tmpdir2, None, 'index.kid indexz.kid master.kid')
    _create(tmpdir2, 'members', 'stuff.kid master.kid')
    kid.path.append(tmpdir1)
    kid.path.append(tmpdir2)
    # create another temporary directory
    tmpdir3 = mkdtemp(prefix='kid_test_templatepath3_')
    _create(tmpdir3, None, 'test.kid base.kid')

def teardown_module(module):
    kid.path.remove(tmpdir1)
    kid.path.remove(tmpdir2)
    rmtree(tmpdir1)
    rmtree(tmpdir2)
    rmtree(tmpdir3)

def test_simple_file_in_root():
    assert kfind('index.kid') == normpath(joinpath(tmpdir1, 'index.kid'))
    assert kfind('indexz.kid') == normpath(joinpath(tmpdir2, 'indexz.kid'))

def test_file_in_directory():
    assert kfind(joinpath('members', 'index.kid')) == \
        normpath(joinpath(tmpdir1, 'members', 'index.kid'))
    path = joinpath('shared', 'errors', 'error1.kid')
    assert kfind(path) == normpath(joinpath(tmpdir1, path))

def test_no_exist():
    assert kfind('noexist.kid') == None

def test_find_relative():
    rel = normpath(joinpath(tmpdir1, 'shared', 'error.kid'))
    expected = normpath(joinpath(tmpdir1, 'shared', 'message.kid'))
    assert kfind('message.kid', rel=rel) == expected

def test_crawl_path():
    rel = normpath(joinpath(tmpdir1, 'nonmembers', 'stuff.kid'))
    expected = normpath(joinpath(tmpdir2, 'master.kid'))
    assert kfind('master.kid', rel=rel) == expected

def test_mod_python_bug():
    '''This recreates the problem reported in ticket #110.'''
    assert kfind('base.kid', rel=joinpath(tmpdir3, 'test.kid')) \
        == joinpath(tmpdir3, 'base.kid')
