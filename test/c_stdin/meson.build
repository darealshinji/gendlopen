gen_hdr = custom_target('c_test.h',
    output : 'c_test.h',
    input : '../helloworld.txt',
    command : ['read_from_stdin.sh', host_machine.system(), gendlopen_bin, '@OUTPUT@', '@INPUT@']
)

e = executable('main', ['main.c', gen_hdr],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C generated from STDIN', e, env : ld_library_path)
