#define CROW_MAIN
#include "crow_all.h"
#include "json.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <mutex>
#include <cstdlib> // getenv

using json = nlohmann::json;
using namespace std;

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

// Global data
vector<Book> libraryBooks;
vector<User> libraryUsers;
mutex data_mutex;

sqlite3* db;

// --- Helpers ---
Book* findBookById(int id) {
    for (auto& book : libraryBooks)
        if (book.id == id) return &book;
    return nullptr;
}

User* findUserById(int id) {
    for (auto& user : libraryUsers)
        if (user.userId == id) return &user;
    return nullptr;
}

// --- Load data from SQLite
void initDatabase() {
    sqlite3_open("library.db", &db);

    const char* create_books = "CREATE TABLE IF NOT EXISTS books(id INTEGER PRIMARY KEY, title TEXT, author TEXT, isAvailable INTEGER)";
    sqlite3_exec(db, create_books, 0, 0, 0);

    const char* create_users = "CREATE TABLE IF NOT EXISTS users(userId INTEGER PRIMARY KEY, userName TEXT)";
    sqlite3_exec(db, create_users, 0, 0, 0);

    // Load books
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, title, author, isAvailable FROM books", -1, &stmt, 0);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Book b;
        b.id = sqlite3_column_int(stmt, 0);
        b.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        b.author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        b.isAvailable = sqlite3_column_int(stmt, 3) != 0;
        libraryBooks.push_back(b);
    }
    sqlite3_finalize(stmt);

    // Load users
    sqlite3_prepare_v2(db, "SELECT userId, userName FROM users", -1, &stmt, 0);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.userId = sqlite3_column_int(stmt, 0);
        u.userName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        libraryUsers.push_back(u);
    }
    sqlite3_finalize(stmt);
}

// --- Save helpers
void saveBook(const Book& b) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO books(id, title, author, isAvailable) VALUES(?, ?, ?, ?)", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, b.id);
    sqlite3_bind_text(stmt, 2, b.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, b.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, b.isAvailable ? 1 : 0);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void saveUser(const User& u) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO users(userId, userName) VALUES(?, ?)", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, u.userId);
    sqlite3_bind_text(stmt, 2, u.userName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// --- Main ---
int main() {
    initDatabase();

    crow::SimpleApp app;
    app.middleware().add<crow::middleware::CORS>();

    // Get port from Railway environment
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) port = std::stoi(env_p);

    // Routes
    CROW_ROUTE(app, "/books").methods("GET"_method)([]() {
        json arr = json::array();
        for (const auto& b : libraryBooks) arr.push_back(b.to_json());
        return crow::response{json{{"books", arr}}.dump()};
    });

    CROW_ROUTE(app, "/books").methods("POST"_method)([](const crow::request& req) {
        auto x = json::parse(req.body, nullptr, false);
        if (x.is_discarded() || !x.contains("id") || !x.contains("title") || !x.contains("author"))
            return crow::response(400, R"({"success":false,"message":"Missing fields"})");

        Book b = Book::from_json(x);
        libraryBooks.push_back(b);
        saveBook(b);
        return crow::response(R"({"success":true})");
    });

    // Add more routes like /users, issue/return as needed

    app.port(port).multithreaded().run();

    sqlite3_close(db);
    return 0;
}
