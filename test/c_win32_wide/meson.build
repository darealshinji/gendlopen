gen_hdr = custom_target('c_test.h',
    output : 'c_test.h',
    input : '../helloworld.txt',
    command : [gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@']
)

c_args = ['-DUNICODE', '-D_UNICODE']
link_args = []

if arg_syntax != 'msvc'
    c_args += '-municode'
    link_args += '-municode'
endif

e = executable('main', ['main.c', gen_hdr],
    dependencies : dl_dep,
    install : false,
    c_args : c_args,
    link_args : link_args,
    include_directories : include_directories('../00_library')
)

test('C win32 wide characters API', e, env : ld_library_path)
