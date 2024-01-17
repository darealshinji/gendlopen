# options
set(C_COMPILER_BUILD "cc" CACHE FILEPATH "build platform C compiler; needed for cross-building")


# there's a linking bug related to C++23 + regex + MinGW + static linking:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110572
set(CMAKE_CXX_STANDARD 20)


# compiler-specific flags
if(MSVC)
    add_compile_options("/W3;/DWIN32_LEAN_AND_MEAN")
    add_link_options("/subsystem:console")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /release")
else()
    # GCC
    add_compile_options("-Wall;-Wextra")
    if(WIN32)
        add_compile_options("-mconsole;-DWIN32_LEAN_AND_MEAN")
        add_link_options("-mconsole")
    endif()
endif()


# main executable

set(TEMPLATE_H ${CMAKE_BINARY_DIR}/src/template.h)
set(GEN_TEMPLATE_H ${CMAKE_BINARY_DIR}/src/gen_template_h)

add_executable(gendlopen
    args.hxx
    gendlopen.hpp
    generate.cpp
    main.cpp
    parse.cpp
    strcasecmp.h
    tokenize.cpp
    tokenize.hpp
    types.hpp
    ${TEMPLATE_H}
)

if(NOT WIN32)
    install(TARGETS gendlopen DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()


# aux executable
add_executable(gen_template_h EXCLUDE_FROM_ALL gen_template_h.c)


# generate header file

if(CMAKE_CROSSCOMPILING)
    add_custom_command(
        OUTPUT ${TEMPLATE_H}
        COMMAND "${C_COMPILER_BUILD}" ${CMAKE_SOURCE_DIR}/src/gen_template_h.c -o ${GEN_TEMPLATE_H}
        COMMAND ${GEN_TEMPLATE_H} ${CMAKE_SOURCE_DIR}/src/templates
    )
else()
    add_custom_command(
        OUTPUT ${TEMPLATE_H}
        COMMAND ${GEN_TEMPLATE_H} ${CMAKE_SOURCE_DIR}/src/templates
        DEPENDS gen_template_h
    )
endif()

include_directories("${CMAKE_BINARY_DIR}/src")
add_custom_target(do_gen_template_h DEPENDS ${TEMPLATE_H})
add_dependencies(gendlopen do_gen_template_h)

