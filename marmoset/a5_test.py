from __future__ import print_function
import subprocess
import os
import shutil

stdlib_files = [
    'third_party/cs444/stdlib/5.0/java/util/Arrays.java',
    'third_party/cs444/stdlib/5.0/java/io/PrintStream.java',
    'third_party/cs444/stdlib/5.0/java/io/Serializable.java',
    'third_party/cs444/stdlib/5.0/java/io/OutputStream.java',
    'third_party/cs444/stdlib/5.0/java/lang/Short.java',
    'third_party/cs444/stdlib/5.0/java/lang/Boolean.java',
    'third_party/cs444/stdlib/5.0/java/lang/Class.java',
    'third_party/cs444/stdlib/5.0/java/lang/Number.java',
    'third_party/cs444/stdlib/5.0/java/lang/String.java',
    'third_party/cs444/stdlib/5.0/java/lang/Integer.java',
    'third_party/cs444/stdlib/5.0/java/lang/System.java',
    'third_party/cs444/stdlib/5.0/java/lang/Byte.java',
    'third_party/cs444/stdlib/5.0/java/lang/Cloneable.java',
    'third_party/cs444/stdlib/5.0/java/lang/Character.java',
    'third_party/cs444/stdlib/5.0/java/lang/Object.java'
]

test_dir = os.getenv('TEST_SRCDIR', '.')

def do_test(test_name, test_files):
    shutil.rmtree('output', True)
    os.mkdir('output')
    shutil.copyfile(test_dir + '/third_party/cs444/stdlib/5.0/runtime.s', 'output/runtime.s')

    joosc_args = ['joosc'] + test_files + stdlib_files
    joosc_args = [os.path.join(test_dir, a) for a in joosc_args]
    joosc_proc = subprocess.Popen(joosc_args)
    joosc_proc.wait()
    if joosc_proc.returncode != 0:
        print('\nFailed to compile {}! Ret={}.'.format(test_name, joosc_proc.returncode))
        return False

    asm_proc = subprocess.Popen(test_dir + '/asm.sh', shell=True)
    asm_proc.wait()
    if asm_proc.returncode != 0:
        print('\nFailed to assemble {}! Ret={}.'.format(test_name, asm_proc.returncode))
        return False

    prog_proc = subprocess.Popen(['./a.out'], stdout=subprocess.PIPE)
    prog_proc.wait()
    ret = prog_proc.returncode
    if ret == 123:
        print('.', end='')
        return True
    print('\nTest {} failed! Ret={}.'.format(test_name, ret))
    return False

def do_tests():
    root_dir = 'third_party/cs444/assignment_testcases/a5/'
    subdir_to_dir = {}
    dir_to_tests = {}
    for dir_name, subdirs, files in os.walk(root_dir):
        # Separate tests for root dir.
        if dir_name == root_dir:
            for f in files:
                dir_to_tests[f] = [os.path.join(root_dir, f)]
            continue

        key_dir = dir_name

        # Use ancestor test dir as key.
        if dir_name in subdir_to_dir:
            key_dir = subdir_to_dir[dir_name]
        else:
            dir_to_tests[key_dir] = []

        # Append tests.
        dir_to_tests[key_dir].extend([os.path.join(dir_name, f) for f in files])

        # Insert mappings for children to be in same test.
        for subdir in subdirs:
            subdir_to_dir[os.path.join(dir_name, subdir)] = key_dir

    tests = [(name, files) for name, files in dir_to_tests.items()]
    sorted(tests, key=lambda t: t[0])

    shard = int(os.getenv('TEST_SHARD_INDEX', 0))
    num_shards = int(os.getenv('TEST_TOTAL_SHARDS', 1))
    shard_file = os.getenv('TEST_SHARD_STATUS_FILE', None)

    if shard_file is not None:
        with open(shard_file, 'w') as sf:
            sf.write('')

    total_tests = len(tests)
    running_tests = total_tests / num_shards + 1
    num_passed = 0
    d = '.tmp_test_dir_{}'.format(shard)
    i = shard

    shutil.rmtree(d, True)
    os.mkdir(d)
    os.chdir(d)
    print("Doing {}/{} tests in directory {}".format(running_tests, total_tests, d))

    while i < total_tests:
        if do_test(*tests[i]):
            num_passed += 1
        i += num_shards

    shutil.rmtree(d, True)

    print("\nPassed {}/{} tests.".format(num_passed, running_tests))
    return num_passed == running_tests

if __name__ == '__main__':
    success = do_tests()
    if not success:
        exit(1)


