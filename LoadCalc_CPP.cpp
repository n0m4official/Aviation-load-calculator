// main.cpp  (C++17)
// Requires: json.hpp (nlohmann single-header)
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
using std::max;
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

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
    int widthSlots = 1;   // default 1 slot
    string deck = "Any";
    string notes = "";
};

struct ULD {
    string id = "";
    double weight = 0.0;  // initialize to 0
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

double promptDouble(const string& msg) {
    double v; string s;
    while (true) {
        cout << msg; getline(cin, s);
        try { v = stod(s); return v; }
        catch (...) { cout << "Enter a number.\n"; }
    }
}

// Saves the contents of lines to a file
bool saveLoadPlanToFile(const string& filename, const vector<string>& lines) {
    std::ofstream out(filename);
    if (!out.is_open()) return false;
    for (const auto& line : lines) {
        out << line << "\n";
    }
    out.close();
    return true;
}

// Print decks with 1-3 slots per row (top/bottom 1 slot), showing ULD ID and type
void printDeckColumnsASCII(const string& deckName, const Deck& deck, const vector<Slot>& slots, const vector<ULDDBEntry>& uldb, vector<string>* outputLines = nullptr) {
    auto writeLine = [&](const string& s) {
        cout << s << "\n";
        if (outputLines) outputLines->push_back(s);
        };

    writeLine("\n=== " + deckName + " Deck Load Plan (slots=" + std::to_string(deck.slots) + ") ===");
    if (deck.slots == 0) return;

    int boxWidth = 11;
    int boxHeight = 4;

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
        int totalWidth = boxWidth * (int)row.size();
        int leftPad = (deck.rowLength * boxWidth - totalWidth) / 2;

        // Top border
        string line = string(leftPad, ' ');
        for (auto* s : row) line += "+" + string(boxWidth - 2, '-');
        line += "+";
        writeLine(line);

        // ID + type line
        line = string(leftPad, ' ');
        for (auto* s : row) {
            string idLine = " ";
            if (s->occupied) {
                const ULDDBEntry* info = nullptr;
                for (auto& u : uldb) {
                    if (s->occupantId.rfind(u.prefix, 0) == 0) { info = &u; break; }
                }
                string typeStr = info ? "[" + info->uldType + "]" : "";
                string fullText = s->occupantId + typeStr;
                if ((int)fullText.size() > boxWidth - 2) fullText = fullText.substr(0, boxWidth - 2);
                idLine = fullText;
            }
            else if (s->slotType == SlotType::NOSE) idLine = "  N  ";
            else if (s->slotType == SlotType::TAIL) idLine = "  T  ";

            line += "|" + idLine + string(boxWidth - 1 - idLine.size(), ' ');
        }
        line += "|";
        writeLine(line);

        // Slot numbers line
        line = string(leftPad, ' ');
        for (auto* s : row) line += "|" + ("#" + std::to_string(s->index + 1)) + string(boxWidth - 1 - ("#" + std::to_string(s->index + 1)).size(), ' ');
        line += "|";
        writeLine(line);

        // Weight line
        line = string(leftPad, ' ');
        for (auto* s : row) {
            string w = s->occupied ? std::to_string((int)s->occupantWeight) : "";
            line += "|" + w + string(boxWidth - 1 - w.size(), ' ');
        }
        line += "|";
        writeLine(line);

        // Bottom border
        line = string(leftPad, ' ');
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
    auto ulddb = loadULDDB("ulddb.json");
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
    for (int i = 0; i < ac.mainDeck.slots; ++i) {
        mainSlots[i].deckName = "main";
        mainSlots[i].index = i;
        mainSlots[i].arm = ac.mainDeck.slotArms[i];
        if (i < ac.mainDeck.noseSlots) mainSlots[i].slotType = SlotType::NOSE;
        else if (i >= ac.mainDeck.slots - ac.mainDeck.tailSlots) mainSlots[i].slotType = SlotType::TAIL;
    }

    // lower deck
    for (int i = 0; i < ac.lowerDeck.slots; ++i) {
        lowerSlots[i].deckName = "lower";
        lowerSlots[i].index = i;
        lowerSlots[i].arm = ac.lowerDeck.slotArms[i];
        if (i < ac.lowerDeck.noseSlots) lowerSlots[i].slotType = SlotType::NOSE;
        else if (i >= ac.lowerDeck.slots - ac.lowerDeck.tailSlots) lowerSlots[i].slotType = SlotType::TAIL;
    }

    // input ULDs
    int nULDs; cout << "Number of ULDs: "; cin >> nULDs; cin.ignore();
    vector<ULD> ulds;
    for (int i = 0; i < nULDs; ++i) {
        ULD u;
        while (true) { cout << "ULD #" << i + 1 << " ID: "; getline(cin, u.id); if (!u.id.empty()) break; }
        u.weight = promptDouble("ULD " + u.id + " weight (kg): ");
        cout << "ULD type (MAIN / LOWER / ANY): ";
        string t; getline(cin, t);
        if (t == "MAIN") u.type = ULD::Type::MAIN;
        else if (t == "LOWER") u.type = ULD::Type::LOWER;
        else u.type = ULD::Type::ANY;
        cout << "Allow nose/tail? (y/n): "; getline(cin, t); u.allowSpecialSlots = (t == "y" || t == "Y");
        ulds.push_back(u);
    }

    // assign greedily
    vector<Slot*> freeSlots;
    for (auto& s : mainSlots) freeSlots.push_back(&s);
    for (auto& s : lowerSlots) freeSlots.push_back(&s);

    double avgArm = 0.0; for (auto* s : freeSlots) avgArm += s->arm; if (!freeSlots.empty()) avgArm /= freeSlots.size();
    double currentWeight = 0.0, currentMoment = 0.0;
    vector<pair<string, string>> report;

    for (auto& u : ulds) {
        int uWidth = getULDWidth(ulddb, u.id);
        // Add this line near the top of main(), after loading the aircraft DB
        double bestScore = 1e18;

        for (int i = 0; i <= (int)freeSlots.size() - uWidth; ++i) {
            bool canPlace = true;
            for (int w = 0; w < uWidth; ++w) {
                auto* s = freeSlots[i + w];
                if (s->occupied) { canPlace = false; break; }
                if (!u.allowSpecialSlots && (s->slotType == SlotType::NOSE || s->slotType == SlotType::TAIL)) canPlace = false;
                if (u.type == ULD::Type::MAIN && s->deckName != "main") canPlace = false;
                if (u.type == ULD::Type::LOWER && s->deckName != "lower") canPlace = false;
            }
            if (!canPlace) continue;

            double newCG = currentMoment;
            double totalWeight = currentWeight;
            for (int w = 0; w < uWidth; ++w) {
                newCG += u.weight / uWidth * freeSlots[i + w]->arm;
                totalWeight += u.weight / uWidth;
            }
            newCG /= totalWeight;
            double score = fabs(newCG - avgArm);
            if (score < bestScore) { bestScore = score; bestSlot = freeSlots[i]; }
        }

        if (bestSlot) {
            int startIdx = bestSlot->index;
            for (int w = 0; w < uWidth; ++w) {
                Slot* s = nullptr;
                for (auto* fs : freeSlots) if (fs->deckName == bestSlot->deckName && fs->index == startIdx + w) { s = fs; break; }
                if (s) { s->occupied = true; s->occupantId = u.id; s->occupantWeight = u.weight / uWidth; }
            }
            currentMoment += u.weight * bestSlot->arm;
            currentWeight += u.weight;
            report.emplace_back(u.id, bestSlot->deckName + "[" + to_string(bestSlot->index + 1) + "]");
        }
        else report.emplace_back(u.id, "UNASSIGNED");
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
