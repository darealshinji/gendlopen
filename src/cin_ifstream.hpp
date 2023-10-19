#include <iostream>
#include <fstream>
#include <string>


/* wrapper class to enable reading input from
 * a file or std::cin using the same object */
class cin_ifstream
{
private:

    bool m_stdin = false;
    std::ifstream m_ifs;

    inline bool get_stdin(char &c) {
        return std::cin.get(c) ? true : false;
    }

    inline bool get_fstream(char &c) {
        return m_ifs.get(c) ? true : false;
    }

public:

    /* read from std::cin if input is "-" */
    cin_ifstream(const std::string &file)
    {
        if (file == "-") {
            m_stdin = true;
        } else {
            m_ifs.open(file.c_str());
        }
    }

    ~cin_ifstream() {
        close();
    }

    bool is_open() const {
        return m_stdin ? true : m_ifs.is_open();
    }

    void close() {
        if (m_ifs.is_open()) m_ifs.close();
    }

    bool get(char &c) {
        return m_stdin ? get_stdin(c) : get_fstream(c);
    }

    int peek() {
        return m_stdin ? std::cin.peek() : m_ifs.peek();
    }

    bool good() const {
        return m_stdin ? std::cin.good() : m_ifs.good();
    }

    void ignore()
    {
        if (m_stdin) {
            std::cin.ignore();
        } else {
            m_ifs.ignore();
        }
    }
};
