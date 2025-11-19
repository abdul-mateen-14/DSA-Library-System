# Dynamic Array-Based Library System

## Project Description
A web-based library management system using C++ backend (REST API) and HTML/CSS/JS frontend. Demonstrates Data Structures (dynamic arrays), File Handling (JSON persistence), and GUI design. Features include book/user management, issue/return, and admin authentication.

## Features
- Admin login with password.
- Add, search, delete books/users.
- Issue/return books.
- Responsive GUI with tables and modals.
- Data persistence via JSON files.

## Installation and Dependencies
- **Backend**: C++17, CMake, Boost (for Crow). Download headers: [Crow](https://github.com/CrowCpp/Crow), [nlohmann/json](https://github.com/nlohmann/json).
- **Frontend**: None (runs in browser).
- **Docker**: For deployment.

## How to Run
1. **Locally**:
   - Backend: `cd code/backend`, `mkdir build`, `cd build`, `cmake ..`, `make`, `./library_server`.
   - Frontend: Open `ui/index.html` in browser, set `apiUrl` to `http://localhost:8080`.
2. **Deployed**: See Railway.app section.

## Folder Structure
- `code/`: Source files.
- `server/`: Executable and data.
- `ui/`: GUI files.
- `docs/`: Diagrams.
- `testcases/`: Test cases.

## Data Structures Used
- Dynamic arrays (std::vector) for books/users: Efficient resizing, O(1) amortized insert, O(n) search.

## File Handling
- Saves to JSON files in `server/data/`. Loaded on startup, saved on changes.
