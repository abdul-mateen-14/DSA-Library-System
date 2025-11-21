#define CROW_MAIN
#include "crow_all.h"
#include "json.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <mutex>
#include <algorithm>
#include <iostream>


using json = nlohmann::json;
using namespace std;


// ------------ DATA STRUCTS ------------


struct Book {
int id;
string title;
string author;
bool isAvailable = true;


json to_json() const {
return json{{"id", id}, {"title", title}, {"author", author}, {"isAvailable", isAvailable}};
}


static Book from_json(const json& j) {
Book b;
b.id = j.at("id").get<int>();
b.title = j.at("title").get<string>();
b.author = j.at("author").get<string>();
b.isAvailable = j.value("isAvailable", true);
return b;
}
};


struct User {
int userId;
string userName;


json to_json() const {
return json{{"userId", userId}, {"userName", userName}};
}


static User from_json(const json& j) {
User u;
u.userId = j.at("userId").get<int>();
u.userName = j.at("userName").get<string>();
return u;
}
};


// ------------ GLOBAL DATA ------------


vector<Book> libraryBooks;
vector<User> libraryUsers;
mutex data_mutex;


const string adminPassword = "admin123";


sqlite3* db = nullptr;
const char* DB_FILE = "library.db"; // file will be created in working directory


// ------------ SQLITE HELPERS ------------


bool execSQL(const string &sql) {
char* errmsg = nullptr;
}
