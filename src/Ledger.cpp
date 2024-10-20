#include "Ledger.h"

void Ledger::init() {
    // Initialization code if needed
}

void Ledger::addEntry(const String& hash, const String& timestamp, const String& message) {
    ledgerItem newItem = {hash, timestamp, message};
    entries.push_back(newItem);
    if (entries.size() > MAX_ENTRIES) {
        entries.erase(entries.begin());
    }
}

ledgerItem Ledger::getLatestEntry() const {
    if (!entries.empty()) {
        return entries.back();
    }
    return {"", "", ""};
}

std::vector<ledgerItem> Ledger::getAllEntries() const {
    return entries;
}