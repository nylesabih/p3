// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct LogEntry {
    size_t entryID;
    std::string timestamp;
    std::string category;
    std::string message;
    std::string lowerCategory;  // Pre-computed lowercase
    std::string lowerMessage;   // Pre-computed lowercase
};

class LogManager {
public:
    // Core functions
    void loadLogFile(const std::string& filename);
    void runInteractive();

private:
    // Primary containers
    std::vector<LogEntry> masterLog;          // all log entries
    std::vector<size_t> excerptList;          // indices into masterLog
    std::vector<size_t> lastSearchResults;    // from most recent search
    bool hasSearched = false;                 // track if any search has occurred
    bool resultsSorted = false;               // track if lastSearchResults is already sorted
    
    // Indexes for fast lookups
    std::unordered_map<std::string, std::vector<size_t>> categoryIndex;  // category -> entry IDs
    std::unordered_map<std::string, std::vector<size_t>> timestampIndex; // timestamp -> entry IDs
    std::unordered_map<std::string, std::vector<size_t>> wordIndex;      // word -> entry IDs (for keyword search)
    std::vector<std::pair<std::string, size_t>> sortedTimestamps;        // sorted (timestamp, entryID) for range queries

    // Command processor
    void processCommand(const std::string& cmd);

    // Search helpers
    void searchTimestampRange(const std::string& range);
    void searchTimestampExact(const std::string& ts);
    void searchCategory(const std::string& cat);
    void searchKeywords(const std::string& line);
    
    // Helper to sort search results
    void sortSearchResults();
};