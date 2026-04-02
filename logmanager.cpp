// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#include "logmanager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

// Helper function to convert string to lowercase
static string toLower(const string &s) {
    string result;
    result.reserve(s.size());
    for (unsigned char c : s)
        result += static_cast<char>(tolower(c));
    return result;
}

// Load the master log file
void LogManager::loadLogFile(const string &filename) {
    ifstream fin(filename);
    if (!fin) {
        cerr << "Error opening file: " << filename << '\n';
        exit(1);
    }

    string line;
    size_t id = 0;
    while (getline(fin, line)) {
        size_t p1 = line.find('|');
        size_t p2 = line.find('|', p1 + 1);
        if (p1 == string::npos || p2 == string::npos) continue;

        LogEntry entry;
        entry.entryID = id++;
        entry.timestamp = line.substr(0, p1);
        entry.category = line.substr(p1 + 1, p2 - p1 - 1);
        entry.message = line.substr(p2 + 1);
        entry.lowerCategory = toLower(entry.category);
        entry.lowerMessage = toLower(entry.message);
        
        // Build indexes
        categoryIndex[entry.lowerCategory].push_back(entry.entryID);
        timestampIndex[entry.timestamp].push_back(entry.entryID);
        
        // Build word index - extract ALL alphanumeric words
        string combined = entry.category + " " + entry.message;
        string currentWord;
        
        for (unsigned char c : combined) {
            if (isalnum(c)) {
                currentWord += static_cast<char>(tolower(c));
            } else {
                if (!currentWord.empty()) {
                    wordIndex[currentWord].push_back(entry.entryID);
                    currentWord.clear();
                }
            }
        }
        if (!currentWord.empty()) {
            wordIndex[currentWord].push_back(entry.entryID);
        }
        
        masterLog.push_back(entry);
    }

    // Build sorted timestamp vector for range queries
    sortedTimestamps.reserve(masterLog.size());
    for (const auto &entry : masterLog) {
        sortedTimestamps.emplace_back(entry.timestamp, entry.entryID);
    }
    sort(sortedTimestamps.begin(), sortedTimestamps.end());
    
    // Sort word index lists for set_intersection
    for (auto& pair : wordIndex) {
        sort(pair.second.begin(), pair.second.end());
    }

    cout << masterLog.size() << " entries read\n";
}

// Run interactive command loop
void LogManager::runInteractive() {
    string cmd;
    cout << "% ";
    while (getline(cin, cmd)) {
        if (cmd == "q") break;
        processCommand(cmd);
        
        if (cin.fail()) {
            cerr << "cin entered fail state: exiting" << endl;
            exit(1);
        }
        
        cout << "% ";
    }
}

// Helper to sort search results (lazy - only when needed)
void LogManager::sortSearchResults() {
    if (resultsSorted) return;
    
    sort(lastSearchResults.begin(), lastSearchResults.end(), [&](size_t a, size_t b) {
        const auto &A = masterLog[a];
        const auto &B = masterLog[b];
        
        int tsCmp = A.timestamp.compare(B.timestamp);
        if (tsCmp != 0) return tsCmp < 0;
        
        int catCmp = A.lowerCategory.compare(B.lowerCategory);
        if (catCmp != 0) return catCmp < 0;
        
        int msgCmp = A.lowerMessage.compare(B.lowerMessage);
        if (msgCmp != 0) return msgCmp < 0;
        
        return A.entryID < B.entryID;
    });
    
    resultsSorted = true;
}

// Process a single command
void LogManager::processCommand(const string &cmd) {
    if (cmd.empty() || cmd[0] == '#') return;

    istringstream iss(cmd);
    char c;
    iss >> c;

    if (c == 't') {
        string range;
        getline(iss, range);
        if (!range.empty() && range[0] == ' ') range.erase(0, 1);
        searchTimestampRange(range);
    }
    else if (c == 'm') {
        string ts;
        iss >> ts;
        searchTimestampExact(ts);
    }
    else if (c == 'c') {
        string category;
        getline(iss, category);
        if (!category.empty() && category[0] == ' ') category.erase(0, 1);
        searchCategory(category);
    }
    else if (c == 'k') {
        string rest;
        getline(iss, rest);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
        searchKeywords(rest);
    }
    else if (c == 'a') {
        size_t id;
        if (iss >> id && id < masterLog.size()) {
            excerptList.push_back(id);
            cout << "log entry " << id << " appended\n";
        } else {
            cerr << "Invalid entryID\n";
        }
    }
    else if (c == 'r') {
        if (!hasSearched) {
            cerr << "No previous search results\n";
            return;
        }
        sortSearchResults();
        for (auto id : lastSearchResults)
            excerptList.push_back(id);
        cout << lastSearchResults.size() << " log entries appended\n";
    }
    else if (c == 'p') {
        for (size_t i = 0; i < excerptList.size(); ++i) {
            const auto &entry = masterLog[excerptList[i]];
            cout << i << "|" << entry.entryID << "|"
                << entry.timestamp << "|"
                << entry.category << "|"
                << entry.message << "\n";
        }
    }
    else if (c == 'g') {
        if (!hasSearched) {
            cerr << "No previous search results\n";
            return;
        }
        sortSearchResults();  // Lazy sort
        for (auto id : lastSearchResults) {
            const auto &entry = masterLog[id];
            cout << entry.entryID << "|"
                << entry.timestamp << "|"
                << entry.category << "|"
                << entry.message << "\n";
        }
    }
    else if (c == 'd') {
        size_t pos;
        if (iss >> pos && pos < excerptList.size()) {
            excerptList.erase(excerptList.begin() + static_cast<long>(pos));
            cout << "Deleted excerpt list entry " << pos << "\n";
        } else {
            cerr << "Invalid position\n";
        }
    }
    else if (c == 'b') {
        size_t pos;
        if (iss >> pos && pos < excerptList.size()) {
            auto val = excerptList[pos];
            excerptList.erase(excerptList.begin() + static_cast<long>(pos));
            excerptList.insert(excerptList.begin(), val);
            cout << "Moved excerpt list entry " << pos << "\n";
        } else {
            cerr << "Invalid position\n";
        }
    }
    else if (c == 'e') {
        size_t pos;
        if (iss >> pos && pos < excerptList.size()) {
            auto val = excerptList[pos];
            excerptList.erase(excerptList.begin() + static_cast<long>(pos));
            excerptList.push_back(val);
            cout << "Moved excerpt list entry " << pos << "\n";
        } else {
            cerr << "Invalid position\n";
        }
    }
    else if (c == 's') {
        cout << "excerpt list sorted\n";
        if (excerptList.empty()) {
            cout << "(previously empty)\n";
        } else {
            cout << "previous ordering:\n";
            const auto &first = masterLog[excerptList[0]];
            cout << "0|" << first.entryID << "|" << first.timestamp << "|"
                << first.category << "|" << first.message << "\n";
            cout << "...\n";
            const auto &last = masterLog[excerptList.back()];
            cout << (excerptList.size() - 1) << "|" << last.entryID << "|"
                << last.timestamp << "|" << last.category << "|" << last.message << "\n";
        }
        
        sort(excerptList.begin(), excerptList.end(), [&](size_t a, size_t b) {
            const auto &A = masterLog[a];
            const auto &B = masterLog[b];
            
            int tsCmp = A.timestamp.compare(B.timestamp);
            if (tsCmp != 0) return tsCmp < 0;
            
            int catCmp = A.lowerCategory.compare(B.lowerCategory);
            if (catCmp != 0) return catCmp < 0;
            
            int msgCmp = A.lowerMessage.compare(B.lowerMessage);
            if (msgCmp != 0) return msgCmp < 0;
            
            return A.entryID < B.entryID;
        });
        
        if (!excerptList.empty()) {
            cout << "new ordering:\n";
            const auto &first = masterLog[excerptList[0]];
            cout << "0|" << first.entryID << "|" << first.timestamp << "|"
                << first.category << "|" << first.message << "\n";
            cout << "...\n";
            const auto &last = masterLog[excerptList.back()];
            cout << (excerptList.size() - 1) << "|" << last.entryID << "|"
                << last.timestamp << "|" << last.category << "|" << last.message << "\n";
        }
    }
    else if (c == 'l') {
        cout << "excerpt list cleared\n";
        if (excerptList.empty()) {
            cout << "(previously empty)\n";
        } else {
            cout << "previous contents:\n";
            const auto &first = masterLog[excerptList[0]];
            cout << "0|" << first.entryID << "|" << first.timestamp << "|"
                << first.category << "|" << first.message << "\n";
            cout << "...\n";
            const auto &last = masterLog[excerptList.back()];
            cout << (excerptList.size() - 1) << "|" << last.entryID << "|"
                << last.timestamp << "|" << last.category << "|" << last.message << "\n";
        }
        excerptList.clear();
    }
    else {
        cerr << "Unknown command: " << c << '\n';
    }
}

// Search: timestamp range
void LogManager::searchTimestampRange(const string &range) {
    lastSearchResults.clear();
    hasSearched = true;
    resultsSorted = false;

    size_t bar = range.find('|');
    if (bar == string::npos) {
        cerr << "Bad timestamp range\n";
        cout << "Timestamps search: 0 entries found\n";
        return;
    }

    string start = range.substr(0, bar);
    string end = range.substr(bar + 1);
    
    if (start.length() != 14 || end.length() != 14) {
        cerr << "Invalid timestamp length\n";
        cout << "Timestamps search: 0 entries found\n";
        return;
    }

    auto start_it = lower_bound(sortedTimestamps.begin(), sortedTimestamps.end(),
                                make_pair(start, static_cast<size_t>(0)));
    auto end_it = upper_bound(sortedTimestamps.begin(), sortedTimestamps.end(),
                            make_pair(end, SIZE_MAX));

    for (auto it = start_it; it != end_it; ++it) {
        lastSearchResults.push_back(it->second);
    }

    cout << "Timestamps search: " << lastSearchResults.size() << " entries found\n";
}

// Search: exact timestamp
void LogManager::searchTimestampExact(const string &ts) {
    lastSearchResults.clear();
    hasSearched = true;
    resultsSorted = false;
    
    auto it = timestampIndex.find(ts);
    if (it != timestampIndex.end()) {
        lastSearchResults = it->second;
    }
    
    cout << "Timestamp search: " << lastSearchResults.size() << " entries found\n";
}

// Search: category
void LogManager::searchCategory(const string &cat) {
    lastSearchResults.clear();
    hasSearched = true;
    resultsSorted = false;
    
    string needle = toLower(cat);

    auto it = categoryIndex.find(needle);
    if (it != categoryIndex.end()) {
        lastSearchResults = it->second;
    }

    cout << "Category search: " << lastSearchResults.size() << " entries found\n";
}

// Search: keyword(s)
void LogManager::searchKeywords(const string &line) {
    lastSearchResults.clear();
    hasSearched = true;
    resultsSorted = false;

    // Trim leading and trailing spaces
    string input = line;
    while (!input.empty() && isspace(static_cast<unsigned char>(input.front()))) 
        input.erase(input.begin());
    while (!input.empty() && isspace(static_cast<unsigned char>(input.back()))) 
        input.pop_back();
    
    if (input.empty()) {
        cout << "Keyword search: 0 entries found\n";
        return;
    }

    // Extract keywords
    vector<string> keywords;
    string currentWord;
    
    for (unsigned char ch : input) {
        if (isalnum(ch)) {
            currentWord += static_cast<char>(tolower(ch));
        } else {
            if (!currentWord.empty()) {
                keywords.push_back(currentWord);
                currentWord.clear();
            }
        }
    }
    if (!currentWord.empty()) {
        keywords.push_back(currentWord);
    }

    if (keywords.empty()) {
        cout << "Keyword search: 0 entries found\n";
        return;
    }

    // Find smallest keyword list
    size_t minIdx = 0;
    size_t minSize = SIZE_MAX;
    
    for (size_t i = 0; i < keywords.size(); ++i) {
        auto it = wordIndex.find(keywords[i]);
        if (it == wordIndex.end()) {
            cout << "Keyword search: 0 entries found\n";
            return;
        }
        if (it->second.size() < minSize) {
            minSize = it->second.size();
            minIdx = i;
        }
    }
    
    // Start with smallest list
    const auto& minList = wordIndex[keywords[minIdx]];
    vector<size_t> result(minList.begin(), minList.end());
    
    // Intersect with other keywords
    vector<size_t> temp;
    for (size_t i = 0; i < keywords.size(); ++i) {
        if (i == minIdx) continue;
        
        const auto& currentList = wordIndex[keywords[i]];
        
        temp.clear();
        set_intersection(result.begin(), result.end(),
                        currentList.begin(), currentList.end(),
                        back_inserter(temp));
        
        if (temp.empty()) {
            cout << "Keyword search: 0 entries found\n";
            return;
        }
        
        result.swap(temp);
    }
    
    // Remove duplicates
    result.erase(unique(result.begin(), result.end()), result.end());
    
    lastSearchResults = move(result);
    cout << "Keyword search: " << lastSearchResults.size() << " entries found\n";
}