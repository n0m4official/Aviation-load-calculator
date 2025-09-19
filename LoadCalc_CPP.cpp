// main.cpp (C++17)
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cctype>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

// ===== Structs =====
struct Deck {
    int slots = 0;
    int rowLength = 8;
    vector<double> slotArms;
    int noseSlots = 0;
    int tailSlots = 0;
};

struct Aircraft {
    string model;
    Deck mainDeck;
    Deck lowerDeck;
    int mtw = 0;
};

struct ULDDBEntry {
    string prefix = "";
    string uldType = "";
    int widthSlots = 1;
    string deck = "Any";
    string notes = "";
};

struct ULD {
    string id = "";
    double weight = 0.0;
    enum class Type { MAIN, LOWER, ANY } type = Type::ANY;
    bool allowSpecialSlots = true;
};

enum class SlotType { NORMAL, NOSE, TAIL };

struct Slot {
    string deckName;
    int index = 0;
    double arm = 0.0;
    bool occupied = false;
    string occupantId = "";
    double occupantWeight = 0.0;
    SlotType slotType = SlotType::NORMAL;
};

// ANSI color codes
const string RESET = "\033[0m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE = "\033[34m";
const string MAGENTA = "\033[35m";
const string CYAN = "\033[36m";
const string WHITE = "\033[37m";
const string BOLD = "\033[1m";

map<string, string> uldTypeColors = {
    {"LD1", BLUE}, {"LD2", CYAN}, {"LD3", GREEN}, {"LD3-45", GREEN},
    {"LD4", MAGENTA}, {"LD6", YELLOW}, {"LD7", RED}, {"LD8", BOLD + CYAN},
    {"LD9", BOLD + GREEN}, {"LD11", BOLD + RED}, {"LD26", BOLD + MAGENTA},
    {"LD39", BOLD + YELLOW}, {"M1", CYAN}, {"M1H", BLUE}, {"M6", MAGENTA}
};

vector<ULDDBEntry> loadULDDB(const string& path) {
    vector<ULDDBEntry> db;
    ifstream in(path);
    if (!in) return db;

    json j;
    try { in >> j; }
    catch (...) { return db; }
    if (!j.is_array()) return db;

    for (auto& entry : j) {
        ULDDBEntry e;
        e.prefix = entry.value("Prefix", "");
        e.uldType = entry.value("ULD Type", "");
        e.widthSlots = entry.value("Width (slots)", 1);
        e.deck = entry.value("Deck", "Any");
        e.notes = entry.value("Notes", "");
        db.push_back(e);
    }
    return db;
}

// Helper to find widthSlots by ULD ID prefix
int getULDWidth(const vector<ULDDBEntry>& db, const string& uldId) {
    for (auto& e : db) {
        if (uldId.rfind(e.prefix, 0) == 0) return e.widthSlots; // prefix matches start of ULD ID
    }
    return 1; // default 1 slot
}

vector<double> generateDefaultArms(int n, double foreArm = 10.0, double aftArm = 40.0) {
    vector<double> arms;
    if (n <= 0) return arms;
    if (n == 1) { arms.push_back((foreArm + aftArm) / 2.0); return arms; }
    for (int i = 0; i < n; ++i) {
        double t = double(i) / double(n - 1);
        arms.push_back(foreArm * (1 - t) + aftArm * t);
    }
    return arms;
}

map<string, Aircraft> loadAircraftDB(const string& path) {
    map<string, Aircraft> db;
    ifstream in(path);
    if (!in) return db;
    json j;
    try { in >> j; }
    catch (...) { return db; }
    if (!j.is_array()) return db;

    for (auto& entry : j) {
        Aircraft a;
        a.model = entry.value("model", "");
        a.mtw = entry.value("mtw", 0);

        if (entry.contains("mainDeck")) {
            auto& m = entry["mainDeck"];
            a.mainDeck.slots = m.value("slots", 0);
            a.mainDeck.rowLength = m.value("rowLength", 8);
            a.mainDeck.noseSlots = m.value("noseSlots", 0);
            a.mainDeck.tailSlots = m.value("tailSlots", 0);
            if (m.contains("slotArms") && m["slotArms"].is_array())
                for (auto& v : m["slotArms"]) a.mainDeck.slotArms.push_back((double)v);
        }

        if (entry.contains("lowerDeck")) {
            auto& l = entry["lowerDeck"];
            a.lowerDeck.slots = l.value("slots", 0);
            a.lowerDeck.rowLength = l.value("rowLength", 8);
            a.lowerDeck.noseSlots = l.value("noseSlots", 0);
            a.lowerDeck.tailSlots = l.value("tailSlots", 0);
            if (l.contains("slotArms") && l["slotArms"].is_array())
                for (auto& v : l["slotArms"]) a.lowerDeck.slotArms.push_back((double)v);
        }

        if (!a.model.empty()) db[a.model] = a;
    }
    return db;
}

// ===== Utility =====
double promptDouble(const string& msg) {
    double v; string s;
    while (true) {
        cout << msg;
        getline(cin, s);
        try { v = stod(s); return v; }
        catch (...) { cout << "Enter a number.\n"; }
    }
}

int promptInt(const string& msg) {
    int v; string s;
    while (true) {
        cout << msg;
        getline(cin, s);
        try { v = stoi(s); return v; }
        catch (...) { cout << "Enter an integer.\n"; }
    }
}

bool saveLoadPlanToFile(const string& filename, const vector<string>& lines) {
    ofstream out(filename);
    if (!out.is_open()) return false;
    for (const auto& line : lines) out << line << "\n";
    return true;
}

// Print decks with 1-3 slots per row (top/bottom 1 slot), showing ULD ID and type
// Print decks with support for multi-slot ULDs
void printDeckColumnsASCII(const string& deckName, const Deck& deck,
    const vector<Slot>& slots, const vector<ULDDBEntry>& uldb,
    vector<string>* outputLines = nullptr)
{
    auto writeLine = [&](const string& s) {
        cout << s << "\n";
        if (outputLines) outputLines->push_back(s);
        };

    writeLine("\n=== " + deckName + " Deck Load Plan (slots=" + to_string(deck.slots) + ") ===");
    if (deck.slots == 0) return;

    int boxWidth = 11;

    // rows: center 3, edges 1
    vector<vector<const Slot*>> rows;
    if (!slots.empty()) rows.push_back({ &slots[0] });
    int idx = 1;
    while (idx < (int)slots.size() - 1) {
        int remaining = (int)slots.size() - 1 - idx;
        int rowSize = min(3, remaining);
        vector<const Slot*> row;
        for (int i = 0; i < rowSize; ++i) row.push_back(&slots[idx + i]);
        rows.push_back(row);
        idx += rowSize;
    }
    if (idx < (int)slots.size()) rows.push_back({ &slots.back() });

    for (auto& row : rows) {
        // Top border
        string line;
        for (auto* s : row) {
            line += "+";
            if (s->occupied && !s->occupantId.empty()) {
                int width = 1;
                for (size_t k = 1; k < row.size(); ++k) {
                    if (row[k - 1]->occupantId == row[k]->occupantId) width++;
                }
                line += string(boxWidth * width - 2, '-');
            }
            else {
                line += string(boxWidth - 2, '-');
            }
        }
        line += "+";
        writeLine(line);

        // Content lines (id/type)
        line.clear();
        for (auto* s : row) {
            if (s->occupied) {
                const ULDDBEntry* info = nullptr;
                for (auto& u : uldb) {
                    if (s->occupantId.rfind(u.prefix, 0) == 0) { info = &u; break; }
                }
                string typeStr = info ? "[" + info->uldType + "]" : "";
                string fullText = s->occupantId + typeStr;
                if ((int)fullText.size() > boxWidth - 2)
                    fullText = fullText.substr(0, boxWidth - 2);
                line += "|" + fullText + string(boxWidth - 1 - fullText.size(), ' ');
            }
            else {
                string idLine = (s->slotType == SlotType::NOSE ? "  N  " :
                    s->slotType == SlotType::TAIL ? "  T  " : "");
                line += "|" + idLine + string(boxWidth - 1 - idLine.size(), ' ');
            }
        }
        line += "|";
        writeLine(line);

        // Slot numbers
        line.clear();
        for (auto* s : row) {
            string n = "#" + to_string(s->index + 1);
            line += "|" + n + string(boxWidth - 1 - n.size(), ' ');
        }
        line += "|";
        writeLine(line);

        // Weights
        line.clear();
        for (auto* s : row) {
            string w = s->occupied ? to_string((int)s->occupantWeight) : "";
            line += "|" + w + string(boxWidth - 1 - w.size(), ' ');
        }
        line += "|";
        writeLine(line);

        // Bottom border
        line.clear();
        for (auto* s : row) line += "+" + string(boxWidth - 2, '-');
        line += "+";
        writeLine(line);
    }
}

// --- Assign nose/tail slots automatically ---
void assignSpecialSlots(Aircraft& ac) {
    int entryMainNoseSlots = 0;
    int entryMainTailSlots = 0;
    int entryLowerNoseSlots = 0;
    int entryLowerTailSlots = 0;

    // Any aircraft larger than B757 or A300 gets main deck special slots
    if (ac.mainDeck.slots > 0) {
        entryMainNoseSlots = 1;
        entryMainTailSlots = 1;
    }
    if (ac.lowerDeck.slots > 0) {
        entryLowerNoseSlots = 1;
        entryLowerTailSlots = 1;
    }

    // Create slot vectors
    vector<Slot> mainSlots(ac.mainDeck.slots);
    vector<Slot> lowerSlots(ac.lowerDeck.slots);

    // main deck
    for (int i = 0; i < ac.mainDeck.slots; ++i) {
        mainSlots[i].deckName = "main";
        mainSlots[i].index = i;
        mainSlots[i].arm = ac.mainDeck.slotArms[i];
        if (i < entryMainNoseSlots) mainSlots[i].slotType = SlotType::NOSE;
        else if (i >= ac.mainDeck.slots - entryMainTailSlots) mainSlots[i].slotType = SlotType::TAIL;
    }

    // lower deck
    for (int i = 0; i < ac.lowerDeck.slots; ++i) {
        lowerSlots[i].deckName = "lower";
        lowerSlots[i].index = i;
        lowerSlots[i].arm = ac.lowerDeck.slotArms[i];
        if (i < entryLowerNoseSlots) lowerSlots[i].slotType = SlotType::NOSE;
        else if (i >= ac.lowerDeck.slots - entryLowerTailSlots) lowerSlots[i].slotType = SlotType::TAIL;
    }

    // Return by reference (or use globally in main)
    ac.mainDeck.slotArms = ac.mainDeck.slotArms;
    ac.lowerDeck.slotArms = ac.lowerDeck.slotArms;
}


int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    auto ulddb = loadULDDB("uld_db.json");
    Slot* bestSlot = nullptr;
    cout << "=== Manual ULD Load Planner ===\n";

    auto db = loadAircraftDB("aircraft_db.json");
    if (!db.empty()) { cout << "Aircraft in DB:\n"; for (auto& kv : db) cout << " - " << kv.first << "\n"; }

    cout << "Enter aircraft model: ";
    string model; getline(cin, model);
    Aircraft ac;
    if (!model.empty() && db.count(model)) {
        ac = db[model];
        cout << "Using DB entry for " << model << "\n";
    }
    else {
        cout << "Custom aircraft\n";
        cout << "Main deck slots: "; cin >> ac.mainDeck.slots;
        cout << "Lower deck slots: "; cin >> ac.lowerDeck.slots; cin.ignore();
        ac.model = model.empty() ? "CUSTOM" : model;
    }

    if ((int)ac.mainDeck.slotArms.size() != ac.mainDeck.slots) ac.mainDeck.slotArms = generateDefaultArms(ac.mainDeck.slots, 18.0, 36.0);
    if ((int)ac.lowerDeck.slotArms.size() != ac.lowerDeck.slots) ac.lowerDeck.slotArms = generateDefaultArms(ac.lowerDeck.slots, 12.0, 28.0);

    // create slot vectors
    vector<Slot> mainSlots(ac.mainDeck.slots), lowerSlots(ac.lowerDeck.slots);

    // main deck
    // Collect all free slots into one vector
    vector<Slot*> freeSlots;
    for (auto& s : mainSlots) freeSlots.push_back(&s);
    for (auto& s : lowerSlots) freeSlots.push_back(&s);

    // assign greedily (safe version with multi-slot placement)
    vector<ULD> ulds;
    int nULDs = promptInt("Number of ULDs: ");
    for (int i = 0; i < nULDs; ++i) {
        ULD u;
        while (true) {
            cout << "ULD #" << (i + 1) << " ID: ";
            getline(cin, u.id);
            if (!u.id.empty()) break;
        }
        u.weight = promptDouble("ULD " + u.id + " weight (kg): ");
        cout << "ULD type (MAIN / LOWER / ANY): ";
        string t; getline(cin, t);
        // Normalize input to uppercase
        std::transform(t.begin(), t.end(), t.begin(), ::toupper);
        if (t == "MAIN") u.type = ULD::Type::MAIN;
        else if (t == "LOWER") u.type = ULD::Type::LOWER;
        else u.type = ULD::Type::ANY;
        cout << "Allow nose/tail? (y/n): "; getline(cin, t);
        // Accept "y", "Y", "yes", "YES" (case-insensitive)
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        u.allowSpecialSlots = (t == "y" || t == "yes");
        ulds.push_back(u);
    }

    // Add a check for empty DBs after loading:
    if (ulddb.empty()) {
        cout << RED << "Warning: ULD database is empty or missing. Multi-slot ULDs may not be recognized." << RESET << "\n";
    }
    if (db.empty()) {
        cout << RED << "Warning: Aircraft database is empty or missing. Only custom aircraft can be entered." << RESET << "\n";
    }

    double avgArm = 0.0;
    for (auto* s : freeSlots) avgArm += s->arm;
    if (!freeSlots.empty()) avgArm /= freeSlots.size();

    double currentWeight = 0.0, currentMoment = 0.0;
    vector<pair<string, string>> report;

    for (auto& u : ulds) {
        int uWidth = getULDWidth(ulddb, u.id);
        if (uWidth <= 0) uWidth = 1; ;

        // filter slots by deck type
        vector<Slot*> candidates;
        for (auto* s : freeSlots) {
            if ((u.type == ULD::Type::MAIN && s->deckName != "main") ||
                (u.type == ULD::Type::LOWER && s->deckName != "lower"))
                continue;
            if (!u.allowSpecialSlots && (s->slotType == SlotType::NOSE || s->slotType == SlotType::TAIL))
                continue;
            candidates.push_back(s);
        }

        std::sort(candidates.begin(), candidates.end(),
            [](Slot* a, Slot* b) { return a->index < b->index; });

        bool placed = false;
        for (size_t i = 0; i + uWidth <= candidates.size(); ++i) {
            bool consecutive = true;
            for (int w = 1; w < uWidth; ++w) {
                if (candidates[i + w]->index != candidates[i + w - 1]->index + 1) {
                    consecutive = false; break;
                }
            }
            if (!consecutive) continue;

            // place ULD
            for (int w = 0; w < uWidth; ++w) {
                candidates[i + w]->occupied = true;
                candidates[i + w]->occupantId = u.id;
                candidates[i + w]->occupantWeight = u.weight / uWidth;
            }

            currentWeight += u.weight;
            currentMoment += u.weight * candidates[i]->arm;

            report.emplace_back(u.id,
                candidates[i]->deckName + "[" + to_string(candidates[i]->index + 1) + "]");
            placed = true;
            break;
        }

        if (!placed) {
            report.emplace_back(u.id, "UNASSIGNED");
        }

        // cleanup: remove occupied from freeSlots
        freeSlots.erase(std::remove_if(freeSlots.begin(), freeSlots.end(),
            [](Slot* s) { return s->occupied; }), freeSlots.end());
    }

    // report
    cout << "\n=== Assignment Results ===\n";
    cout << left << setw(12) << "ULD ID" << setw(22) << "Assigned Slot" << setw(10) << "Weight(kg)" << "\n";
    cout << string(46, '-') << "\n";
    for (auto& x : report) {
        double w = 0; for (auto& u : ulds) if (u.id == x.first) { w = u.weight; break; }
        cout << left << setw(12) << x.first << setw(22) << x.second << setw(10) << w << "\n";
    }

    // Print decks
    vector<string> loadPlanLines;
    printDeckColumnsASCII("Main", ac.mainDeck, mainSlots, ulddb, &loadPlanLines);
    printDeckColumnsASCII("Lower", ac.lowerDeck, lowerSlots, ulddb, &loadPlanLines);

    if (saveLoadPlanToFile("loadplan.txt", loadPlanLines)) {
        cout << "Load plan saved to loadplan.txt\n";
    }
    else {
        cout << "Failed to save load plan.\n";
    }

    cout << "\nDone.\n";
    return 0;
}
