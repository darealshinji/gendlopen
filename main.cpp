/**
 * Copyright (C) 2022, 2023 djcj@gmx.de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include <string.h>

#ifdef _WIN32
# include <io.h>
# define F_OK 0
# define access _access_s
# ifndef strcasecmp
# define strcasecmp _stricmp
# endif /*strcasecmp*/
#else
#include <unistd.h>
#endif

#include "gendlopen.hpp"
#include "cmdline.h"

#define SET(X, ...) \
  header.X(__VA_ARGS__); \
  body.X(__VA_ARGS__)


static const char *argv0 = "";

inline bool file_exists(const char *path)
{
  if (access(path, F_OK) == 0) {
    std::cerr << argv0 << ": file already exists: " << path << std::endl;
    return true;
  }
  return false;
}

inline bool strcase_ends_on(const std::string &s, const char *suf)
{
  if (s.empty() || !suf || !*suf) {
    return false;
  }

  size_t suflen = strlen(suf);

  if (s.size() < suflen) {
    return false;
  }

  return (strcasecmp(s.c_str() + (s.size() - suflen), suf) == 0);
}

static bool check_argument(char *ptr, const char *arg_long, const char arg_short)
{
  if (!ptr || *ptr == 0) {
    fprintf(stderr, "%s: empty argument given for option '--%s' ('-%c')\n",
            argv0, arg_long, arg_short);
    return false;
  }
  return true;
}

static bool check_all_arguments(struct gengetopt_args_info &args)
{
#define EMPTYARG(ARG_L, ARG_S) \
  (args.ARG_L##_given && !check_argument(args.ARG_L##_arg, #ARG_L , ARG_S))

  if (EMPTYARG(input,   'i') ||
      EMPTYARG(output,  'o') ||
      EMPTYARG(target,  't') ||
      EMPTYARG(custom,  'c') ||
      EMPTYARG(name,    'n') ||
      EMPTYARG(library, 'l'))
  {
    return false;
  }

#undef EMPTYARG

  for (unsigned int i = 0; i < args.def_given; i++) {
    if (!check_argument(args.def_arg[i], "def", 'D')) {
      return false;
    }
  }

  return true;
}


int main(int argc, char **argv)
{
  static struct gengetopt_args_info args;
  gendlopen header, body;
  const char *output = NULL;
  const char *body_ext = "c";
  std::string output_body;
  int target = gendlopen::trgt_c;
  int ret = 1;

  argv0 = argv[0];

  if (argc < 2) {
    cmdline_parser_print_help();
    return 1;
  }

  if (cmdline_parser(argc, argv, &args) != 0) {
    return 1;
  }

  if (!args.input_given) {
    std::cerr << argv0 << ": '--input' ('-i') option required" << std::endl;
    goto JMP_END;
  }

  /* check if any empty argument was given */
  if (!check_all_arguments(args)) {
    goto JMP_END;
  }

  /* save output in separate header and source files */
  if (args.separate_files_given) {
    if (!args.output_given) {
      std::cerr << argv0 << ": '--output' ('-o') option required by '--separate-files' ('-s')"
        << std::endl;
      goto JMP_END;
    }
    header.add_def("_HEADER");
    body.add_def("_SOURCE");
  }

  /* set output and check if output exists */
  if (args.output_given) {
    output = args.output_arg;

    if (!args.force_given && file_exists(output)) {
      goto JMP_END;
    }

    output_body = output;

    if (strcase_ends_on(output_body, ".h")) {
      output_body.replace(output_body.size()-1, 1, body_ext);
    }
    else if (strcase_ends_on(output_body, ".hpp") ||
             strcase_ends_on(output_body, ".hxx"))
    {
      output_body.replace(output_body.size()-3, 3, body_ext);
    } else {
      output_body += '.';
      output_body += body_ext;
    }

    if (!args.force_given && file_exists(output_body.c_str())) {
      goto JMP_END;
    }
  }

  if (args.target_given) {
    if (strcasecmp(args.target_arg, "cpp") == 0) {
      target = gendlopen::trgt_cpp;
      body_ext = "cpp";
    } else if (strcasecmp(args.target_arg, "c") == 0) {
      target = gendlopen::trgt_c;
    } else if (strcasecmp(args.target_arg, "cpp-static") == 0) {
      target = gendlopen::trgt_cpp_static;
      body_ext = "cpp";
    } else {
      std::cerr << argv0 << ": unknown argument for option '--target' ('-t'):"
        << args.target_arg << std::endl;
      goto JMP_END;
    }
  }

  if (args.custom_given) {
    SET(custom_template, args.custom_arg);
  }

  if (args.library_given) {
    SET(default_lib, args.library_arg, !args.no_quote_given);
  }

  for (unsigned int i = 0; i < args.def_given; i++) {
    SET(add_def, args.def_arg[i]);
  }

  if (args.win32_given) {
    SET(add_def, "_W32");
  }

  if (args.atexit_given) {
    SET(add_def, "_ATEXIT");
  }

  SET(name, args.name_given ? args.name_arg : CMDLINE_PARSER_PACKAGE);
  SET(randomness, !args.no_random_given);
  SET(conv_conditionals, args.convert_given);
  SET(keep_whitespaces, args.keep_spaces_given);
  SET(win_linebreaks, args.win_linebreaks_given);

  if (header.parse(args.input_arg, output, target)) {
    ret = 0;
  } else {
    goto JMP_END;
  }

  if (args.separate_files_given &&
      !body.parse(args.input_arg, output_body.c_str(), target))
  {
    ret = 1;
  }

JMP_END:

  cmdline_parser_free(&args);

  return ret;
}
