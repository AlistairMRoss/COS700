#include "Ledger.h"

void Ledger::init() {
    // Initialize ledger if needed
}

void Ledger::addEntry(const String& entry) {
    entries.push_back(entry);
    if (entries.size() > MAX_ENTRIES) {
        entries.erase(entries.begin());
    }
}

String Ledger::getLatestEntry() const {
    if (!entries.empty()) {
        return entries.back();
    }
    return "";
}