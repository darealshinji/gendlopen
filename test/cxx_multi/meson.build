gen_srcs = custom_target('cxx_test.cpp',
    output : ['cxx_test.cpp', 'cxx_test.hpp'],
    input : '../helloworld.txt',
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT1@', '-format=C++',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DBUILDING_STATIC', '-DGDO_ENABLE_AUTOLOAD'
    ]
)

e = executable('main', ['main.cpp', 'cb.cpp', 'call2.cpp', gen_srcs],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C++ multiple source files', e, env : ld_library_path)
