if host_is_win32
    # UUID
    example_uuid_win_h = custom_target('example_uuid_win.h',
        output : 'example_uuid_win.h',
        input : 'uuid_win_ast.txt',
        command : [
            gendlopen_bin, '@INPUT@',
            '-force', '-out', '@OUTPUT@',
            '-library=rpcrt4.dll',
            '-SUuidCreate', '-SUuidToStringA', '-SRpcStringFreeA'
        ]
    )

    executable('uuid', ['uuid_win.c', example_uuid_win_h],
        dependencies : dl_dep,
        install : false
    )
else
    # GTK+2
    if gtk_dep.found()
        example_gtk_h = custom_target('example_gtk.h',
            output : 'example_gtk.h',
            input : 'gtk.txt',
            command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']
        )

        executable('gtk', ['gtk.c', example_gtk_h],
            dependencies : [dl_dep, gtk_dep],
            c_args : wno_deprecated_declarations,
            install : false
        )
    endif

    # msgbox
    if x11_dep.found() and gtk_dep.found()
        gen_hdrs = []

        foreach dep : [
            ['example_msgbox_fltk.h', 'msgbox_fltk.txt'],
            ['example_msgbox_gtk.h', 'msgbox_gtk.txt'],
            ['example_msgbox_sdl.h', 'msgbox_sdl.txt'],
            ['example_msgbox_x11.h', 'msgbox_x11.txt']
        ]

            gen_hdrs += custom_target(dep[0],
                output : dep[0],
                input : dep[1],
                command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']
            )
        endforeach

        executable('msgbox', ['msgbox.c', gen_hdrs],
            dependencies : [dl_dep, gtk_dep, x11_dep],
            install : false,
            c_args : wno_deprecated_declarations,
            include_directories : include_directories('thirdparty/sdl')
        )
    endif

    if glib_dep.found()
        glib_dep2 = glib_dep.partial_dependency(compile_args : true)

        # GLib hook
        gen_hdr = custom_target('example_hook.h',
            output : 'example_hook.h',
            input : 'hook_glib.txt',
            command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']
        )

        executable('hook', ['hook.c', 'hook.h', gen_hdr],
            dependencies : [dl_dep, glib_dep2],
            install : false
        )

        # preload
        shared_module('preload', ['preload.c', 'hook.h', gen_hdr],
            name_prefix : '',
            dependencies : [dl_dep, glib_dep2],
            install : false
        )

        executable('preload_main', ['preload_main.c', 'hook.h'],
            dependencies : [glib_dep],
            install : false
        )

        # shim
        shared_library('glib-2.0', ['shim.c', 'hook.h', gen_hdr],
            dependencies : [dl_dep, glib_dep2],
            soversion : '0',
            install : false
        )

        executable('shim_main', ['shim_main.c', 'hook.h'],
            dependencies : [glib_dep],
            install : false
        )

        # libnotify
        gen_hdrs = []

        foreach dep : [
            ['example_notify.h', 'notify.txt'],
            ['example_notify_gobject.h', 'notify_gobject.txt']
        ]

            gen_hdrs += custom_target(dep[0],
                output : dep[0],
                input : dep[1],
                command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']
            )
        endforeach

        executable('notify', ['notify.c', gen_hdrs],
            dependencies : [dl_dep, glib_dep2],
            include_directories : include_directories('thirdparty'),
            install : false
        )
    endif

    # UUID
    gen_hdr = custom_target('example_uuid_nix.h',
        output : 'example_uuid_nix.h',
        input : 'uuid_nix_ast.txt',
        command : [
            gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@',
            '-library=api:1:uuid',
            '-Suuid_generate_random', '-Suuid_is_null', '-Suuid_unparse_lower'
        ]
    )

    executable('uuid', ['uuid_nix.c', gen_hdr],
        dependencies : dl_dep,
        include_directories : include_directories('thirdparty/uuid'),
        install : false
    )
endif

