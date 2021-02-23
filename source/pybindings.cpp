#include <pybind11/pybind11.h>

#include <iostream>
#include <pqxx/pqxx>
#include <msgpack.hpp>

struct your_class {
    int a;
    std::string b;
    MSGPACK_DEFINE(a, b);
};
std::string msg() {
    std::stringstream ss;
    msgpack::pack(ss, your_class{42,"hello"});
    return ss.str();
}

using namespace std;
using namespace pqxx;
int pq() {
   std::string sql;

   try {
      connection C("dbname = postgres user = postgres password = postgres \
      hostaddr = 127.0.0.1 port = 5432");
      if (C.is_open()) {
         cout << "Opened database successfully: " << C.dbname() << endl;
      } else {
         cout << "Can't open database" << endl;
         return 1;
      }

      /* Create SQL statement */
      sql = R"(CREATE TABLE COMPANY(
      ID INT PRIMARY KEY     NOT NULL,
      NAME           TEXT    NOT NULL,
      AGE            INT     NOT NULL,
      ADDRESS        CHAR(50),
      SALARY         REAL );)";

      /* Create a transactional object. */
      work W(C);

      /* Execute SQL query */
      W.exec( sql );
      W.commit();
      cout << "Table created successfully" << endl;
      C.disconnect ();

      return 0;
   } catch (const std::exception &e) {
      cerr << e.what() << std::endl;
      return 1;
   }
}
int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(freedomdb, m) {
    m.doc() = "Python version of FreedomDB library"; // optional module docstring

    m.def("add", &add, "A function which adds two numbers");
    m.def("msg", &msg, "MessagePack!");
    m.def("pq", &pq, "PostgreSQL!");
}
