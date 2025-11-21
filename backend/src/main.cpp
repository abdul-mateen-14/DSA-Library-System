#define CROW_MAIN
#include "crow_all.h"
#include "json.hpp"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <vector>
#include <string>
#include <mutex>

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

// ------------ MYSQL DATABASE FUNCTIONS ------------

sql::Driver* driver;
sql::Connection* con;

void initDatabase() {
    try {
        driver = get_driver_instance();
        // Connect to InfinityFree MySQL
        con = driver->connect("tcp://sql301.infinityfree.com:3306", "if0_38791730", "YOUR_PASSWORD_HERE");  // Replace with your actual password
        con->setSchema("if0_38791730_db_lms");

        // Create tables if not exist
        sql::Statement* stmt = con->createStatement();
        stmt->execute("CREATE TABLE IF NOT EXISTS books (id INT PRIMARY KEY, title VARCHAR(255), author VARCHAR(255), isAvailable TINYINT(1))");
        stmt->execute("CREATE TABLE IF NOT EXISTS users (userId INT PRIMARY KEY, userName VARCHAR(255))");
        delete stmt;

        // Load data into memory
        loadBooksFromDB();
        loadUsersFromDB();
    } catch (sql::SQLException &e) {
        cerr << "MySQL Error: " << e.what() << endl;
    }
}

void loadBooksFromDB() {
    lock_guard<mutex> lock(data_mutex);
    libraryBooks.clear();
    try {
        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT id, title, author, isAvailable FROM books");
        while (res->next()) {
            Book b;
            b.id = res->getInt("id");
            b.title = res->getString("title");
            b.author = res->getString("author");
            b.isAvailable = res->getBoolean("isAvailable");
            libraryBooks.push_back(b);
        }
        delete res;
        delete stmt;
    } catch (sql::SQLException &e) {}
}

void loadUsersFromDB() {
    lock_guard<mutex> lock(data_mutex);
    libraryUsers.clear();
    try {
        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT userId, userName FROM users");
        while (res->next()) {
            User u;
            u.userId = res->getInt("userId");
            u.userName = res->getString("userName");
            libraryUsers.push_back(u);
        }
        delete res;
        delete stmt;
    } catch (sql::SQLException &e) {}
}

void saveBookToDB(const Book& b) {
    lock_guard<mutex> lock(data_mutex);
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement("INSERT INTO books (id, title, author, isAvailable) VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE title=?, author=?, isAvailable=?");
        pstmt->setInt(1, b.id);
        pstmt->setString(2, b.title);
        pstmt->setString(3, b.author);
        pstmt->setBoolean(4, b.isAvailable);
        pstmt->setString(5, b.title);
        pstmt->setString(6, b.author);
        pstmt->setBoolean(7, b.isAvailable);
        pstmt->execute();
        delete pstmt;
    } catch (sql::SQLException &e) {}
}

void saveUserToDB(const User& u) {
    lock_guard<mutex> lock(data_mutex);
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users (userId, userName) VALUES (?, ?) ON DUPLICATE KEY UPDATE userName=?");
        pstmt->setInt(1, u.userId);
        pstmt->setString(2, u.userName);
        pstmt->setString(3, u.userName);
        pstmt->execute();
        delete pstmt;
    } catch (sql::SQLException &e) {}
}

void deleteBookFromDB(int id) {
    lock_guard<mutex> lock(data_mutex);
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement("DELETE FROM books WHERE id = ?");
        pstmt->setInt(1, id);
        pstmt->execute();
        delete pstmt;
    } catch (sql::SQLException &e) {}
}

void deleteUserFromDB(int id) {
    lock_guard<mutex> lock(data_mutex);
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement("DELETE FROM users WHERE userId = ?");
        pstmt->setInt(1, id);
        pstmt->execute();
        delete pstmt;
    } catch (sql::SQLException &e) {}
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

    delete con;
    return 0;
}
