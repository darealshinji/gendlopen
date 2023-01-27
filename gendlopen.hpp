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
#ifndef GENDLOPEN_HPP
#define GENDLOPEN_HPP

#include <fstream>
#include <string>
#include <vector>


class gendlopen
{
public:
  enum {
    trgt_non,
    trgt_c,
    trgt_cpp,
    trgt_cpp_static
  };

  struct cond {
    bool b;
    bool b_cond;
    bool b_else;
    size_t line_no;
    std::string s_cond;
    std::string s_else;
    std::string s_endif;
  };

private:
  std::vector<std::string> m_definitions;
  const char *m_name = NULL;
  const char *m_custom_template = NULL;
  std::string m_default_lib;
  bool m_randomness = true;
  bool m_conv_conditionals = false;
  bool m_keep_whitespaces = false;
  bool m_win_linebreaks = false;
  static bool m_first_line_printed;
  static std::string m_linebuf;

public:
  gendlopen() {}
  ~gendlopen() {}

  bool parse(const char *input, const char *output, int target);

  void name(const char *s) {m_name = s;}
  void default_lib(const char *s, bool quotes);
  void custom_template(const char *s) {m_custom_template = s;}
  void add_def(const std::string &s) {m_definitions.push_back(s);}

  void randomness(bool b) {m_randomness = b;}
  void conv_conditionals(bool b) {m_conv_conditionals = b;}
  void keep_whitespaces(bool b) {m_keep_whitespaces = b;}
  void win_linebreaks(bool b) {m_win_linebreaks = b;}

private:
  bool preprocess(std::string &line, const size_t line_no, std::vector<struct cond> &stack);
  inline bool defined(const std::string &s);
  bool token_is_if_or_ifnot(const std::string &s, struct cond &con, const size_t line_no);
  bool token_is_elif_or_elifnot(const std::string &s, struct cond &con, const size_t line_no);
  void print_line(std::fstream &fs, const std::string &line, bool is_macro);
  inline const char *endl (bool is_macro) const;

  inline static void strip_spaces(std::string &s);
  static bool valid_token(const std::string &token, const size_t line_no);
  static void replace_all(const char *from, const std::string &to, std::string &s);
  static bool getline_wrap(std::fstream &fs, std::string &line, const char *&p);
  static bool replace_token(std::string &s, const char *token, const char *token_new, const char *suffix);
  static bool read_prototypes(const char *input, std::vector<struct func> &list);
};

#endif //GENDLOPEN_HPP
