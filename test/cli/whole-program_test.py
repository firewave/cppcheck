import os
import pytest
import json
from testutils import cppcheck

__script_dir = os.path.dirname(os.path.abspath(__file__))

# TODO: use dedicated addon


def __create_compile_commands(dir, filePaths, definesList=[[None]]):
    j = []
    for i, fp in enumerate(filePaths):
        f = os.path.basename(fp)

        for defines in (definesList[i] if len(definesList) > i else [None]):
            obj = {
                'directory': os.path.dirname(os.path.abspath(fp)),
                'command': 'gcc -c {} {}'.format(f, ' '.join('-D{}'.format(define) for define in defines) if defines is not None else ''),
                'file': f
            }
        j.append(obj)
    compile_commands = os.path.join(dir, 'compile_commmands.json')
    with open(compile_commands, 'wt') as f:
        f.write(json.dumps(j))
    return compile_commands


def __test_addon_suppress_inline(extra_args):
    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=information,style',
        '--disable=missingInclude',  # TODO: remove
        '--inline-suppr',
        '--error-exitcode=1',
        'whole-program/whole1.c',
        'whole-program/whole2.c'
    ]
    args += extra_args
    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == []
    assert stdout == ''
    assert ret == 0, stdout


def test_addon_suppress_inline():
    __test_addon_suppress_inline(['-j1'])


@pytest.mark.xfail(strict=True)
def test_addon_suppress_inline_j():
    __test_addon_suppress_inline(['-j2'])


def __test_addon_suppress_inline_project(tmpdir, extra_args):
    compile_db = __create_compile_commands(tmpdir, [
        os.path.join(__script_dir, 'whole-program', 'whole1.c'),
        os.path.join(__script_dir, 'whole-program', 'whole2.c')
    ])

    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=information,style',
        '--disable=missingInclude',  # TODO: remove
        '--inline-suppr',
        '--error-exitcode=1',
        '--project={}'.format(compile_db)
    ]
    args += extra_args
    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == []
    assert stdout == ''
    assert ret == 0, stdout


def test_addon_suppress_inline_project(tmpdir):
    __test_addon_suppress_inline_project(tmpdir, ['-j1'])


@pytest.mark.xfail(strict=True)
def test_addon_suppress_inline_project_j(tmpdir):
    __test_addon_suppress_inline_project(tmpdir, ['-j2'])



# @pytest.mark.xfail(strict=True)
def test_unused_A_B():
    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=all',
        '--error-exitcode=1',
        'whole-program/configs_unused.c'
    ]

    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == [
        "whole-program/configs_unused.c:3:9: style: struct member 'X::x' is never used. [unusedStructMember]",
        "whole-program/configs_unused.c:2:1: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-2.3]",
    ]
    assert stdout == ''
    assert ret == 1, stdout


@pytest.mark.xfail(strict=True)
def test_unused_with_project_A_and_B(tmpdir):
    # A and B config
    configs_unused_file = os.path.join(__script_dir, 'whole-program', 'configs_unused.c')
    compile_db = __create_compile_commands(tmpdir, [configs_unused_file],
        [["A", "B"]]
    )
    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=all',
        '--error-exitcode=1',
        '--project={}'.format(compile_db)
    ]
    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == [
        # "{}:3:9: style: struct member 'X::x' is never used. [unusedStructMember]".format(configs_unused_file),
        # "{}:6:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-5.9]".format(configs_unused_file),  # this one is due to duplicate ctu-info content
        # "{}:2:1: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-2.3]".format(configs_unused_file),
    ]
    assert stdout == ''
    assert ret == 0, stdout


def test_unused_with_project_A_or_B(tmpdir):
    # A or B configs
    configs_unused_file = os.path.join(__script_dir, 'whole-program', 'configs_unused.c')
    compile_db = __create_compile_commands(tmpdir, [configs_unused_file],
        [["A"], ["B"]]
    )
    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=all',
        '--error-exitcode=1',
        '--project={}'.format(compile_db)
    ]
    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == [
    ]
    assert stdout == ''
    assert ret == 0, stdout

@pytest.mark.xfail(strict=True)
def test_unused_with_project_B_or_A(tmpdir):
    # A or B configs
    configs_unused_file = os.path.join(__script_dir, 'whole-program', 'configs_unused.c')
    compile_db = __create_compile_commands(tmpdir, [configs_unused_file],
        [["B"], ["A"]]
    )
    args = [
        '-q',
        '--addon=misra',
        '--template=simple',
        '--enable=all',
        '--error-exitcode=1',
        '--project={}'.format(compile_db)
    ]
    ret, stdout, stderr = cppcheck(args, cwd=__script_dir)
    lines = stderr.splitlines()
    assert lines == [
        # "{}:3:9: style: struct member 'X::x' is never used. [unusedStructMember]".format(configs_unused_file),
        # "{}:6:0: style: The function 'func' is never used. [unusedFunction]".format(configs_unused_file),
        # "{}:2:1: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-2.3]".format(configs_unused_file),
    ]
    assert stdout == ''
    assert ret == 0, stdout