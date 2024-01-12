
# python -m pytest test-proj2.py

import json
import os
from testutils import create_gui_project_file, cppcheck

__script_dir = os.path.dirname(os.path.abspath(__file__))

COMPILE_COMMANDS_JSON = 'compile_commands.json'

ERR_A = ('%s:1:7: error: Division by zero. [zerodiv]\n' +
         'x = 3 / 0;\n' +
         '      ^\n') % os.path.join('a', 'a.c')
ERR_B = ('%s:1:7: error: Division by zero. [zerodiv]\n' +
         'x = 3 / 0;\n' +
         '      ^\n') % os.path.join('b', 'b.c')


def realpath(s):
    return os.path.realpath(s).replace('\\', '/')


def create_compile_commands(compdb_path):
    if not os.path.isdir(compdb_path):
        os.makedirs(compdb_path)
    j = [{'directory': realpath(os.path.join(__script_dir, 'proj2', 'a')), 'command': 'gcc -c a.c', 'file': 'a.c'},
         {'directory': realpath(os.path.join(__script_dir, 'proj2')), 'command': 'gcc -c b/b.c', 'file': 'b/b.c'}]
    with open(os.path.join(compdb_path, COMPILE_COMMANDS_JSON), 'wt') as f:
        f.write(json.dumps(j))


# TODO: why is this missing the progress messages?
def test_file_filter():
    ret, stdout, stderr = cppcheck(['proj2/', '--file-filter=proj2/a/*'], cwd=__script_dir)
    file1 = os.path.join('proj2', 'a', 'a.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        'Checking %s: AAA...' % file1
    ]
    assert ret == 0, stdout

    ret, stdout, stderr = cppcheck(['proj2/', '--file-filter=proj2/b*'], cwd=__script_dir)
    file2 = os.path.join('proj2', 'b', 'b.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file2,
    ]
    assert ret == 0, stdout


def test_local_path(tmpdir):
    create_compile_commands(tmpdir)
    ret, stdout, stderr = cppcheck(['--project={}'.format(COMPILE_COMMANDS_JSON)], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.join(__script_dir, 'proj2', 'b', 'b.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done'
    ]
    assert ret == 0, stdout


# TODO: we actually need to trigger the --force limit
def test_local_path_force(tmpdir):
    create_compile_commands(tmpdir)
    ret, stdout, stderr = cppcheck(['--project={}'.format(COMPILE_COMMANDS_JSON), '--force'], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.join(__script_dir, 'proj2', 'b', 'b.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        'Checking %s: AAA...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done',
    ]
    assert ret == 0, stdout


# TODO: we actually need to hit the threshold
def test_local_path_maxconfigs(tmpdir):
    create_compile_commands(tmpdir)
    ret, stdout, stderr = cppcheck(['--project={}'.format(COMPILE_COMMANDS_JSON), '--max-configs=2'], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.join(__script_dir, 'proj2', 'b', 'b.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        'Checking %s: AAA...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done',
    ]
    assert ret == 0, stdout


def test_relative_path(tmpdir):
    create_compile_commands(os.path.join(tmpdir, 'proj2'))
    ret, stdout, stderr = cppcheck(['--project={}'.format(os.path.join('proj2', COMPILE_COMMANDS_JSON))], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.join(__script_dir, 'proj2', 'b', 'b.c')
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done'
    ]
    assert ret == 0, stdout


def test_absolute_path(tmpdir):
    create_compile_commands(tmpdir)
    ret, stdout, stderr = cppcheck(['--project={}'.format(os.path.join(tmpdir, COMPILE_COMMANDS_JSON))])
    file1 = os.path.realpath(os.path.join(__script_dir, 'proj2', 'a', 'a.c'))
    file2 = os.path.realpath(os.path.join(__script_dir, 'proj2', 'b', 'b.c'))
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done'
    ]
    assert ret == 0, stdout


# TODO
def test_gui_project_loads_compile_commands_1(tmpdir):
    create_compile_commands(os.path.join(tmpdir, 'proj2'))
    ret, stdout, stderr = cppcheck(['--project={}'.format(os.path.join(__script_dir, 'proj2', 'proj2.cppcheck'))], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.realpath(os.path.join(__script_dir, 'proj2', 'b', 'b.c'))
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done'
    ]
    assert ret == 0, stdout


def test_gui_project_loads_compile_commands_2(tmpdir):
    create_compile_commands(os.path.join(tmpdir, 'proj2'))
    exclude_path_1 = 'proj2/b'
    create_gui_project_file(os.path.join(tmpdir, 'proj2', 'test.cppcheck'),
                            import_project=COMPILE_COMMANDS_JSON,
                            exclude_paths=[exclude_path_1])
    ret, stdout, stderr = cppcheck(['--project=proj2/test.cppcheck'], cwd=tmpdir)
    file1 = os.path.join(__script_dir, 'proj2', 'a', 'a.c')
    file2 = os.path.realpath(os.path.join(__script_dir, 'proj2', 'b', 'b.c'))
    lines = stdout.splitlines()
    assert lines == [
        'Checking %s ...' % file1,
        '1/2 files checked 50% done',
        'Checking %s ...' % file2,
        '2/2 files checked 100% done'
    ]
    assert ret == 0, stdout


def test_gui_project_loads_relative_vs_solution():
    create_gui_project_file('test.cppcheck', import_project='proj2/proj2.sln')
    ret, stdout, stderr = cppcheck(['--project=test.cppcheck'])
    file1 = os.path.join('proj2', 'a', 'a.c')
    file2 = os.path.join('proj2', 'b', 'b.c')
    assert ret == 0, stdout
    assert stdout.find('Checking %s Debug|Win32...' % file1) >= 0
    assert stdout.find('Checking %s Debug|x64...' % file1) >= 0
    assert stdout.find('Checking %s Release|Win32...' % file1) >= 0
    assert stdout.find('Checking %s Release|x64...' % file1) >= 0
    assert stdout.find('Checking %s Debug|Win32...' % file2) >= 0
    assert stdout.find('Checking %s Debug|x64...' % file2) >= 0
    assert stdout.find('Checking %s Release|Win32...' % file2) >= 0
    assert stdout.find('Checking %s Release|x64...' % file2) >= 0

def test_gui_project_loads_absolute_vs_solution():
    create_gui_project_file('test.cppcheck', import_project=realpath('proj2/proj2.sln'))
    ret, stdout, stderr = cppcheck(['--project=test.cppcheck'])
    file1 = os.path.realpath('proj2/a/a.c')
    file2 = os.path.realpath('proj2/b/b.c')
    print(stdout)
    assert ret == 0, stdout
    assert stdout.find('Checking %s Debug|Win32...' % file1) >= 0
    assert stdout.find('Checking %s Debug|x64...' % file1) >= 0
    assert stdout.find('Checking %s Release|Win32...' % file1) >= 0
    assert stdout.find('Checking %s Release|x64...' % file1) >= 0
    assert stdout.find('Checking %s Debug|Win32...' % file2) >= 0
    assert stdout.find('Checking %s Debug|x64...' % file2) >= 0
    assert stdout.find('Checking %s Release|Win32...' % file2) >= 0
    assert stdout.find('Checking %s Release|x64...' % file2) >= 0

def test_gui_project_loads_relative_vs_solution_2():
    create_gui_project_file('test.cppcheck', root_path='proj2', import_project='proj2/proj2.sln')
    ret, stdout, stderr = cppcheck(['--project=test.cppcheck'])
    assert ret == 0, stdout
    assert stderr == ERR_A + ERR_B

def test_gui_project_loads_relative_vs_solution_with_exclude():
    create_gui_project_file('test.cppcheck', root_path='proj2', import_project='proj2/proj2.sln', exclude_paths=['b'])
    ret, stdout, stderr = cppcheck(['--project=test.cppcheck'])
    assert ret == 0, stdout
    assert stderr == ERR_A

def test_gui_project_loads_absolute_vs_solution_2():
    create_gui_project_file('test.cppcheck',
                            root_path=realpath('proj2'),
                            import_project=realpath('proj2/proj2.sln'))
    ret, stdout, stderr = cppcheck(['--project=test.cppcheck'])
    assert ret == 0, stdout
    assert stderr == ERR_A + ERR_B
