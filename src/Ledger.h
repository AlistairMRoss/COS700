#ifndef LEDGER_H
#define LEDGER_H

#include <Arduino.h>
#include <vector>

struct ledgerItem {
    String hash;
    String timestamp;
    String message;
};

class Ledger {
public:
    void init();
    void addEntry(const String& hash, const String& timestamp, const String& message = "");
    ledgerItem getLatestEntry() const;
    std::vector<ledgerItem> getAllEntries() const;
    
private:
    std::vector<ledgerItem> entries;
    const size_t MAX_ENTRIES = 7;
};

#endif