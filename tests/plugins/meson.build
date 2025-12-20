plugins = []

foreach num : ['a', 'b', 'c']
    plugins += shared_module('plugin_'+num, 'plugin_'+num+'.c',
        name_prefix : '',
        install : false)
endforeach

gen_hdr = custom_target('load_plugins.h',
    depends : plugins,
    output : 'load_plugins.h',
    input : 'interface.txt',
    command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@', '-format=plugin'])

e = executable('load_plugins', 'load_plugins.cpp', gen_hdr,
    override_options : test_overrides,
    cpp_args : test_flags,
    build_rpath : test_rpath,
    install : false)

test('C load plugins (C++ source file)', e)

