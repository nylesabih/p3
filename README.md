# High-Performance Log Manager

A specialized C++ tool designed for processing and querying large-scale server log files. This system utilizes advanced data structures and indexing techniques to provide sub-second search times across millions of log entries.

## 🚀 Key Features

* **Fast Data Ingest**: Efficiently parses large log files with custom timestamp, category, and message fields.
* **Multi-Index Architecture**:
    * **Timestamp Index**: $O(\log n)$ range and exact matches using sorted vectors and binary search.
    * **Category Index**: $O(1)$ average-time lookups using hash maps (`unordered_map`).
    * **Keyword Search**: Full-text indexing of all words within log messages and categories, supporting multi-word intersections.
* **Interactive Command Shell**: A custom REPL (Read-Eval-Print Loop) for real-time querying.
* **Excerpt List Management**: Allows users to curate a specific "excerpt" of logs, with support for reordering, deleting, and clearing.

## 🛠 Technical Stack

* **Language**: C++11
* **Data Structures**: `std::vector`, `std::unordered_map`, `std::set_intersection`, `std::lower_bound`.
* **Optimization**: 
    * Pass-by-reference and `reserve()` to minimize memory reallocations.
    * Pre-computed lowercase strings for case-insensitive searching.
    * Index-based "excerpt" management to avoid duplicating large strings in memory.

## 💻 Usage

### Build
Compile the project using the included `Makefile`:
```bash
make
