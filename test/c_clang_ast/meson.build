# testing Clang AST input, generated with:
# clang -Xclang -ast-dump -DCLANG_AST ../00_library/helloworld.h > ast.txt

gen_hdr = custom_target('c_test.h',
    output : 'c_test.h',
    input : 'ast.txt',
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@',
        '-Phelloworld_hello',
        '-Shelloworld_init_argv',
		'-Shelloworld_callback',
		'-Shelloworld_buffer',
		'-Shelloworld_release'
    ]
)

e = executable('main', ['main.c', gen_hdr],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C generated from clang AST', e, env : ld_library_path)
