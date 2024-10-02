#ifndef LEDGER_H
#define LEDGER_H

#include <Arduino.h>
#include <vector>

class Ledger {
public:
    void init();
    void addEntry(const String& entry);
    String getLatestEntry() const;
    
private:
    std::vector<String> entries;
    const size_t MAX_ENTRIES = 7;
};

struct ledgerItem {
    String hash;
};

#endif