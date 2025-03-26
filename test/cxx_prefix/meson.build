gen_hdr = custom_target('cxx_test.hpp',
    output : 'cxx_test.hpp',
    input : '../helloworld.txt',
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@',
        '-format=C++', '-prefix=MyPrefix']
)

e = executable('main', ['main.cpp', gen_hdr],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C++ custom symbol prefix', e, env : ld_library_path)
