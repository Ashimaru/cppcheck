/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2023 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "checkfunctions.h"
#include "errortypes.h"
#include "library.h"
#include "settings.h"
#include "standards.h"
#include "fixture.h"
#include "tokenize.h"

#include <list>
#include <sstream> // IWYU pragma: keep
#include <string>

class TestFunctions : public TestFixture {
public:
    TestFunctions() : TestFixture("TestFunctions") {}

private:
    Settings settings;

    void run() override {
        settings.severity.enable(Severity::style);
        settings.severity.enable(Severity::warning);
        settings.severity.enable(Severity::performance);
        settings.severity.enable(Severity::portability);
        settings.certainty.enable(Certainty::inconclusive);
        settings.standards.c = Standards::C11;
        settings.standards.cpp = Standards::CPP11;
        LOAD_LIB_2(settings.library, "std.cfg");
        LOAD_LIB_2(settings.library, "posix.cfg");
        settings.libraries.emplace_back("posix");

        // Prohibited functions
        TEST_CASE(prohibitedFunctions_posix);
        TEST_CASE(prohibitedFunctions_index);
        TEST_CASE(prohibitedFunctions_qt_index); // FP when using the Qt function 'index'?
        TEST_CASE(prohibitedFunctions_rindex);
        TEST_CASE(prohibitedFunctions_var); // no false positives for variables
        TEST_CASE(prohibitedFunctions_gets); // dangerous function
        TEST_CASE(prohibitedFunctions_alloca);
        TEST_CASE(prohibitedFunctions_declaredFunction); // declared function ticket #3121
        TEST_CASE(prohibitedFunctions_std_gets); // test std::gets
        TEST_CASE(prohibitedFunctions_multiple); // multiple use of obsolete functions
        TEST_CASE(prohibitedFunctions_c_declaration); // c declared function
        TEST_CASE(prohibitedFunctions_functionWithBody); // function with body
        TEST_CASE(prohibitedFunctions_crypt); // Non-reentrant function
        TEST_CASE(prohibitedFunctions_namespaceHandling);

        // Invalid function usage
        TEST_CASE(invalidFunctionUsage1);
        TEST_CASE(invalidFunctionUsageStrings);

        // Math function usage
        TEST_CASE(mathfunctionCall_fmod);
        TEST_CASE(mathfunctionCall_sqrt);
        TEST_CASE(mathfunctionCall_log);
        TEST_CASE(mathfunctionCall_acos);
        TEST_CASE(mathfunctionCall_asin);
        TEST_CASE(mathfunctionCall_pow);
        TEST_CASE(mathfunctionCall_atan2);
        TEST_CASE(mathfunctionCall_precision);

        // Ignored return value
        TEST_CASE(checkIgnoredReturnValue);
        TEST_CASE(checkIgnoredErrorCode);

        // memset..
        TEST_CASE(memsetZeroBytes);
        TEST_CASE(memsetInvalid2ndParam);

        // missing "return"
        TEST_CASE(checkMissingReturn);

        // std::move for locar variable
        TEST_CASE(returnLocalStdMove1);
        TEST_CASE(returnLocalStdMove2);
        TEST_CASE(returnLocalStdMove3);
        TEST_CASE(returnLocalStdMove4);

        TEST_CASE(returnLocalStdMove5);

        TEST_CASE(negativeMemoryAllocationSizeError); // #389

        TEST_CASE(checkLibraryMatchFunctions);

        TEST_CASE(checkUseStandardLibrary1);
        TEST_CASE(checkUseStandardLibrary2);
        TEST_CASE(checkUseStandardLibrary3);
        TEST_CASE(checkUseStandardLibrary4);
        TEST_CASE(checkUseStandardLibrary5);
        TEST_CASE(checkUseStandardLibrary6);
        TEST_CASE(checkUseStandardLibrary7);
        TEST_CASE(checkUseStandardLibrary8);
        TEST_CASE(checkUseStandardLibrary9);
        TEST_CASE(checkUseStandardLibrary10);
        TEST_CASE(checkUseStandardLibrary11);
        TEST_CASE(checkUseStandardLibrary12);
        TEST_CASE(checkUseStandardLibrary13);
        TEST_CASE(checkUseStandardLibrary14);
    }

#define check(...) check_(__FILE__, __LINE__, __VA_ARGS__)
    void check_(const char* file, int line, const char code[], const char filename[] = "test.cpp", const Settings* settings_ = nullptr) {
        // Clear the error buffer..
        errout.str("");

        if (!settings_)
            settings_ = &settings;

        // Tokenize..
        Tokenizer tokenizer(settings_, this);
        std::istringstream istr(code);
        ASSERT_LOC(tokenizer.tokenize(istr, filename), file, line);

        runChecks<CheckFunctions>(&tokenizer, settings_, this);
    }

    void prohibitedFunctions_posix() {
        check("void f()\n"
              "{\n"
              "    bsd_signal(SIGABRT, SIG_IGN);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (style) Obsolescent function 'bsd_signal' called. It is recommended to use 'sigaction' instead.\n", errout.str());

        check("int f()\n"
              "{\n"
              "    int bsd_signal(0);\n"
              "    return bsd_signal;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f()\n"
              "{\n"
              "    struct hostent *hp;\n"
              "    if(!hp = gethostbyname(\"127.0.0.1\")) {\n"
              "        exit(1);\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'gethostbyname' called. It is recommended to use 'getaddrinfo' instead.\n", errout.str());

        check("void f()\n"
              "{\n"
              "    long addr;\n"
              "    addr = inet_addr(\"127.0.0.1\");\n"
              "    if(!hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)) {\n"
              "        exit(1);\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (style) Obsolescent function 'gethostbyaddr' called. It is recommended to use 'getnameinfo' instead.\n", errout.str());

        check("void f()\n"
              "{\n"
              "    usleep( 1000 );\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (style) Obsolescent function 'usleep' called. It is recommended to use 'nanosleep' or 'setitimer' instead.\n", errout.str());
    }

    void prohibitedFunctions_index() {
        check("namespace n1 {\n"
              "    int index(){ return 1; };\n"
              "}\n"
              "int main()\n"
              "{\n"
              "    n1::index();\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("std::size_t f()\n"
              "{\n"
              "    std::size_t index(0);\n"
              "    index++;\n"
              "    return index;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int f()\n"
              "{\n"
              "    return this->index();\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f()\n"
              "{\n"
              "    int index( 0 );\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("const char f()\n"
              "{\n"
              "    const char var[6] = \"index\";\n"
              "    const char i = index(var, 0);\n"
              "    return i;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'index' called. It is recommended to use 'strchr' instead.\n",
                      errout.str());
    }

    void prohibitedFunctions_qt_index() {
        check("void TDataModel::forceRowRefresh(int row) {\n"
              "    emit dataChanged(index(row, 0), index(row, columnCount() - 1));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (style) Obsolescent function 'index' called. It is recommended to use 'strchr' instead.\n", errout.str());
    }

    void prohibitedFunctions_rindex() {
        check("void f()\n"
              "{\n"
              "    int rindex( 0 );\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f()\n"
              "{\n"
              "    const char var[7] = \"rindex\";\n"
              "    print(rindex(var, 0));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'rindex' called. It is recommended to use 'strrchr' instead.\n", errout.str());
    }


    void prohibitedFunctions_var() {
        check("class Fred {\n"
              "public:\n"
              "    Fred() : index(0) { }\n"
              "    int index;\n"
              "};");
        ASSERT_EQUALS("", errout.str());
    }

    void prohibitedFunctions_gets() {
        check("void f()\n"
              "{\n"
              "    char *x = gets(a);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());

        check("void f()\n"
              "{\n"
              "    foo(x, gets(a));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());
    }

    void prohibitedFunctions_alloca() {
        check("void f()\n"
              "{\n"
              "    char *x = alloca(10);\n"
              "}", "test.cpp");  // #4382 - there are no VLAs in C++
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'alloca' called.\n", errout.str());

        check("void f()\n"
              "{\n"
              "    char *x = alloca(10);\n"
              "}", "test.c");
        ASSERT_EQUALS("[test.c:3]: (warning) Obsolete function 'alloca' called. In C99 and later it is recommended to use a variable length array instead.\n", errout.str());

        settings.standards.c = Standards::C89;
        settings.standards.cpp = Standards::CPP03;
        check("void f()\n"
              "{\n"
              "    char *x = alloca(10);\n"
              "}", "test.cpp");  // #4382 - there are no VLAs in C++
        ASSERT_EQUALS("", errout.str());

        check("void f()\n"
              "{\n"
              "    char *x = alloca(10);\n"
              "}", "test.c"); // #7558 - no alternative to alloca in C89
        ASSERT_EQUALS("", errout.str());

        check("void f()\n"
              "{\n"
              "    char *x = alloca(10);\n"
              "}", "test.c");
        ASSERT_EQUALS("", errout.str());
        settings.standards.c = Standards::C11;
        settings.standards.cpp = Standards::CPP11;
    }

    // ticket #3121
    void prohibitedFunctions_declaredFunction() {
        check("int ftime ( int a )\n"
              "{\n"
              "    return a;\n"
              "}\n"
              "int main ()\n"
              "{\n"
              "    int b ; b = ftime ( 1 ) ;\n"
              "    return 0 ;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    // test std::gets
    void prohibitedFunctions_std_gets() {
        check("void f(char * str)\n"
              "{\n"
              "    char *x = std::gets(str);\n"
              "    char *y = gets(str);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n"
                      "[test.cpp:4]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());
    }

    // multiple use
    void prohibitedFunctions_multiple() {
        check("void f(char * str)\n"
              "{\n"
              "    char *x = std::gets(str);\n"
              "    usleep( 1000 );\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n"
                      "[test.cpp:4]: (style) Obsolescent function 'usleep' called. It is recommended to use 'nanosleep' or 'setitimer' instead.\n", errout.str());
    }

    void prohibitedFunctions_c_declaration() {
        check("char * gets ( char * c ) ;\n"
              "int main ()\n"
              "{\n"
              "    char s [ 10 ] ;\n"
              "    gets ( s ) ;\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());

        check("int getcontext(ucontext_t *ucp);\n"
              "void f (ucontext_t *ucp)\n"
              "{\n"
              "    getcontext ( ucp ) ;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (portability) Obsolescent function 'getcontext' called. Applications are recommended to be rewritten to use POSIX threads.\n", errout.str());
    }

    void prohibitedFunctions_functionWithBody() {
        check("char * gets ( char * c ) { return c; }\n"
              "int main ()\n"
              "{\n"
              "    char s [ 10 ] ;\n"
              "    gets ( s ) ;\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void prohibitedFunctions_crypt() {
        check("void f(char *pwd)\n"
              "{\n"
              "    char *cpwd;"
              "    crypt(pwd, cpwd);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function crypt() is not used.\n"
                      "[test.cpp:3]: (portability) Non reentrant function 'crypt' called. For threadsafe applications it is recommended to use the reentrant replacement function 'crypt_r'.\n", errout.str());

        check("void f()\n"
              "{\n"
              "    char *pwd = getpass(\"Password:\");"
              "    char *cpwd;"
              "    crypt(pwd, cpwd);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function crypt() is not used.\n"
                      "[test.cpp:3]: (portability) Non reentrant function 'crypt' called. For threadsafe applications it is recommended to use the reentrant replacement function 'crypt_r'.\n", errout.str());

        check("int f()\n"
              "{\n"
              "    int crypt = 0;"
              "    return crypt;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void prohibitedFunctions_namespaceHandling() {
        check("void f()\n"
              "{\n"
              "    time_t t = 0;"
              "    auto lt = std::localtime(&t);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (portability) Non reentrant function 'localtime' called. For threadsafe applications it is recommended to use the reentrant replacement function 'localtime_r'.\n", errout.str());

        // Passed as function argument
        check("void f()\n"
              "{\n"
              "    printf(\"Magic guess: %d\", getpwent());\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (portability) Non reentrant function 'getpwent' called. For threadsafe applications it is recommended to use the reentrant replacement function 'getpwent_r'.\n", errout.str());

        // Pass return value
        check("void f()\n"
              "{\n"
              "    time_t t = 0;"
              "    struct tm *foo = localtime(&t);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (portability) Non reentrant function 'localtime' called. For threadsafe applications it is recommended to use the reentrant replacement function 'localtime_r'.\n", errout.str());

        // Access via global namespace
        check("void f()\n"
              "{\n"
              "    ::getpwent();\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function getpwent() is not used.\n"
                      "[test.cpp:3]: (portability) Non reentrant function 'getpwent' called. For threadsafe applications it is recommended to use the reentrant replacement function 'getpwent_r'.\n", errout.str());

        // Be quiet on function definitions
        check("int getpwent()\n"
              "{\n"
              "    return 123;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Be quiet on other namespaces
        check("void f()\n"
              "{\n"
              "    foobar::getpwent();\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Be quiet on class member functions
        check("void f()\n"
              "{\n"
              "    foobar.getpwent();\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void invalidFunctionUsage1() {
        check("void f() { memset(a,b,sizeof(a)!=12); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        check("void f() { memset(a,b,sizeof(a)!=0); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        check("void f() { memset(a,b,!c); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        // Ticket #6990
        check("void f(bool c) { memset(a,b,c); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());
        check("void f() { memset(a,b,true); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        // Ticket #6588 (c mode)
        check("void record(char* buf, int n) {\n"
              "  memset(buf, 0, n < 255);\n"           /* KO */
              "  memset(buf, 0, n < 255 ? n : 255);\n" /* OK */
              "}", "test.c");
        ASSERT_EQUALS("[test.c:2]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        // Ticket #6588 (c++ mode)
        check("void record(char* buf, int n) {\n"
              "  memset(buf, 0, n < 255);\n"           /* KO */
              "  memset(buf, 0, n < 255 ? n : 255);\n" /* OK */
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid memset() argument nr 3. A non-boolean value is required.\n", errout.str());

        check("int boolArgZeroIsInvalidButOneIsValid(int a, int param) {\n"
              "  return div(a, param > 0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid div() argument nr 2. The value is 0 or 1 (boolean) but the valid values are ':-1,1:'.\n", errout.str());

        check("void boolArgZeroIsValidButOneIsInvalid(int param) {\n"
              "  strtol(a, b, param > 0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid strtol() argument nr 3. The value is 0 or 1 (boolean) but the valid values are '0,2:36'.\n", errout.str());

        check("void f() { strtol(a,b,1); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strtol() argument nr 3. The value is 1 but the valid values are '0,2:36'.\n", errout.str());

        check("void f() { strtol(a,b,10); }");
        ASSERT_EQUALS("", errout.str());

        check("void f(std::vector<int>& v) {\n" //  #10754
              "    int N = -1;\n"
              "    for (long i = 0; i < g(); i++)\n"
              "        N = h(N);\n"
              "    v.resize(N);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:5]: (warning) Invalid v.resize() argument nr 1. The value is -1 but the valid values are '0:'.\n", errout.str());

        check("void f(std::vector<int>& v, int N) {\n"
              "    if (N < -1)\n"
              "        return;\n"
              "    v.resize(N);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:2] -> [test.cpp:4]: (warning) Either the condition 'N<-1' is redundant or v.resize() argument nr 1 can have invalid value. The value is -1 but the valid values are '0:'.\n",
                      errout.str());

        check("void f(std::vector<int>& v, int N) {\n"
              "    if (N == -1) {}\n"
              "    v.resize(N);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:2] -> [test.cpp:3]: (warning) Either the condition 'N==-1' is redundant or v.resize() argument nr 1 can have invalid value. The value is -1 but the valid values are '0:'.\n",
                      errout.str());

        check("void f(std::vector<int>& v, int N, bool b) {\n"
              "    if (b)\n"
              "        N = -1;\n"
              "    v.resize(N);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (warning) Invalid v.resize() argument nr 1. The value is -1 but the valid values are '0:'.\n",
                      errout.str());

        check("void f(std::vector<int>& v) {\n"
              "    int N = -1;\n"
              "    v.resize(N);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid v.resize() argument nr 1. The value is -1 but the valid values are '0:'.\n",
                      errout.str());
    }

    void invalidFunctionUsageStrings() {
        check("size_t f() { char x = 'x'; return strlen(&x); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("size_t f() { return strlen(&x); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f(char x) { return strlen(&x); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("size_t f() { char x = '\\0'; return strlen(&x); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() {\n"
              "  char x;\n"
              "  if (y)\n"
              "    x = '\\0';\n"
              "  else\n"
              "    x = 'a';\n"
              "  return strlen(&x);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:7]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("int f() { char x = '\\0'; return strcmp(\"Hello world\", &x); }");
        ASSERT_EQUALS("", errout.str());

        check("int f() { char x = 'x'; return strcmp(\"Hello world\", &x); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strcmp() argument nr 2. A nul-terminated string is required.\n", errout.str());

        check("size_t f(char x) { char * y = &x; return strlen(y); }");
        TODO_ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", "", errout.str());

        check("size_t f(char x) { char * y = &x; char *z = y; return strlen(z); }");
        TODO_ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", "", errout.str());

        check("size_t f() { char x = 'x'; char * y = &x; char *z = y; return strlen(z); }");
        TODO_ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", "", errout.str());

        check("size_t f() { char x = '\\0'; char * y = &x; char *z = y; return strlen(z); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() { char x[] = \"Hello world\"; return strlen(x); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f(char x[]) { return strlen(x); }");
        ASSERT_EQUALS("", errout.str());

        check("int f(char x, char y) { return strcmp(&x, &y); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strcmp() argument nr 1. A nul-terminated string is required.\n"
                      "[test.cpp:1]: (error) Invalid strcmp() argument nr 2. A nul-terminated string is required.\n", errout.str());

        check("size_t f() { char x[] = \"Hello world\"; return strlen(&x[0]); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() { char* x = \"Hello world\"; return strlen(&x[0]); }");
        ASSERT_EQUALS("", errout.str());

        check("struct S {\n"
              "  char x;\n"
              "};\n"
              "size_t f() {\n"
              "  S s1 = {0};\n"
              "  S s2;\n;"
              "  s2.x = 'x';\n"
              "  size_t l1 = strlen(&s1.x);\n"
              "  size_t l2 = strlen(&s2.x);\n"
              "  return l1 + l2;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:8]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n"
                      "[test.cpp:9]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("const char x = 'x'; size_t f() { return strlen(&x); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("struct someStruct {\n"
              "    union {\n"
              "        struct {\n"
              "            uint16_t nr;\n"
              "            uint8_t d[40];\n"
              "        } data;\n"
              "        char buf[42];\n"
              "    } x;\n"
              "};\n"
              "int f(struct someStruct * const tp, const int k)\n"
              "{\n"
              "    return strcmp(&tp->x.buf[k], \"needle\");\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("struct someStruct {\n"
              "    char buf[42];\n"
              "};\n"
              "int f(struct someStruct * const tp, const int k)\n"
              "{\n"
              "    return strcmp(&tp->buf[k], \"needle\");\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("const char x = 'x'; size_t f() { char y = x; return strlen(&y); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("const char x = '\\0'; size_t f() { return strlen(&x); }");
        ASSERT_EQUALS("", errout.str());

        check("const char x = '\\0'; size_t f() { char y = x; return strlen(&y); }");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() {\n"
              "  char * a = \"Hello world\";\n"
              "  char ** b = &a;\n"
              "  return strlen(&b[0][0]);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() {\n"
              "  char ca[] = \"asdf\";\n"
              "  return strlen((char*) &ca);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #5225
        check("int main(void)\n"
              "{\n"
              "  char str[80] = \"hello worl\";\n"
              "  char d = 'd';\n"
              "  strcat(str, &d);\n"
              "  puts(str);\n"
              "  return 0;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (error) Invalid strcat() argument nr 2. A nul-terminated string is required.\n", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[1] = { \'x\' };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid fopen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[2] = { \'x\', \'y\' };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid fopen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[3] = { \'x\', \'y\' ,\'\\0\' };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[3] = { \'x\', \'\\0\' ,\'y\' };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[3] = { \'x\', \'y\' };\n" // implicit '\0' added at the end
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[] = { \'\\0\' };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[] = { \'0\' + 42 };\n" // no size is explicitly defined, no implicit '\0' is added
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid fopen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[] = { \'0\' + 42, \'x\' };\n" // no size is explicitly defined, no implicit '\0' is added
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid fopen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[2] = { \'0\' + 42 };\n" // implicitly '\0' added at the end because size is set to 2
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("FILE* f(void) {\n"
              "  const char fileName[] = { };\n"
              "  return fopen(fileName, \"r\"); \n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void scanMetaTypes()\n" // don't crash
              "{\n"
              "    QVector<int> metaTypes;\n"
              "    for (int mtId = 0; mtId <= QMetaType::User; ++mtId) {\n"
              "        const auto name = QMetaType::typeName(mtId);\n"
              "        if (strstr(name, \"GammaRay::\") != name)\n"
              "            metaTypes.push_back(mtId);\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int f() {\n"
              "    const char c[3] = \"abc\";\n"
              "    return strlen(c);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid strlen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("int f() {\n"
              "    const wchar_t c[3] = L\"abc\";\n"
              "    return wcslen(c);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid wcslen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("void f(char* dest) {\n"
              "    char if_name[(IF_NAMESIZE > 21 ? IF_NAMESIZE : 21) + 1] = \"%\";\n"
              "    strcat(dest, if_name);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("int f() {\n"
              "    const char c[3] = \"ab\\0\";\n"
              "    return strlen(c);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(int n) {\n" // #11179
              "    char s[8] = \"        \";\n"
              "    n = (n + 1) % 100;\n"
              "    sprintf(s, \"lwip%02d\", n);\n"
              "    s[strlen(s)] = ' ';\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("size_t f() { wchar_t x = L'x'; return wcslen(&x); }");
        ASSERT_EQUALS("[test.cpp:1]: (error) Invalid wcslen() argument nr 1. A nul-terminated string is required.\n", errout.str());

        check("void f() { char a[10] = \"1234567890\"; puts(a); }", "test.c"); // #1770
        ASSERT_EQUALS("[test.c:1]: (error) Invalid puts() argument nr 1. A nul-terminated string is required.\n", errout.str());
    }

    void mathfunctionCall_sqrt() {
        // sqrt, sqrtf, sqrtl
        check("void foo()\n"
              "{\n"
              "    std::cout <<  sqrt(-1) << std::endl;\n"
              "    std::cout <<  sqrtf(-1) << std::endl;\n"
              "    std::cout <<  sqrtl(-1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid sqrt() argument nr 1. The value is -1 but the valid values are '0.0:'.\n"
                      "[test.cpp:4]: (error) Invalid sqrtf() argument nr 1. The value is -1 but the valid values are '0.0:'.\n"
                      "[test.cpp:5]: (error) Invalid sqrtl() argument nr 1. The value is -1 but the valid values are '0.0:'.\n", errout.str());

        // implementation-defined behaviour for "finite values of x<0" only:
        check("void foo()\n"
              "{\n"
              "    std::cout <<  sqrt(-0.) << std::endl;\n"
              "    std::cout <<  sqrtf(-0.) << std::endl;\n"
              "    std::cout <<  sqrtl(-0.) << std::endl;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  sqrt(1) << std::endl;\n"
              "    std::cout <<  sqrtf(1) << std::endl;\n"
              "    std::cout <<  sqrtl(1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void mathfunctionCall_log() {
        // log,log10,logf,logl,log10f,log10l,log2,log2f,log2l,log1p,log1pf,log1pl
        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(-2) << std::endl;\n"
              "    std::cout <<  logf(-2) << std::endl;\n"
              "    std::cout <<  logl(-2) << std::endl;\n"
              "    std::cout <<  log10(-2) << std::endl;\n"
              "    std::cout <<  log10f(-2) << std::endl;\n"
              "    std::cout <<  log10l(-2) << std::endl;\n"
              "    std::cout <<  log2(-2) << std::endl;\n"
              "    std::cout <<  log2f(-2) << std::endl;\n"
              "    std::cout <<  log2l(-2) << std::endl;\n"
              "    std::cout <<  log1p(-3) << std::endl;\n"
              "    std::cout <<  log1pf(-3) << std::endl;\n"
              "    std::cout <<  log1pl(-3) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid log() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:4]: (error) Invalid logf() argument nr 1. The value is -2 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:5]: (error) Invalid logl() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:6]: (error) Invalid log10() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:7]: (error) Invalid log10f() argument nr 1. The value is -2 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:8]: (error) Invalid log10l() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:9]: (error) Invalid log2() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:10]: (error) Invalid log2f() argument nr 1. The value is -2 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:11]: (error) Invalid log2l() argument nr 1. The value is -2 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:3]: (warning) Passing value -2 to log() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing value -2 to logf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing value -2 to logl() leads to implementation-defined result.\n"
                      "[test.cpp:6]: (warning) Passing value -2 to log10() leads to implementation-defined result.\n"
                      "[test.cpp:7]: (warning) Passing value -2 to log10f() leads to implementation-defined result.\n"
                      "[test.cpp:8]: (warning) Passing value -2 to log10l() leads to implementation-defined result.\n"
                      "[test.cpp:9]: (warning) Passing value -2 to log2() leads to implementation-defined result.\n"
                      "[test.cpp:10]: (warning) Passing value -2 to log2f() leads to implementation-defined result.\n"
                      "[test.cpp:11]: (warning) Passing value -2 to log2l() leads to implementation-defined result.\n"
                      "[test.cpp:12]: (warning) Passing value -3 to log1p() leads to implementation-defined result.\n"
                      "[test.cpp:13]: (warning) Passing value -3 to log1pf() leads to implementation-defined result.\n"
                      "[test.cpp:14]: (warning) Passing value -3 to log1pl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(-1) << std::endl;\n"
              "    std::cout <<  logf(-1) << std::endl;\n"
              "    std::cout <<  logl(-1) << std::endl;\n"
              "    std::cout <<  log10(-1) << std::endl;\n"
              "    std::cout <<  log10f(-1) << std::endl;\n"
              "    std::cout <<  log10l(-1) << std::endl;\n"
              "    std::cout <<  log2(-1) << std::endl;\n"
              "    std::cout <<  log2f(-1) << std::endl;\n"
              "    std::cout <<  log2l(-1) << std::endl;\n"
              "    std::cout <<  log1p(-2) << std::endl;\n"
              "    std::cout <<  log1pf(-2) << std::endl;\n"
              "    std::cout <<  log1pl(-2) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid log() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:4]: (error) Invalid logf() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:5]: (error) Invalid logl() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:6]: (error) Invalid log10() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:7]: (error) Invalid log10f() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:8]: (error) Invalid log10l() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:9]: (error) Invalid log2() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:10]: (error) Invalid log2f() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:11]: (error) Invalid log2l() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:3]: (warning) Passing value -1 to log() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing value -1 to logf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing value -1 to logl() leads to implementation-defined result.\n"
                      "[test.cpp:6]: (warning) Passing value -1 to log10() leads to implementation-defined result.\n"
                      "[test.cpp:7]: (warning) Passing value -1 to log10f() leads to implementation-defined result.\n"
                      "[test.cpp:8]: (warning) Passing value -1 to log10l() leads to implementation-defined result.\n"
                      "[test.cpp:9]: (warning) Passing value -1 to log2() leads to implementation-defined result.\n"
                      "[test.cpp:10]: (warning) Passing value -1 to log2f() leads to implementation-defined result.\n"
                      "[test.cpp:11]: (warning) Passing value -1 to log2l() leads to implementation-defined result.\n"
                      "[test.cpp:12]: (warning) Passing value -2 to log1p() leads to implementation-defined result.\n"
                      "[test.cpp:13]: (warning) Passing value -2 to log1pf() leads to implementation-defined result.\n"
                      "[test.cpp:14]: (warning) Passing value -2 to log1pl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(-1.0) << std::endl;\n"
              "    std::cout <<  logf(-1.0) << std::endl;\n"
              "    std::cout <<  logl(-1.0) << std::endl;\n"
              "    std::cout <<  log10(-1.0) << std::endl;\n"
              "    std::cout <<  log10f(-1.0) << std::endl;\n"
              "    std::cout <<  log10l(-1.0) << std::endl;\n"
              "    std::cout <<  log2(-1.0) << std::endl;\n"
              "    std::cout <<  log2f(-1.0) << std::endl;\n"
              "    std::cout <<  log2l(-1.0) << std::endl;\n"
              "    std::cout <<  log1p(-2.0) << std::endl;\n"
              "    std::cout <<  log1pf(-2.0) << std::endl;\n"
              "    std::cout <<  log1pl(-2.0) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid log() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:4]: (error) Invalid logf() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:5]: (error) Invalid logl() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:6]: (error) Invalid log10() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:7]: (error) Invalid log10f() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:8]: (error) Invalid log10l() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:9]: (error) Invalid log2() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:10]: (error) Invalid log2f() argument nr 1. The value is -1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:11]: (error) Invalid log2l() argument nr 1. The value is -1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:3]: (warning) Passing value -1.0 to log() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing value -1.0 to logf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing value -1.0 to logl() leads to implementation-defined result.\n"
                      "[test.cpp:6]: (warning) Passing value -1.0 to log10() leads to implementation-defined result.\n"
                      "[test.cpp:7]: (warning) Passing value -1.0 to log10f() leads to implementation-defined result.\n"
                      "[test.cpp:8]: (warning) Passing value -1.0 to log10l() leads to implementation-defined result.\n"
                      "[test.cpp:9]: (warning) Passing value -1.0 to log2() leads to implementation-defined result.\n"
                      "[test.cpp:10]: (warning) Passing value -1.0 to log2f() leads to implementation-defined result.\n"
                      "[test.cpp:11]: (warning) Passing value -1.0 to log2l() leads to implementation-defined result.\n"
                      "[test.cpp:12]: (warning) Passing value -2.0 to log1p() leads to implementation-defined result.\n"
                      "[test.cpp:13]: (warning) Passing value -2.0 to log1pf() leads to implementation-defined result.\n"
                      "[test.cpp:14]: (warning) Passing value -2.0 to log1pl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(-0.1) << std::endl;\n"
              "    std::cout <<  logf(-0.1) << std::endl;\n"
              "    std::cout <<  logl(-0.1) << std::endl;\n"
              "    std::cout <<  log10(-0.1) << std::endl;\n"
              "    std::cout <<  log10f(-0.1) << std::endl;\n"
              "    std::cout <<  log10l(-0.1) << std::endl;\n"
              "    std::cout <<  log2(-0.1) << std::endl;\n"
              "    std::cout <<  log2f(-0.1) << std::endl;\n"
              "    std::cout <<  log2l(-0.1) << std::endl;\n"
              "    std::cout <<  log1p(-1.1) << std::endl;\n"
              "    std::cout <<  log1pf(-1.1) << std::endl;\n"
              "    std::cout <<  log1pl(-1.1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid log() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:4]: (error) Invalid logf() argument nr 1. The value is -0.1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:5]: (error) Invalid logl() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:6]: (error) Invalid log10() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:7]: (error) Invalid log10f() argument nr 1. The value is -0.1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:8]: (error) Invalid log10l() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:9]: (error) Invalid log2() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:10]: (error) Invalid log2f() argument nr 1. The value is -0.1 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:11]: (error) Invalid log2l() argument nr 1. The value is -0.1 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:3]: (warning) Passing value -0.1 to log() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing value -0.1 to logf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing value -0.1 to logl() leads to implementation-defined result.\n"
                      "[test.cpp:6]: (warning) Passing value -0.1 to log10() leads to implementation-defined result.\n"
                      "[test.cpp:7]: (warning) Passing value -0.1 to log10f() leads to implementation-defined result.\n"
                      "[test.cpp:8]: (warning) Passing value -0.1 to log10l() leads to implementation-defined result.\n"
                      "[test.cpp:9]: (warning) Passing value -0.1 to log2() leads to implementation-defined result.\n"
                      "[test.cpp:10]: (warning) Passing value -0.1 to log2f() leads to implementation-defined result.\n"
                      "[test.cpp:11]: (warning) Passing value -0.1 to log2l() leads to implementation-defined result.\n"
                      "[test.cpp:12]: (warning) Passing value -1.1 to log1p() leads to implementation-defined result.\n"
                      "[test.cpp:13]: (warning) Passing value -1.1 to log1pf() leads to implementation-defined result.\n"
                      "[test.cpp:14]: (warning) Passing value -1.1 to log1pl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(0) << std::endl;\n"
              "    std::cout <<  logf(0.) << std::endl;\n"
              "    std::cout <<  logl(0.0) << std::endl;\n"
              "    std::cout <<  log10(0.0) << std::endl;\n"
              "    std::cout <<  log10f(0) << std::endl;\n"
              "    std::cout <<  log10l(0.) << std::endl;\n"
              "    std::cout <<  log2(0.) << std::endl;\n"
              "    std::cout <<  log2f(0.0) << std::endl;\n"
              "    std::cout <<  log2l(0) << std::endl;\n"
              "    std::cout <<  log1p(-1.) << std::endl;\n"
              "    std::cout <<  log1pf(-1.0) << std::endl;\n"
              "    std::cout <<  log1pl(-1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid log() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:4]: (error) Invalid logf() argument nr 1. The value is 0 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:5]: (error) Invalid logl() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:6]: (error) Invalid log10() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:7]: (error) Invalid log10f() argument nr 1. The value is 0 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:8]: (error) Invalid log10l() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:9]: (error) Invalid log2() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:10]: (error) Invalid log2f() argument nr 1. The value is 0 but the valid values are '1.4013e-45:'.\n"
                      "[test.cpp:11]: (error) Invalid log2l() argument nr 1. The value is 0 but the valid values are '4.94066e-324:'.\n"
                      "[test.cpp:3]: (warning) Passing value 0 to log() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing value 0. to logf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing value 0.0 to logl() leads to implementation-defined result.\n"
                      "[test.cpp:6]: (warning) Passing value 0.0 to log10() leads to implementation-defined result.\n"
                      "[test.cpp:7]: (warning) Passing value 0 to log10f() leads to implementation-defined result.\n"
                      "[test.cpp:8]: (warning) Passing value 0. to log10l() leads to implementation-defined result.\n"
                      "[test.cpp:9]: (warning) Passing value 0. to log2() leads to implementation-defined result.\n"
                      "[test.cpp:10]: (warning) Passing value 0.0 to log2f() leads to implementation-defined result.\n"
                      "[test.cpp:11]: (warning) Passing value 0 to log2l() leads to implementation-defined result.\n"
                      "[test.cpp:12]: (warning) Passing value -1. to log1p() leads to implementation-defined result.\n"
                      "[test.cpp:13]: (warning) Passing value -1.0 to log1pf() leads to implementation-defined result.\n"
                      "[test.cpp:14]: (warning) Passing value -1 to log1pl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  log(1E-3)         << std::endl;\n"
              "    std::cout <<  logf(1E-3)        << std::endl;\n"
              "    std::cout <<  logl(1E-3)        << std::endl;\n"
              "    std::cout <<  log10(1E-3)       << std::endl;\n"
              "    std::cout <<  log10f(1E-3)      << std::endl;\n"
              "    std::cout <<  log10l(1E-3)      << std::endl;\n"
              "    std::cout <<  log2(1E-3)        << std::endl;\n"
              "    std::cout <<  log2f(1E-3)       << std::endl;\n"
              "    std::cout <<  log2l(1E-3)       << std::endl;\n"
              "    std::cout <<  log1p(-1+1E-3)    << std::endl;\n"
              "    std::cout <<  log1pf(-1+1E-3)   << std::endl;\n"
              "    std::cout <<  log1pl(-1+1E-3)   << std::endl;\n"
              "    std::cout <<  log(1.0E-3)       << std::endl;\n"
              "    std::cout <<  logf(1.0E-3)      << std::endl;\n"
              "    std::cout <<  logl(1.0E-3)      << std::endl;\n"
              "    std::cout <<  log10(1.0E-3)     << std::endl;\n"
              "    std::cout <<  log10f(1.0E-3)    << std::endl;\n"
              "    std::cout <<  log10l(1.0E-3)    << std::endl;\n"
              "    std::cout <<  log2(1.0E-3)      << std::endl;\n"
              "    std::cout <<  log2f(1.0E-3)     << std::endl;\n"
              "    std::cout <<  log2l(1.0E-3)     << std::endl;\n"
              "    std::cout <<  log1p(-1+1.0E-3)  << std::endl;\n"
              "    std::cout <<  log1pf(-1+1.0E-3) << std::endl;\n"
              "    std::cout <<  log1pl(-1+1.0E-3) << std::endl;\n"
              "    std::cout <<  log(1.0E+3)       << std::endl;\n"
              "    std::cout <<  logf(1.0E+3)      << std::endl;\n"
              "    std::cout <<  logl(1.0E+3)      << std::endl;\n"
              "    std::cout <<  log10(1.0E+3)     << std::endl;\n"
              "    std::cout <<  log10f(1.0E+3)    << std::endl;\n"
              "    std::cout <<  log10l(1.0E+3)    << std::endl;\n"
              "    std::cout <<  log2(1.0E+3)      << std::endl;\n"
              "    std::cout <<  log2f(1.0E+3)     << std::endl;\n"
              "    std::cout <<  log2l(1.0E+3)     << std::endl;\n"
              "    std::cout <<  log1p(1.0E+3)     << std::endl;\n"
              "    std::cout <<  log1pf(1.0E+3)    << std::endl;\n"
              "    std::cout <<  log1pl(1.0E+3)    << std::endl;\n"
              "    std::cout <<  log(2.0)          << std::endl;\n"
              "    std::cout <<  logf(2.0)         << std::endl;\n"
              "    std::cout <<  logf(2.0f)        << std::endl;\n"
              "    std::cout <<  log10(2.0)        << std::endl;\n"
              "    std::cout <<  log10f(2.0)       << std::endl;\n"
              "    std::cout <<  log10f(2.0f)      << std::endl;\n"
              "    std::cout <<  log2(2.0)         << std::endl;\n"
              "    std::cout <<  log2f(2.0)        << std::endl;\n"
              "    std::cout <<  log2f(2.0f)       << std::endl;\n"
              "    std::cout <<  log1p(2.0)        << std::endl;\n"
              "    std::cout <<  log1pf(2.0)       << std::endl;\n"
              "    std::cout <<  log1pf(2.0f)      << std::endl;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::string *log(0);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #3473 - no warning if "log" is a variable
        check("Fred::Fred() : log(0) { }");
        ASSERT_EQUALS("", errout.str());

        // #5748
        check("void f() { foo.log(0); }");
        ASSERT_EQUALS("", errout.str());
    }

    void mathfunctionCall_acos() {
        // acos, acosf, acosl
        check("void foo()\n"
              "{\n"
              " return acos(-1)     \n"
              "    + acos(0.1)      \n"
              "    + acos(0.0001)   \n"
              "    + acos(0.01)     \n"
              "    + acos(1.0E-1)   \n"
              "    + acos(-1.0E-1)  \n"
              "    + acos(+1.0E-1)  \n"
              "    + acos(0.1E-1)   \n"
              "    + acos(+0.1E-1)  \n"
              "    + acos(-0.1E-1)  \n"
              "    + acosf(-1)      \n"
              "    + acosf(0.1)     \n"
              "    + acosf(0.0001)  \n"
              "    + acosf(0.01)    \n"
              "    + acosf(1.0E-1)  \n"
              "    + acosf(-1.0E-1) \n"
              "    + acosf(+1.0E-1) \n"
              "    + acosf(0.1E-1)  \n"
              "    + acosf(+0.1E-1) \n"
              "    + acosf(-0.1E-1) \n"
              "    + acosl(-1)      \n"
              "    + acosl(0.1)     \n"
              "    + acosl(0.0001)  \n"
              "    + acosl(0.01)    \n"
              "    + acosl(1.0E-1)  \n"
              "    + acosl(-1.0E-1) \n"
              "    + acosl(+1.0E-1) \n"
              "    + acosl(0.1E-1)  \n"
              "    + acosl(+0.1E-1) \n"
              "    + acosl(-0.1E-1);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  acos(1.1) << std::endl;\n"
              "    std::cout <<  acosf(1.1) << std::endl;\n"
              "    std::cout <<  acosl(1.1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid acos() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:4]: (error) Invalid acosf() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:5]: (error) Invalid acosl() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  acos(-1.1) << std::endl;\n"
              "    std::cout <<  acosf(-1.1) << std::endl;\n"
              "    std::cout <<  acosl(-1.1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid acos() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:4]: (error) Invalid acosf() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:5]: (error) Invalid acosl() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n", errout.str());
    }

    void mathfunctionCall_asin() {
        // asin, asinf, asinl
        check("void foo()\n"
              "{\n"
              " return asin(1)      \n"
              "    + asin(-1)       \n"
              "    + asin(0.1)      \n"
              "    + asin(0.0001)   \n"
              "    + asin(0.01)     \n"
              "    + asin(1.0E-1)   \n"
              "    + asin(-1.0E-1)  \n"
              "    + asin(+1.0E-1)  \n"
              "    + asin(0.1E-1)   \n"
              "    + asin(+0.1E-1)  \n"
              "    + asin(-0.1E-1)  \n"
              "    + asinf(1)       \n"
              "    + asinf(-1)      \n"
              "    + asinf(0.1)     \n"
              "    + asinf(0.0001)  \n"
              "    + asinf(0.01)    \n"
              "    + asinf(1.0E-1)  \n"
              "    + asinf(-1.0E-1) \n"
              "    + asinf(+1.0E-1) \n"
              "    + asinf(0.1E-1)  \n"
              "    + asinf(+0.1E-1) \n"
              "    + asinf(-0.1E-1) \n"
              "    + asinl(1)       \n"
              "    + asinl(-1)      \n"
              "    + asinl(0.1)     \n"
              "    + asinl(0.0001)  \n"
              "    + asinl(0.01)    \n"
              "    + asinl(1.0E-1)  \n"
              "    + asinl(-1.0E-1) \n"
              "    + asinl(+1.0E-1) \n"
              "    + asinl(0.1E-1)  \n"
              "    + asinl(+0.1E-1) \n"
              "    + asinl(-0.1E-1);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  asin(1.1) << std::endl;\n"
              "    std::cout <<  asinf(1.1) << std::endl;\n"
              "    std::cout <<  asinl(1.1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid asin() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:4]: (error) Invalid asinf() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:5]: (error) Invalid asinl() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  asin(-1.1) << std::endl;\n"
              "    std::cout <<  asinf(-1.1) << std::endl;\n"
              "    std::cout <<  asinl(-1.1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid asin() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:4]: (error) Invalid asinf() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n"
                      "[test.cpp:5]: (error) Invalid asinl() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n", errout.str());
    }

    void mathfunctionCall_pow() {
        // pow, powf, powl
        check("void foo()\n"
              "{\n"
              "    std::cout <<  pow(0,-10) << std::endl;\n"
              "    std::cout <<  powf(0,-10) << std::endl;\n"
              "    std::cout <<  powl(0,-10) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Passing values 0 and -10 to pow() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing values 0 and -10 to powf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing values 0 and -10 to powl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  pow(0,10) << std::endl;\n"
              "    std::cout <<  powf(0,10) << std::endl;\n"
              "    std::cout <<  powl(0,10) << std::endl;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void mathfunctionCall_atan2() {
        // atan2
        check("void foo()\n"
              "{\n"
              "    std::cout <<  atan2(1,1)         ;\n"
              "    std::cout <<  atan2(-1,-1)       ;\n"
              "    std::cout <<  atan2(0.1,1)       ;\n"
              "    std::cout <<  atan2(0.0001,100)  ;\n"
              "    std::cout <<  atan2(0.0,1e-1)    ;\n"
              "    std::cout <<  atan2(1.0E-1,-3)   ;\n"
              "    std::cout <<  atan2(-1.0E-1,+2)  ;\n"
              "    std::cout <<  atan2(+1.0E-1,0)   ;\n"
              "    std::cout <<  atan2(0.1E-1,3)    ;\n"
              "    std::cout <<  atan2(+0.1E-1,1)   ;\n"
              "    std::cout <<  atan2(-0.1E-1,8)   ;\n"
              "    std::cout <<  atan2f(1,1)        ;\n"
              "    std::cout <<  atan2f(-1,-1)      ;\n"
              "    std::cout <<  atan2f(0.1,1)      ;\n"
              "    std::cout <<  atan2f(0.0001,100) ;\n"
              "    std::cout <<  atan2f(0.0,1e-1)   ;\n"
              "    std::cout <<  atan2f(1.0E-1,-3)  ;\n"
              "    std::cout <<  atan2f(-1.0E-1,+2) ;\n"
              "    std::cout <<  atan2f(+1.0E-1,0)  ;\n"
              "    std::cout <<  atan2f(0.1E-1,3)   ;\n"
              "    std::cout <<  atan2f(+0.1E-1,1)  ;\n"
              "    std::cout <<  atan2f(-0.1E-1,8)  ;\n"
              "    std::cout <<  atan2l(1,1)        ;\n"
              "    std::cout <<  atan2l(-1,-1)      ;\n"
              "    std::cout <<  atan2l(0.1,1)      ;\n"
              "    std::cout <<  atan2l(0.0001,100) ;\n"
              "    std::cout <<  atan2l(0.0,1e-1)   ;\n"
              "    std::cout <<  atan2l(1.0E-1,-3)  ;\n"
              "    std::cout <<  atan2l(-1.0E-1,+2) ;\n"
              "    std::cout <<  atan2l(+1.0E-1,0)  ;\n"
              "    std::cout <<  atan2l(0.1E-1,3)   ;\n"
              "    std::cout <<  atan2l(+0.1E-1,1)  ;\n"
              "    std::cout <<  atan2l(-0.1E-1,8)  ;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  atan2(0,0) << std::endl;\n"
              "    std::cout <<  atan2f(0,0) << std::endl;\n"
              "    std::cout <<  atan2l(0,0) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Passing values 0 and 0 to atan2() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing values 0 and 0 to atan2f() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing values 0 and 0 to atan2l() leads to implementation-defined result.\n", errout.str());
    }

    void mathfunctionCall_fmod() {
        // fmod, fmodl, fmodf
        check("void foo()\n"
              "{\n"
              "    std::cout <<  fmod(1.0,0) << std::endl;\n"
              "    std::cout <<  fmodf(1.0,0) << std::endl;\n"
              "    std::cout <<  fmodl(1.0,0) << std::endl;\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid fmod() argument nr 2. The value is 0 but the valid values are '!0.0'.\n"
                      "[test.cpp:4]: (error) Invalid fmodf() argument nr 2. The value is 0 but the valid values are '!0.0'.\n"
                      "[test.cpp:5]: (error) Invalid fmodl() argument nr 2. The value is 0 but the valid values are '!0.0'.\n"
                      "[test.cpp:3]: (warning) Passing values 1.0 and 0 to fmod() leads to implementation-defined result.\n"
                      "[test.cpp:4]: (warning) Passing values 1.0 and 0 to fmodf() leads to implementation-defined result.\n"
                      "[test.cpp:5]: (warning) Passing values 1.0 and 0 to fmodl() leads to implementation-defined result.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    std::cout <<  fmod(1.0,1) << std::endl;\n"
              "    std::cout <<  fmodf(1.0,1) << std::endl;\n"
              "    std::cout <<  fmodl(1.0,1) << std::endl;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void mathfunctionCall_precision() {
        check("void foo() {\n"
              "    print(exp(x) - 1);\n"
              "    print(log(1 + x));\n"
              "    print(1 - erf(x));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (style) Expression 'exp(x) - 1' can be replaced by 'expm1(x)' to avoid loss of precision.\n"
                      "[test.cpp:3]: (style) Expression 'log(1 + x)' can be replaced by 'log1p(x)' to avoid loss of precision.\n"
                      "[test.cpp:4]: (style) Expression '1 - erf(x)' can be replaced by 'erfc(x)' to avoid loss of precision.\n", errout.str());

        check("void foo() {\n"
              "    print(exp(x) - 1.0);\n"
              "    print(log(1.0 + x));\n"
              "    print(1.0 - erf(x));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (style) Expression 'exp(x) - 1' can be replaced by 'expm1(x)' to avoid loss of precision.\n"
                      "[test.cpp:3]: (style) Expression 'log(1 + x)' can be replaced by 'log1p(x)' to avoid loss of precision.\n"
                      "[test.cpp:4]: (style) Expression '1 - erf(x)' can be replaced by 'erfc(x)' to avoid loss of precision.\n", errout.str());

        check("void foo() {\n"
              "    print(exp(3 + x*f(a)) - 1);\n"
              "    print(log(x*4 + 1));\n"
              "    print(1 - erf(34*x + f(x) - c));\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (style) Expression 'exp(x) - 1' can be replaced by 'expm1(x)' to avoid loss of precision.\n"
                      "[test.cpp:3]: (style) Expression 'log(1 + x)' can be replaced by 'log1p(x)' to avoid loss of precision.\n"
                      "[test.cpp:4]: (style) Expression '1 - erf(x)' can be replaced by 'erfc(x)' to avoid loss of precision.\n", errout.str());

        check("void foo() {\n"
              "    print(2*exp(x) - 1);\n"
              "    print(1 - erf(x)/2.0);\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void checkIgnoredReturnValue() {
        Settings settings2;
        settings2.severity.enable(Severity::warning);
        const char xmldata[] = "<?xml version=\"1.0\"?>\n"
                               "<def version=\"2\">\n"
                               "  <function name=\"mystrcmp,foo::mystrcmp\">\n"
                               "    <use-retval/>\n"
                               "    <arg nr=\"1\"/>\n"
                               "    <arg nr=\"2\"/>\n"
                               "  </function>\n"
                               "</def>";
        ASSERT(settings2.library.loadxmldata(xmldata, sizeof(xmldata)));

        check("void foo() {\n"
              "  mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:2]: (warning) Return value of function mystrcmp() is not used.\n", errout.str());

        check("void foo() {\n"
              "  foo::mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:2]: (warning) Return value of function foo::mystrcmp() is not used.\n", errout.str());

        check("void f() {\n"
              "  foo x;\n"
              "  x.mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function x.mystrcmp() is not used.\n", errout.str());

        check("bool mystrcmp(char* a, char* b);\n" // cppcheck sees a custom strcmp definition, but it returns a value. Assume it is the one specified in the library.
              "void foo() {\n"
              "    mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function mystrcmp() is not used.\n", errout.str());

        check("void mystrcmp(char* a, char* b);\n" // cppcheck sees a custom strcmp definition which returns void!
              "void foo() {\n"
              "    mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    class mystrcmp { mystrcmp() {} };\n" // strcmp is a constructor definition here
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    return mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    return foo::mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    if(mystrcmp(a, b));\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    bool b = mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        // #6194
        check("void foo() {\n"
              "    MyStrCmp mystrcmp(x, y);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        // #6197
        check("void foo() {\n"
              "    abc::def.mystrcmp(a,b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        // #6233
        check("int main() {\n"
              "    auto lambda = [](double value) {\n"
              "        double rounded = floor(value + 0.5);\n"
              "        printf(\"Rounded value = %f\\n\", rounded);\n"
              "    };\n"
              "    lambda(13.3);\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #6669
        check("void foo(size_t size) {\n"
              "   void * res{malloc(size)};\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #7447
        check("void foo() {\n"
              "   int x{mystrcmp(a,b)};\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        // #7905
        check("void foo() {\n"
              "   int x({mystrcmp(a,b)});\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n" // don't crash
              "  DEBUG(123)(mystrcmp(a,b))(fd);\n"
              "}", "test.c", &settings2);
        check("struct teststruct {\n"
              "    int testfunc1() __attribute__ ((warn_unused_result)) { return 1; }\n"
              "    [[nodiscard]] int testfunc2() { return 1; }\n"
              "    void foo() { testfunc1(); testfunc2(); }\n"
              "};\n"
              "int main() {\n"
              "    teststruct TestStruct1;\n"
              "    TestStruct1.testfunc1();\n"
              "    TestStruct1.testfunc2();\n"
              "    return 0;\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:4]: (warning) Return value of function testfunc1() is not used.\n"
                      "[test.cpp:4]: (warning) Return value of function testfunc2() is not used.\n"
                      "[test.cpp:8]: (warning) Return value of function TestStruct1.testfunc1() is not used.\n"
                      "[test.cpp:9]: (warning) Return value of function TestStruct1.testfunc2() is not used.\n", errout.str());

        // #9006
        check("template <typename... a> uint8_t b(std::tuple<uint8_t> d) {\n"
              "  std::tuple<a...> c{std::move(d)};\n"
              "  return std::get<0>(c);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("struct A { int x; };\n"
              "template <class... Ts>\n"
              "A f(int x, Ts... xs) {\n"
              "    return {std::move(x), static_cast<int>(xs)...};\n"
              "}\n"
              "A g() { return f(1); }");
        ASSERT_EQUALS("", errout.str());

        // #8412 - unused operator result
        check("void foo() {\n"
              "  !mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:2]: (warning) Return value of function mystrcmp() is not used.\n", errout.str());

        check("void f(std::vector<int*> v) {\n"
              "    delete *v.begin();\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void checkIgnoredErrorCode() {
        Settings settings2;
        settings2.addEnabled("style");
        const char xmldata[] = "<?xml version=\"1.0\"?>\n"
                               "<def version=\"2\">\n"
                               "  <function name=\"mystrcmp\">\n"
                               "    <use-retval type=\"error-code\"/>\n"
                               "    <arg nr=\"1\"/>\n"
                               "    <arg nr=\"2\"/>\n"
                               "  </function>\n"
                               "</def>";
        ASSERT(settings2.library.loadxmldata(xmldata, sizeof(xmldata)));

        check("void foo() {\n"
              "  mystrcmp(a, b);\n"
              "}", "test.cpp", &settings2);
        ASSERT_EQUALS("[test.cpp:2]: (style) Error code from the return value of function mystrcmp() is not used.\n", errout.str());
    }

    void memsetZeroBytes() {
        check("void f() {\n"
              "    memset(p, 10, 0x0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) memset() called to fill 0 bytes.\n", errout.str());

        check("void f() {\n"
              "    memset(p, sizeof(p), 0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) memset() called to fill 0 bytes.\n", errout.str());

        check("void f() {\n"
              "    memset(p, sizeof(p), i);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #6269 false positives in case of overloaded standard library functions
        check("class c {\n"
              "  void memset( int i );\n"
              "  void f( void )   {\n"
              "     memset( 0 );\n"
              "  }\n"
              "};");
        ASSERT_EQUALS("", errout.str());

        // #7285
        check("void f() {\n"
              "    memset(&tm, sizeof(tm), 0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) memset() called to fill 0 bytes.\n", errout.str());

    }

    void memsetInvalid2ndParam() {
        check("void f() {\n"
              "    int* is = new int[10];\n"
              "    memset(is, 1.0f, 40);\n"
              "    int* is2 = new int[10];\n"
              "    memset(is2, 0.1f, 40);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (portability) The 2nd memset() argument '1.0f' is a float, its representation is implementation defined.\n"
                      "[test.cpp:5]: (portability) The 2nd memset() argument '0.1f' is a float, its representation is implementation defined.\n", errout.str());

        check("void f() {\n"
              "    int* is = new int[10];\n"
              "    float g = computeG();\n"
              "    memset(is, g, 40);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (portability) The 2nd memset() argument 'g' is a float, its representation is implementation defined.\n", errout.str());

        check("void f() {\n"
              "    int* is = new int[10];\n"
              "    memset(is, 0.0f, 40);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n" // FP
              "    float x = 2.3f;\n"
              "    memset(a, (x?64:0), 40);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n"
              "    short ss[] = {1, 2};\n"
              "    memset(ss, 256, 4);\n"
              "    short ss2[2];\n"
              "    memset(ss2, -129, 4);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) The 2nd memset() argument '256' doesn't fit into an 'unsigned char'.\n"
                      "[test.cpp:5]: (warning) The 2nd memset() argument '-129' doesn't fit into an 'unsigned char'.\n", errout.str());

        check("void f() {\n"
              "    int is[10];\n"
              "    memset(is, 0xEE, 40);\n"
              "    unsigned char* cs = malloc(256);\n"
              "    memset(cs, -1, 256);\n"
              "    short* ss[30];\n"
              "    memset(ss, -128, 60);\n"
              "    char cs2[30];\n"
              "    memset(cs2, 255, 30);\n"
              "    char cs3[30];\n"
              "    memset(cs3, 0, 30);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n"
              "    int is[10];\n"
              "    const int i = g();\n"
              "    memset(is, 1.0f + i, 40);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (portability) The 2nd memset() argument '1.0f+i' is a float, its representation is implementation defined.\n", errout.str());
    }

    void checkMissingReturn() {
        check("int f() {}");
        ASSERT_EQUALS("[test.cpp:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        {
            const char code[] = "int main(void) {}";
            Settings s;

            s.standards.c = Standards::C89;
            check(code, "test.c", &s); // c code (c89)
            ASSERT_EQUALS("[test.c:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

            s.standards.c = Standards::C99;
            check(code, "test.c", &s); // c code (c99)
            ASSERT_EQUALS("", errout.str());

            check(code, "test.cpp", &s); // c++ code
            ASSERT_EQUALS("", errout.str());
        }

        check("F(A,B) { x=1; }");
        ASSERT_EQUALS("", errout.str());

        check("auto foo4() -> void {}");
        ASSERT_EQUALS("", errout.str());

        check("void STDCALL foo() {}");
        ASSERT_EQUALS("", errout.str());

        check("void operator=(int y) { x=y; }");
        ASSERT_EQUALS("", errout.str());

        check("int f() {\n"
              "back:\n"
              "    return 0;\n"
              "ng:\n"
              "    x=y;\n"
              "    goto back;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        // unreachable code..
        check("int foo(int x) {\n"
              "  return 1;\n"
              "  (void)x;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int foo(int x) {\n"
              "  if (x) goto out;\n"
              "  return 1;\n"
              "out:\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        // switch
        check("int f() {\n"
              "    switch (x) {\n"
              "        case 1: break;\n" // <- error
              "        case 2: return 1;\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        check("int f() {\n"
              "    switch (x) {\n"
              "        case 1: return 2; break;\n"
              "        default: return 1;\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("bool test(unsigned char v1, int v2) {\n"
              "    switch (v1) {\n"
              "        case 0:\n"
              "            switch (v2) {\n"
              "            case 48000:\n"
              "                break;\n"
              "            }\n"
              "            return false;\n"
              "        default:\n"
              "            return true;\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // if/else
        check("int f(int x) {\n"
              "    if (x) {\n"
              "        return 1;\n"
              "    }\n" // <- error (missing else)
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        check("int f(int x) {\n"
              "    if (x) {\n"
              "        ;\n" // <- error
              "    } else {\n"
              "        return 1;\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        check("int f() {\n"
              "    if (!0) {\n"
              "        return 1;\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int f() {\n"
              "    if (!0) {}\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Found an exit path from function with non-void return type that has missing return statement\n", errout.str());

        // loop
        check("int f(int x) {\n"
              "    while (1) {\n"
              "        dostuff();\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // return {..}
        check("std::pair<int, int> typeDecl(int tok) {\n"
              "    if (!tok)\n"
              "        return {};\n"
              "    else\n"
              "        return {1, 2};\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // noreturn function
        check("int f(int x) { exit(0); }");
        ASSERT_EQUALS("", errout.str());

        check("int f(int x) { assert(0); }");
        ASSERT_EQUALS("", errout.str());

        check("int f(int x) { if (x) return 1; else return bar({1}, {}); }");
        ASSERT_EQUALS("", errout.str());

        check("auto f() -> void {}"); // #10342
        ASSERT_EQUALS("", errout.str());

        check("struct S1 {\n" // #7433
              "    S1& operator=(const S1& r) { if (this != &r) { i = r.i; } }\n"
              "    int i;\n"
              "};\n"
              "struct S2 {\n"
              "    S2& operator=(const S2& s) { if (this != &s) { j = s.j; } return *this; }\n"
              "    int j;\n"
              "};\n"
              "struct S3 {\n"
              "    S3& operator=(const S3& t) { if (this != &t) { k = t.k; return *this; } }\n"
              "    int k;\n"
              "};\n");
        ASSERT_EQUALS("[test.cpp:2]: (error) Found an exit path from function with non-void return type that has missing return statement\n"
                      "[test.cpp:10]: (error) Found an exit path from function with non-void return type that has missing return statement\n",
                      errout.str());

        // #11171
        check("std::enable_if_t<sizeof(uint64_t) == 8> f() {}");
        ASSERT_EQUALS("", errout.str());

        check("std::enable_if_t<sizeof(uint64_t) == 8, int> f() {}");
        ASSERT_EQUALS(
            "[test.cpp:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n",
            errout.str());

        check("template<class T> std::enable_if_t<std::is_same<T, int>{}, int> f(T) {}");
        ASSERT_EQUALS(
            "[test.cpp:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n",
            errout.str());

        check("template<class T> std::enable_if_t<std::is_same<T, int>{}> f(T) {}");
        ASSERT_EQUALS("", errout.str());

        check("typename std::enable_if<sizeof(uint64_t) == 8>::type f() {}");
        ASSERT_EQUALS("", errout.str());

        check("typename std::enable_if<sizeof(uint64_t) == 8, int>::type f() {}");
        ASSERT_EQUALS(
            "[test.cpp:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n",
            errout.str());

        check("template<class T> typename std::enable_if<std::is_same<T, int>{}, int>::type f(T) {}");
        ASSERT_EQUALS(
            "[test.cpp:1]: (error) Found an exit path from function with non-void return type that has missing return statement\n",
            errout.str());

        check("template<class T> typename std::enable_if<std::is_same<T, int>{}>::type f(T) {}");
        ASSERT_EQUALS("", errout.str());
    }

    // NRVO check
    void returnLocalStdMove1() {
        check("struct A{}; A f() { A var; return std::move(var); }");
        ASSERT_EQUALS("[test.cpp:1]: (performance) Using std::move for returning object by-value from function will affect copy elision optimization."
                      " More: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-return-move-local\n", errout.str());
    }

    // RVO, C++03 ctor style
    void returnLocalStdMove2() {
        check("struct A{}; A f() { return std::move( A() ); }");
        ASSERT_EQUALS("[test.cpp:1]: (performance) Using std::move for returning object by-value from function will affect copy elision optimization."
                      " More: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-return-move-local\n", errout.str());
    }

    // RVO, new ctor style
    void returnLocalStdMove3() {
        check("struct A{}; A f() { return std::move(A{}); }");
        ASSERT_EQUALS("[test.cpp:1]: (performance) Using std::move for returning object by-value from function will affect copy elision optimization."
                      " More: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-return-move-local\n", errout.str());
    }

    // Function argument
    void returnLocalStdMove4() {
        check("struct A{}; A f(A a) { return std::move(A{}); }");
        ASSERT_EQUALS("[test.cpp:1]: (performance) Using std::move for returning object by-value from function will affect copy elision optimization."
                      " More: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-return-move-local\n", errout.str());
    }

    void returnLocalStdMove5() {
        check("struct A{} a; A f1() { return std::move(a); }\n"
              "A f2() { volatile A var; return std::move(var); }");
        ASSERT_EQUALS("", errout.str());

        check("struct S { std::string msg{ \"abc\" }; };\n"
              "std::unique_ptr<S> get(std::vector<std::unique_ptr<S>>& v) {\n"
              "    return std::move(v.front());\n"
              "}\n"
              "int main() {\n"
              "    std::vector<std::unique_ptr<S>> v;\n"
              "    v.emplace_back(std::make_unique<S>());\n"
              "    auto p = get(v);\n"
              "    std::cout << p->msg;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void negativeMemoryAllocationSizeError() { // #389
        check("void f() {\n"
              "   int *a;\n"
              "   a = malloc( -10 );\n"
              "   free(a);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Invalid malloc() argument nr 1. The value is -10 but the valid values are '0:'.\n", errout.str());

        check("void f() {\n"
              "   int *a;\n"
              "   a = alloca( -10 );\n"
              "   free(a);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'alloca' called.\n"
                      "[test.cpp:3]: (error) Invalid alloca() argument nr 1. The value is -10 but the valid values are '0:'.\n", errout.str());
    }

    void checkLibraryMatchFunctions() {
        const auto settings_old = settings;
        settings.checkLibrary = true;
        settings.daca = true;

        check("void f() {\n"
              "    lib_func();"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(void* v) {\n"
              "    lib_func(v);"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (information) --check-library: There is no matching configuration for function lib_func()\n", errout.str());

        // #10105
        check("class TestFixture {\n"
              "protected:\n"
              "        bool prepareTest(const char testname[]);\n"
              "};\n"
              "\n"
              "class TestMemleak : private TestFixture {\n"
              "        void run() {\n"
              "                do { prepareTest(\"testFunctionReturnType\"); } while (false);\n"
              "        }\n"
              "\n"
              "        void testFunctionReturnType() {\n"
              "        }\n"
              "};");
        ASSERT_EQUALS("", errout.str());

        // #11183
        check("#include <string>\n"
              "\n"
              "extern void cb(const std::string&);\n"
              "\n"
              "void f() {\n"
              "    cb(std::string(\"\"));\n"
              "}");
        TODO_ASSERT_EQUALS("", "[test.cpp:6]: (information) --check-library: There is no matching configuration for function cb()\n", errout.str());

        // #7375
        check("void f() {\n"
              "    struct S { int i; char c; };\n"
              "    size_t s = sizeof(S);\n"
              "    static_assert(s == 9);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(char) {}\n"
              "void g() {\n"
              "    f(int8_t(1));\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(std::uint64_t& u) {\n"
              "    u = std::uint32_t(u) * std::uint64_t(100);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f() { throw(1); }\n"); // #8958
        ASSERT_EQUALS("", errout.str());

        check("using namespace std;\n"
              "void f() { throw range_error(\"abc\"); }\n");
        ASSERT_EQUALS("", errout.str());

        check("class C {\n" // #9002
              "public:\n"
              "    static int f() { return 1; }\n"
              "};\n"
              "void g() { C::f(); }\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(const std::vector<std::string>& v) {\n" // #11223
              "    for (const auto& s : v)\n"
              "        s.find(\"\");\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function s.find() is not used.\n", errout.str());

        check("void f() {\n"
              "    auto* p = new std::vector<int>(5);\n"
              "    p->push_back(1);\n"
              "    auto* q = new std::vector<int>{ 5, 7 };\n"
              "    q->push_back(1);\n"
              "    auto* r = new std::vector<int>;\n"
              "    r->push_back(1);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n"
              "    auto p = std::make_shared<std::vector<int>>();\n"
              "    p->push_back(1);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(std::vector<std::vector<int>>& v) {\n"
              "    auto it = v.begin();\n"
              "    it->push_back(1);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n"
              "    auto v = std::vector<int>{};\n"
              "    v.push_back(1);\n"
              "    auto w = std::vector<int>{ 1, 2, 3 };\n"
              "    w.push_back(1);\n"
              "    auto x = std::vector<int>(1);\n"
              "    x.push_back(1);\n"
              "}\n");
        TODO_ASSERT_EQUALS("",
                           "[test.cpp:7]: (information) --check-library: There is no matching configuration for function auto::push_back()\n",
                           errout.str());

        check("void f() {\n"
              "    auto p(std::make_shared<std::vector<int>>());\n"
              "    p->push_back(1);\n"
              "    auto q{ std::make_shared<std::vector<int>>{} };\n"
              "    q->push_back(1);\n"
              "}\n");
        TODO_ASSERT_EQUALS("",
                           "[test.cpp:3]: (information) --check-library: There is no matching configuration for function auto::push_back()\n"
                           "[test.cpp:5]: (information) --check-library: There is no matching configuration for function auto::push_back()\n",
                           errout.str());

        check("struct F { void g(int); };\n"
              "void f(std::list<F>& l) {\n"
              "    std::list<F>::iterator it;\n"
              "    for (it = l.begin(); it != l.end(); ++it)\n"
              "        it->g(0);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("auto f() {\n"
              "    return std::runtime_error(\"abc\");\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("struct S {\n" // #11543
              "    S() {}\n"
              "    std::vector<std::shared_ptr<S>> v;\n"
              "    void f(int i) const;\n"
              "};\n"
              "void S::f(int i) const {\n"
              "    for (const std::shared_ptr<S>& c : v)\n"
              "        c->f(i);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("namespace N {\n"
              "    struct S { static const std::set<std::string> s; };\n"
              "}\n"
              "void f() {\n"
              "    const auto& t = N::S::s;\n"
              "    if (t.find(\"abc\") != t.end()) {}\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f(std::vector<std::unordered_map<int, std::unordered_set<int>>>& v, int i, int j) {\n"
              "    auto& s = v[i][j];\n"
              "    s.insert(0);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("int f(const std::vector<std::string>& v, int i, char c) {\n"
              "    const auto& s = v[i];\n"
              "    return s.find(c);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void f() {\n" // #11604
              "    int (*g)() = nullptr;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        settings = settings_old;
    }

    void checkUseStandardLibrary1() {
        check("void f(void* dest, void const* src, const size_t count) {\n"
              "    size_t i;\n"
              "    for (i = 0; count > i; ++i)\n"
              "        (reinterpret_cast<uint8_t*>(dest))[i] = (reinterpret_cast<const uint8_t*>(src))[i];\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (style) Consider using std::memcpy instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary2() {
        check("void f(void* dest, void const* src, const size_t count) {\n"
              "    for (size_t i = 0; i < count; i++) {\n"
              "        (reinterpret_cast<uint8_t*>(dest))[i] = (reinterpret_cast<const uint8_t*>(src))[i];\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memcpy instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary3() {
        check("void f(void* dst, const void* src, const size_t count) {\n"
              "    size_t i;\n"
              "    for (i = 0; count > i; i++)\n"
              "        ((char*)dst)[i] = ((const char*)src)[i];\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (style) Consider using std::memcpy instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary4() {
        check("void f(void* dst, void* src, const size_t size) {\n"
              "    for (size_t i = 0; i < size; i += 1) {\n"
              "        ((int8_t*)dst)[i] = ((int8_t*)src)[i];\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memcpy instead of loop.\n", errout.str());
    }

    // different indexes
    void checkUseStandardLibrary5() {
        check("void f(void* dst, void* src, const size_t size, const size_t from_idx) {\n"
              "    for (size_t i = 0; i < size; ++i) {\n"
              "        ((int8_t*)dst)[i] = ((int8_t*)src)[from_idx];\n"
              "}}\n");
        ASSERT_EQUALS("", errout.str());
    }

    // unknown count
    void checkUseStandardLibrary6() {
        check("void f(void* dst, void* src, const size_t size) {\n"
              "    for (size_t i = 0; ((int8_t*)src)[i] != 0; ++i) {\n"
              "        ((int8_t*)dst)[i] = ((int8_t*)src)[i];\n"
              "}}\n");
        ASSERT_EQUALS("", errout.str());
    }

    // increment with 2
    void checkUseStandardLibrary7() {
        check("void f(void* dst, void* src, const size_t size) {\n"
              "    for (size_t i = 0; i < size; i += 2) {\n"
              "        ((int8_t*)dst)[i] = ((int8_t*)src)[i];\n"
              "}}\n");
        ASSERT_EQUALS("", errout.str());
    }

    // right argument of assignment could be static_cast, functional cast, c-style and implicit cast
    // functional cast case not covered
    void checkUseStandardLibrary8() {
        check("void f(void* dest, const size_t count) {\n"
              "    size_t i;\n"
              "    for (i = 0; i < count; ++i)\n"
              "        (reinterpret_cast<int8_t*>(dest))[i] = static_cast<const int8_t>(0);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary9() {
        check("void f(void* dest, const size_t count) {\n"
              "    for (size_t i = 0; i < count; i++) {\n"
              "        (reinterpret_cast<uint8_t*>(dest))[i] = (static_cast<const uint8_t>(0));\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary10() {
        check("void f(void* dst, const size_t size) {\n"
              "    size_t i;\n"
              "    for (i = 0; i < size; i++)\n"
              "        ((char*)dst)[i] = (const char)0;\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary11() {
        check("void f(void* dst, const size_t size) {\n"
              "    for (size_t i = 0; i < size; i += 1) {\n"
              "        ((int8_t*)dst)[i] = ((int8_t)0);\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary12() {
        check("void f(void* dst, const size_t size) {\n"
              "    for (size_t i = 0; i < size; i += 1) {\n"
              "        ((int8_t*)dst)[i] = 42;\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary13() {
        check("void f(void* dest, const size_t count) {\n"
              "    for (size_t i = 0; i < count; i++) {\n"
              "        reinterpret_cast<unsigned char*>(dest)[i] = '0';\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }

    void checkUseStandardLibrary14() {
        check("void f(void* dest) {\n"
              "    for (size_t i = 0; i < sizeof(int)*(15 + 42/2 - 7); i++) {\n"
              "        reinterpret_cast<unsigned char*>(dest)[i] = '0';\n"
              "}}\n");
        ASSERT_EQUALS("[test.cpp:3]: (style) Consider using std::memset instead of loop.\n", errout.str());
    }
};

REGISTER_TEST(TestFunctions)
