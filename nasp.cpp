#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <random>
#include <algorithm>
#include <limits>

namespace fs = std::filesystem;
using namespace std;
using namespace std::chrono;

static const int MAX_LEVEL = 16;
static const double P = 0.5;

// ===================== SKIP LIST NODE =====================
struct SkipNode {
    double key;
    vector<SkipNode*> forward;

    SkipNode(double k, int level)
        : key(k), forward(level + 1, nullptr) {
    }
};

// ===================== SKIP LIST =====================
class SkipList {
private:
    int level;
    SkipNode* header;
    mt19937 rng;

    int randomLevel() {
        int lvl = 0;
        while (((double)rng() / rng.max()) < P && lvl < MAX_LEVEL)
            lvl++;
        return lvl;
    }

public:
    SkipList() : level(0), rng(random_device{}()) {
        header = new SkipNode(numeric_limits<double>::lowest(), MAX_LEVEL);
    }

    ~SkipList() {
        SkipNode* curr = header->forward[0];
        while (curr) {
            SkipNode* next = curr->forward[0];
            delete curr;
            curr = next;
        }
        delete header;
    }

    // -------- INSERT --------
    void insert(double key, bool randomize) {
        vector<SkipNode*> update(MAX_LEVEL + 1);
        SkipNode* current = header;

        for (int i = level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
            update[i] = current;
        }

        current = current->forward[0];

        if (!current || current->key != key) {
            int newLevel = randomize ? randomLevel() : 0;

            if (newLevel > level) {
                for (int i = level + 1; i <= newLevel; i++)
                    update[i] = header;
                level = newLevel;
            }

            SkipNode* newNode = new SkipNode(key, newLevel);
            for (int i = 0; i <= newLevel; i++) {
                newNode->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = newNode;
            }
        }
    }

    // -------- SEARCH --------
    bool search(double key) {
        SkipNode* current = header;
        for (int i = level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
        }
        current = current->forward[0];
        return current && current->key == key;
    }

    // -------- DELETE --------
    void remove(double key) {
        vector<SkipNode*> update(MAX_LEVEL + 1);
        SkipNode* current = header;

        for (int i = level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
            update[i] = current;
        }

        current = current->forward[0];

        if (current && current->key == key) {
            for (int i = 0; i <= level; i++) {
                if (update[i]->forward[i] != current)
                    break;
                update[i]->forward[i] = current->forward[i];
            }

            delete current;

            while (level > 0 && header->forward[level] == nullptr)
                level--;
        }
    }
};

// ===================== UCITAVANJE PODATAKA =====================
vector<double> ucitaj_podatke(const string& putanja) {
    vector<double> rezultati;
    ifstream fajl(putanja);
    if (!fajl.is_open()) return rezultati;

    string sadrzaj;
    stringstream ss_buf;
    ss_buf << fajl.rdbuf();
    sadrzaj = ss_buf.str();
    replace(sadrzaj.begin(), sadrzaj.end(), ',', ' ');

    stringstream ss(sadrzaj);
    double broj;
    while (ss >> broj) rezultati.push_back(broj);

    return rezultati;
}

// ===================== MAIN =====================
int main() {

    string path = "Testiranje";

    if (!fs::exists(path)) {
        cout << "GRESKA: Putanja ne postoji!" << endl;
        return 1;
    }

    ofstream csv("rezultati2_skiplist.csv");
    csv << "Struktura,Verzija,Distribucija,Datoteka,N,Vrijeme_ms\n";

    vector<fs::path> datoteke;
    for (const auto& e : fs::recursive_directory_iterator(path))
        if (e.is_regular_file())
            datoteke.push_back(e.path());

    sort(datoteke.begin(), datoteke.end());

    int broj_pokretanja = 5;

    for (const auto& f : datoteke) {

        vector<double> data = ucitaj_podatke(f.string());
        if (data.empty()) continue;

        string dat = f.filename().string();
        string dist = f.parent_path().filename().string();
        int N = data.size();

        cout << "Testiram: " << dat << " (N=" << N << ")" << endl;

        auto benchmark = [&](string verzija, bool randomize) {

            long long ukupno = 0;

            for (int i = 0; i < broj_pokretanja; i++) {
                SkipList sl;

                auto start = high_resolution_clock::now();

                for (double x : data) sl.insert(x, randomize);
                for (double x : data) sl.search(x);
                for (double x : data) sl.remove(x);

                auto stop = high_resolution_clock::now();
                ukupno += duration_cast<milliseconds>(stop - start).count();
            }

            csv << "SkipList," << verzija << "," << dist << ","
                << dat << "," << N << ","
                << (double)ukupno / broj_pokretanja << "ms\n";
            csv.flush();
            };

        benchmark("Random", true);
        benchmark("Deterministic", false);
    }

    csv.close();
    cout << "\nTestiranje zavrseno! Rezultati su u 'rezultati_skiplist.csv'." << endl;
    return 0;
}
