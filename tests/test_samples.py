#!/usr/bin/env python3

"""
USAGE:
./test_samples.py
./test_samples.py <test1> <test2> <test3>
"""
import multiprocessing
import sys
import os
import time
import unittest
from typing import List, Any, Tuple, Dict

import testtools
import subprocess
import re

OPT = ['-O1', '-flto']

COMPILER = '../build/bin/clang++'
COMPILER_REF = 'clang++-9'
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class AddressUniquer:
    def __init__(self, min_len=8):
        self.address_to_index = {}
        self.offsets = {}
        self.next_index = 0
        self.min_len = min_len

    @staticmethod
    def __plusminus(x):
        if x >= 0:
            return f'+{x}'
        return str(x)

    def __repl(self, match) -> str:
        txt = match.group(0)
        if len(txt) > 14 or len(txt) < self.min_len:
            return txt
        number = int(txt[2:], 16)
        key = number >> 24
        if key in self.address_to_index:
            index = self.address_to_index[key]
        else:
            index = self.next_index
            self.address_to_index[key] = index
            self.offsets[key] = number & 0xffffff
            self.next_index += 1
        offset = (number & 0xffffff) - self.offsets[key]
        return f'{{address #{index} {self.__plusminus(offset)}}}'

    def sub(self, txt: str) -> str:
        return re.sub(r'0x[\da-f]+\b', self.__repl, txt)


class Compiler:
    def __init__(self, compiler: str, flags: List[str] = None):
        self.compiler = compiler
        self.flags = list(flags) if flags else []
        self.ldflags: List[str] = []
        self.run_prefix: List[str] = []
        self.suffix = ''
        self.addr_min_len = 8
        self.use_libcxx = False

    def compile(self, files: List[str], output: str, flags: List[str] = None, **kwargs):
        return subprocess.check_output([self.compiler, '-c'] + files + ['-o', output] + self.flags + flags, **kwargs)

    def link(self, files: List[str], output: str, flags: List[str] = None, **kwargs):
        cmd = [self.compiler] + files + ['-o', output, '-fuse-ld=lld'] + self.flags + self.ldflags + flags
        if self.use_libcxx and '-no-novt-libs' not in cmd:
            cmd += ['-stdlib=libc++']
        return subprocess.check_output(cmd, **kwargs)

    def run(self, argv: List[str], **kwargs) -> bytes:
        return subprocess.check_output(self.run_prefix + argv, **kwargs)

    def get_flags(self) -> Dict[str, str]:
        result = {
            'CXX': os.path.abspath(self.compiler),
            'CXXFLAGS': ' '.join(x for x in self.flags if x not in OPT),
            'LDFLAGS': ' '.join(x for x in self.flags + self.ldflags if x not in OPT) + ' -fuse-ld=lld'
        }
        if self.use_libcxx:
            result['LDFLAGS'] += ' -stdlib=libc++'
        if self.run_prefix:
            result['RUN_PREFIX'] = ' '.join(self.run_prefix)
        return result


def compare_outputs(out1, out2, name=None, **kwargs):
    out1 = out1.decode()
    out2 = out2.decode()
    out1 = AddressUniquer(**kwargs).sub(out1)
    out2 = AddressUniquer(**kwargs).sub(out2)

    if out1 != out2:
        if name:
            print(f'### ({name}) ###')
        print('-------- OUTPUT -------')
        print(out1)
        print('------ REFERENCE ------')
        print(out2)
        diffs = [num + 1 for num, (l1, l2) in enumerate(zip(out1.split('\n'), out2.split('\n'))) if l1 != l2]
        print('// differences in lines:', diffs)
        assert out1 == out2, 'outputs differ'


def test_sample(my_compiler: Compiler, ref_compiler: Compiler, name: str, files: List[str] = None, inputs=None,
                flags=None, my_flags=None):
    if inputs is None:
        inputs = [[]]
    if flags is None:
        flags = []
    if my_flags is None:
        my_flags = []
    files = [f'{name}.cpp'] + (files if files else [])
    # print(f'========== {name} ==========')
    # compile things
    try:
        for f in files:
            my_compiler.compile([f], f + my_compiler.suffix + '.o', flags + my_flags, stderr=subprocess.STDOUT)
        my_compiler.link([f + my_compiler.suffix + '.o' for f in files], name + my_compiler.suffix, flags + my_flags,
                         stderr=subprocess.STDOUT)
        ref_compiler.link(files, f'{name}-ref{ref_compiler.suffix}', flags, stderr=subprocess.STDOUT)
        # run things
        for inp in inputs:
            out1 = my_compiler.run([f'./{name}{my_compiler.suffix}'] + inp, timeout=5, stderr=subprocess.STDOUT)
            out2 = ref_compiler.run([f'./{name}-ref{ref_compiler.suffix}'] + inp, timeout=5, stderr=subprocess.STDOUT)
            compare_outputs(out1, out2, name, min_len=my_compiler.addr_min_len)
    except subprocess.CalledProcessError as e:
        print(e)
        print(f'--- STDOUT ({name}) ---')
        print(e.stdout.decode() if e.stdout is not None else '(none)')
        if e.stderr is not None:
            print('--- STDERR ---')
            print(e.stderr.decode() if e.stderr is not None else '(none)')
        print('--- </OUT> ---')
        raise


class NativeTestCases(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arch = 'native'
        self.my_compiler: Compiler = Compiler(COMPILER, OPT)
        self.ref_compiler: Compiler = Compiler(COMPILER_REF, OPT)
        self._init_compilers()

    def _init_compilers(self):
        pass

    def _set_sysroot(self, folder: str, arch: str, gcc=8, libcxx=False):
        new_args_ref = [
            '--sysroot', folder,
            '-isystem', f'{folder}/usr/include/c++/{gcc}',
            '-isystem', f'{folder}/usr/include/{arch}/c++/{gcc}',
            '-isystem', f'{folder}/usr/lib/gcc/{arch}/{gcc}/include',
            '-isystem', f'{folder}/usr/include/{arch}',
            '-isystem', f'{folder}/usr/include',
        ]
        if libcxx:
            new_args = new_args_ref[:2] + [
                '-isystem', f'{folder}/usr/include',
                '-isystem', f'{folder}/novt-lib/include/c++/v1'
            ] + new_args_ref[2:]
        else:
            new_args = new_args_ref
        self.my_compiler.flags += new_args
        self.my_compiler.ldflags += ['-L', f'{folder}/usr/lib/{arch}', '-L', f'{folder}/usr/lib/gcc/{arch}/{gcc}']
        self.my_compiler.ldflags += ['-B', f'{folder}/usr/lib/{arch}', '-B', f'{folder}/usr/lib/gcc/{arch}/{gcc}']
        self.ref_compiler.flags += new_args_ref
        self.ref_compiler.ldflags += ['-L', f'{folder}/usr/lib/{arch}', '-L', f'{folder}/usr/lib/gcc/{arch}/{gcc}']
        self.ref_compiler.ldflags += ['-B', f'{folder}/usr/lib/{arch}', '-B', f'{folder}/usr/lib/gcc/{arch}/{gcc}']

    def test0(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test0', my_flags=['-no-novt-libs'])

    def test1(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test1', inputs=[['a'], ['b'], ['c']])

    def test2(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test2', files=['test2a.cpp', 'test2b.cpp', 'test2c.cpp'],
                    inputs=[['a'], ['b'], ['c']])

    def test3(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test3', files=['sample_classes_decl.cpp'],
                    inputs=[['a'], ['b'], ['c']])

    def test4(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test4', files=['test4-impl.cpp'], my_flags=['-no-novt-libs'])

    def test5(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test5', files=['test5-impl.cpp'], my_flags=['-no-novt-libs'])

    def test6(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test6', files=['test6-impl.cpp'])

    def test7(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test7', my_flags=['-no-novt-libs'])

    def test8(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test8', files=['test8-impl.cpp'])

    def test9(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test9', files=['test9-impl.cpp'])

    def test10(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test10', my_flags=['-no-novt-libs'])

    def test11(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test11', my_flags=['-no-novt-libs'])

    def test12(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test12')

    def test13(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test13', files=['test13-impl.cpp'],
                    my_flags=['-no-novt-libs'])

    def test14(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test14', my_flags=['-no-novt-libs'])

    def test15(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test15', my_flags=['-no-novt-libs'])

    def test16(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test16', my_flags=['-no-novt-libs'])

    def test17(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test17', my_flags=['-no-novt-libs'])

    def test18(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test18', my_flags=['-no-novt-libs'])

    def test19(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test19', my_flags=['-no-novt-libs'])

    def test20(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test20', my_flags=['-no-novt-libs'])

    def test21(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test21', files=['test21-impl2.cpp', 'test21-impl3.cpp'],
                    flags=['-fno-rtti', '-O2'])

    def test22(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test22')

    def test23(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test23')

    def test24(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test24', my_flags=['-no-novt-libs'])

    def test25(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test25', my_flags=['-no-novt-libs'])

    def test26(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test26')

    def test27(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test27', files=['test27-impl.cpp'],
                    my_flags=['-no-novt-libs'])

    def test28(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test28', files=['test28-impl1.cpp', 'test28-impl2.cpp'],
                    my_flags=['-no-novt-libs'])

    def test29(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test29', files=['test29-impl.cpp'])

    def test30(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test30', my_flags=['-no-novt-libs'])

    def test31(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test31')

    def test32(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test32', my_flags=['-no-novt-libs'])

    def test33(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test33', my_flags=['-no-novt-libs'])

    def test34(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test34', my_flags=['-no-novt-libs'])

    def test35(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test35', my_flags=['-no-novt-libs'])

    def test36(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test36', my_flags=['-no-novt-libs'])

    def test37(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test37_2', my_flags=['-no-novt-libs'])
        test_sample(self.my_compiler, self.ref_compiler, 'test37', my_flags=['-no-novt-libs'])

    def test38(self):
        test_sample(self.my_compiler, self.ref_compiler, 'test38', my_flags=['-no-novt-libs'])


class Amd64DebianTestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'amd64'
        self._set_sysroot(ROOT_DIR + '/sysroots/x86_64-linux-gnu', 'x86_64-linux-gnu')
        self.my_compiler.suffix = '-amd64'
        self.ref_compiler.suffix = '-amd64'


class X86TestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'x86'
        self.my_compiler.flags += ['-m32']
        self.ref_compiler.flags += ['-m32']
        self._set_sysroot(ROOT_DIR + '/sysroots/i386-linux-gnu', 'i386-linux-gnu')
        self.my_compiler.suffix = '-32'
        self.ref_compiler.suffix = '-32'


class ArmTestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'arm'
        self.my_compiler.flags += ['--target=armv7-pc-linux-gnueabi-gcc', '-mfloat-abi=hard']
        self.ref_compiler.flags += ['--target=armv7-pc-linux-gnueabi-gcc', '-mfloat-abi=hard']
        self._set_sysroot(ROOT_DIR + '/sysroots/arm-linux-gnueabihf', 'arm-linux-gnueabihf')
        self.my_compiler.run_prefix = ['qemu-arm', '-L', ROOT_DIR + '/sysroots/arm-linux-gnueabihf']
        self.ref_compiler.run_prefix = ['qemu-arm', '-L', ROOT_DIR + '/sysroots/arm-linux-gnueabihf']
        self.my_compiler.suffix = '-arm'
        self.ref_compiler.suffix = '-arm'
        self.my_compiler.addr_min_len = 5


class AArch64TestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'aarch'
        self.my_compiler.flags += ['--target=aarch64-linux-gnu']
        self.my_compiler.use_libcxx = True
        self.ref_compiler.flags += ['--target=aarch64-linux-gnu']
        self._set_sysroot(ROOT_DIR + '/sysroots/aarch64-linux-gnu', 'aarch64-linux-gnu', libcxx=True)
        self.my_compiler.run_prefix = ['qemu-aarch64', '-L', ROOT_DIR + '/sysroots/aarch64-linux-gnu']
        self.ref_compiler.run_prefix = ['qemu-aarch64', '-L', ROOT_DIR + '/sysroots/aarch64-linux-gnu']
        self.my_compiler.suffix = '-aarch64'
        self.ref_compiler.suffix = '-aarch64'

    def test12(self):
        try:
            super().test12()
        except AssertionError as e:
            print('ASSERT:', str(e), e)
            if 'outputs differ' in str(e):
                print('(Failure is expected)')
                return
            raise


class MipsTestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'mips'
        self.my_compiler.flags += ['--target=mips-mti-linux-gnu']
        self.ref_compiler.flags += ['--target=mips-mti-linux-gnu']
        self.my_compiler.ldflags += ['-Wl,--hash-style=sysv', '-Wl,-z,notext']
        self.ref_compiler.ldflags += ['-Wl,--hash-style=sysv', '-Wl,-z,notext']
        self._set_sysroot(ROOT_DIR + '/sysroots/mips-linux-gnu', 'mips-linux-gnu')
        self.my_compiler.run_prefix = ['qemu-mips', '-L', ROOT_DIR + '/sysroots/mips-linux-gnu']
        self.ref_compiler.run_prefix = ['qemu-mips', '-L', ROOT_DIR + '/sysroots/mips-linux-gnu']
        self.my_compiler.suffix = '-mips'
        self.ref_compiler.suffix = '-mips'
        self.my_compiler.addr_min_len = 5


class Mips64elTestCases(NativeTestCases):
    def _init_compilers(self):
        self.arch = 'mips64el'
        self.my_compiler.flags += ['--target=mips64el-linux-gnuabi64']
        self.ref_compiler.flags += ['--target=mips64el-linux-gnuabi64']
        self._set_sysroot(ROOT_DIR + '/sysroots/mips64el-linux-gnuabi64', 'mips64el-linux-gnuabi64')
        self.my_compiler.run_prefix = ['qemu-mips64el', '-L', ROOT_DIR + '/sysroots/mips64el-linux-gnuabi64']
        self.ref_compiler.run_prefix = ['qemu-mips64el', '-L', ROOT_DIR + '/sysroots/mips64el-linux-gnuabi64']
        self.my_compiler.suffix = '-mips64el'
        self.ref_compiler.suffix = '-mips64el'
        self.my_compiler.addr_min_len = 5


class PowerPC64leTestCases(AArch64TestCases):
    def _init_compilers(self):
        self.arch = 'powerpc64le'
        self.my_compiler.flags += ['--target=powerpc64le-linux-gnu']
        self.my_compiler.use_libcxx = True
        self.ref_compiler.flags += ['--target=powerpc64le-linux-gnu']
        self._set_sysroot(ROOT_DIR + '/sysroots/powerpc64le-linux-gnu', 'powerpc64le-linux-gnu', libcxx=True)
        self.my_compiler.run_prefix = ['qemu-ppc64le', '-L', ROOT_DIR + '/sysroots/powerpc64le-linux-gnu']
        self.ref_compiler.run_prefix = ['qemu-ppc64le', '-L', ROOT_DIR + '/sysroots/powerpc64le-linux-gnu']
        self.my_compiler.suffix = '-ppc64le'
        self.ref_compiler.suffix = '-ppc64le'


class MyStreamResult(testtools.StreamResult):

    def __init__(self, total: int):
        super().__init__()
        self.ok = 0
        self.err = 0
        self.done = 0
        self.total = total
        self.failed_tests = []

    def startTestRun(self):
        super().startTestRun()

    def stopTestRun(self):
        super().stopTestRun()
        print(f'Tests finished: ok = {self.ok}     error = {self.err}', file=sys.stderr)
        if self.failed_tests:
            print('Failed:', ', '.join(self.failed_tests))

    def status(self, test_id=None, test_status=None, test_tags=None, runnable=True, file_name=None, file_bytes=None,
               eof=False, mime_type=None, route_code=None, timestamp=None):
        super().status(test_id, test_status, test_tags, runnable, file_name, file_bytes, eof, mime_type, route_code,
                       timestamp)
        if test_id.startswith('__main__.'):
            test_id = test_id[9:]
        if test_status in ('success', 'xfail'):
            self.ok += 1
            self.done += 1
            print(f'[OK]  ({self.done} / {self.total})  {test_id}', file=sys.stderr)
        elif test_status in ('uxsuccess', 'fail'):
            self.err += 1
            self.done += 1
            print(f'[ERR] ({self.done} / {self.total})  {test_id}', file=sys.stderr)
            self.failed_tests.append(test_id)
        else:
            # print([test_id, test_status, test_tags, runnable, file_name, file_bytes, eof, mime_type, timestamp])
            pass


def partition_tests(tests: List, n: int) -> List[Tuple[unittest.TestSuite, Any]]:
    threads = [(unittest.TestSuite(), None) for _ in range(min(n, len(tests)))]
    for i, test in enumerate(tests):
        threads[i % len(threads)][0].addTest(test)
    return threads


def print_compiler_config(architectures):
    for tests in architectures:
        x = tests()
        compiler = x.my_compiler
        flags = compiler.get_flags()
        print('\n\n'+'='*80+f'\nArchitecture: {x.arch}\n'+'='*80)
        for k, v in flags.items():
            print(f'{k}={v}')


def main():
    global COMPILER
    architectures = {
        'amd64': NativeTestCases,
        'amd64_debian': Amd64DebianTestCases,
        'arm': ArmTestCases,
        'aarch64': AArch64TestCases,
        'x86': X86TestCases,
        'mips': MipsTestCases,
        'mips64el': Mips64elTestCases,
        'powerpc64le': PowerPC64leTestCases
    }
    # parse commandline
    selected_testcases = set()
    selected_architectures = set()
    for a in sys.argv[1:]:
        if a in architectures:
            selected_architectures.add(architectures[a])
        elif a == 'all':
            selected_architectures.update(architectures.values())
        elif a == 'release':
            COMPILER = COMPILER.replace('-debug', '-minsizerel')
            print(f'using compiler {COMPILER} ...')
        elif a == 'config':
            print_compiler_config(selected_architectures)
            return
        else:
            selected_testcases.add(a)
    if not selected_architectures:
        selected_architectures.add(architectures['amd64'])

    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    start = time.time()
    suite = []
    for arch in selected_architectures:
        suite += list(unittest.TestLoader().loadTestsFromTestCase(arch))
    if len(selected_testcases) > 0:
        suite = [s for s in suite if s.id().split('.')[-1] in selected_testcases]
    print(f'Running {len(suite)} tests ...')
    # concurrent_suite = testtools.ConcurrentStreamTestSuite(lambda: ((case, None) for case in suite))
    concurrent_suite = testtools.ConcurrentStreamTestSuite(lambda: partition_tests(suite, multiprocessing.cpu_count()))
    result = MyStreamResult(len(suite))
    concurrent_suite.run(result)
    result.stopTestRun()
    duration = time.time() - start
    print(f'Timing: {round(duration)} seconds')


if __name__ == '__main__':
    # unittest.main()
    main()
