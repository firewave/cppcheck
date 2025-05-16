import subprocess
import pathlib

cmd = './testrunner TestBufferOverrun'

with subprocess.Popen(cmd.split(), stderr=subprocess.PIPE, universal_newlines=True) as p:
    out = p.stderr.read().strip()

file = '/home/user/CLionProjects/cppcheck/test/testbufferoverrun.cpp'

text = pathlib.Path(file).read_text()

lines = out.splitlines()
for l in lines:
    if not '0SEP0' in l:
        continue
    parts = l.split('0SEP0')
    text = text.replace(parts[0], parts[1])

with open(file, 'wt') as f:
    f.write(text)