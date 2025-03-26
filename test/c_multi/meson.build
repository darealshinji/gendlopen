gen_srcs = custom_target('c_test.c',
    output : ['c_test.c', 'c_test.h'],
    input : '../helloworld.txt',
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT1@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DBUILDING_STATIC', '-DGDO_ENABLE_AUTOLOAD'
    ]
)

e = executable('main', ['main.c', 'cb.c', 'call2.c', gen_srcs],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C multiple source files', e, env : ld_library_path)
