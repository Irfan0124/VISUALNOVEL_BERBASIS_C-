/*
 * ============================================================
 *  MANGA RESET: Dunia yang Kulupa
 *  Terminal Story Game — C++
 * ============================================================
 *  Struktur Data yang Digunakan:
 *  1. Array       : pool event per chapter, array pilihan
 *  2. Struct      : Event, Choice, Character, GameState, ChapterNode
 *  3. Pointer     : navigasi linked list (EventNode*)
 *  4. Linked List : daftar event aktif per sesi (Single Linked List)
 *  5. Stack       : save/restore GameState saat restart
 *  6. Queue       : display monolog restart baris per baris
 *  7. Graph/Tree  : struktur chapter sebagai directed graph
 *
 *  CRUD:
 *  - Tampilkan : render event aktif + poin ke layar
 *  - Tambah    : insertNode() saat random pick event
 *  - Ubah      : update poin di GameState setiap pilihan
 *  - Hapus     : deleteNode() setelah event selesai
 * ============================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <algorithm>

using namespace std;

// ============================================================
// KONSTANTA
// ============================================================

const int MAX_CHOICES    = 5;
const int POOL_SIZE      = 10;
const int MIN_EVENTS     = 4;
const int MAX_EVENTS     = 6;
const int STARTING_POINT = 50;
const int WIN_POINT      = 100;

// ============================================================
// STRUCT DEFINITIONS
// ============================================================

// Struct: satu pilihan dalam sebuah event
struct Choice {
    string text;
    int    pointDelta;   // poin yang berubah saat pilihan ini dipilih
};

// Struct: satu event / aktivitas yang bisa dipilih player
struct Event {
    int    id;
    string title;        // nama event di menu
    string narration;    // narasi saat event dibuka
    Choice choices[MAX_CHOICES];
    int    choiceCount;
};

// Struct: karakter dalam cerita
struct Character {
    string name;
    string role;
    string description;
};

// Struct: state game saat ini
struct GameState {
    int  currentChapter;   // 1 atau 2
    int  points;           // poin saat ini
    int  restartCount;     // total restart sepanjang game
    bool gameOver;
    bool reachedEnding;
};

// ============================================================
// LINKED LIST — Event Aktif
// ============================================================

struct EventNode {
    Event       data;
    EventNode*  next;
};

struct ActiveEventList {
    EventNode* head;
    int        count;

    ActiveEventList() : head(nullptr), count(0) {}

    // CRUD: Tambah — insert node baru di akhir list
    void insertNode(const Event& e) {
        EventNode* node = new EventNode{e, nullptr};
        if (!head) {
            head = node;
        } else {
            EventNode* cur = head;
            while (cur->next) cur = cur->next;
            cur->next = node;
        }
        count++;
    }

    // CRUD: Hapus — hapus node berdasarkan id event
    bool deleteNode(int eventId) {
        if (!head) return false;
        if (head->data.id == eventId) {
            EventNode* tmp = head;
            head = head->next;
            delete tmp;
            count--;
            return true;
        }
        EventNode* cur = head;
        while (cur->next && cur->next->data.id != eventId) {
            cur = cur->next;
        }
        if (cur->next) {
            EventNode* tmp = cur->next;
            cur->next = tmp->next;
            delete tmp;
            count--;
            return true;
        }
        return false;
    }

    // CRUD: Tampilkan — cetak semua event aktif sebagai menu
    void displayMenu() const {
        cout << "\n  Apa yang ingin kamu lakukan?\n";
        cout << "  ------------------------------\n";
        EventNode* cur = head;
        int i = 1;
        while (cur) {
            cout << "  [" << i << "] " << cur->data.title << "\n";
            cur = cur->next;
            i++;
        }
        cout << "  ------------------------------\n";
    }

    // Ambil event berdasarkan posisi menu (1-based)
    Event* getByMenuIndex(int idx) {
        EventNode* cur = head;
        int i = 1;
        while (cur) {
            if (i == idx) return &cur->data;
            cur = cur->next;
            i++;
        }
        return nullptr;
    }

    bool isEmpty() const { return head == nullptr; }

    ~ActiveEventList() {
        EventNode* cur = head;
        while (cur) {
            EventNode* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }
};

// ============================================================
// STACK — Save/Restore GameState
// ============================================================

struct StateStack {
    stack<GameState> stk;

    void save(const GameState& gs) {
        stk.push(gs);
    }

    // Ambil state awal chapter (bottom of stack untuk chapter ini)
    GameState getChapterInitial() {
        if (stk.empty()) return {1, STARTING_POINT, 0, false, false};
        return stk.top();
    }

    void clear() {
        while (!stk.empty()) stk.pop();
    }
};

// ============================================================
// QUEUE — Display Monolog Restart
// ============================================================

void displayMonologQueue(const vector<string>& lines) {
    queue<string> q;
    for (const auto& l : lines) q.push(l);
    cout << "\n";
    while (!q.empty()) {
        cout << "  " << q.front() << "\n";
        q.pop();
    }
    cout << "\n";
}

// ============================================================
// GRAPH — Struktur Chapter
// ============================================================
/*
 *  Directed Graph:
 *
 *  [Chapter 1] --( poin >= 100 )--> [Chapter 2] --( poin >= 100 )--> [Ending]
 *       ^                                ^
 *       |                                |
 *  (poin <= 0, restart)           (poin <= 0, restart)
 */

struct ChapterNode {
    int         id;          // 1, 2, atau 99 (ending)
    string      title;
    string      setting;
    int         nextChapterId;   // id chapter berikutnya jika menang
    int         failChapterId;   // id chapter yang diulang jika kalah
};

// Graph chapter disimpan sebagai array of nodes
const int CHAPTER_COUNT = 3;
ChapterNode chapterGraph[CHAPTER_COUNT];

void buildChapterGraph() {
    chapterGraph[0] = {1, "Chapter 1: Hari Pertama sebagai Haru",
                       "Pagi hari di rumah keluarga Mizushima", 2, 1};
    chapterGraph[1] = {2, "Chapter 2: Antara Sekolah dan Impian",
                       "Sekolah dan sepulang sekolah",           99, 2};
    chapterGraph[2] = {99, "Ending: Perjanjian dengan Ayah",
                       "Rumah — malam hari",                     -1, -1};
}

ChapterNode* findChapterNode(int id) {
    for (int i = 0; i < CHAPTER_COUNT; i++) {
        if (chapterGraph[i].id == id) return &chapterGraph[i];
    }
    return nullptr;
}

// ============================================================
// DATA: KARAKTER
// ============================================================

Character characters[] = {
    {"Haru Mizushima", "protagonist",
     "Pelajar SMA kelas 3. Pemimpi diam-diam yang ingin jadi content creator."},
    {"Kenji Mizushima", "npc",
     "Ayah Haru. Keras, disiplin, tapi menyimpan kepedulian yang dalam."},
    {"Ibu Mizushima", "npc",
     "Ibu Haru. Lembut, penengah, sering diam tapi selalu memperhatikan."},
    {"Hana Mizushima", "npc",
     "Kakak Haru, 21 tahun. Standar keluarga. Bekerja di perusahaan bagus."},
    {"Dito", "npc",
     "Teman sekelas Haru. Santai, sering ngajak main."},
    {"Nisa", "npc",
     "Teman sekelas Haru. Energik, update soal tren konten."},
    {"Kaito", "npc",
     "Content creator SMA yang menginspirasi Haru."},
};

// ============================================================
// DATA: POOL EVENT CHAPTER 1 (10 event)
// ============================================================

Event poolChapter1[POOL_SIZE];

void buildPoolChapter1() {

    // Event 0: Alarm pagi
    poolChapter1[0] = {
        100, "Matikan alarm & mulai pagi",
        "Alarm berbunyi pukul 05.30. Di meja ada jadwal harian\n"
        "  yang ditulis Ayah dengan spidol merah.\n"
        "  '05.45 - Olahraga pagi (lari 2 km)'\n"
        "  Di pojok kertas: 'Terlambat = tidak ada uang jajan seminggu.'\n\n"
        "  Kamu menatap jadwal itu. Masih ngantuk.",
        {
            {"Langsung bangun dan mulai lari sesuai jadwal.", +25},
            {"Tidur lagi 15 menit, lari bisa nanti.", -20},
            {"Lari di tempat saja di kamar — lebih praktis.", +5},
        }, 3
    };

    // Event 1: Sarapan & pertanyaan Ayah
    poolChapter1[1] = {
        101, "Sarapan bersama Ayah",
        "Meja makan. Ayah menaruh koran dan menatapmu.\n"
        "  \"Haru. Nilai ulangan matematika minggu lalu.\"\n"
        "  Bukan pertanyaan. Pernyataan. Ia sudah tahu nilaimu: 78.",
        {
            {"Jawab jujur: \"78, Ayah. Aku akan lebih giat.\"", +25},
            {"Diam saja dan terus makan.", -15},
            {"Berbohong: \"Sudah bagus, di atas rata-rata kelas.\"", -20},
            {"Alihkan topik: \"Ibu, masakannya enak hari ini.\"", -10},
        }, 4
    };

    // Event 2: Ngobrol dengan Hana
    poolChapter1[2] = {
        102, "Ngobrol dengan Hana (kakak)",
        "Hana sudah rapi dengan seragam kantornya.\n"
        "  Ia menuang kopi sambil berkata pelan:\n"
        "  \"Haru, Ayah lagi banyak pikiran soal kerjaan.\n"
        "   Jangan bikin masalah dulu, ya.\"\n\n"
        "  Kamu menatapnya.",
        {
            {"Angguk dan bilang: \"Iya, Kak. Makasih info-nya.\"", +20},
            {"Balik ke kamar tanpa menjawab.", -10},
            {"Tanya balik: \"Emang kenapa, Kak? Cerita dong.\"", +15},
            {"Senyum saja tanpa berkata apa-apa.", +5},
        }, 4
    };

    // Event 3: Menemukan video Kaito
    poolChapter1[3] = {
        103, "Buka HP sebelum berangkat",
        "Kamu sempat buka HP sebelum berangkat.\n"
        "  Notifikasi: Kaito upload video baru.\n"
        "  Judulnya: 'Cara Aku Mulai dari 0 Subscriber'\n\n"
        "  Ayah ada di ruang sebelah. Waktu 10 menit sebelum berangkat.",
        {
            {"Tonton sebentar — 10 menit cukup untuk highlight-nya.", +15},
            {"Simpan dulu untuk nonton nanti, sekarang fokus berangkat.", +20},
            {"Langsung balas komentar di videonya.", -10},
            {"Tutup HP dan lupakan.", -5},
        }, 4
    };

    // Event 4: Jadwal tambahan Ayah
    poolChapter1[4] = {
        104, "Ayah menambahkan jadwal baru",
        "Ayah keluar dari kamarnya dengan selembar kertas.\n"
        "  \"Mulai minggu ini, tambah satu jam belajar malam. Jam 8 sampai 9.\"\n"
        "  Ia menempelkan kertas itu di lemari.\n\n"
        "  Itu artinya kamu tidak bisa rekam konten malam hari.",
        {
            {"\"Baik, Ayah.\" — Terima tanpa komplain.", +20},
            {"\"Tapi Ayah, aku sudah punya jadwal sendiri—\" (potong di tengah)", -20},
            {"Diam, tapi sudah mulai berpikir cara menyiasatinya.", +10},
            {"\"Boleh aku belajar sendiri caranya, Ayah?\"", +15},
        }, 4
    };

    // Event 5: Bertemu Ibu di dapur
    poolChapter1[5] = {
        105, "Bantu Ibu di dapur",
        "Ibu sedang mencuci piring sendirian.\n"
        "  Kamu bisa langsung pergi ke kamar,\n"
        "  atau sebentar bantu.",
        {
            {"Langsung tawarkan diri bantu cuci piring.", +25},
            {"Bilang mau bantu tapi malah cuma berdiri.", +5},
            {"Langsung ke kamar — ada yang ingin dikerjakan.", -5},
            {"Tanya Ibu soal Ayah: \"Ibu, Ayah kenapa akhir-akhir ini...\"", +15},
        }, 4
    };

    // Event 6: Laptop tersembunyi
    poolChapter1[6] = {
        106, "Cek laptop tersembunyi",
        "Di balik tumpukan binder, laptop tua itu menunggumu.\n"
        "  Channel YouTube: 47 subscriber.\n"
        "  Ada draft video yang belum diupload.\n"
        "  Suara rumah sudah mulai sepi.",
        {
            {"Edit draft video dengan teliti, tapi jangan upload dulu.", +20},
            {"Upload langsung tanpa diedit.", -15},
            {"Tutup laptop — terlalu berisiko sekarang.", +5},
            {"Rekam video baru dengan berbisik pelan.", +25},
            {"Buka komentar lama dan baca satu per satu.", +10},
        }, 5
    };

    // Event 7: Telepon teman
    poolChapter1[7] = {
        107, "Dito telepon mengajak keluar",
        "HP bergetar. Dito nelpon.\n"
        "  \"Haru! Nongkrong yuk, ada yang baru buka dekat sekolah.\n"
        "   Nisa juga ikut. Santai aja, paling sejam.\"\n\n"
        "  Ayah ada di rumah.",
        {
            {"Tolak dengan sopan: \"Nggak bisa hari ini, Dit.\"", +20},
            {"Langsung iya tanpa pikir panjang.", -25},
            {"Minta waktu dua jam lagi, baru keluar.", -10},
            {"Tanya dulu ke Ibu apakah boleh keluar.", +15},
        }, 4
    };

    // Event 8: Malam — belajar atau konten
    poolChapter1[8] = {
        108, "Pilihan malam: belajar atau konten",
        "Pukul 20.00. Satu jam bebas sebelum jadwal belajar Ayah.\n"
        "  Di mejamu: buku matematika dan laptop tersembunyi.\n"
        "  Kamu hanya bisa pilih satu.",
        {
            {"Belajar matematika dulu — nilai 78 harus naik.", +20},
            {"Buka laptop — satu jam cukup untuk rekam intro video.", +15},
            {"Tiduran sambil scroll HP.", -20},
            {"Belajar setengah jam, sisanya buat catatan ide konten.", +25},
        }, 4
    };

    // Event 9: Cermin pagi
    poolChapter1[9] = {
        109, "Momen depan cermin",
        "Kamu berdiri di depan cermin kamar mandi.\n"
        "  Wajah Haru menatap balik.\n"
        "  Untuk pertama kali, kamu sadar betapa beratnya hidup\n"
        "  yang sedang kamu jalani — sebagai dirinya.\n\n"
        "  Kamu bicara ke cermin.",
        {
            {"\"Aku bisa. Haru pasti bisa.\" — Ucapkan dengan yakin.", +25},
            {"Diam saja dan pergi.", 0},
            {"\"Kenapa hidup ini harus sekeras ini?\" — Keluh.", -10},
            {"Latihan senyum untuk kamera video.", +15},
        }, 4
    };
}

// ============================================================
// DATA: POOL EVENT CHAPTER 2 (10 event)
// ============================================================

Event poolChapter2[POOL_SIZE];

void buildPoolChapter2() {

    // Event 0: Pagi di sekolah
    poolChapter2[0] = {
        200, "Pagi di kelas — ulangan mendadak",
        "Guru masuk dan langsung taruh lembar soal.\n"
        "  \"Ulangan mendadak. Materi dua minggu terakhir.\"\n"
        "  Kelas langsung riuh. Kamu belum review sama sekali.",
        {
            {"Tetap tenang — kerjakan semampu yang diingat.", +20},
            {"Coba lirik jawaban Nisa yang duduk di sebelah.", -25},
            {"Kosongin yang tidak tahu, fokus yang bisa dikerjakan.", +15},
            {"Minta izin ke toilet untuk buka catatan HP.", -20},
        }, 4
    };

    // Event 1: Istirahat — Dito ajak nonton live Kaito
    poolChapter2[1] = {
        201, "Istirahat — Kaito lagi live",
        "Nisa lari ke mejamu sambil pegang HP.\n"
        "  \"Haru! Kaito lagi live! Dia lagi bahas cara monetisasi\n"
        "   channel kecil. Ini pas banget buat kamu!\"\n\n"
        "  Masih ada 20 menit istirahat.",
        {
            {"Tonton live-nya sebentar — ini ilmu yang berguna.", +20},
            {"Minta Nisa rekam/screenshot bagian pentingnya saja.", +15},
            {"Ignore — fokus review pelajaran untuk jam selanjutnya.", +10},
            {"Tonton sambil makan, tapi setengah konsentrasi.", +5},
        }, 4
    };

    // Event 2: Guru BK memanggil
    poolChapter2[2] = {
        202, "Dipanggil Guru BK",
        "\"Haru, masuk sebentar.\"\n\n"
        "  Guru BK menutup pintu. Di mejanya ada print-out.\n"
        "  Nilai Haru semester ini. Rata-rata: 76.\n"
        "  \"Kamu siswa yang cerdas. Tapi kenapa nilainya segini?\"",
        {
            {"Jujur: \"Aku sedang cari cara seimbangkan semua hal, Bu.\"", +25},
            {"Diam dan mengangguk saja.", -5},
            {"Berjanji nilai akan naik tanpa penjelasan.", +10},
            {"Alihkan: \"Saya tidak tahu, Bu. Mungkin soalnya sulit.\"", -15},
        }, 4
    };

    // Event 3: Nisa tanya soal channel
    poolChapter2[3] = {
        203, "Nisa tanya soal channel YouTube",
        "Nisa duduk di sebelahmu waktu pulang.\n"
        "  \"Haru, aku tahu kamu punya channel YouTube.\n"
        "   Aku nggak sengaja nemu. Videomu... bagus, Har.\n"
        "   Kenapa nggak pernah cerita?\"\n\n"
        "  Jantungmu berdegup.",
        {
            {"Ceritakan semuanya — impian, ketakutan, dan rencana.", +25},
            {"Minta dia diam dan tidak cerita ke siapapun.", -10},
            {"Pura-pura tidak tahu channel yang dimaksud.", -20},
            {"Bilang itu cuma iseng, tidak serius.", -5},
            {"Minta pendapat Nisa soal video-videomu.", +20},
        }, 5
    };

    // Event 4: Pulang — ketemu Ayah di depan pintu
    poolChapter2[4] = {
        204, "Ketemu Ayah di depan pintu",
        "Ayah berdiri di depan pintu saat kamu pulang.\n"
        "  Jam 16.05. Lima menit terlambat dari jadwal.\n"
        "  Ia menatapmu tanpa kata-kata.",
        {
            {"Minta maaf langsung dan jelaskan alasannya.", +20},
            {"Pura-pura tidak sadar soal keterlambatan.", -20},
            {"Diam dan masuk rumah dengan cepat.", -10},
            {"\"Maaf, Ayah. Ada yang perlu aku ceritakan.\"", +25},
        }, 4
    };

    // Event 5: Sore — tawaran kolaborasi dari Kaito
    poolChapter2[5] = {
        205, "DM dari Kaito",
        "HP-mu berbunyi. DM masuk.\n"
        "  Dari: @KaitoCreates\n"
        "  \"Hei, aku nemu channel-mu. Kontenmu raw dan jujur.\n"
        "   Mau kolaborasi video bareng minggu depan?\"\n\n"
        "  Tanganmu gemetar sedikit.",
        {
            {"Balas: \"Mau banget! Kapan dan di mana?\"", -15},
            {"Balas: \"Terima kasih. Boleh aku pikir dulu?\"", +25},
            {"Tidak balas dulu — terlalu overwhelmed.", -5},
            {"Balas dan langsung tanya soal teknis kolaborasi.", +15},
        }, 4
    };

    // Event 6: Malam — konfrontasi tagihan internet
    poolChapter2[6] = {
        206, "Ayah tunjukkan tagihan internet",
        "Makan malam. Ayah menaruh kertas di tengah meja.\n"
        "  Tagihan internet. Ada highlight di satu baris:\n"
        "  akses ke platform video, pukul 22.15, 47 menit.\n\n"
        "  \"Jelaskan.\"",
        {
            {"Minta maaf dan berjanji tidak mengulangi.", -10},
            {"Tunjukkan statistik channel: 47 subscriber, engagement rate.", +25},
            {"Marah: \"Aku hanya ingin sedikit kebebasan!\"", -25},
            {"Diam dan terima hukuman.", -15},
            {"\"Ayah, boleh aku cerita sesuatu?\" — minta bicara baik-baik.", +20},
        }, 5
    };

    // Event 7: Nulis di buku catatan
    poolChapter2[7] = {
        207, "Nulis rencana di buku catatan",
        "Kamu buka buku catatan dan mulai nulis.\n"
        "  Dua kolom: AKADEMIK dan KONTEN.\n"
        "  Kalau ada proposal konkret untuk Ayah,\n"
        "  mungkin ia mau mendengarkan.",
        {
            {"Tulis target nilai + jadwal konten yang realistis.", +25},
            {"Nulis impian besar tanpa detail konkret.", +5},
            {"Tutup buku — terlalu lelah untuk berpikir malam ini.", -10},
            {"Tulis surat untuk Ayah alih-alih proposal.", +15},
        }, 4
    };

    // Event 8: Malam — rekam video rahasia
    poolChapter2[8] = {
        208, "Rekam video rahasia malam ini",
        "Pukul 22.00. Rumah sepi.\n"
        "  Laptop terbuka. Kamera siap.\n"
        "  Tapi kali ini bukan sekadar vlog —\n"
        "  kamu ingin bikin sesuatu yang benar-benar jujur.",
        {
            {"Rekam video curahan hati soal keluarga dan impian.", +20},
            {"Rekam tapi tidak upload — simpan untuk diri sendiri.", +15},
            {"Upload video lama yang sudah diedit.", +5},
            {"Matikan laptop — terlalu berisiko.", -5},
        }, 4
    };

    // Event 9: Pagi sebelum sekolah — momen dengan Ibu
    poolChapter2[9] = {
        209, "Momen pagi bersama Ibu",
        "Ibu duduk sendiri di dapur pagi-pagi.\n"
        "  Ia memegang cangkir teh.\n"
        "  Saat kamu masuk, ia tersenyum.\n"
        "  \"Haru, Ibu mau bicara sebentar.\"",
        {
            {"Duduk dan dengarkan.", +25},
            {"\"Maaf Bu, buru-buru. Nanti saja ya.\"", -10},
            {"Duduk tapi sambil lihat HP.", -5},
            {"Tanya duluan: \"Ibu mau bilang apa?\"", +15},
        }, 4
    };
}

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

void clearScreen() {
    // Portable clear
    cout << string(3, '\n');
}

void pressEnter() {
    cout << "\n  [Tekan ENTER untuk melanjutkan...]";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void printSeparator() {
    cout << "\n  ============================================================\n";
}

void printLine() {
    cout << "  ------------------------------------------------------------\n";
}

int getInput(int maxVal) {
    int c;
    while (true) {
        cout << "\n  > Pilihanmu: ";
        if (cin >> c && c >= 1 && c <= maxVal) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return c;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  Masukkan angka antara 1 dan " << maxVal << ".\n";
    }
}

// ============================================================
// MONOLOG RESTART (Array of strings)
// ============================================================

const string restartMonologs[] = {
    // restart 1
    "\"Eh—?!\"\n\n"
    "  Dunia berputar. Semuanya gelap sedetik.\n"
    "  Dan aku... di sini lagi.\n\n"
    "  Oke. Oke. Tenang. Aku salah pilih sesuatu.\n"
    "  Coba lagi. Kali ini lebih hati-hati.",

    // restart 2
    "\"Lagi?!\"\n\n"
    "  Sensasinya seperti ditarik mundur paksa.\n"
    "  Dua kali. Aku sudah gagal dua kali.\n\n"
    "  Tapi aku mulai mengerti polanya.\n"
    "  Haru butuh keseimbangan — bukan cuma berani, tapi juga bijak.",

    // restart 3
    "Kali ini tidak ada teriakan.\n\n"
    "  Aku sudah tahu rasanya. Dingin. Gelap. Kembali lagi.\n\n"
    "  Tiga kali. Oke.\n"
    "  Aku harus berpikir seperti Haru — bukan seperti diri sendiri.\n"
    "  Apa yang akan dilakukan seseorang yang mau menang\n"
    "  tanpa kehilangan segalanya?",

    // restart 4+
    "Dunia berputar. Kembali lagi.\n\n"
    "  Sudah berapa kali ini?\n"
    "  Tapi aku semakin dekat. Aku bisa merasakannya.\n\n"
    "  Kali ini — aku sudah tahu lebih banyak.\n"
    "  Ayo, Haru. Ayo.",
};
const int MONOLOG_COUNT = 4;

// ============================================================
// RANDOM PICK EVENT (Fisher-Yates)
// ============================================================

void randomPickEvents(Event* pool, int poolSize,
                      int pickMin, int pickMax,
                      ActiveEventList& activeList) {
    // Buat index array dan shuffle
    vector<int> indices(poolSize);
    for (int i = 0; i < poolSize; i++) indices[i] = i;

    // Fisher-Yates shuffle
    for (int i = poolSize - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(indices[i], indices[j]);
    }

    // Tentukan jumlah event yang dipilih
    int pickCount = pickMin + (rand() % (pickMax - pickMin + 1));

    // Insert ke active list (CRUD: Tambah)
    for (int i = 0; i < pickCount; i++) {
        activeList.insertNode(pool[indices[i]]);
    }
}

// ============================================================
// DISPLAY CHAPTER HEADER
// ============================================================

void displayChapterHeader(ChapterNode* cn, int points) {
    printSeparator();
    cout << "\n  " << cn->title << "\n";
    cout << "  Setting: " << cn->setting << "\n";
    printLine();
    cout << "  Poin Keberhasilan: " << points << " / " << WIN_POINT << "\n";
    printSeparator();
}

// ============================================================
// DISPLAY POIN BAR
// ============================================================

void displayPointBar(int points) {
    cout << "\n  [Poin: " << points << "/100]  ";
    int filled = max(0, min(points, 100)) / 5;
    cout << "[";
    for (int i = 0; i < 20; i++) cout << (i < filled ? "#" : "-");
    cout << "]\n";
}

// ============================================================
// PLAY SINGLE EVENT
// ============================================================
// Mengembalikan perubahan poin

int playEvent(Event* ev, int currentPoints) {
    printSeparator();
    cout << "\n  === " << ev->title << " ===\n\n";
    cout << "  " << ev->narration << "\n";
    printLine();

    // Tampilkan pilihan
    cout << "\n  Apa yang kamu lakukan?\n\n";
    for (int i = 0; i < ev->choiceCount; i++) {
        cout << "  [" << (i + 1) << "] " << ev->choices[i].text << "\n";
    }

    int pick = getInput(ev->choiceCount);
    Choice& chosen = ev->choices[pick - 1];

    // CRUD: Ubah — update poin
    int newPoints = currentPoints + chosen.pointDelta;

    printLine();
    if (chosen.pointDelta > 0) {
        cout << "\n  + " << chosen.pointDelta << " poin! ("
             << currentPoints << " -> " << newPoints << ")\n";
    } else if (chosen.pointDelta < 0) {
        cout << "\n  " << chosen.pointDelta << " poin. ("
             << currentPoints << " -> " << newPoints << ")\n";
    } else {
        cout << "\n  Poin tidak berubah. (" << currentPoints << ")\n";
    }

    pressEnter();
    return chosen.pointDelta;
}

// ============================================================
// DISPLAY RESTART
// ============================================================

void displayRestart(int restartCount) {
    printSeparator();
    cout << "\n  ...waktu berhenti.\n";
    cout << "  Dunia di sekitarmu retak seperti kaca.\n";
    cout << "  Semuanya ditarik kembali — ke awal.\n";
    printSeparator();
    pressEnter();

    cout << "\n  [ RESTART #" << restartCount << " ]\n";

    int idx = min(restartCount - 1, MONOLOG_COUNT - 1);
    vector<string> lines;
    // Split monolog per baris untuk ditampilkan via Queue
    string monolog = restartMonologs[idx];
    string line;
    for (char c : monolog) {
        if (c == '\n') {
            lines.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty()) lines.push_back(line);

    // Queue display
    displayMonologQueue(lines);
    pressEnter();
}

// ============================================================
// DISPLAY ENDING
// ============================================================

void displayEnding(int totalRestarts) {
    printSeparator();
    cout << "\n";
    cout << "  ENDING: Perjanjian dengan Ayah\n";
    printSeparator();
    cout << "\n"
         << "  \"Aku punya usul, Ayah.\"\n\n"
         << "  Kamu membuka halaman baru di buku catatan.\n"
         << "  Menulis dua kolom: AKADEMIK dan KONTEN.\n\n"
         << "  \"Nilai rata-rata di atas 85 — itu janjiku.\n"
         << "   Sebagai gantinya, aku minta dua jam setiap Jumat dan Sabtu.\n"
         << "   Kalau nilaiku turun, aku sendiri yang akan berhenti.\"\n\n"
         << "  Ayah mengambil kertas itu. Membaca dua kali.\n"
         << "  Lalu ia mengambil pena dari sakunya.\n"
         << "  Dan menandatanganinya.\n\n"
         << "  Satu tanda tangan kecil yang rasanya lebih besar\n"
         << "  dari semua kata yang pernah tidak terucapkan di antara mereka.\n\n";
    printLine();
    cout << "\n"
         << "  Enam bulan kemudian:\n"
         << "  Channel Haru: 8.700 subscriber.\n"
         << "  Nilai rata-rata Haru: 87.\n\n"
         << "  Di video terbaru, ada komentar dari akun tanpa foto profil:\n"
         << "  \"Semangat. - Ayah\"\n\n"
         << "  Haru tidak membalas.\n"
         << "  Tapi ia screenshot dan menjadikannya wallpaper.\n\n";
    printSeparator();
    cout << "\n"
         << "              [ ENDING — SELESAI ]\n\n"
         << "   \"Kompromi bukan berarti kalah —\n"
         << "    kadang itu cara paling elegan untuk menang bersama.\"\n\n";
    printSeparator();
    cout << "\n  Total restart: " << totalRestarts << " kali.\n";
    cout << "  Terima kasih sudah memainkan MANGA RESET.\n\n";
}

// ============================================================
// PLAY CHAPTER
// ============================================================
// Return: true = lanjut ke chapter berikutnya, false = game selesai

bool playChapter(int chapterId, Event* pool, GameState& gs, StateStack& stateStack) {
    ChapterNode* cn = findChapterNode(chapterId);
    if (!cn) return false;

    // Save state awal chapter
    gs.points = STARTING_POINT;
    gs.currentChapter = chapterId;
    stateStack.clear();
    stateStack.save(gs);

    while (true) {
        // Buat sesi baru: random pick event
        ActiveEventList activeList;
        randomPickEvents(pool, POOL_SIZE, MIN_EVENTS, MAX_EVENTS, activeList);

        displayChapterHeader(cn, gs.points);
        cout << "\n  " << activeList.count << " aktivitas tersedia sesi ini.\n";
        pressEnter();

        // Loop sesi: player pilih event satu per satu
        while (!activeList.isEmpty()) {
            clearScreen();
            displayChapterHeader(cn, gs.points);
            displayPointBar(gs.points);

            // CRUD: Tampilkan — render menu event aktif
            activeList.displayMenu();

            // Tambahkan opsi lihat karakter
            cout << "  [" << (activeList.count + 1) << "] Lihat daftar karakter\n";
            printLine();

            int menuChoice = getInput(activeList.count + 1);

            // Opsi lihat karakter
            if (menuChoice == activeList.count + 1) {
                printSeparator();
                cout << "\n  --- Karakter dalam Cerita ---\n\n";
                for (const auto& ch : characters) {
                    cout << "  " << ch.name << " (" << ch.role << ")\n";
                    cout << "  " << ch.description << "\n\n";
                }
                pressEnter();
                continue;
            }

            // Ambil event yang dipilih
            Event* selectedEvent = activeList.getByMenuIndex(menuChoice);
            if (!selectedEvent) continue;

            int eventId = selectedEvent->id;

            // Main event — CRUD: Ubah (poin berubah)
            int delta = playEvent(selectedEvent, gs.points);
            gs.points += delta;

            // CRUD: Hapus — hapus event dari linked list setelah selesai
            activeList.deleteNode(eventId);

            // Cek poin
            if (gs.points >= WIN_POINT) {
                cout << "\n  Poin mencapai " << gs.points
                     << "! Kamu berhasil melewati chapter ini!\n";
                pressEnter();
                return true;   // lanjut ke chapter berikutnya
            }
            if (gs.points <= 0) {
                gs.restartCount++;
                displayRestart(gs.restartCount);
                // Restore ke awal chapter
                int savedRestarts = gs.restartCount;
                gs = stateStack.getChapterInitial();
                gs.restartCount = savedRestarts;
                gs.points = STARTING_POINT;
                stateStack.save(gs);
                // Mulai sesi baru (break inner loop)
                break;
            }
        }

        // Semua event habis tapi poin masih 1-99: sesi baru
        if (gs.points > 0 && gs.points < WIN_POINT) {
            cout << "\n  Semua aktivitas selesai. Poin: " << gs.points << "\n";
            cout << "  Belum cukup untuk lanjut. Sesi baru dimulai...\n";
            pressEnter();
            // Loop while(true) akan membuat sesi baru dengan random event berbeda
        }
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    // Build semua data
    buildChapterGraph();
    buildPoolChapter1();
    buildPoolChapter2();

    // Init game state
    GameState gs = {1, STARTING_POINT, 0, false, false};
    StateStack stateStack;

    // ── Title Screen ─────────────────────────────────────────
    printSeparator();
    cout << "\n";
    cout << "        MANGA RESET: Dunia yang Kulupa\n";
    cout << "\n";
    cout << "  Sebuah cerita tentang pilihan, keluarga, dan impian.\n";
    cout << "  Kumpulkan 100 poin di setiap chapter untuk melanjutkan.\n";
    cout << "  Poin habis — kamu kembali ke awal chapter.\n";
    cout << "\n";
    printSeparator();
    pressEnter();

    // ── Prolog ───────────────────────────────────────────────
    printSeparator();
    cout << "\n  Prolog\n";
    printSeparator();
    cout << "\n"
         << "  Namaku Ren. 18 tahun. Otaku.\n\n"
         << "  Hari itu biasa saja — pulang dari toko manga,\n"
         << "  kantong plastik di tangan, langit sore berwarna oranye.\n\n"
         << "  Dan kemudian... truk.\n\n"
         << "  Aku tidak sempat berteriak.\n\n"
         << "  ---\n\n"
         << "  Saat mataku terbuka, ini bukan kamarku.\n"
         << "  Ada poster jadwal pelajaran. Meja belajar penuh buku.\n"
         << "  Satu laptop tua yang sengaja disembunyikan di balik binder.\n\n"
         << "  Di cermin di depanku... bukan wajahku.\n\n"
         << "  \"Ini... Haru?\"\n\n"
         << "  Haru Mizushima. MC dari sebuah manga slice of life\n"
         << "  yang aku baca sekitar 5 tahun lalu.\n\n"
         << "  Hidupnya keras. Keluarganya keras.\n"
         << "  Dan ada cerita yang harus kujalani.\n\n"
         << "  Semoga aku ingat cukup banyak untuk tidak merusaknya.\n\n";
    pressEnter();

    // ── Game Loop — Graph traversal ──────────────────────────
    // Mulai dari node Chapter 1 di graph
    int currentChapterId = 1;

    while (true) {
        ChapterNode* cn = findChapterNode(currentChapterId);
        if (!cn) break;

        // Ending node
        if (currentChapterId == 99) {
            displayEnding(gs.restartCount);
            break;
        }

        // Pilih pool yang sesuai
        Event* pool = (currentChapterId == 1) ? poolChapter1 : poolChapter2;

        bool won = playChapter(currentChapterId, pool, gs, stateStack);

        if (won) {
            // Traversal graph: pindah ke node berikutnya
            currentChapterId = cn->nextChapterId;
        }
    }

    printSeparator();
    cout << "  [ Program selesai. ]\n";
    printSeparator();
    cout << "\n";

    return 0;
}