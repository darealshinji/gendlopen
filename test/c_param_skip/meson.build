gen_hdr = custom_target('c_test.h',
    output : 'c_test.h',
    input : '../helloworld.txt',
    command : [gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@', '-param=skip']
)

e = executable('main', ['main.c', gen_hdr],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C parameter names skipped', e, env : ld_library_path)
