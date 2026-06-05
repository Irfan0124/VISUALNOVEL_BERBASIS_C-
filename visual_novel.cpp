/*
 * ============================================================
 *  MANGA RESET: Dunia yang Kulupa
 *  Terminal Story Game - C++
 *  Struktur Data yang Digunakan:
 * ============================================================
 *  1. Array       : pool event per chapter, array pilihan - Syarif
 *  2. Struct      : Event, Choice, Character, GameState, ChapterNode - Irfan
 *  3. Pointer     : navigasi linked list (EventNode*) - Shauqy
 *  4. Linked List : daftar event aktif per sesi (Single Linked List) - Jefry
 *  5. Queue       : display monolog restart baris per baris - Mars
 *  6. Graph/Tree  : struktur chapter sebagai directed graph - Hakim
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

// random logic
#include <cstdlib>
#include <ctime>
#include <limits>

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

const int CHAPTER_COUNT = 3;

// ============================================================
// GRAPH - Adjacency Matrix & Traversal
// ============================================================

int chapterAdj[CHAPTER_COUNT][CHAPTER_COUNT]; // adjacency matrix
int dfsVisited[CHAPTER_COUNT];                // penanda node sudah dikunjungi

// ============================================================
// STRUCT DEFINITIONS
// ============================================================

// Struct: satu pilihan dalam sebuah event
struct Choice {
    string text;
    int    pointDelta;   // poin yang berubah saat pilihan ini dipilih
    string response;     // narasi yang muncul setelah pilihan dipilih
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
    bool secretFlag;       // *** SECRET ENDING FLAG ***
    bool syarifTrustGiven; // apakah pemain sudah jawab DM Syarif dengan benar
};

// ============================================================
// LINKED LIST - Event Aktif
// ============================================================

struct EventNode {
    Event       data;
    EventNode*  next;
};

struct ActiveEventList {
    EventNode* head;
    int        count;

    ActiveEventList() : head(nullptr), count(0) {}

    // CRUD: Tambah - insert node baru di akhir list
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

    // CRUD: Hapus - hapus node berdasarkan id event
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

    // CRUD: Tampilkan - cetak semua event aktif sebagai menu
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
// QUEUE - Display Monolog Restart (implementasi manual)
// ============================================================

void pressEnter() {
    cout << "\n[Tekan ENTER untuk melanjutkan...]";
    string dummy;
    getline(cin, dummy);
}

const int QUEUE_CAPACITY = 100;

struct MonologQueue {
    string data[QUEUE_CAPACITY];
    int count;

    void create() { count = 0; }

    bool isEmpty() { return count == 0; }
    bool isFull()  { return count == QUEUE_CAPACITY; }

    void enqueue(const string& s) {
        if (!isFull()) {
            data[count] = s;
            count++;
        }
    }

    string dequeue(){
        if (!isEmpty()){
            string text = data[0];
            count -= 1;
            for (int i=0;i<count;i++){
                data[i] = data[i+1];
            }
            data[count] = "";

            if (text == "") { 
                pressEnter();
            }

            return text;
        } else {
            cout << "Antrian Kosong";
            return "Antrian Kosong";
        }
    };
};

void displayMonologQueue(string lines[], int lineCount) {
    MonologQueue q;
    q.create();

    for (int i = 0; i < lineCount; i++) q.enqueue(lines[i]);

    cout << "\n";
    while (!q.isEmpty()) {
        cout << "  " << q.dequeue() << "\n";
    }
    cout << "\n";
}

// ============================================================
// GRAPH - Struktur Chapter (Directed Graph)
// ============================================================
/*
 *  Node index:
 *    0 = Chapter 1
 *    1 = Chapter 2
 *    2 = Ending (id 99)
 *
 *  Edge:
 *    0 -> 1  (Chapter 1 menang  -> Chapter 2)
 *    0 -> 0  (Chapter 1 kalah   -> restart Chapter 1)
 *    1 -> 2  (Chapter 2 menang  -> Ending)
 *    1 -> 1  (Chapter 2 kalah   -> restart Chapter 2)
 */

struct ChapterNode {
    int    id;
    string title;
    string setting;
    int    nextChapterId;
    int    failChapterId;
};

ChapterNode chapterGraph[CHAPTER_COUNT];

void initGraph() {
    for (int i = 0; i < CHAPTER_COUNT; i++)
        for (int j = 0; j < CHAPTER_COUNT; j++)
            chapterAdj[i][j] = 0;
}

void tambahEdge(int dari, int tujuan) {
    chapterAdj[dari][tujuan] = 1;
}

int chapterIdToIndex(int id) {
    for (int i = 0; i < CHAPTER_COUNT; i++)
        if (chapterGraph[i].id == id) return i;
    return -1;
}

int getNextChapterIndex(int currentIndex) {
    for (int i = 0; i < CHAPTER_COUNT; i++) {
        if (chapterAdj[currentIndex][i] == 1 && i != currentIndex)
            return i;
    }
    return -1;
}

void buildChapterGraph() {
    chapterGraph[0] = {1,  "Chapter 1: Hari Pertama sebagai Yosef",
                       "Pagi hari di rumah keluarga Situmorang", 2,  1};
    chapterGraph[1] = {2,  "Chapter 2: Antara Sekolah dan Impian",
                       "Sekolah dan sepulang sekolah",           99, 2};
    chapterGraph[2] = {99, "Ending: Perjanjian dengan Ayah",
                       "Rumah - malam hari",                     -1, -1};

    initGraph();
    tambahEdge(0, 1);
    tambahEdge(0, 0);
    tambahEdge(1, 2);
    tambahEdge(1, 1);
}

ChapterNode* findChapterNode(int id) {
    for (int i = 0; i < CHAPTER_COUNT; i++)
        if (chapterGraph[i].id == id) return &chapterGraph[i];
    return nullptr;
}


// ============================================================
// DATA: KARAKTER
// ============================================================

Character characters[] = {
    {"Yosef Situmorang", "protagonist",
     "Pelajar SMA kelas 3. Pemimpi diam-diam yang ingin jadi content creator."},
    {"Kejo Situmorang", "npc",
     "Ayah Yosef. Keras, disiplin, tapi menyimpan kepedulian yang dalam."},
    {"Arnawati Situmorang", "npc",
     "Ibu Yosef. Lembut, penengah, sering diam tapi selalu memperhatikan."},
    {"Hanif Situmorang", "npc",
     "Kakak Yosef, 21 tahun. Standar keluarga. Bekerja di perusahaan bagus."},
    {"Akmal", "npc",
     "Teman sekelas Yosef. Santai, sering ngajak main."},
    {"Dimas", "npc",
     "Teman sekelas Yosef. Energik, update soal tren konten."},
    {"Syarif", "npc",
     "Content creator SMA yang menginspirasi Yosef. Apakah dia hanya NPC biasa...?"},
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
            {
                "Langsung bangun dan mulai lari sesuai jadwal.", +25,
                "Kamu lempar selimut dan turun dari kasur. Dingin, tapi kaki ini\n"
                "  bergerak juga. Lari pagi selesai tepat waktu - dan entah kenapa,\n"
                "  pagi ini terasa lebih ringan dari biasanya."
            },
            {
                "Tidur lagi 15 menit, lari bisa nanti.", -20,
                "Lima belas menit jadi tiga puluh. Tiga puluh jadi empat puluh lima.\n"
                "  Kamu bangun dengan panik - jadwal Ayah sudah terlewat, dan ada\n"
                "  rasa bersalah yang menempel sepanjang pagi."
            },
            {
                "Lari di tempat saja di kamar - lebih praktis.", +5,
                "Kamu lari di tempat sambil lirik-lirik cermin. Lumayan. Tidak ideal.\n"
                "  Tapi setidaknya tubuhmu bergerak, dan hati nuranimu tidak terlalu berisik."
            },
        }, 3
    };

    // Event 1: Sarapan & pertanyaan Ayah
    poolChapter1[1] = {
        101, "Sarapan bersama Ayah",
        "Meja makan. Ayah menaruh koran dan menatapmu.\n"
        "  \"Yosef. Nilai ulangan matematika minggu lalu.\"\n"
        "  Bukan pertanyaan. Pernyataan. Ia sudah tahu nilaimu: 78.",
        {
            {
                "Jawab jujur: \"78, Ayah. Aku akan lebih giat.\"", +25,
                "Ayah diam sejenak. Lalu ia mengangguk pelan.\n"
                "  Tidak ada pujian - tapi tidak ada ceramah juga. Itu sudah cukup."
            },
            {
                "Diam saja dan terus makan.", -15,
                "Keheningan di meja makan terasa sangat berat.\n"
                "  Ayah akhirnya menaruh koran dan berdiri tanpa berkata apa-apa.\n"
                "  Kamu tahu artinya."
            },
            {
                "Berbohong: \"Sudah bagus, di atas rata-rata kelas.\"", -20,
                "Ayah menatapmu dengan tatapan yang kamu kenal -\n"
                "  tatapan 'aku sudah tahu'. Ia membuka koran lagi.\n"
                "  'Bicara yang benar, Yosef.'"
            },
            {
                "Alihkan topik: \"Ibu, masakannya enak hari ini.\"", -10,
                "Ibu tersenyum kecil. Tapi Ayah tidak bergeming.\n"
                "  Ia hanya menatapmu sekali lagi sebelum balik ke korannya.\n"
                "  Topiknya menggantung di udara."
            },
        }, 4
    };

    // Event 2: Ngobrol dengan Hanif
    poolChapter1[2] = {
        102, "Ngobrol dengan Hanif (kakak)",
        "Hanif sudah rapi dengan seragam kantornya.\n"
        "  Ia menuang kopi sambil berkata pelan:\n"
        "  \"Yosef, Ayah lagi banyak pikiran soal kerjaan.\n"
        "   Jangan bikin masalah dulu, ya.\"\n\n"
        "  Kamu menatapnya.",
        {
            {
                "Angguk dan bilang: \"Iya, Kak. Makasih info-nya.\"", +20,
                "Hanif menepuk bahumu pelan. 'Bagus.'\n"
                "  Hanya satu kata - tapi dari Hanif, itu banyak."
            },
            {
                "Balik ke kamar tanpa menjawab.", -10,
                "Hanif memanggil namamu sekali, tapi kamu sudah di ujung lorong.\n"
                "  Kamu dengar helaan napasnya dari dapur."
            },
            {
                "Tanya balik: \"Emang kenapa, Kak? Cerita dong.\"", +15,
                "Hanif sedikit kaget. Lalu ia duduk dan cerita sebentar\n"
                "  soal Ayah yang sedang stres proyek. Kamu menyimak.\n"
                "  Setidaknya sekarang kamu lebih paham situasinya."
            },
            {
                "Senyum saja tanpa berkata apa-apa.", +5,
                "Hanif menatapmu sebentar, lalu balik menatap gelasnya.\n"
                "  'Ya sudah. Hati-hati.' Singkat, tapi hangatnya ada."
            },
        }, 4
    };

    // Event 3: Menemukan video Syarif
    poolChapter1[3] = {
        103, "Buka HP sebelum berangkat",
        "Kamu sempat buka HP sebelum berangkat.\n"
        "  Notifikasi: Syarif upload video baru.\n"
        "  Judulnya: 'Cara Aku Mulai dari 0 Subscriber'\n\n"
        "  Ayah ada di ruang sebelah. Waktu 10 menit sebelum berangkat.",
        {
            {
                "Tonton sebentar - 10 menit cukup untuk highlight-nya.", +15,
                "Kamu tonton highlight-nya. Ada satu kalimat yang Syarif bilang\n"
                "  yang langsung kamu catat di memo: 'Konsistensi mengalahkan\n"
                "  konten sempurna.' Itu sudah cukup untuk hari ini."
            },
            {
                "Simpan dulu untuk nonton nanti, sekarang fokus berangkat.", +20,
                "Kamu taruh HP ke saku dan berangkat. Tapi pikiran soal video itu\n"
                "  menempel sepanjang perjalanan - dan itu bukan hal yang buruk.\n"
                "  Ide-ide kecil mulai berkecambah di kepala."
            },
            {
                "Langsung balas komentar di videonya.", -10,
                "Kamu tulis komentar panjang lebar. Tiba-tiba Ayah muncul di pintu.\n"
                "  'Berangkat.' Kamu panik, tutup HP, dan buru-buru keluar.\n"
                "  Konsentrasimu sudah buyar sebelum hari dimulai."
            },
            {
                "Tutup HP dan lupakan.", -5,
                "Kamu tutup HP. Benar secara keputusan - tapi ada sedikit\n"
                "  rasa menyesal yang menggantung. Mungkin itu informasi\n"
                "  yang kamu butuhkan hari ini."
            },
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
            {
                "\"Baik, Ayah.\" - Terima tanpa komplain.", +20,
                "Ayah mengangguk. Perang kecil ini tidak terjadi -\n"
                "  setidaknya untuk hari ini. Kamu menarik napas\n"
                "  dan mulai berpikir bagaimana cara menyiasatinya."
            },
            {
                "\"Tapi Ayah, aku sudah punya jadwal sendiri-\" (potong di tengah)", -20,
                "Ayah memotong kalimatmu dengan satu tatapan.\n"
                "  Malam itu terasa sangat, sangat panjang."
            },
            {
                "Diam, tapi sudah mulai berpikir cara menyiasatinya.", +10,
                "Kamu diam. Tapi otakmu sudah berputar -\n"
                "  jam 6 pagi sebelum semua orang bangun, mungkin?\n"
                "  Ada celah. Selalu ada celah kalau mau dicari."
            },
            {
                "\"Boleh aku belajar sendiri caranya, Ayah?\"", +15,
                "Ayah sedikit terkejut dengan pertanyaan itu. Ia diam sebentar.\n"
                "  'Kalau nilaimu bagus, kita bisa bicara soal metode.'\n"
                "  Itu bukan penolakan - itu negosiasi terselubung."
            },
        }, 4
    };

    // Event 5: Bertemu Ibu di dapur
    poolChapter1[5] = {
        105, "Bantu Ibu di dapur",
        "Ibu sedang mencuci piring sendirian.\n"
        "  Kamu bisa langsung pergi ke kamar,\n"
        "  atau sebentar bantu.",
        {
            {
                "Langsung tawarkan diri bantu cuci piring.", +25,
                "Ibu tersenyum - senyum yang berbeda dari biasanya.\n"
                "  Bukan senyum basa-basi. Senyum yang bilang 'terima kasih\n"
                "  sudah memperhatikan'. Kalian cuci piring dalam diam yang hangat."
            },
            {
                "Bilang mau bantu tapi malah cuma berdiri.", +5,
                "Kamu akhirnya bantu juga, meski canggung.\n"
                "  Ibu tidak berkomentar. Tapi ia tidak mengusirmu juga."
            },
            {
                "Langsung ke kamar - ada yang ingin dikerjakan.", -5,
                "Kamu masuk kamar. Di balik pintu, kamu dengar bunyi piring\n"
                "  yang terus bergerak - dan suara itu entah kenapa\n"
                "  terasa lebih berat dari biasanya."
            },
            {
                "Tanya Ibu soal Ayah: \"Ibu, Ayah kenapa akhir-akhir ini...\"", +15,
                "Ibu menaruh piring dan menatapmu. Lalu ia bercerita pelan -\n"
                "  tentang Ayah, tentang tekanan kerja, tentang bagaimana\n"
                "  rumah ini sedang dalam mode 'tahan napas'.\n"
                "  Kamu menyimak setiap kata."
            },
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
            {
                "Edit draft video dengan teliti, tapi jangan upload dulu.", +20,
                "Kamu edit dengan teliti - potong bagian yang terlalu lama,\n"
                "  perbaiki audio. Hasilnya jauh lebih baik dari semula.\n"
                "  Kamu simpan dan tutup laptop dengan rasa puas yang jarang kamu rasakan."
            },
            {
                "Upload langsung tanpa diedit.", -15,
                "Video terupload dalam kondisi mentah. Satu jam kemudian\n"
                "  kamu lihat komentar pertama: 'audio-nya berisik banget.'\n"
                "  Kamu gigit bibir. Terlambat untuk ditarik."
            },
            {
                "Tutup laptop - terlalu berisiko sekarang.", +5,
                "Kamu tutup laptop. Bukan karena menyerah -\n"
                "  karena kamu memilih momen yang lebih tepat.\n"
                "  Itu keputusan yang lebih dewasa dari yang kamu sadari."
            },
            {
                "Rekam video baru dengan berbisik pelan.", +25,
                "Berbisik ke kamera, kamu rekam. Justru karena terbata dan pelan,\n"
                "  videonya terasa lebih intim, lebih nyata.\n"
                "  'Ini yang terbaik yang pernah aku buat,' bisikmu ke diri sendiri."
            },
            {
                "Buka komentar lama dan baca satu per satu.", +10,
                "Kamu baca komentar-komentar lama - ada yang singkat, ada yang panjang.\n"
                "  Satu komentar dari enam bulan lalu:\n"
                "  'kontenmu polos tapi mengena.' Kamu senyum sendiri."
            },
        }, 5
    };

    // Event 7: Telepon teman
    poolChapter1[7] = {
        107, "Akmal telepon mengajak keluar",
        "HP bergetar. Akmal nelpon.\n"
        "  \"Yosef! Nongkrong yuk, ada yang baru buka dekat sekolah.\n"
        "   Dimas juga ikut. Santai aja, paling sejam.\"\n\n"
        "  Ayah ada di rumah.",
        {
            {
                "Tolak dengan sopan: \"Nggak bisa hari ini, Dit.\"", +20,
                "Akmal bilang 'ya sudah' dengan nada biasa -\n"
                "  tidak kecewa, tidak marah. Dan kamu lega.\n"
                "  Sore ini masih milikmu."
            },
            {
                "Langsung iya tanpa pikir panjang.", -25,
                "Kamu keluar. Sejam jadi dua jam. Saat pulang,\n"
                "  Ayah sudah berdiri di depan pintu.\n"
                "  Kamu tidak perlu melihat ekspresinya untuk tahu."
            },
            {
                "Minta waktu dua jam lagi, baru keluar.", -10,
                "Dua jam kemudian kamu bergabung -\n"
                "  tapi pikiran soal Ayah tidak pergi.\n"
                "  Kamu tidak bisa menikmati satu pun momen di sana."
            },
            {
                "Tanya dulu ke Ibu apakah boleh keluar.", +15,
                "Ibu mempertimbangkan sebentar. 'Sejam saja.'\n"
                "  Kamu mengangguk. Dan kali ini, perginya terasa ringan\n"
                "  karena izin sudah di tangan."
            },
        }, 4
    };

    // Event 8: Malam - belajar atau konten
    poolChapter1[8] = {
        108, "Pilihan malam: belajar atau konten",
        "Pukul 20.00. Satu jam bebas sebelum jadwal belajar Ayah.\n"
        "  Di mejamu: buku matematika dan laptop tersembunyi.\n"
        "  Kamu hanya bisa pilih satu.",
        {
            {
                "Belajar matematika dulu - nilai 78 Yosefs naik.", +20,
                "Kamu buka bab persamaan kuadrat dan mulai dari soal pertama.\n"
                "  Tiga puluh menit kemudian, ada sesuatu yang mulai masuk akal.\n"
                "  Nilai 78 mungkin bukan takdir."
            },
            {
                "Buka laptop - satu jam cukup untuk rekam intro video.", +15,
                "Kamu rekam intro - singkat, tapi kamu puas.\n"
                "  Sebelum tidur, kamu putar ulang sekali.\n"
                "  'Ini cukup bagus,' bisikmu dalam gelap."
            },
            {
                "Tiduran sambil scroll HP.", -20,
                "HP, scroll, HP, scroll. Tiba-tiba jam 22.30.\n"
                "  Dan kamu belum mengerjakan apa-apa.\n"
                "  Hati kecilmu berbisik bahwa ini bukan keputusan terbaik."
            },
            {
                "Belajar setengah jam, sisanya buat catatan ide konten.", +25,
                "Tiga puluh menit belajar, tiga puluh menit nulis ide konten.\n"
                "  Waktu habis, tapi kamu tidur dengan perasaan\n"
                "  bahwa hari ini tidak sia-sia."
            },
        }, 4
    };

    // Event 9: Cermin pagi
    poolChapter1[9] = {
        109, "Momen depan cermin",
        "Kamu berdiri di depan cermin kamar mandi.\n"
        "  Wajah Yosef menatap balik.\n"
        "  Untuk pertama kali, kamu sadar betapa beratnya hidup\n"
        "  yang sedang kamu jalani - sebagai dirinya.\n\n"
        "  Kamu bicara ke cermin.",
        {
            {
                "\"Aku bisa. Yosef pasti bisa.\" - Ucapkan dengan yakin.", +25,
                "Suaramu gemetar sedikit di awal - tapi makin lama makin kuat.\n"
                "  Kamu menatap wajah Yosef di cermin dan untuk pertama kali,\n"
                "  kamu percaya bahwa ia benar-benar bisa."
            },
            {
                "Diam saja dan pergi.", 0,
                "Kamu pergi tanpa berkata apa-apa.\n"
                "  Tapi bayangan di cermin itu tidak ikut pergi dari pikiranmu\n"
                "  sepanjang hari."
            },
            {
                "\"Kenapa hidup ini sekeras ini?\" - Keluh.", -10,
                "Kata-kata itu keluar begitu saja.\n"
                "  Dan entah kenapa, setelah diucapkan,\n"
                "  bebannya tidak berkurang - malah sedikit bertambah."
            },
            {
                "Latihan senyum untuk kamera video.", +15,
                "Kamu latihan senyum tiga kali. Yang terakhir terasa paling alami.\n"
                "  Kamu simpan ekspresi itu di memori -\n"
                "  mungkin berguna untuk kamera nanti."
            },
        }, 4
    };
}

// ============================================================
// DATA: POOL EVENT CHAPTER 2 (10 event + 1 secret event)
// ============================================================

Event poolChapter2[POOL_SIZE];

// *** SECRET EVENT - tidak masuk pool normal, dipanggil khusus ***
Event secretEventSyarif;

void buildPoolChapter2() {

    // Event 0: Pagi di sekolah
    poolChapter2[0] = {
        200, "Pagi di kelas - ulangan mendadak",
        "Guru masuk dan langsung taruh lembar soal.\n"
        "  \"Ulangan mendadak. Materi dua minggu terakhir.\"\n"
        "  Kelas langsung riuh. Kamu belum review sama sekali.",
        {
            {
                "Tetap tenang - kerjakan semampu yang diingat.", +20,
                "Kamu tulis apa yang kamu ingat - beberapa kosong, tapi banyak terisi.\n"
                "  Saat kertas dikumpulkan, kamu tidak merasa hancur.\n"
                "  Itu sudah lebih dari cukup untuk kondisi tidak siap."
            },
            {
                "Coba lirik jawaban Dimas yang duduk di sebelah.", -25,
                "Dimas sadar dan menutupi kertasnya. Guru melirik.\n"
                "  Kamu pura-pura batuk. Jantungmu berdegup\n"
                "  sampai bel berbunyi - dan hasilnya tetap tidak memuaskan."
            },
            {
                "Kosongin yang tidak tahu, fokus yang bisa dikerjakan.", +15,
                "Strategi yang baik. Kamu tidak buang waktu di soal yang buntu -\n"
                "  fokus ke yang bisa, dan hasilnya lumayan\n"
                "  untuk kondisi tidak siap sama sekali."
            },
            {
                "Minta izin ke toilet untuk buka catatan HP.", -20,
                "Guru melihatmu keluar dan menatap curiga saat kamu kembali.\n"
                "  Kamu tidak menemukan banyak - dan kecemasan itu\n"
                "  merusak konsentrasi di sisa ulangan."
            },
        }, 4
    };

    // Event 1: Istirahat - Syarif lagi live
    poolChapter2[1] = {
        201, "Istirahat - Syarif lagi live",
        "Dimas lari ke mejamu sambil pegang HP.\n"
        "  \"Yosef! Syarif lagi live! Dia lagi bahas cara monetisasi\n"
        "   channel kecil. Ini pas banget buat kamu!\"\n\n"
        "  Masih ada 20 menit istirahat.",
        {
            {
                "Tonton live-nya sebentar - ini ilmu yang berguna.", +20,
                "Kamu tonton dengan serius, catat tiga poin penting di memo.\n"
                "  Bel masuk berbunyi tepat saat Syarif mengucapkan kalimat terakhirnya.\n"
                "  Timing yang sempurna."
            },
            {
                "Minta Dimas rekam/screenshot bagian pentingnya saja.", +15,
                "Dimas dengan senang hati screenshot beberapa slide.\n"
                "  Kamu dapat intinya tanpa kehilangan waktu istirahat.\n"
                "  Efisien - dan Dimas senang merasa berguna."
            },
            {
                "Ignore - fokus review pelajaran untuk jam selanjutnya.", +10,
                "Kamu buka buku dan review materi.\n"
                "  Tidak dapat ilmu Syarif hari ini - tapi paling tidak,\n"
                "  kamu masuk kelas berikutnya dengan lebih siap."
            },
            {
                "Tonton sambil makan, tapi setengah konsentrasi.", +5,
                "Kamu tonton sambil makan, tapi setengah perhatianmu di sana\n"
                "  setengahnya di makanan. Kamu dapat beberapa poin,\n"
                "  tapi tidak semuanya masuk dengan baik."
            },
        }, 4
    };

    // Event 2: Guru BK memanggil
    poolChapter2[2] = {
        202, "Dipanggil Guru BK",
        "\"Yosef, masuk sebentar.\"\n\n"
        "  Guru BK menutup pintu. Di mejanya ada print-out.\n"
        "  Nilai Yosef semester ini. Rata-rata: 76.\n"
        "  \"Kamu siswa yang cerdas. Tapi kenapa nilainya segini?\"",
        {
            {
                "Jujur: \"Aku sedang cari cara seimbangkan semua hal, Bu.\"", +25,
                "Guru BK diam sejenak. Lalu ia tersenyum -\n"
                "  bukan senyum kasihan, tapi senyum yang mengerti.\n"
                "  'Kalau butuh bicara, pintu saya selalu terbuka, Yosef.'"
            },
            {
                "Diam dan mengangguk saja.", -5,
                "Guru BK menghela napas. 'Kamu tidak mau cerita?'\n"
                "  Kamu tetap diam. Sesi itu berakhir canggung\n"
                "  dan tidak ada yang terselesaikan."
            },
            {
                "Berjanji nilai akan naik tanpa penjelasan.", +10,
                "Guru BK menuliskan sesuatu di buku catatannya.\n"
                "  'Baik. Saya pegang janjimu.'\n"
                "  Tidak banyak - tapi setidaknya ada arah yang disepakati."
            },
            {
                "Alihkan: \"Saya tidak tahu, Bu. Mungkin soalnya sulit.\"", -15,
                "Guru BK menatapmu lama. 'Yosef, itu bukan jawaban yang aku cari.'\n"
                "  Kamu tahu - tapi kamu tidak punya jawaban\n"
                "  yang lebih baik saat itu."
            },
        }, 4
    };

    // Event 3: Dimas tanya soal channel
    poolChapter2[3] = {
        203, "Dimas tanya soal channel YouTube",
        "Dimas duduk di sebelahmu waktu pulang.\n"
        "  \"Yosef, aku tahu kamu punya channel YouTube.\n"
        "   Aku nggak sengaja nemu. Videomu... bagus.\n"
        "   Kenapa nggak pernah cerita?\"\n\n"
        "  Jantungmu berdegup.",
        {
            {
                "Ceritakan semuanya - impian, ketakutan, dan rencana.", +25,
                "Dimas mendengarkan dengan serius. Matanya tidak menghakimi -\n"
                "  malah berbinar sedikit. 'Yosef... ini keren. Serius.'\n"
                "  Untuk pertama kali, ada orang yang tahu.\n"
                "  Dan rasanya tidak seburuk yang kamu bayangkan."
            },
            {
                "Minta dia diam dan tidak cerita ke siapapun.", -10,
                "Dimas mengangguk pelan, tapi ada ekspresi di wajahnya\n"
                "  yang sulit dibaca. Kamu berhasil menutupnya -\n"
                "  tapi entah untuk berapa lama."
            },
            {
                "Pura-pura tidak tahu channel yang dimaksud.", -20,
                "Dimas menatapmu dengan ekspresi terluka.\n"
                "  'Aku sudah lihat videonya. Aku nggak bohong.'\n"
                "  Kamu memalingkan muka - dan jarak itu terasa\n"
                "  tiba-tiba lebih jauh dari sebelumnya."
            },
            {
                "Bilang itu cuma iseng, tidak serius.", -5,
                "Dimas mengangguk. Tapi matanya bilang ia tidak sepenuhnya percaya.\n"
                "  Kesempatan untuk terbuka itu lewat begitu saja."
            },
            {
                "Minta pendapat Dimas soal video-videomu.", +20,
                "Dimas langsung antusias. Ia buka videomu satu per satu\n"
                "  dan kasih komentar jujur. Beberapa kritiknya perih -\n"
                "  tapi semuanya berguna."
            },
        }, 5
    };

    // Event 4: Pulang - ketemu Ayah di depan pintu
    poolChapter2[4] = {
        204, "Ketemu Ayah di depan pintu",
        "Ayah berdiri di depan pintu saat kamu pulang.\n"
        "  Jam 16.05. Lima menit terlambat dari jadwal.\n"
        "  Ia menatapmu tanpa kata-kata.",
        {
            {
                "Minta maaf langsung dan jelaskan alasannya.", +20,
                "Ayah mendengarkan. Ia tidak langsung memotong.\n"
                "  Itu sudah langkah maju yang besar dari biasanya."
            },
            {
                "Pura-pura tidak sadar soal keterlambatan.", -20,
                "Ayah masuk ke dalam tanpa berkata apa-apa.\n"
                "  Dan diam itu lebih berat dari amarah."
            },
            {
                "Diam dan masuk rumah dengan cepat.", -10,
                "Kamu masuk cepat. Tapi langkah Ayah di belakangmu\n"
                "  terdengar berat - dan kamu tahu topik ini\n"
                "  tidak akan selesai dengan sendirinya."
            },
            {
                "\"Maaf, Ayah. Ada yang perlu aku ceritakan.\"", +25,
                "Ayah menaikkan satu alis. Lalu ia melangkah ke samping,\n"
                "  membiarkanmu masuk. 'Baik. Cerita.'\n"
                "  Dua kata - tapi itu undangan yang jarang ia berikan."
            },
        }, 4
    };

    // Event 5: DM dari Syarif
    // *** KUNCI SECRET ENDING: pilihan [2] -> syarifTrustGiven = true ***
    poolChapter2[5] = {
        205, "DM dari Syarif",
        "HP-mu berbunyi. DM masuk.\n"
        "  Dari: @SyarifCreates\n"
        "  \"Hei, aku nemu channel-mu. Kontenmu raw dan jujur.\n"
        "   Mau kolaborasi video bareng minggu depan?\"\n\n"
        "  Tanganmu gemetar sedikit.\n\n"
        "  [Petunjuk: Jawaban yang tepat mungkin membuka jalan\n"
        "   yang tidak terduga...]",
        {
            {
                "Balas: \"Mau banget! Kapan dan di mana?\"", -15,
                "Kamu kirim langsung. Tapi begitu HP ditaruh, kamu sadar -\n"
                "  kamu belum tanya apapun soal konsep, format, atau persiapan.\n"
                "  Panik kecil mulai merayap."
            },
            {
                // *** PILIHAN KUNCI SECRET ENDING ***
                "Balas: \"Terima kasih. Boleh aku pikir dulu?\"\n"
                "  Lalu tanya: \"Btw, aku penasaran... kamu siapa sebenarnya?\"",
                +25,
                "Syarif tidak langsung balas.\n"
                "  Tiga menit berlalu. Lalu pesan masuk:\n"
                "  'Sure, take your time.'\n"
                "  Tapi ada jeda sebelum pesan kedua muncul:\n"
                "  '...pertanyaan yang menarik. Belum ada yang tanya itu sebelumnya.'\n\n"
                "  Sesuatu di perutmu terasa aneh.\n"
                "  Bukan gugup biasa. Ini berbeda.\n"
                "  Seperti kamu baru saja mengetuk pintu yang seharusnya tidak ada."
            },
            {
                "Tidak balas dulu - terlalu overwhelmed.", -5,
                "Kamu stare ke layar selama dua menit, lalu taruh HP.\n"
                "  DM itu masih di sana. Belum terlambat -\n"
                "  tapi jendela kesempatan tidak terbuka selamanya."
            },
            {
                "Balas dan langsung tanya soal teknis kolaborasi.", +15,
                "Kamu tanya soal platform, durasi, dan topik.\n"
                "  Syarif terkesan dengan pertanyaan yang spesifik.\n"
                "  'Kamu serius ya. Bagus.' Fondasi yang kuat untuk kolaborasi."
            },
        }, 4
    };

    // Event 6: Malam - konfrontasi tagihan internet
    poolChapter2[6] = {
        206, "Ayah tunjukkan tagihan internet",
        "Makan malam. Ayah menaruh kertas di tengah meja.\n"
        "  Tagihan internet. Ada highlight di satu baris:\n"
        "  akses ke platform video, pukul 22.15, 47 menit.\n\n"
        "  \"Jelaskan.\"",
        {
            {
                "Minta maaf dan berjanji tidak mengulangi.", -10,
                "Ayah mengangguk pelan. Tapi kamu bisa lihat\n"
                "  ia tidak puas dengan jawaban itu.\n"
                "  Malam ini selesai - tapi masalahnya belum."
            },
            {
                "Tunjukkan statistik channel: 47 subscriber, engagement rate.", +25,
                "Ayah menatap angka-angka itu. Lama.\n"
                "  Lalu ia menaruh kertas itu kembali ke meja - tapi tidak melipatnya.\n"
                "  'Lanjut makan dulu.' Kamu tahu itu bukan akhir percakapan.\n"
                "  Tapi bukan penolakan juga."
            },
            {
                "Marah: \"Aku hanya ingin sedikit kebebasan!\"", -25,
                "Suaramu lebih keras dari yang kamu rencanakan.\n"
                "  Ibu meletakkan sendoknya. Ayah berdiri.\n"
                "  Malam itu tidak ada yang bisa diperbaiki lagi."
            },
            {
                "Diam dan terima hukuman.", -15,
                "Kamu terima. Tapi ada yang hancur sedikit di dalam -\n"
                "  dan Ayah juga tampaknya merasakannya.\n"
                "  Diam bukan selalu aman."
            },
            {
                "\"Ayah, boleh aku cerita sesuatu?\" - minta bicara baik-baik.", +20,
                "Ayah terdiam. Ibu menatapnya.\n"
                "  Lalu Ayah duduk kembali. 'Baik. Bicara.'\n"
                "  Ini belum selesai - tapi setidaknya pintunya terbuka."
            },
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
            {
                "Tulis target nilai + jadwal konten yang realistis.", +25,
                "Dua kolom, target konkret, jadwal yang masuk akal.\n"
                "  Kamu lihat hasilnya dan untuk pertama kali,\n"
                "  impian itu terasa seperti rencana sungguhan."
            },
            {
                "Nulis impian besar tanpa detail konkret.", +5,
                "Kamu tulis impian besar dengan tinta penuh semangat.\n"
                "  Tidak ada detail - tapi energinya ada.\n"
                "  Mungkin detail bisa menyusul besok."
            },
            {
                "Tutup buku - terlalu lelah untuk berpikir malam ini.", -10,
                "Kamu tutup buku. Lelah itu nyata -\n"
                "  dan kamu putuskan untuk membiarkannya.\n"
                "  Tapi besok, semua masalah ini masih akan menunggu."
            },
            {
                "Tulis surat untuk Ayah alih-alih proposal.", +15,
                "Kamu tulis surat - bukan untuk dikirim, tapi untuk dimengerti.\n"
                "  Setiap kalimat yang kamu tulis ke Ayah\n"
                "  membantumu memahami apa sebenarnya yang ingin kamu katakan padanya."
            },
        }, 4
    };

    // Event 8: Malam - rekam video rahasia
    poolChapter2[8] = {
        208, "Rekam video rahasia malam ini",
        "Pukul 22.00. Rumah sepi.\n"
        "  Laptop terbuka. Kamera siap.\n"
        "  Tapi kali ini bukan sekadar vlog -\n"
        "  kamu ingin bikin sesuatu yang benar-benar jujur.",
        {
            {
                "Rekam video curahan hati soal keluarga dan impian.", +20,
                "Kamu bicara ke kamera tentang segalanya -\n"
                "  Ayah, impian, takut, harap. Rekaman selesai.\n"
                "  Kamu tidak langsung upload. Tapi ada sesuatu yang lega\n"
                "  keluar setelah mengatakannya keras-keras."
            },
            {
                "Rekam tapi tidak upload - simpan untuk diri sendiri.", +15,
                "Kamu rekam dan simpan. Itu untuk dirimu sendiri -\n"
                "  dan itu cukup. Tidak semua konten harus dilihat orang lain\n"
                "  untuk bermakna."
            },
            {
                "Upload video lama yang sudah diedit.", +5,
                "Video lama yang sudah diedit itu akhirnya terupload.\n"
                "  Sederhana, tapi jujur. Kamu tutup laptop\n"
                "  dan tidur dengan perasaan yang tidak terlalu berat."
            },
            {
                "Matikan laptop - terlalu berisiko.", -5,
                "Kamu matikan laptop. Malam ini tidak ada yang terekam.\n"
                "  Tapi pikiran itu tidak ikut mati -\n"
                "  ia hanya menunggu di kepalamu sampai besok."
            },
        }, 4
    };

    // Event 9: Pagi sebelum sekolah - momen dengan Ibu
    poolChapter2[9] = {
        209, "Momen pagi bersama Ibu",
        "Ibu duduk sendiri di dapur pagi-pagi.\n"
        "  Ia memegang cangkir teh.\n"
        "  Saat kamu masuk, ia tersenyum.\n"
        "  \"Yosef, Ibu mau bicara sebentar.\"",
        {
            {
                "Duduk dan dengarkan.", +25,
                "Ibu bercerita pelan - tentang Ayah muda yang punya mimpi serupa,\n"
                "  tentang pilihan-pilihan yang ia buat, tentang kenapa ia keras.\n"
                "  Kamu menyimak setiap kata. Dan tiba-tiba\n"
                "  Ayah terasa sedikit lebih manusiawi."
            },
            {
                "\"Maaf Bu, buru-buru. Nanti saja ya.\"", -10,
                "Ibu menatap punggungmu. Ia tidak memanggil lagi.\n"
                "  Kamu tidak tahu apa yang ingin ia katakan -\n"
                "  dan mungkin kamu tidak akan pernah tahu hari itu."
            },
            {
                "Duduk tapi sambil lihat HP.", -5,
                "Kamu duduk tapi matamu di HP.\n"
                "  Ibu mulai bicara, tapi kamu hanya menangkap separuhnya.\n"
                "  Saat ia selesai, kamu mengangguk - tapi tidak benar-benar mendengar."
            },
            {
                "Tanya duluan: \"Ibu mau bilang apa?\"", +15,
                "Ibu tersenyum kecil.\n"
                "  'Ibu mau bilang... Ibu bangga kamu sudah bertahan sejauh ini.'\n"
                "  Itu saja. Tapi kamu menahan napas\n"
                "  agar tidak terlalu keliatan terharu."
            },
        }, 4
    };

    // *** SECRET EVENT 210: Pertemuan Lanjutan dengan Syarif ***
    // Event ini hanya dimasukkan ke pool jika syarifTrustGiven == true
    secretEventSyarif = {
        210, "[???] Syarif menghubungi lagi - ada yang aneh",
        "Tengah malam. Kamu hampir tertidur.\n"
        "  HP berbunyi. DM dari @SyarifCreates.\n\n"
        "  Tapi kali ini bukan tentang kolaborasi.\n\n"
        "  'Kamu tadi tanya siapa aku sebenarnya.'\n"
        "  'Aku akan jawab - tapi tidak lewat chat.'\n"
        "  'Besok pukul 05.00 pagi. Taman belakang sekolah. Datang sendiri.'\n\n"
        "  Kamu menatap layar. Jantungmu berdegup tidak wajar.\n"
        "  Ada sesuatu di balik kata-kata itu yang terasa...\n"
        "  bukan dari dunia ini.",
        {
            {
                "Pergi - rasa ingin tahu lebih kuat dari rasa takut.", +30,
                "Alarm kamu set pukul 04.45.\n"
                "  Kamu tidak tahu kenapa, tapi kamu yakin.\n"
                "  Ada sesuatu yang Syarif tahu - tentangmu,\n"
                "  tentang situasi ini, tentang kenapa kamu ada di sini.\n\n"
                "  Kamu pergi."
            },
            {
                "Balas: \"Ini lelucon? Siapa yang kasih nomormu?\"", -5,
                "Syarif tidak balas.\n"
                "  Keesokan paginya, saat kamu cek akun @SyarifCreates -\n"
                "  akunnya tidak ada.\n"
                "  Seperti tidak pernah ada."
            },
            {
                "Pergi tapi ajak Dimas - lebih aman.", -10,
                "Kamu DM Dimas. Tapi HP-mu mati tiba-tiba.\n"
                "  Saat kamu tiba di taman - tidak ada siapapun.\n"
                "  Hanya angin pagi dan rasa sesal yang sulit dijelaskan."
            },
            {
                "Abaikan. Ini terlalu aneh.", -20,
                "Kamu taruh HP dan coba tidur.\n"
                "  Tapi kamu tidak bisa.\n"
                "  Dan pagi harinya, DM itu sudah terhapus sendiri.\n"
                "  Seperti tidak pernah ada."
            },
        }, 4
    };
}

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

void clearScreen() {
    cout << "\n\n\n";
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

void dfsGraph(int idx) {
    dfsVisited[idx] = 1;
    cout << "  [" << idx << "] " << chapterGraph[idx].title << "\n";
    for (int i = 0; i < CHAPTER_COUNT; i++) {
        if (chapterAdj[idx][i] == 1 && !dfsVisited[i]) {
            cout << "       \u2514\u2500> ";
            dfsGraph(i);
        }
    }
}

void displayChapterGraph() {
    printSeparator();
    cout << "\n  Peta Alur Cerita (Directed Graph - DFS Traversal):\n\n";
    for (int i = 0; i < CHAPTER_COUNT; i++) dfsVisited[i] = 0;
    dfsGraph(0);

    cout << "\n  Adjacency Matrix:\n";
    cout << "       ";
    for (int i = 0; i < CHAPTER_COUNT; i++) cout << "[" << i << "] ";
    cout << "\n";
    for (int i = 0; i < CHAPTER_COUNT; i++) {
        cout << "  [" << i << "]  ";
        for (int j = 0; j < CHAPTER_COUNT; j++)
            cout << " " << chapterAdj[i][j] << "  ";
        cout << "\n";
    }
    printSeparator();
}

// ============================================================
// MONOLOG RESTART
// ============================================================

const string restartMonologs[] = {
    "\"Eh-?!\"\n\n"
    "  Dunia berputar. Semuanya gelap sedetik.\n"
    "  Dan aku... di sini lagi.\n\n"
    "  Oke. Oke. Tenang. Aku salah pilih sesuatu.\n"
    "  Coba lagi. Kali ini lebih hati-hati.",

    "\"Lagi?!\"\n\n"
    "  Sensasinya seperti ditarik mundur paksa.\n"
    "  Dua kali. Aku sudah gagal dua kali.\n\n"
    "  Tapi aku mulai mengerti polanya.\n"
    "  Yosef butuh keseimbangan - bukan cuma berani, tapi juga bijak.",

    "Kali ini tidak ada teriakan.\n\n"
    "  Aku sudah tahu rasanya. Dingin. Gelap. Kembali lagi.\n\n"
    "  Tiga kali. Oke.\n"
    "  Aku harus berpikir seperti Yosef - bukan seperti diri sendiri.\n"
    "  Apa yang akan dilakukan seseorang yang mau menang\n"
    "  tanpa kehilangan segalanya?",

    "Dunia berputar. Kembali lagi.\n\n"
    "  Sudah berapa kali ini?\n"
    "  Tapi aku semakin dekat. Aku bisa merasakannya.\n\n"
    "  Kali ini - aku sudah tahu lebih banyak.\n"
    "  Ayo, Yosef. Ayo.",
};
const int MONOLOG_COUNT = 4;

// ============================================================
// RANDOM PICK EVENT (Fisher-Yates)
// ============================================================

void randomPickEvents(Event* pool, int poolSize,
                      int pickMin, int pickMax,
                      ActiveEventList& activeList) {
    int indices[POOL_SIZE];
    for (int i = 0; i < poolSize; i++) indices[i] = i;

    for (int i = poolSize - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    int pickCount = pickMin + (rand() % (pickMax - pickMin + 1));
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
    int clamped = points < 0 ? 0 : (points > 100 ? 100 : points);
    int filled  = clamped / 5;
    cout << "[";
    for (int i = 0; i < 20; i++) cout << (i < filled ? "#" : "-");
    cout << "]\n";
}

// ============================================================
// PLAY SINGLE EVENT
// ============================================================

int playEvent(Event* ev, int currentPoints) {
    printSeparator();
    cout << "\n  === " << ev->title << " ===\n\n";
    cout << "  " << ev->narration << "\n";
    printLine();

    cout << "\n  Apa yang kamu lakukan?\n\n";
    for (int i = 0; i < ev->choiceCount; i++) {
        cout << "  [" << (i + 1) << "] " << ev->choices[i].text << "\n";
    }

    int pick = getInput(ev->choiceCount);
    Choice& chosen = ev->choices[pick - 1];

    int newPoints = currentPoints + chosen.pointDelta;

    printLine();
    cout << "\n  " << chosen.response << "\n";
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
    cout << "  Semuanya ditarik kembali - ke awal.\n";
    printSeparator();
    pressEnter();

    cout << "\n  [ RESTART #" << restartCount << " ]\n";

    int idx = (restartCount - 1 < MONOLOG_COUNT - 1)
          ? restartCount - 1
          : MONOLOG_COUNT - 1;

    const int MAX_LINES = 50;
    string lines[MAX_LINES];
    int    lineCount = 0;

    string monolog = restartMonologs[idx];
    string line;
    for (int i = 0; i < (int)monolog.size(); i++) {
        if (monolog[i] == '\n') {
            if (lineCount < MAX_LINES) lines[lineCount++] = line;
            line.clear();
        } else {
            line += monolog[i];
        }
    }
    if (!line.empty() && lineCount < MAX_LINES) lines[lineCount++] = line;

    displayMonologQueue(lines, lineCount);
    pressEnter();
}

// ============================================================
// *** SECRET ENDING ***
// ============================================================

void displaySecretEnding(int totalRestarts) {
    printSeparator();
    cout << "\n";
    cout << "  *** S E C R E T   E N D I N G ***\n";
    cout << "  Raja Bayangan\n";
    printSeparator();

    pressEnter();

    // === Scene 1: Taman Belakang Sekolah ===
    printSeparator();
    cout << "\n  [ Taman Belakang Sekolah - Pukul 05.00 Pagi ]\n";
    printSeparator();
    cout << "\n"
         << "  Langit masih gelap.\n"
         << "  Taman belakang sekolah terasa berbeda di jam ini -\n"
         << "  hening, tapi bukan hening yang kosong.\n"
         << "  Seperti hening yang menahan napas.\n\n"
         << "  Syarif berdiri di bawah pohon beringin tua.\n"
         << "  Punggungnya menghadapmu.\n\n"
         << "  \"Kamu datang.\"\n\n"
         << "  Suaranya sama. Tapi ada sesuatu yang bergeser -\n"
         << "  seperti nada di bawah kata-katanya yang bukan milik anak SMA.\n\n";
    pressEnter();

    cout << "  Ia berbalik.\n\n"
         << "  Wajah Syarif sama. Seragam yang sama.\n"
         << "  Tapi matanya - matanya berbeda.\n"
         << "  Bukan coklat biasa. Ada sesuatu di dalamnya yang bergerak\n"
         << "  seperti permukaan air di malam hari.\n\n"
         << "  \"Aku sudah menunggu seseorang yang bertanya pertanyaan itu.\"\n\n"
         << "  \"Selama sembilan kali reset.\"\n\n";
    pressEnter();

    // === Scene 2: Pengungkapan ===
    printSeparator();
    cout << "\n  [ Kebenaran ]\n";
    printSeparator();
    cout << "\n"
         << "  Kamu membeku.\n\n"
         << "  \"...Kamu tahu?\"\n\n"
         << "  Syarif tersenyum - dan untuk pertama kali, senyum itu\n"
         << "  tidak terasa seperti senyum konten kreator.\n"
         << "  Itu senyum seseorang yang sudah sangat, sangat tua.\n\n"
         << "  \"Aku tahu kamu bukan Yosef. Aku tahu kamu dari dunia lain.\n"
         << "   Aku tahu ini bukan pertama kalinya kamu di sini.\"\n\n"
         << "  Ia melangkah mendekatimu.\n\n"
         << "  \"Karena aku yang menaruhmu di sini.\"\n\n";
    pressEnter();

    cout << "  Dunia di sekitarmu seolah berhenti berputar.\n\n"
         << "  \"Namaku bukan Syarif. Nama itu hanya... baju.\"\n"
         << "  \"Dalam bahasa yang manusia sudah lupakan sejak seribu tahun lalu,\n"
         << "   namaku adalah Azar-Rein. Raja Bayangan. Penguasa batas antara\n"
         << "   fiksi dan realitas.\"\n\n"
         << "  Ia mengangkat tangannya - dan di antara jari-jarinya,\n"
         << "  kamu bisa melihat sesuatu yang mustahil:\n"
         << "  halaman-halaman manga yang bergerak seperti cahaya.\n\n"
         << "  \"Duniamu - dunia Ren - adalah salah satu dari ribuan cerita\n"
         << "   yang kulindungi. Tapi terkadang, seseorang dari luar cerita\n"
         << "   jatuh ke dalamnya. Dan ketika itu terjadi...\"\n\n"
         << "  \"...mereka menjadi kunci yang bisa mengubah segalanya.\"\n\n";
    pressEnter();

    // === Scene 3: Pilihan Final ===
    printSeparator();
    cout << "\n  [ Pilihan Terakhir ]\n";
    printSeparator();
    cout << "\n"
         << "  Syarif - Azar-Rein - menatapmu dalam.\n\n"
         << "  \"Ada dua hal yang bisa kulakukan untukmu.\"\n\n"
         << "  \"Pertama: aku kembalikan kamu ke duniamu. Ke tubuh Ren.\n"
         << "   Kamu melanjutkan hidupmu. Yosef akan menjalani ceritanya sendiri.\n"
         << "   Tanpa intervensi. Seperti seharusnya.\"\n\n"
         << "  \"Kedua: kamu tetap di sini. Selesaikan cerita Yosef.\n"
         << "   Bantu ia sampai ending yang sesungguhnya.\n"
         << "   Tapi sebagai gantinya - kamu tidak akan ingat cara pulang.\"\n\n"
         << "  Ia menatapmu, menunggu.\n\n";

    cout << "  Apa jawabanmu?\n\n";
    cout << "  [1] \"Aku ingin pulang ke duniaku.\"\n";
    cout << "  [2] \"Aku akan tetap dan selesaikan cerita Yosef.\"\n";
    cout << "  [3] \"Aku tidak percaya padamu. Buktikan dulu.\"\n\n";

    int finalChoice = getInput(3);

    printSeparator();

    if (finalChoice == 1) {
        // === Ending Percabangan A: Pulang ===
        cout << "\n  [ SECRET ENDING A: Pulang ]\n";
        printSeparator();
        cout << "\n"
             << "  \"Aku ingin pulang.\"\n\n"
             << "  Azar-Rein mengangguk pelan. Tidak ada ekspresi kecewa.\n"
             << "  Tidak ada paksaan.\n\n"
             << "  \"Itu jawaban yang jujur. Aku hormati itu.\"\n\n"
             << "  Ia menjentikkan jarinya.\n"
             << "  Halaman-halaman manga berputar di sekitar kalian -\n"
             << "  semakin cepat, semakin terang, sampai semuanya putih.\n\n"
             << "  Dan kemudian -\n\n"
             << "  ---\n\n"
             << "  Matamu terbuka.\n"
             << "  Langit-langit kamarmu sendiri.\n"
             << "  Bau familiar - kamar Ren. Duniamu.\n\n"
             << "  Di atas mejamu, manga yang kamu beli hari itu masih terbuka\n"
             << "  di halaman terakhir yang sempat kamu baca.\n"
             << "  Yosef Situmorang tersenyum dari panel terakhir.\n\n"
             << "  Kamu tidak tahu bagaimana ceritanya berakhir.\n"
             << "  Tapi entah kenapa, kamu yakin ia baik-baik saja.\n\n"
             << "  Mungkin karena kamu sudah belajar -\n"
             << "  bahkan dari luar ceritanya.\n\n";
    } else if (finalChoice == 2) {
        // === Ending Percabangan B: Tetap ===
        cout << "\n  [ SECRET ENDING B: Penjaga Cerita ]\n";
        printSeparator();
        cout << "\n"
             << "  \"Aku akan tetap.\"\n\n"
             << "  Ada sesuatu yang berubah di mata Azar-Rein.\n"
             << "  Bukan terkejut. Lebih seperti... lega.\n\n"
             << "  \"Aku tidak menyangka.\"\n\n"
             << "  \"Dari sembilan orang sebelummu - tidak ada yang menjawab ini.\"\n\n"
             << "  Ia menaruh tangannya di bahumu.\n\n"
             << "  \"Maka aku beri kamu satu hal yang tidak kuberi siapapun:\n"
             << "   kesadaran penuh. Kamu akan tahu kamu bukan Yosef,\n"
             << "   tapi kamu akan bisa selesaikan ceritanya - dengan caramu.\"\n\n"
             << "  \"Dan ketika cerita ini selesai...\"\n\n"
             << "  Ia tersenyum, dan untuk pertama kali, senyum itu terasa tulus.\n\n"
             << "  \"...mungkin aku yang akan memintamu untuk cerita lain.\"\n\n"
             << "  ---\n\n"
             << "  Enam bulan kemudian, channel Yosef mencapai 8.700 subscriber.\n"
             << "  Ayah menandatangani perjanjian itu.\n"
             << "  Ceritanya selesai - dengan baik.\n\n"
             << "  Dan di taman belakang sekolah yang sama,\n"
             << "  Azar-Rein berdiri di bawah pohon beringin.\n"
             << "  Ia membuka buku - bukan manga. Sesuatu yang lebih tua.\n\n"
             << "  Di dalamnya, ada nama Ren. Dan di sebelahnya,\n"
             << "  sebuah judul cerita baru yang belum kamu kenal.\n\n"
             << "  Ia tersenyum.\n\n"
             << "  \"Sampai jumpa di cerita berikutnya.\"\n\n";
    } else {
        // === Ending Percabangan C: Tantang ===
        cout << "\n  [ SECRET ENDING C: Yang Menantang Raja ]\n";
        printSeparator();
        cout << "\n"
             << "  \"Buktikan dulu.\"\n\n"
             << "  Azar-Rein terdiam.\n\n"
             << "  Satu detik.\n"
             << "  Dua detik.\n\n"
             << "  Lalu ia tertawa.\n\n"
             << "  Bukan tawa jahat. Bukan tawa meremehkan.\n"
             << "  Tawa seseorang yang tidak pernah mendapat jawaban ini.\n\n"
             << "  \"Dari semua jiwa yang pernah jatuh ke dalam cerita,\"\n"
             << "  ia berkata, masih tertawa kecil,\n"
             << "  \"tidak ada yang pernah berani bilang itu kepadaku.\"\n\n"
             << "  Ia menatapmu lama.\n\n"
             << "  \"Baik. Bukti.\"\n\n"
             << "  Ia mengangkat tangan - dan di antara jari-jarinya,\n"
             << "  muncul sebuah halaman manga.\n"
             << "  Di dalamnya: panel truk. Panel langit oranye. Panel Ren.\n\n"
             << "  Harimu yang terakhir - dari sudut pandang orang lain.\n\n"
             << "  \"Itu cukup?\"\n\n"
             << "  ---\n\n"
             << "  Kamu tidak menjawab.\n"
             << "  Karena kamu sudah tahu jawabannya.\n\n"
             << "  Dan Azar-Rein, untuk pertama kali dalam seribu tahun,\n"
             << "  menemukan seseorang yang tidak mau begitu saja percaya -\n"
             << "  bahkan kepada Raja Bayangan.\n\n"
             << "  \"Kuanggap itu 'ya'.\"\n\n"
             << "  \"Lanjutkan ceritanya. Aku akan mengawasi.\"\n\n"
             << "  \"Dan mungkin - suatu hari - kamu yang akan menantangku\n"
             << "   untuk lebih dari sekadar bukti.\"\n\n";
    }

    printSeparator();
    cout << "\n"
         << "              [ SECRET ENDING - SELESAI ]\n\n"
         << "   \"Ada cerita yang ditulis untuk dibaca,\n"
         << "    dan ada yang ditulis untuk dijalani.\n"
         << "    Kamu salah satu dari yang kedua.\"\n"
         << "                              - Azar-Rein\n\n";
    printSeparator();
    cout << "\n  Total restart: " << totalRestarts << " kali.\n";
    cout << "  SECRET ENDING berhasil ditemukan!\n";
    cout << "  Terima kasih sudah memainkan MANGA RESET.\n\n";
}

// ============================================================
// DISPLAY ENDING (Normal)
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
         << "  \"Nilai rata-rata di atas 85 - itu janjiku.\n"
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
         << "  Channel Yosef: 8.700 subscriber.\n"
         << "  Nilai rata-rata Yosef: 87.\n\n"
         << "  Di video terbaru, ada komentar dari akun tanpa foto profil:\n"
         << "  \"Semangat. - Ayah\"\n\n"
         << "  Yosef tidak membalas.\n"
         << "  Tapi ia screenshot dan menjadikannya wallpaper.\n\n";
    printSeparator();
    cout << "\n"
         << "              [ ENDING - SELESAI ]\n\n"
         << "   \"Kompromi bukan berarti kalah -\n"
         << "    kadang itu cara paling elegan untuk menang bersama.\"\n\n";
    printSeparator();
    cout << "\n  Total restart: " << totalRestarts << " kali.\n";
    cout << "  Terima kasih sudah memainkan MANGA RESET.\n\n";
}

// ============================================================
// PLAY CHAPTER
// ============================================================

bool playChapter(int chapterId, Event* pool, GameState& gs) {
    ChapterNode* cn = findChapterNode(chapterId);
    if (!cn) return false;

    gs.points = STARTING_POINT;
    gs.currentChapter = chapterId;
    bool isRestart = false;

    // *** Jika Chapter 2 dan syarifTrustGiven, masukkan secret event ke pool sesi ini ***
    // Kita tandai apakah secret event sudah pernah dimainkan sesi ini
    bool secretEventUsed = false;

    while (true) {
        ActiveEventList activeList;
        randomPickEvents(pool, POOL_SIZE, MIN_EVENTS, MAX_EVENTS, activeList);

        // *** SECRET: Sisipkan secret event di Chapter 2 jika syarat terpenuhi ***
        if (chapterId == 2 && gs.syarifTrustGiven && !secretEventUsed) {
            activeList.insertNode(secretEventSyarif);
        }

        displayChapterHeader(cn, gs.points);
        cout << "\n  " << activeList.count << " aktivitas tersedia sesi ini.\n";

        // *** Hint kecil jika secret flag aktif ***
        if (chapterId == 2 && gs.syarifTrustGiven && !secretEventUsed) {
            cout << "\n  [Petunjuk tersembunyi terdeteksi di sesi ini...]\n";
        }

        pressEnter();

        while (!activeList.isEmpty()) {
            clearScreen();
            displayChapterHeader(cn, gs.points);
            displayPointBar(gs.points);

            activeList.displayMenu();
            cout << "  [" << (activeList.count + 1) << "] Lihat daftar karakter\n";
            printLine();

            int menuChoice = getInput(activeList.count + 1);

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

            Event* selectedEvent = activeList.getByMenuIndex(menuChoice);
            if (!selectedEvent) continue;

            int eventId = selectedEvent->id;

            // *** Cek apakah ini event DM Syarif (205) untuk set flag ***
            bool isDMSyarif = (eventId == 205);

            int delta = playEvent(selectedEvent, gs.points);
            gs.points += delta;

            // *** SET FLAG: jika pemain pilih opsi [2] di event 205 (delta = +25 dan response mengandung tanda tangan kunci) ***
            // Cara deteksinya: kita track sebelum playEvent, apakah choice index yang dipilih adalah index ke-1 (0-based)
            // Namun karena playEvent tidak return choice index, kita gunakan trik:
            // delta == +25 dari event 205 adalah HANYA pilihan ke-2 (index 1)
            if (isDMSyarif && delta == 25) {
                gs.syarifTrustGiven = true;
                cout << "\n  [Sesuatu terasa berbeda setelah pesan itu...]\n";
                pressEnter();
            }

            // *** Cek apakah secret event dimainkan ***
            if (eventId == 210) {
                secretEventUsed = true;
                // Pilihan [1] (delta = +30) mengaktifkan secret ending
                if (delta == 30) {
                    gs.secretFlag = true;
                    cout << "\n  [Sebuah jalur tersembunyi terbuka...]\n";
                    pressEnter();
                }
            }

            activeList.deleteNode(eventId);

            if (gs.points >= WIN_POINT) {
                cout << "\n  Poin mencapai " << gs.points
                     << "! Kamu berhasil melewati chapter ini!\n";
                pressEnter();
                return true;
            }
            if (gs.points <= 0) {
                gs.restartCount++;
                displayRestart(gs.restartCount);
                gs.points = STARTING_POINT;
                isRestart = true;
                secretEventUsed = false; // reset per sesi
                break;
            }
        }

        if (gs.points > 0 && gs.points < WIN_POINT && !isRestart) {
            cout << "\n  Semua aktivitas selesai. Poin: " << gs.points << "\n";
            cout << "  Belum cukup untuk lanjut. Sesi baru dimulai...\n";
            pressEnter();
        } else {
            isRestart = false;
        }
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    buildChapterGraph();
    buildPoolChapter1();
    buildPoolChapter2();

    // Init game state - tambah secretFlag dan syarifTrustGiven
    GameState gs = {1, STARTING_POINT, 0, false, false, false, false};

    // ── Title Screen ─────────────────────────────────────────
    printSeparator();
    cout << "\n";
    cout << "        MANGA RESET: Dunia yang Kulupa\n";
    cout << "\n";
    cout << "  Sebuah cerita tentang pilihan, keluarga, dan impian.\n";
    cout << "  Kumpulkan 100 poin di setiap chapter untuk melanjutkan.\n";
    cout << "  Poin habis - kamu kembali ke awal chapter.\n";
    cout << "\n";
    cout << "  [ Ada sesuatu yang tersembunyi di balik cerita ini... ]\n";
    cout << "\n";
    printSeparator();
    pressEnter();

    // ── Prolog ───────────────────────────────────────────────
    printSeparator();
    cout << "\n  Prolog\n";
    printSeparator();
    cout << "\n"
         << "  Namaku Ren. 18 tahun. Otaku.\n\n"
         << "  Hari itu biasa saja - pulang dari toko manga,\n"
         << "  kantong plastik di tangan, langit sore berwarna oranye.\n\n"
         << "  Dan kemudian... truk.\n\n"
         << "  Aku tidak sempat berteriak.\n\n"
         << "  ---\n\n"
         << "  Saat mataku terbuka, ini bukan kamarku.\n"
         << "  Ada poster jadwal pelajaran. Meja belajar penuh buku.\n"
         << "  Satu laptop tua yang sengaja disembunyikan di balik binder.\n\n"
         << "  Di cermin di depanku... bukan wajahku.\n\n"
         << "  \"Ini... Yosef?\"\n\n"
         << "  Yosef Situmorang. MC dari sebuah manga slice of life\n"
         << "  yang aku baca sekitar 5 tahun lalu.\n\n"
         << "  Hidupnya keras. Keluarganya keras.\n"
         << "  Dan ada cerita yang harus kujalani.\n\n"
         << "  Semoga aku ingat cukup banyak untuk tidak merusaknya.\n\n";
    pressEnter();

    // ── Game Loop ─────────────────────────────────────────────
    int currentIndex = 0;

    while (true) {
        ChapterNode* cn = &chapterGraph[currentIndex];

        if (cn->id == 99) {
            displayChapterGraph();

            // *** Cek secret ending sebelum ending normal ***
            if (gs.secretFlag) {
                printSeparator();
                cout << "\n  Sebelum kamu sampai ke ending yang kamu kenal -\n";
                cout << "  ada sesuatu yang masih belum terselesaikan.\n";
                cout << "  Sebuah pertemuan. Sebuah kebenaran.\n";
                printSeparator();
                pressEnter();
                displaySecretEnding(gs.restartCount);
            } else {
                displayEnding(gs.restartCount);

                // *** Hint secret ending jika syarat hampir terpenuhi ***
                if (gs.syarifTrustGiven && !gs.secretFlag) {
                    printSeparator();
                    cout << "\n  [ Kamu dekat dengan sesuatu yang tersembunyi.\n";
                    cout << "    Mungkin lain kali, jawaban yang berbeda\n";
                    cout << "    akan membuka pintu yang berbeda pula. ]\n";
                    printSeparator();
                }
            }
            break;
        }

        Event* pool = (cn->id == 1) ? poolChapter1 : poolChapter2;

        bool won = playChapter(cn->id, pool, gs);

        if (won) {
            int nextIndex = getNextChapterIndex(currentIndex);
            if (nextIndex == -1) break;
            currentIndex = nextIndex;
        }
    }

    printSeparator();
    cout << "  [ Program selesai. ]\n";
    printSeparator();
    cout << "\n";

    return 0;
}