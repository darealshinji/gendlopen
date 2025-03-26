gen_srcs = custom_target('libhelloworld_dl.c',
    output : ['libhelloworld_dl.c', 'libhelloworld_dl.h'],
    input : '../helloworld.txt',
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT0@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DBUILDING_STATIC', '-DGDO_ENABLE_AUTOLOAD'
    ]
)

lib = static_library('helloworld_dl', gen_srcs,
    include_directories : include_directories('../00_library')
)

e = executable('main', 'main.c',
    dependencies : dl_dep,
    link_with : lib,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C fake static library', e, env : ld_library_path)

