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
int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
if (rc != SQLITE_OK) {
if (errmsg) {
cerr << "SQLite error: " << errmsg << endl;
sqlite3_free(errmsg);
}
return false;
}
return true;
}


void initDatabase();
void loadBooksFromDB();
void loadUsersFromDB();
void saveBookToDB(const Book& b);
void saveUserToDB(const User& u);
void deleteBookFromDB(int id);
void deleteUserFromDB(int id);
// ------------ DATABASE FUNCTIONS IMPLEMENTATION ------------
sqlite3_bind_int(stmt, 1, b.id);
sqlite3_bind_text(stmt, 2, b.title.c_str(), -1, SQLITE_TRANSIENT);
sqlite3_bind_text(stmt, 3, b.author.c_str(), -1, SQLITE_TRANSIENT);
sqlite3_bind_int(stmt, 4, b.isAvailable ? 1 : 0);
if (sqlite3_step(stmt) != SQLITE_DONE) {
cerr << "Failed to execute insert book: " << sqlite3_errmsg(db) << endl;
}
sqlite3_finalize(stmt);
}


void saveUserToDB(const User& u) {
lock_guard<mutex> lock(data_mutex);
const char* sql = "INSERT OR REPLACE INTO users (userId, userName) VALUES (?, ?);";
sqlite3_stmt* stmt = nullptr;
if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
cerr << "Failed to prepare insert user: " << sqlite3_errmsg(db) << endl;
return;
}
sqlite3_bind_int(stmt, 1, u.userId);
sqlite3_bind_text(stmt, 2, u.userName.c_str(), -1, SQLITE_TRANSIENT);
if (sqlite3_step(stmt) != SQLITE_DONE) {
cerr << "Failed to execute insert user: " << sqlite3_errmsg(db) << endl;
}
sqlite3_finalize(stmt);
}


void deleteBookFromDB(int id) {
lock_guard<mutex> lock(data_mutex);
const char* sql = "DELETE FROM books WHERE id = ?;";
sqlite3_stmt* stmt = nullptr;
if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
cerr << "Failed to prepare delete book: " << sqlite3_errmsg(db) << endl;
return;
}
sqlite3_bind_int(stmt, 1, id);
if (sqlite3_step(stmt) != SQLITE_DONE) {
cerr << "Failed to execute delete book: " << sqlite3_errmsg(db) << endl;
}
sqlite3_finalize(stmt);
}


void deleteUserFromDB(int id) {
lock_guard<mutex> lock(data_mutex);
const char* sql = "DELETE FROM users WHERE userId = ?;";
sqlite3_stmt* stmt = nullptr;
if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
cerr << "Failed to prepare delete user: " << sqlite3_errmsg(db) << endl;
return;
}
sqlite3_bind_int(stmt, 1, id);
if (sqlite3_step(stmt) != SQLITE_DONE) {
cerr << "Failed to execute delete user: " << sqlite3_errmsg(db) << endl;
}
sqlite3_finalize(stmt);
}
// ------------ HELPERS ------------


bool checkAdminPassword(const string& pwd) { return pwd == adminPassword; }


Book* findBookById(int id) {
for (auto& book : libraryBooks) if (book.id == id) return &book;
return nullptr;
}


User* findUserById(int id) {
for (auto& user : libraryUsers) if (user.userId == id) return &user;
return nullptr;
}
// ------------ MAIN ------------


int main() {
initDatabase();


crow::SimpleApp app;
app.middleware().add<crow::middleware::CORS>();


// Admin Login
CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([](const crow::request& req){
auto x = json::parse(req.body, nullptr, false);
if (x.is_discarded() || !x.contains("password"))
return crow::response(400, R"({"success":false, "message":"Missing password"})");
bool valid = checkAdminPassword(x["password"].get<string>());
json resp = {{"success", valid}};
return crow::response{resp.dump()};
});


// Books: GET all
CROW_ROUTE(app, "/books").methods("GET"_method)([](){
json arr = json::array();
for (const auto& book : libraryBooks) arr.push_back(book.to_json());
json resp = {{"books", arr}};
return crow::response{resp.dump()};
});
// Books: POST add
CROW_ROUTE(app, "/books").methods("POST"_method)([](const crow::request& req){
auto x = json::parse(req.body, nullptr, false);
if (x.is_discarded() || !x.contains("id") || !x.contains("title") || !x.contains("author"))
return crow::response(400, R"({"success":false, "message":"Required fields missing"})");
int id = x["id"].get<int>();
lock_guard<mutex> lock(data_mutex);
if (findBookById(id)) return crow::response(409, R"({"success":false,"message":"Book ID exists"})");
Book b = Book::from_json(x);
libraryBooks.push_back(b);
saveBookToDB(b);
return crow::response(R"({"success":true})");
});


// Books: GET by ID
CROW_ROUTE(app, "/books/<int>").methods("GET"_method)([](int id){
Book* b = findBookById(id);
if (!b) return crow::response(404, R"({"success":false,"message":"Book not found"})");
return crow::response(b->to_json().dump());
});


// Books: DELETE by ID
CROW_ROUTE(app, "/books/<int>").methods("DELETE"_method)([](int id){
lock_guard<mutex> lock(data_mutex);
auto it = remove_if(libraryBooks.begin(), libraryBooks.end(), [id](const Book& b){ return b.id == id; });
if (it == libraryBooks.end()) return crow::response(404, R"({"success":false,"message":"Book not found"})");
libraryBooks.erase(it, libraryBooks.end());
deleteBookFromDB(id);
return crow::response(R"({"success":true})");
});
// Books: Issue
CROW_ROUTE(app, "/books/<int>/issue").methods("POST"_method)([](const crow::request& req, int bookId){
auto x = json::parse(req.body, nullptr, false);
if (x.is_discarded() || !x.contains("userId"))
return crow::response(400, R"({"success":false,"message":"userId missing"})");
int userId = x["userId"].get<int>();
lock_guard<mutex> lock(data_mutex);
Book* b = findBookById(bookId);
if (!b) return crow::response(404, R"({"success":false,"message":"Book not found"})");
if (!b->isAvailable) return crow::response(409, R"({"success":false,"message":"Book issued"})");
User* u = findUserById(userId);
if (!u) return crow::response(404, R"({"success":false,"message":"User not found"})");
b->isAvailable = false;
saveBookToDB(*b);
return crow::response(R"({"success":true})");
});


// Books: Return
CROW_ROUTE(app, "/books/<int>/return").methods("POST"_method)([](int bookId){
lock_guard<mutex> lock(data_mutex);
Book* b = findBookById(bookId);
if (!b) return crow::response(404, R"({"success":false,"message":"Book not found"})");
if (b->isAvailable) return crow::response(409, R"({"success":false,"message":"Book not issued"})");
b->isAvailable = true;
saveBookToDB(*b);
return crow::response(R"({"success":true})");
});
// Users: GET all
CROW_ROUTE(app, "/users").methods("GET"_method)([](){
json arr = json::array();
for (const auto& u : libraryUsers) arr.push_back(u.to_json());
json resp = {{"users", arr}};
return crow::response{resp.dump()};
});


// Users: POST add
CROW_ROUTE(app, "/users").methods("POST"_method)([](const crow::request& req){
auto x = json::parse(req.body, nullptr, false);
if (x.is_discarded() || !x.contains("userId") || !x.contains("userName"))
return crow::response(400, R"({"success":false,"message":"Required fields missing"})");
int uid = x["userId"].get<int>();
lock_guard<mutex> lock(data_mutex);
if (findUserById(uid)) return crow::response(409, R"({"success":false,"message":"User ID exists"})");
User u = User::from_json(x);
libraryUsers.push_back(u);
saveUserToDB(u);
return crow::response(R"({"success":true})");
});
app.port(8080).multithreaded().run();


if (db) sqlite3_close(db);
return 0;
}
