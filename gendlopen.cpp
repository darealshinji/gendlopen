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
#include <fstream>
#include <regex>
#include <vector>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
/* linker will need rpcrt4.lib */
#include <rpc.h>
#endif

#include "gendlopen.hpp"

/* no need to check for '\n' */
#define WHITESPACE  " \t\r\v\f"

#define static_strlen(x)  (sizeof(x)-1)


struct func {
  std::string type;
  std::string symbol;
  std::string args;
};


/* small wrapper class to enable reading input from
 * a file or STDIN using the same object */
class my_ifstream
{
private:
  bool m_stdin = false;
  std::ifstream m_ifs;

public:

  my_ifstream(const char *file) {
    if (strcmp(file, "-") == 0) {
      m_stdin = true;
    } else {
      m_ifs.open(file);
    }
  }

  ~my_ifstream() {
    if (m_ifs.is_open()) m_ifs.close();
  }

  bool is_open() { return m_stdin ? true : m_ifs.is_open(); }
  int get() { return m_stdin ? std::cin.get() : m_ifs.get(); }
  int peek() { return m_stdin ? std::cin.peek() : m_ifs.peek(); }
  void ignore() { get(); }
};


/* c_header and cxx_header must end on newline */

static const char c_header[] = {
#include "c_header.h"
  /**/, '\n', 0x00
};

static const char cxx_header[] = {
#include "cxx_header.h"
  /**/, '\n', 0x00
};

static const char lib_macros[] = {
#include "lib_macros.h"
  /**/, 0x00
};

bool gendlopen::m_first_line_printed = false;
std::string gendlopen::m_linebuf;


/* strip leading and trailing spaces */
inline void gendlopen::strip_spaces(std::string &s)
{
  while (isspace(s.back())) s.pop_back();
  while (isspace(s.front())) s.erase(0, 1);
}

bool gendlopen::valid_token(const std::string &token, const size_t line_no)
{
  for (const char &c : token) {
    if (!isalnum(c) && c != '_') {
      std::cerr << "warning: not a valid token (line " << line_no
        << " ignored): " << token << std::endl;
      return false;
    }
  }
  return true;
}

/* replace all occurences of 'from' with 'to' inside string 's' */
void gendlopen::replace_all(const char *from, const std::string &to, std::string &s)
{
  size_t from_len = strlen(from);
  size_t pos = 0;

  if (from_len == 0) return;

  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from_len, to);
    pos += to.size();
  }
}

/* read lines either using std::getline() or from a const char array */
bool gendlopen::getline_wrap(std::fstream &fs, std::string &line, const char *&p)
{
  if (!p) {
    return std::getline(fs, line) ? true : false;
  }

  std::string s;

  for ( ; *p != '\0'; p++) {
    if (*p == '\n') {
      line = s;
      p++;
      return true;
    } else {
      s.push_back(*p);
    }
  }

  line = s;

  return false;
}

/* check 's' for 'token' and replace it with 'token_new' if found */
bool gendlopen::replace_token(std::string &s, const char *token, const char *token_new,
                              const char *suffix = NULL)
{
  const size_t len = strlen(token);

  if (s.size() > len+1 && s.compare(0, len, token) == 0) {
    s.replace(0, len, token_new);
    s.pop_back(); /* remove trailing '@' */

    if (suffix) {
      s += suffix;
    }
    return true;
  }

  return false;
}

/* read prototypes from text file, comments will be ignored; however this is
 * not a full C parser, so make sure the input is formatted correctly */
bool gendlopen::read_prototypes(const char *input, std::vector<struct func> &list)
{
  enum {
    COMMENT_NONE,
    COMMENT_ASTERISK,
    COMMENT_DOUBLE_SLASH
  };
  char mode = COMMENT_NONE;
  std::vector<std::string> data;
  std::string line;
  my_ifstream fs(input);

  if (!fs.is_open()) {
    std::cerr << "error opening file for reading: " << input << std::endl;
    return false;
  }

  for (int c = fs.get(); c != EOF && c != '\0'; c = fs.get())
  {
    /* we're in comment mode:
     * skip characters and check for comment end */
    switch (mode) {
      case COMMENT_ASTERISK:
        if (c == '*' && fs.peek() == '/') {
          mode = COMMENT_NONE;
          fs.ignore();
          if (!line.empty() && line.back() != ' ') {
            line.push_back(' ');
          }
        }
        continue;
      case COMMENT_DOUBLE_SLASH:
        if (c == '\n') {
          mode = COMMENT_NONE;
        }
        continue;
    }

    switch (c) {
      /* input end */
      case ';':
        if (!line.empty()) {
          if (line.back() == ' ') line.pop_back();
          data.push_back(line);
        }
        line.clear();
        continue;

      /* possible comment begin */
      case '/':
        switch (fs.peek()) {
          case '/':
            mode = COMMENT_DOUBLE_SLASH;
            fs.ignore();
            continue;
          case '*':
            mode = COMMENT_ASTERISK;
            fs.ignore();
            continue;
        }
        break;

      /* replace whitespaces with
       * a single space character */
      case '\f':
      case '\v':
      case '\r':
      case '\n':
      case '\t':
      case ' ':
        if (!line.empty() && line.back() != ' ') {
          line.push_back(' ');
        }
        continue;

      default:
        line.push_back(c);
        break;
    }
  }

  while (line.back() == ' ') {
    line.pop_back();
  }

  if (!line.empty()) {
    data.push_back(line);
  }

  std::regex reg(
    "(.*?[\\*|\\s])"  /* type */
    "([A-Za-z0-9_]*)" /* symbol */
    "[?|\\s]*\\("
    "(.*?)\\)"        /* args */
  );

  for (const auto &e : data) {
    std::smatch m;

    if (!std::regex_match(e, m, reg) || m.size() != 4) {
      std::cerr << "error: malformed function prototype: " << e << std::endl;
      return false;
    }

    struct func f = { m[1], m[2], m[3] };

    /* cosmetics */
    if (f.type.back() == ' ') f.type.pop_back();
    if (f.args.back() == ' ') f.args.pop_back();
    if (f.args.empty()) f.args = "void";

    list.push_back(f);
  }

  return true;
}

/* save previous line to file or stdout and save the new line into our
 * line buffer; we do this because we don't know yet where a macro block
 * ends and we don't want it do end with a trailing backslash */
void gendlopen::print_line(std::fstream &fs, const std::string &line, bool is_macro)
{
  if (!m_first_line_printed) {
    m_linebuf = line;
    m_first_line_printed = true;
    return;
  }

  if (fs.is_open()) {
    fs << m_linebuf << endl(is_macro);
  } else {
    std::cout << m_linebuf << endl(is_macro);
  }

  m_linebuf = line;
}

inline const char *gendlopen::endl(bool is_macro) const {
  const char *s[2] = { "\\\n", "\\\r\n" };
  return s[m_win_linebreaks] + !is_macro;
}

/* check if string "s" was defined */
inline bool gendlopen::defined(const std::string &s)
{
  for (const auto &e : m_definitions) {
    if (s == e) return true;
  }
  return false;
}

bool gendlopen::token_is_if_or_ifnot(const std::string &s, struct cond &con, const size_t line_no)
{
  std::smatch m;
  std::regex reg("@(IF|IFNOT):([A-Za-z0-9_]*)@");

  if (!std::regex_match(s, m, reg) || m.size() != 3 || !valid_token(m[2], line_no)) {
    return false;
  }

  if (m[1] == "IFNOT") {
    con.b = !defined(m[2]);
  } else {
    con.b = defined(m[2]);
  }

  con.s_cond = s;
  con.b_cond = con.b;
  con.line_no = line_no;

  con.s_else = "@ELSE:";
  con.s_else += m[2];
  con.s_else += '@';

  con.s_endif = "@ENDIF:";
  con.s_endif += m[2];
  con.s_endif += '@';

  return true;
}

bool gendlopen::token_is_elif_or_elifnot(const std::string &s, struct cond &con, const size_t line_no)
{
  std::smatch m;
  std::regex reg("@(ELIF|ELIFNOT):([A-Za-z0-9_]*)@");

  if (!std::regex_match(s, m, reg) || m.size() != 3 || !valid_token(m[2], line_no)) {
    return false;
  }

  if (m[1] == "ELIFNOT") {
    con.b = !defined(m[2]);
  } else {
    con.b = defined(m[2]);
  }

  con.s_cond = s;
  con.line_no = line_no;

  return true;
}

/* set default library name with or without quotes */
void gendlopen::default_lib(const char *s, bool quotes)
{
  if (quotes) {
    m_default_lib = "\"";
    m_default_lib += s;
    m_default_lib += '"';
  } else {
    m_default_lib = s;
  }
}

/* pre-process simple conditionals:
 * @IF:value@
 * @IFNOT:value@
 * @ELSE:value@
 * @ENDIF:value@
 *
 * return values:
 * true: keep line
 * false: skip line
 */
bool gendlopen::preprocess(std::string &line, const size_t line_no, std::vector<struct cond> &stack)
{
  std::string copy = line;
  strip_spaces(copy);

  /* skip lines beginning or ending with @X@ */
  if (copy.compare(0, 3, "@X@") == 0 ||
      (copy.size() > 3 && copy.compare(copy.size()-3, 3, "@X@") == 0))
  {
    return false;
  }

  /* convert our own conditionals to C preprocessor
   * ones (no error checking) */
  if (m_conv_conditionals) {
    if (copy.size() > static_strlen("@IF:") &&
        copy.front() == '@' &&
        copy.back() == '@' &&
        copy.find_first_of(WHITESPACE) == std::string::npos)
    {
      if (copy == "@ELSE@") {
        line = "#else";
      } else if (copy == "@ENDIF@") {
        line = "#endif";
      } else if (
        replace_token(copy, "@IF:", "#ifdef ") ||
        replace_token(copy, "@IFNOT:", "#ifndef ") ||
        replace_token(copy, "@ELIF:", "#elif defined( ", " )") ||
        replace_token(copy, "@ELIFNOT:", "#elif !defined( ", " )") ||
        replace_token(copy, "@ELSE:", "#else //") ||
        replace_token(copy, "@ENDIF:", "#endif //"))
      {
        size_t pos;

        /* get original indentation */
        if ((pos = line.find_first_not_of(WHITESPACE)) > 1) {
          copy.insert(0, line.substr(0, pos));
        }
        line = copy;
      }
    }

    return true;
  }
  else  /* resolve conditionals */
  {
    /* is it a single token? */
    if (copy.size() > 2 &&
        copy.front() == '@' &&
        copy.back() == '@' &&
        copy.find_first_of(WHITESPACE) == std::string::npos)
    {
      /* check for @ELSE@ / @ENDIF@ / @ELIF@ */
      if (!stack.empty()) {
        struct cond con = {0};

        if (copy == stack.back().s_endif || copy == "@ENDIF@") {
          stack.pop_back();
          return false;
        }
        else if (copy == stack.back().s_else || copy == "@ELSE@")
        {
          if (stack.back().b_else) {
            std::cerr << "warning: redundant @ELSE@ at line " << line_no
              << ": " << copy << std::endl;
          }
          stack.back().b_cond = stack.back().b = !stack.back().b_cond;
          stack.back().b_else = true;
          return false;
        }
        /* @ELIF@ / @ELIFNOT@ */
        else if (token_is_elif_or_elifnot(copy, con, line_no))
        {
          if (stack.back().b_else) {
            std::cerr << "warning: @ELIF@ / @ELIFNOT@ after @ELSE@ at line "
              << line_no << ": " << copy << std::endl;
          }

          if (stack.back().b_cond == true) {
            /* conditional was already set true, so disable this part */
            stack.back().b = false;
          } else {
            /* conditional wasn't true yet, set it to the bool value
             * from the current @ELIF@ part */
            stack.back().b = con.b;
            stack.back().b_cond = con.b;
            stack.back().s_cond = con.s_cond;
            stack.back().line_no = con.line_no;
          }
          return false;
        }
      }

      /* check for new @IF@/@IFNOT@ */
      std::smatch m;
      struct cond con = {0};

      if (token_is_if_or_ifnot(copy, con, line_no)) {
        if (!stack.empty() && stack.back().b == false) {
          con.b = false;
          con.b_cond = true;
        }
        stack.push_back(con);
        return false;
      }
      else if (copy == "@ELSE@" ||
               copy == "@ENDIF@" ||
               std::regex_match(copy, m, std::regex("@(ELSE|ENDIF):([A-Za-z0-9_]*)@")))
      {
        std::cerr << "warning: line " << line_no
          << " is missing a matching @IF@/@IFNOT@ line: "
          << copy << std::endl;
      }
    }

    if (!stack.empty() && stack.back().b == false) {
      return false;
    }
  }

  return true;
}

/* parse files */
bool gendlopen::parse(const char *input, const char *output, int target)
{
  std::fstream fs, fout;
  std::string line, nameupper, headerguard, loop_block, indent;
  std::vector<struct func> list;
  std::vector<struct cond> stack;
  const char *data = NULL;
  const char *static_s = "static ";
  char rndm[9] = {0};
  bool is_loop_sequence = false;
  bool is_macro = false;
  size_t pos = std::string::npos;
  size_t line_no = 0;

  if (output) {
    fout.open(output, std::fstream::out);

    if (!fout.is_open()) {
      std::cerr << "error opening file for writing: " << output << std::endl;
      return false;
    }
  }

  /* read function prototypes from input file */

  if (!read_prototypes(input, list)) {
    std::cerr << "error reading prototype data from file: "
      << input << std::endl;
    return false;
  }

  /* don't print this info if output is set to STDOUT */

  if (fout.is_open()) {
    std::cout << list.size() << " prototypes found:\n" << std::endl;
    for (const auto &e : list) {
      std::cout << e.type << ' ' << e.symbol << '(' << e.args << ");" << std::endl;
    }
    std::cout << std::endl;
  }

  /* read template and print modified lines */

  if (m_custom_template) {
    fs.open(m_custom_template, std::fstream::in);

    if (!fs.is_open()) {
      std::cerr << "error opening template file for reading: "
        << m_custom_template << std::endl;
      return false;
    }
  } else {
    switch (target) {
      case gendlopen::trgt_c:
        data = c_header;
        break;
      case gendlopen::trgt_cpp:
        data = cxx_header;
        break;
      case gendlopen::trgt_cpp_static:
        data = cxx_header;
        add_def("_CXXSTATIC");
        break;
      default:
        std::cerr << "error: no template was specified" << std::endl;
        return false;
    }
  }

  /* uppercase name */
  for (const char *p = m_name; *p != 0; p++) {
    nameupper += toupper(*p);
  }

  /* header guard string */
  if (output) {
    headerguard = output;
#ifdef _WIN32
    pos = headerguard.find_last_of("\\/");
#else
    pos = headerguard.rfind('/');
#endif

    if (pos != std::string::npos) {
      headerguard.erase(0, pos+1);
    }

    for (size_t i=0; i < headerguard.size(); i++) {
      if (isalnum(headerguard[i])) {
        headerguard[i] = toupper(headerguard[i]);
      } else {
        headerguard[i] = '_';
      }
    }
  } else {
    headerguard = nameupper + "_H";
  }

  /* create random string */
  if (m_randomness) {
    unsigned char buf[3];
#ifdef _WIN32
    UUID uu;
    RPC_STATUS status = UuidCreate(&uu);

    if (status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY) {
      std::cerr << "error: UuidCreate() returned error code: " << status << std::endl;
      return false;
    }
    memcpy(&buf, &uu.Data1, sizeof(buf));
#else
    FILE *fp = fopen("/dev/random", "r");

    if (!fp || fread(&buf, 1, sizeof(buf), fp) != sizeof(buf)) {
      std::cerr << "error: failed to read random data from /dev/random" << std::endl;
      if (fp) fclose(fp);
      return false;
    }
#endif
    snprintf(rndm, 8, "_%02X%02X%02X", buf[0], buf[1], buf[2]);
  }

  if (!m_default_lib.empty()) {
    add_def("_DEFLIB");
  }

  /* don't use "static" keyword if saving to separate files */
  if (defined("_HEADER") || defined("_SOURCE")) {
    static_s = "";
  }

  std::string linebuf;

  /* parse template lines */
  while (getline_wrap(fs, line, data))
  {
    std::string def, def_orig;
    bool is_defined = false;
    bool front = true;

    line_no++;

    if (!preprocess(line, line_no, stack)) {
      continue;
    }

    /* use a copy to make it easier to handle
     * leading and trailing whitespaces */
    std::string copy = line;
    strip_spaces(copy);

    if (!copy.empty())
    {
      /* macro/loop blocks */
      if (copy.size() >= static_strlen("@LOOP_END@") &&
          copy.front() == '@' &&
          copy.back() == '@')
      {
        /* macro block */
        if (copy == "@MACRO_START@") {
          is_macro = true;
          indent.clear();
          continue;
        } else if (copy == "@MACRO_END@") {
          is_macro = false;
          indent.clear();
          continue;
        }
        /* loop block */
        else if (copy == "@LOOP_START@") {
          is_loop_sequence = true;
          loop_block.clear();
          continue;
        } else if (copy == "@LOOP_END@") {
          for (const auto &e : list) {
            std::string s = loop_block;
            replace_all("@TYPE@", e.type, s);
            replace_all("@SYMBOL@", e.symbol, s);
            replace_all("@ARGS@", e.args, s);
            print_line(fout, s, is_macro);
          }

          is_loop_sequence = false;
          loop_block.clear();
          continue;
        }
      }

      /* check for definitions */
      if (copy.size() > 4) {
        std::smatch m1, m2;
        std::regex reg_beg("^@(D[:!])([A-Za-z0-9_]*)@.*");
        std::regex reg_end(".*@(D[:!])([A-Za-z0-9_]*)@");

        if (std::regex_match(copy, m1, reg_beg) && m1.size() == 3 && valid_token(m1[2], line_no)) {
          is_defined = (m1[1] == "D:");
          def = m1[2];
          def_orig = "@";
          def_orig += m1[1];
          def_orig += m1[2];
          def_orig += '@';
        } else if (std::regex_match(copy, m2, reg_end) && m2.size() == 3 && valid_token(m2[2], line_no)) {
          is_defined = (m2[1] == "D:");
          def = m2[2];
          def_orig = "@";
          def_orig += m2[1];
          def_orig += m2[2];
          def_orig += '@';
          front = false;
        }

        /* found a valid token */
        if (!def.empty()) {
          if (!m_conv_conditionals) {
            bool found = defined(def);

            /* skip line based on definitions */
            if ((is_defined && !found) || (!is_defined && found)) {
              continue;
            }
          }

          if (front) {
            /* erase keyword from front */
            if ((pos = line.find(def_orig)) != std::string::npos) {
              line.erase(pos, def_orig.size());
            }
          } else {
            /* erase keyword and trailing spaces from back */
            if ((pos = line.rfind(def_orig)) != std::string::npos) {
              line.erase(pos, def_orig.size());

              if (!m_keep_whitespaces) {
                while (isspace(line.back())) line.pop_back();
              }
            }
          }

          /* convert conditionals into C preprocessor code */
          if (m_conv_conditionals) {
            std::string s = is_defined ? "#ifdef " : "#ifndef ";
            s += def + endl(false);
            line.insert(0, s);
            line += endl(false);
            line += "#endif";
          }
        }
      }
    }

    /* whitespaces line */
    if (copy.empty()) {
      if (is_loop_sequence) {
        if (!loop_block.empty()) {
          loop_block += endl(is_macro);
        }
        loop_block += indent;
      } else {
        print_line(fout, indent, is_macro);
      }
      continue;
    }

    /* simple replacements */
    if (m_conv_conditionals && m_default_lib.empty()) {
      m_default_lib = "NULL";
    }
    replace_all("@DEFAULT_LIB@", m_default_lib, line);
    replace_all("@LIB_LIBEXT@", lib_macros, line);
    replace_all("@NAME@", m_name, line);
    replace_all("@NAMEUPPER@", nameupper, line);
    replace_all("@HEADERGUARD@", headerguard, line);
    replace_all("@STATIC@", static_s, line);
    replace_all("@R@", rndm, line);

    /* update macro indentation */
    if (is_macro && (pos = line.find_first_not_of(WHITESPACE)) > 1) {
      indent = line.substr(0, pos);
    }

    /* eat up content for loop block */
    if (is_loop_sequence) {
      if (!loop_block.empty()) {
        loop_block += endl(is_macro);
      }
      loop_block += line;
      continue;
    }

    if (line.find("@TYPE@") != std::string::npos ||
        line.find("@SYMBOL@") != std::string::npos ||
        line.find("@ARGS@") != std::string::npos)
    {
      /* prototype iterations */
      for (const auto &e : list) {
        std::string s = line;
        replace_all("@TYPE@", e.type, s);
        replace_all("@SYMBOL@", e.symbol, s);
        replace_all("@ARGS@", e.args, s);
        print_line(fout, s, is_macro);
      }
    } else {
      print_line(fout, line, is_macro);
    }
  }

  /* print last line */
  print_line(fout, "", false);

  if (!stack.empty()) {
    std::cerr << "warning: missing a matching @ENDIF@ line: line "
      << stack.back().line_no << ' ' << stack.back().s_cond << std::endl;
  }

  if (fout.is_open()) {
    std::cout << "data successfully written to `" << output << "'" << std::endl;
  }

  return true;
}

