/*
 * ============================================================
 *  MANGA RESET: Dunia yang Kulupa
 *  A Terminal Story Game in C++
 * ============================================================
 *  Data Structures Used:
 *  - Array       : dialog pools, choice arrays, reset monologs
 *  - Struct      : Character, Scene, Choice, GameState
 *  - Pointer     : Scene navigation via node pointers
 *  - Linked List : Story chapter chain & dialog history
 *  - Stack       : Game state save/restore for reset system
 *  - Queue       : Dialog display queue (sequential narration)
 *  - Graph/Tree  : Branching story tree (choices -> outcomes)
 * ============================================================
 */

#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <vector>
#include <limits>
#include <cstdlib>

using namespace std;

// ============================================================
// STRUCT DEFINITIONS
// ============================================================

struct Character {
    string name;
    string role;         // "protagonist", "npc"
    string description;
};

struct Choice {
    string text;
    int    nextSceneId;  // -1 = wrong choice (triggers reset)
    bool   isCorrect;
};

struct Scene {
    int           id;
    string        title;
    string        narration;        // main story text
    vector<string> dialogs;         // sequential dialog lines
    Choice        choices[4];       // up to 4 choices (array)
    int           choiceCount;
    bool          isEnding;
    int           endingType;       // 1, 2, 3
};

struct GameState {
    int  currentSceneId;
    int  resetCount;
    bool gameOver;
    bool reachedEnding;
};

// ============================================================
// LINKED LIST — Dialog History
// ============================================================

struct DialogNode {
    string     text;
    DialogNode* next;
};

struct DialogHistory {
    DialogNode* head;
    int         count;

    DialogHistory() : head(nullptr), count(0) {}

    void push(const string& text) {
        DialogNode* node = new DialogNode{text, head};
        head = node;
        count++;
    }

    void printAll() const {
        cout << "\n--- Riwayat Pilihanmu ---\n";
        DialogNode* cur = head;
        // collect to print in order
        vector<string> temp;
        while (cur) {
            temp.push_back(cur->text);
            cur = cur->next;
        }
        for (int j = (int)temp.size() - 1; j >= 0; j--) {
            cout << "  " << (count - j) << ". " << temp[j] << "\n";
        }
        cout << "------------------------\n";
    }

    ~DialogHistory() {
        DialogNode* cur = head;
        while (cur) {
            DialogNode* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }
};

// ============================================================
// LINKED LIST — Story Chapter Chain
// ============================================================

struct ChapterNode {
    int          sceneId;
    string       chapterTitle;
    ChapterNode* next;
};

struct ChapterList {
    ChapterNode* head;
    ChapterNode* tail;

    ChapterList() : head(nullptr), tail(nullptr) {}

    void append(int sceneId, const string& title) {
        ChapterNode* node = new ChapterNode{sceneId, title, nullptr};
        if (!tail) { head = tail = node; }
        else { tail->next = node; tail = node; }
    }

    void printProgress(int currentId) const {
        cout << "\n--- Progress Cerita ---\n";
        ChapterNode* cur = head;
        while (cur) {
            if (cur->sceneId == currentId)
                cout << "  >> " << cur->chapterTitle << " (sekarang)\n";
            else if (cur->sceneId < currentId)
                cout << "  [v] " << cur->chapterTitle << "\n";
            cur = cur->next;
        }
        cout << "-----------------------\n";
    }

    ~ChapterList() {
        ChapterNode* cur = head;
        while (cur) {
            ChapterNode* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }
};

// ============================================================
// STACK — Game State for Reset System
// ============================================================

struct StateStack {
    stack<GameState> stk;

    void save(const GameState& gs) { stk.push(gs); }

    GameState restore() {
        if (!stk.empty()) {
            GameState gs = stk.top();
            // don't pop — we restore to the very first state always
            return gs;
        }
        return {0, 0, false, false};
    }

    GameState getInitial() {
        // get bottom of stack = initial state
        stack<GameState> tmp = stk;
        GameState init;
        while (!tmp.empty()) { init = tmp.top(); tmp.pop(); }
        return init;
    }
};

// ============================================================
// QUEUE — Dialog Display
// ============================================================

void displayDialogQueue(vector<string>& lines) {
    queue<string> q;
    for (auto& l : lines) q.push(l);

    while (!q.empty()) {
        cout << q.front() << "\n";
        q.pop();
    }
}

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

void pressEnter() {
    cout << "\n[Tekan ENTER untuk melanjutkan...]";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void printSeparator() {
    cout << "\n============================================================\n";
}

void printLine() {
    cout << "------------------------------------------------------------\n";
}

int getChoice(int max) {
    int c;
    while (true) {
        cout << "\n> Pilihanmu: ";
        if (cin >> c && c >= 1 && c <= max) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return c;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  Masukkan angka antara 1 dan " << max << ".\n";
    }
}

// ============================================================
// RESET MONOLOGS — Array of strings
// ============================================================

const string resetMonologs[] = {
    // reset 1
    "\"Eh—?!\"\n\n"
    "Dunia berputar. Langit jadi gelap. Aku merasakan tubuhku seperti\n"
    "ditarik paksa ke belakang, ke dalam sesuatu yang dingin dan kosong.\n\n"
    "Dan tiba-tiba... aku di sini lagi.\n\n"
    "\"W-wait. Ini... ini bukan pertama kalinya aku lihat ini...\"\n\n"
    "Aku salah. Pasti aku salah pilih. Sial.\n"
    "Baik. Oke. Aku harus ingat-ingat lagi cerita manga itu...\n",

    // reset 2
    "\"TIDAK TIDAK TIDAK!\"\n\n"
    "Lagi. Lagi dunia ini menarikku mundur.\n"
    "Sensasinya seperti tenggelam — tapi ke atas. Aneh banget.\n\n"
    "Dan semuanya... reset.\n\n"
    "\"Ugh, aku udah dua kali gagal. Dua kali!\"\n\n"
    "Aku mengepalkan tangan. Coba tenang. Pikir.\n"
    "Manga itu... judulnya apa ya... aku baca 5 tahun lalu, saat kelas 2 SMP.\n"
    "Kenapa aku nggak ingat-ingat lagi waktu itu?!\n",

    // reset 3
    "Kali ini tidak ada teriakan.\n\n"
    "Aku sudah tahu rasanya. Dunia berputar, gelap, dingin.\n"
    "Lalu muncul lagi. Di sini. Di awal.\n\n"
    "\"...Tiga kali.\"\n\n"
    "Aku duduk diam sebentar sebelum semuanya benar-benar muncul lagi.\n"
    "Oke. Sistemnya jelas: ada satu pilihan yang benar di setiap situasi.\n"
    "Kalau salah, balik ke awal. Tanpa ampun.\n\n"
    "Aku harus berpikir seperti pembaca manga, bukan seperti diri sendiri.\n"
    "Apa yang akan dilakukan MC yang benar?\n",

    // reset 4
    "\"...Ha.\"\n\n"
    "Aku ketawa kecil waktu dunia itu mulai berputar lagi.\n"
    "Empat kali. Sudah empat kali.\n\n"
    "Entah kenapa sekarang rasanya lebih seperti permainan daripada mimpi buruk.\n"
    "Atau mungkin aku mulai gila? Dua kemungkinan yang sama valid.\n\n"
    "\"Oke, Haru.\" — itu nama MC di manga ini kalau aku nggak salah ingat —\n"
    "\"Kamu tipe anak yang gimana sih?\"\n\n"
    "Aku mulai mengingat. Sedikit demi sedikit.\n",

    // reset 5+
    "Dunia berputar. Gelap. Dingin. Muncul lagi.\n\n"
    "Sudah berapa kali ini? Aku sudah berhenti menghitung.\n\n"
    "Tapi aku semakin dekat. Aku bisa merasakannya.\n"
    "Setiap kali aku gagal, aku mendapat satu petunjuk lebih.\n\n"
    "\"Aku pasti bisa. Pasti.\"\n\n"
    "Kali ini — aku sudah ingat lebih banyak.\n"
    "Ayo, Haru. Ayo.\n",
};
const int RESET_MONOLOG_COUNT = 5;

// ============================================================
// SCENE DATABASE — Story Tree (Graph/Tree)
// ============================================================
/*
 *  Scene Graph:
 *
 *  0 (Prolog)
 *  └─► 1 (Bangun Pagi)
 *      ├─► RESET (salah)
 *      └─► 2 (Sarapan)
 *          ├─► RESET (salah)
 *          └─► 3 (Pulang Sekolah)
 *              ├─► RESET (salah)
 *              └─► 4 (Malam — Buat Konten)
 *                  ├─► RESET (salah)
 *                  └─► 5 (Ketahuan)
 *                      ├─► RESET (salah)
 *                      └─► 6 (Konfrontasi)
 *                          ├─► RESET (salah)
 *                          └─► 7 (Persimpangan)
 *                              ├─► ENDING 1: Patuh
 *                              ├─► ENDING 2: Lari
 *                              └─► ENDING 3: Negosiasi
 */

Scene scenes[20];
int   sceneCount = 0;

void buildSceneDatabase() {

    // ── SCENE 0 : Prolog ─────────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id            = 0;
        s.title         = "Prolog: Dunia yang Kulupa";
        s.choiceCount   = 0;
        s.isEnding      = false;
        s.narration =
            "Namaku Ren. 18 tahun. Otaku.\n\n"
            "Hari itu biasa saja. Aku pulang dari toko manga langgananku,\n"
            "menenteng tiga volume baru sambil mempertimbangkan mau baca yang mana\n"
            "duluan. Langit sore berwarna oranye. Jalanan ramai.\n\n"
            "Dan kemudian...\n\n"
            "...truk.\n\n"
            "Aku tidak sempat berteriak.\n\n"
            "---\n\n"
            "Saat mataku terbuka, ini bukan kamarku.\n"
            "Ini bukan langit-langit putih yang biasa kulihat setiap pagi.\n\n"
            "Ini kamar orang lain. Rapi. Akademis. Ada poster jadwal pelajaran\n"
            "di dinding. Ada meja belajar penuh buku. Ada satu laptop tua\n"
            "di pojokan yang tampaknya sengaja disembunyikan di balik tumpukan binder.\n\n"
            "Dan di cermin di depanku...\n"
            "bukan wajahku.\n\n"
            "\"Ini... Haru?\"\n\n"
            "Suara yang keluar dari mulutku bukan suaraku.\n"
            "Tapi aku ingat nama itu.\n\n"
            "Haru Mizushima. MC dari sebuah manga slice of life yang aku baca\n"
            "sekitar 5 tahun lalu. Judulnya... apa ya...\n"
            "Aku sudah lupa banyak detailnya.\n\n"
            "Yang aku ingat: hidupnya keras. Keluarganya keras.\n"
            "Dan ada satu cerita yang harus dijalani.\n\n"
            "Aku tidak punya pilihan lain selain... menjalaninya.\n\n"
            "Semoga aku ingat cukup banyak untuk tidak merusaknya.\n";
        s.dialogs = {};
    }

    // ── SCENE 1 : Bangun Pagi ────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 1;
        s.title       = "Chapter 1: Pagi di Keluarga Mizushima";
        s.isEnding    = false;
        s.narration =
            "Alarm berbunyi pukul 05.30.\n\n"
            "Dari bawah terdengar suara langkah kaki tegas — itu pasti Ayah.\n"
            "Suara pintu dapur dibuka. Suara kompor. Aroma nasi.\n\n"
            "Di meja belajar ada selembar kertas. Jadwal harian yang ditulis tangan\n"
            "dengan spidol merah oleh seseorang — pasti orang tua Haru.\n\n"
            "  05.30 - Bangun\n"
            "  05.45 - Olahraga pagi (lari 2 km)\n"
            "  06.15 - Mandi & siap\n"
            "  06.45 - Sarapan bersama\n"
            "  07.00 - Berangkat sekolah\n\n"
            "Di pojok kertas ada tulisan kecil: 'Kalau terlambat satu menit,\n"
            "tidak ada uang jajan seminggu.'\n\n"
            "Aku menatap jadwal itu. Lari 2 km sebelum subuh?\n"
            "Ini serius?\n\n"
            "Tapi aku ingat samar-samar — di manga ini, ada momen pagi yang jadi\n"
            "turning point. Haru melakukan sesuatu sebelum sarapan.\n"
            "Apa ya...\n";
        s.dialogs = {};
        s.choiceCount = 3;
        s.choices[0] = {"Ikuti jadwal — langsung lari 2 km sesuai yang tertulis.", 2, true};
        s.choices[1] = {"Tidur lagi sebentar. Masih ngantuk dan capek.", -1, false};
        s.choices[2] = {"Duduk di meja belajar dan mulai belajar — lebih produktif daripada lari.", -1, false};
    }

    // ── SCENE 2 : Sarapan ────────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 2;
        s.title       = "Chapter 2: Meja Makan Keluarga Mizushima";
        s.isEnding    = false;
        s.narration =
            "Meja makan keluarga Mizushima adalah tempat paling tegang di rumah ini.\n\n"
            "Ayah — Mizushima Kenji — duduk di ujung meja dengan koran di tangan.\n"
            "Ia tidak melihat ke arahku. Tapi kehadirannya terasa seperti tekanan fisik.\n"
            "Ibu menyajikan miso sup tanpa bicara.\n\n"
            "Kakakku — Hana, 21 tahun — sudah duduk rapi dengan seragam kantornya.\n"
            "Ia adalah 'standar' yang selalu disebut Ayah: 'Lihat kakakmu, nilai sempurna,\n"
            "kerja di perusahaan bagus, tidak memalukan.'\n\n"
            "Aku — Haru — duduk dan mulai makan.\n\n"
            "Ayah menaruh koran. Menatapku.\n\n"
            "\"Haru. Nilai ulangan matematika minggu lalu.\"\n\n"
            "Bukan pertanyaan. Pernyataan. Ia sudah tahu.\n\n"
            "Aku ingat samar-samar — ada momen di manga ini soal nilai.\n"
            "Haru mendapat nilai berapa ya... 78? 82?\n"
            "Yang penting, jawabannya bukan sempurna.\n\n"
            "Ayah menunggu.\n";
        s.dialogs = {
            "Ayah: \"Nilai ulangan matematika minggu lalu.\"\n",
        };
        s.choiceCount = 3;
        s.choices[0] = {"Jawab jujur dengan tenang: \"78, Ayah. Aku akan lebih giat belajar.\"", 3, true};
        s.choices[1] = {"Diam saja dan pura-pura tidak dengar.", -1, false};
        s.choices[2] = {"Berbohong: \"Sudah bagus kok, Ayah. Di atas rata-rata kelas.\"", -1, false};
    }

    // ── SCENE 3 : Pulang Sekolah ─────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 3;
        s.title       = "Chapter 3: Sepulang Sekolah";
        s.isEnding    = false;
        s.narration =
            "Sekolah berakhir pukul 15.30.\n\n"
            "Teman sekelas Haru — Dito dan Nisa — mengajakku mampir ke kafe kecil\n"
            "dekat sekolah. Mereka tahu aku jarang bisa keluar.\n\n"
            "Dito: \"Ayolah, Haru. Sekali-kali. Kita nggak lama.\"\n"
            "Nisa: \"Iya, ada konten kreator yang lagi live di sana. Tau nggak, si Kaito?\n"
            "       Dia yang bikin video tentang kehidupan SMA itu. Seru banget.\"\n\n"
            "Hatiku bereaksi ke nama itu.\n"
            "Kaito. Ya — aku ingat. Di manga ini Kaito adalah orang yang pertama kali\n"
            "menginspirasi Haru untuk mulai membuat konten.\n\n"
            "Tapi ada jadwal. Haru harus pulang sebelum pukul 16.00.\n"
            "Kalau terlambat, Ayah sudah menunggu di depan pintu dengan muka datar\n"
            "yang sepuluh kali lebih menakutkan daripada ekspresi marah.\n\n"
            "Aku melihat jam. 15.40.\n";
        s.dialogs = {
            "Dito: \"Ayolah, Haru. Sekali-kali.\"\n",
            "Nisa: \"Ada Kaito lagi live di sana! Yang bikin video kehidupan SMA itu.\"\n",
        };
        s.choiceCount = 3;
        s.choices[0] = {"Tolak dengan sopan dan langsung pulang tepat waktu.", 4, true};
        s.choices[1] = {"Ikut ke kafe — mumpung ada Kaito, ini kesempatan langka.", -1, false};
        s.choices[2] = {"Minta Nisa kirimkan rekaman live-nya saja, lalu pulang.", -1, false};
    }

    // ── SCENE 4 : Malam — Buat Konten ────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 4;
        s.title       = "Chapter 4: Kamar dan Rahasia";
        s.isEnding    = false;
        s.narration =
            "Pukul 22.00. Rumah sudah sepi.\n\n"
            "Suara TV dari kamar orang tua sudah mati sejak setengah jam lalu.\n"
            "Hana sudah tidur. Ayah dan Ibu sudah tidur.\n\n"
            "Aku duduk di depan laptop tua yang tersembunyi di balik binder.\n"
            "Layarnya redup — aku sudah pasang kecerahan paling rendah.\n"
            "Kipasnya berisik, jadi aku taruh bantal tipis di bawahnya.\n\n"
            "Di channel YouTube Haru yang masih kecil — 47 subscriber —\n"
            "ada draft video yang belum diupload.\n\n"
            "Judulnya: 'Hari Biasa Pelajar yang Tidak Bebas — Vlog #3'\n\n"
            "Aku menatap tombol upload.\n\n"
            "Di manga ini... apa yang dilakukan Haru malam itu?\n"
            "Aku ingat ada momen krusial. Ia membuat sesuatu.\n"
            "Bukan sekadar upload. Tapi sesuatu yang lebih... personal.\n";
        s.dialogs = {};
        s.choiceCount = 4;
        s.choices[0] = {"Upload video yang sudah ada, lalu tidur.", -1, false};
        s.choices[1] = {"Rekam video baru — curahan hati soal keluarga — dengan berbisik pelan.", 5, true};
        s.choices[2] = {"Tutup laptop dan tidur. Terlalu berisiko malam ini.", -1, false};
        s.choices[3] = {"Edit video lama sampai sempurna, tapi tidak upload.", -1, false};
    }

    // ── SCENE 5 : Ketahuan ───────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 5;
        s.title       = "Chapter 5: Suara di Balik Pintu";
        s.isEnding    = false;
        s.narration =
            "Aku sedang berbisik ke kamera — menceritakan hari ini, perasaan tertekan,\n"
            "mimpi yang terasa semakin jauh — ketika...\n\n"
            "Ketukan pintu.\n\n"
            "Bukan ketukan biasa. Tiga kali. Pelan. Tapi tegas.\n\n"
            "Aku mematikan laptop dalam satu gerakan. Jantungku berhenti sedetik.\n\n"
            "\"Haru.\"\n\n"
            "Suara Ayah.\n\n"
            "Hening. Ia menunggu di luar.\n\n"
            "Aku menatap pintu. Laptop sudah mati. Kabel sudah kusembunyikan.\n"
            "Tapi apakah ia mendengar?\n\n"
            "Di manga ini, momen ini adalah salah satu yang paling aku ingat.\n"
            "Bukan karena dramanya — tapi karena reaksi Haru.\n"
            "Ia tidak panik. Ia melakukan sesuatu yang sangat spesifik.\n";
        s.dialogs = {
            "Ayah: \"Haru.\"\n",
            "Hening.\n",
            "Ayah: \"Sudah jam berapa sekarang?\"\n",
        };
        s.choiceCount = 4;
        s.choices[0] = {"Buka pintu secepatnya dan bilang: \"Baru saja mau tidur, Ayah.\"", -1, false};
        s.choices[1] = {"Pura-pura sudah tidur — tidak menjawab sama sekali.", -1, false};
        s.choices[2] = {"Jawab dari balik pintu dengan suara mengantuk: \"Sudah mau tidur, Ayah. Maaf.\"", 6, true};
        s.choices[3] = {"Buka pintu dan tanya balik: \"Ada apa, Ayah?\"", -1, false};
    }

    // ── SCENE 6 : Konfrontasi ────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 6;
        s.title       = "Chapter 6: Pagi yang Berbeda";
        s.isEnding    = false;
        s.narration =
            "Keesokan paginya.\n\n"
            "Aku turun ke meja makan seperti biasa.\n"
            "Tapi atmosfernya berbeda.\n\n"
            "Ibu tidak bicara. Hana pergi lebih awal dari biasanya.\n"
            "Dan Ayah — Ayah duduk dengan selembar kertas di tangannya.\n\n"
            "Aku melihat apa itu.\n\n"
            "Laporan tagihan internet bulanan. Dengan highlight kuning di satu baris:\n"
            "akses ke platform video malam hari, pukul 22.15, durasi 47 menit.\n\n"
            "\"Duduk.\"\n\n"
            "Aku duduk.\n\n"
            "Ayah menaruh kertas itu di hadapanku.\n\n"
            "\"Jelaskan.\"\n\n"
            "Ini dia. Momen yang aku takutkan.\n"
            "Dan sekaligus... momen yang aku tunggu.\n"
            "Karena di manga ini, Haru tidak menyerah di sini.\n"
            "Ia melakukan sesuatu. Tapi apa?\n";
        s.dialogs = {
            "Ayah: \"Duduk.\"\n",
            "Ayah: \"Jelaskan.\"\n",
            "Haru menatap kertas tagihan itu.\n",
        };
        s.choiceCount = 4;
        s.choices[0] = {"Minta maaf dan berjanji tidak mengulangi.", -1, false};
        s.choices[1] = {"Marah dan berteriak: \"Aku hanya ingin sedikit kebebasan!\"", -1, false};
        s.choices[2] = {"Tunjukkan statistik channel: 47 subscriber, engagement rate, dan bilang ini serius.", 7, true};
        s.choices[3] = {"Diam saja dan terima hukuman.", -1, false};
    }

    // ── SCENE 7 : Persimpangan — Pilih Ending ────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 7;
        s.title       = "Chapter 7: Persimpangan";
        s.isEnding    = false;
        s.narration =
            "Ayah diam lama setelah melihat statistik channel Haru.\n\n"
            "Bukan diam yang marah. Diam yang... memproses.\n\n"
            "Ibu masuk ke ruang makan dengan teh — ia pasti sudah mendengar semuanya\n"
            "dari dapur.\n\n"
            "Ayah akhirnya bicara:\n"
            "\"Aku tidak pernah bilang kamu harus menjadi apa.\"\n"
            "\"Aku hanya tidak mau kamu menyesal.\"\n\n"
            "Kalimat itu menggantung di udara.\n\n"
            "Haru — aku — menatap Ayah.\n"
            "Dan untuk pertama kali dalam waktu yang lama, aku melihat sesuatu\n"
            "di balik ekspresi keras itu:\n"
            "kekhawatiran.\n\n"
            "Ini persimpangan.\n"
            "Di manga ini ada beberapa jalan yang bisa dipilih Haru.\n"
            "Semua valid. Semua dengan konsekuensinya masing-masing.\n\n"
            "Apa pilihanmu?\n";
        s.dialogs = {
            "Ayah: \"Aku tidak pernah bilang kamu harus menjadi apa.\"\n",
            "Ayah: \"Aku hanya tidak mau kamu menyesal.\"\n",
        };
        s.choiceCount = 3;
        // All 3 lead to endings
        s.choices[0] = {"[ENDING 1] Setuju dengan Ayah — prioritaskan akademik, jadikan konten hobi sampingan.", 10, true};
        s.choices[1] = {"[ENDING 2] Tolak dengan hormat — minta izin resmi untuk serius jadi content creator.", 11, true};
        s.choices[2] = {"[ENDING 3] Tawarkan kompromi — nilai tetap dijaga, tapi diberi waktu khusus untuk konten.", 12, true};
    }

    // ── ENDING 1 ─────────────────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 10;
        s.title       = "ENDING 1: Anak yang Patuh";
        s.isEnding    = true;
        s.endingType  = 1;
        s.choiceCount = 0;
        s.narration =
            "\"Baik, Ayah. Aku mengerti.\"\n\n"
            "Aku menunduk.\n\n"
            "Tidak ada drama. Tidak ada air mata.\n"
            "Hanya... ketenangan yang agak menyakitkan.\n\n"
            "Aku menutup laptop itu rapat-rapat malam itu.\n"
            "Channel dengan 47 subscriber itu tidak diupdate selama tiga bulan.\n\n"
            "Tapi nilai matematikaku naik ke 91.\n"
            "Ayah mengangguk saat melihat rapor.\n"
            "Itu sudah lebih dari cukup untuk Ibu menangis haru.\n\n"
            "Suatu sore, diam-diam, aku membuka channel itu lagi.\n"
            "Bukan untuk upload.\n"
            "Hanya untuk melihat.\n\n"
            "Subscriber masih 47.\n"
            "Tapi ada satu komentar baru dari tiga bulan lalu:\n"
            "\"Haru, video-videomu bikin aku ngerasa nggak sendiri. Kapan upload lagi?\"\n\n"
            "Aku menutup tab itu.\n"
            "Besok masih ada ujian.\n\n"
            "---\n\n"
            "Dunia manga ini berjalan seperti seharusnya.\n"
            "Dan aku — Ren — merasakan sesuatu yang hangat sekaligus berat\n"
            "saat kesadaranku perlahan kembali ke dunia asalku.\n\n"
            "Tidak semua cerita berakhir dengan dramatis.\n"
            "Kadang ending paling nyata adalah yang paling sunyi.\n\n"
            "                    [ ENDING 1 — SELESAI ]\n"
            "          \"Kadang memilih damai adalah bentuk keberanian juga.\"\n";
        s.dialogs = {};
    }

    // ── ENDING 2 ─────────────────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 11;
        s.title       = "ENDING 2: Langkah Sendiri";
        s.isEnding    = true;
        s.endingType  = 2;
        s.choiceCount = 0;
        s.narration =
            "\"Aku minta maaf, Ayah. Tapi ini yang aku mau.\"\n\n"
            "Suara Haru tidak gemetar.\n"
            "Itu mengejutkan Ayah — aku bisa melihatnya dari cara alisnya sedikit naik.\n\n"
            "\"Aku tahu risikonya. Aku tahu ini tidak mudah.\n"
            " Tapi aku ingin mencobanya dengan sungguh-sungguh.\n"
            " Bukan diam-diam. Secara resmi. Dengan persetujuanmu — atau tanpanya.\"\n\n"
            "Hening yang panjang.\n\n"
            "Ayah berdiri. Berjalan ke jendela. Memandang ke luar.\n\n"
            "\"Kamu tidak akan berubah pikiran?\"\n\n"
            "\"Tidak.\"\n\n"
            "Ayah tidak bilang iya. Tapi ia juga tidak bilang tidak.\n"
            "Ia hanya mengangguk satu kali, pelan, dan pergi ke kamarnya.\n\n"
            "Itu sudah cukup.\n\n"
            "Tiga bulan kemudian, channel Haru punya 2.400 subscriber.\n"
            "Video tentang 'Keluarga Keras dan Impian yang Tidak Boleh Mati'\n"
            "ditonton 180.000 kali.\n\n"
            "Ayah tidak pernah menyebut angka itu.\n"
            "Tapi suatu hari Ibu bisik ke Haru:\n"
            "\"Ayahmu... aku lihat dia menonton videomu semalam.\"\n\n"
            "---\n\n"
            "Aku — Ren — tersenyum sebelum dunia ini perlahan memudar.\n\n"
            "Ada yang tidak perlu disetujui untuk tetap dicintai.\n\n"
            "                    [ ENDING 2 — SELESAI ]\n"
            "              \"Berani bukan berarti tidak takut —\n"
            "               berani adalah tetap melangkah meski takut.\"\n";
        s.dialogs = {};
    }

    // ── ENDING 3 ─────────────────────────────────────────────
    {
        Scene& s = scenes[sceneCount++];
        s.id          = 12;
        s.title       = "ENDING 3: Jalan Tengah";
        s.isEnding    = true;
        s.endingType  = 3;
        s.choiceCount = 0;
        s.narration =
            "\"Aku punya usul, Ayah.\"\n\n"
            "Aku membuka halaman baru di buku catatanku.\n"
            "Menulis dua kolom: AKADEMIK dan KONTEN.\n\n"
            "\"Nilai rata-rata di atas 85 — itu janjiku.\n"
            " Sebagai gantinya, aku minta dua jam setiap hari Jumat dan Sabtu.\n"
            " Kalau nilaiku turun di bawah itu, aku sendiri yang akan berhenti.\"\n\n"
            "Ayah menatap kertasku. Lama.\n\n"
            "Ibu meletakkan tehnya. \"Kenji...\"\n\n"
            "Ayah mengambil kertasku. Membaca dua kali.\n"
            "Lalu ia mengambil pena dari sakunya.\n"
            "Dan menandatangani di bawah tulisanku.\n\n"
            "Satu tanda tangan kecil yang rasanya lebih besar\n"
            "dari semua kata-kata yang pernah tidak terucapkan di antara mereka.\n\n"
            "---\n\n"
            "Enam bulan kemudian, channel Haru punya 8.700 subscriber.\n"
            "Nilai rata-rata Haru: 87.\n\n"
            "Di video terbaru Haru — judulnya 'Perjanjian dengan Ayah' —\n"
            "ada komentar dari akun tanpa foto profil:\n"
            "\"Semangat. - Ayah\"\n\n"
            "Haru tidak membalas komentar itu.\n"
            "Tapi ia screenshot dan menjadikannya wallpaper.\n\n"
            "---\n\n"
            "Dunia ini memudar.\n"
            "Aku — Ren — membuka mata di tempat yang familiar:\n"
            "jalanan. Sore. Kantong plastik manga di tangan.\n\n"
            "Tidak ada truk kali ini.\n\n"
            "Aku berjalan pulang. Dan mungkin besok,\n"
            "aku akan mencari manga itu lagi — judulnya apa ya...\n\n"
            "                    [ ENDING 3 — SELESAI ]\n"
            "         \"Kompromi bukan berarti kalah —\n"
            "          kadang itu cara paling elegan untuk menang bersama.\"\n";
        s.dialogs = {};
    }
}

// ============================================================
// SCENE LOOKUP (Graph traversal by ID)
// ============================================================

Scene* findScene(int id) {
    for (int i = 0; i < sceneCount; i++) {
        if (scenes[i].id == id) return &scenes[i];
    }
    return nullptr;
}

// ============================================================
// CHAPTER LIST BUILDER
// ============================================================

void buildChapterList(ChapterList& cl) {
    cl.append(0,  "Prolog: Dunia yang Kulupa");
    cl.append(1,  "Chapter 1: Pagi di Keluarga Mizushima");
    cl.append(2,  "Chapter 2: Meja Makan");
    cl.append(3,  "Chapter 3: Sepulang Sekolah");
    cl.append(4,  "Chapter 4: Kamar dan Rahasia");
    cl.append(5,  "Chapter 5: Suara di Balik Pintu");
    cl.append(6,  "Chapter 6: Pagi yang Berbeda");
    cl.append(7,  "Chapter 7: Persimpangan");
}

// ============================================================
// DISPLAY SCENE
// ============================================================

void displayScene(Scene* s, bool skipProlog, int resetCount) {
    printSeparator();
    cout << "  " << s->title << "\n";
    printSeparator();

    // Skip prolog narration (but not on first run, and not for chapter 0)
    if (skipProlog && s->id == 0 && resetCount == 0) {
        // first time — show full
    }

    // Show narration
    if (!s->narration.empty()) {
        cout << "\n" << s->narration << "\n";
    }

    // Show dialogs via Queue
    if (!s->dialogs.empty()) {
        printLine();
        displayDialogQueue(s->dialogs);
        printLine();
    }
}

// ============================================================
// DISPLAY CHOICES
// ============================================================

int displayChoices(Scene* s) {
    if (s->choiceCount == 0) return 0;

    cout << "\n--- Pilih Tindakanmu ---\n";
    for (int i = 0; i < s->choiceCount; i++) {
        cout << "  " << (i + 1) << ". " << s->choices[i].text << "\n";
    }
    return getChoice(s->choiceCount);
}

// ============================================================
// DISPLAY RESET
// ============================================================

void displayReset(int resetCount) {
    printSeparator();
    cout << "\n  ...waktu berhenti.\n\n";
    cout << "  Dunia di sekitarmu retak seperti kaca.\n";
    cout << "  Dan kemudian semuanya ditarik kembali —\n";
    cout << "  ke titik di mana semuanya dimulai.\n";
    printSeparator();

    pressEnter();

    printSeparator();
    cout << "\n  [ RESET #" << resetCount << " ]\n\n";

    // Get monolog based on reset count
    int idx = (resetCount - 1);
    if (idx >= RESET_MONOLOG_COUNT) idx = RESET_MONOLOG_COUNT - 1;
    cout << resetMonologs[idx] << "\n";
    printSeparator();

    pressEnter();
}

// ============================================================
// DISPLAY ENDING
// ============================================================

void displayEnding(Scene* s) {
    printSeparator();
    cout << "\n  " << s->title << "\n";
    printSeparator();
    cout << "\n" << s->narration << "\n";
    printSeparator();
    cout << "\n  Terima kasih sudah memainkan MANGA RESET: Dunia yang Kulupa.\n\n";
}

// ============================================================
// MAIN GAME LOOP
// ============================================================

int main() {
    buildSceneDatabase();

    ChapterList  chapterList;
    buildChapterList(chapterList);

    DialogHistory history;
    StateStack    stateStack;

    // Initial game state
    GameState initialState = {0, 0, false, false};
    GameState gs           = initialState;
    stateStack.save(gs);

    // ── Title Screen ─────────────────────────────────────────
    printSeparator();
    cout << "\n";
    cout << "        MANGA RESET: Dunia yang Kulupa\n";
    cout << "\n";
    cout << "  Sebuah cerita tentang pilihan, keluarga, dan impian.\n";
    cout << "  Temukan satu-satunya jalur yang benar.\n";
    cout << "  Salah pilih — kamu kembali ke awal.\n";
    cout << "\n";
    printSeparator();

    pressEnter();

    // ── Game Loop ─────────────────────────────────────────────
    while (!gs.gameOver) {
        Scene* current = findScene(gs.currentSceneId);
        if (!current) {
            cout << "Error: Scene tidak ditemukan.\n";
            break;
        }

        // Display scene
        displayScene(current, false, gs.resetCount);

        // Ending check
        if (current->isEnding) {
            displayEnding(current);
            gs.gameOver    = true;
            gs.reachedEnding = true;
            break;
        }

        // Prolog: no choice, just continue
        if (current->choiceCount == 0) {
            pressEnter();
            gs.currentSceneId = 1;
            stateStack.save(gs);
            continue;
        }

        // Show progress for non-prolog scenes
        if (current->id > 0 && current->id < 10) {
            chapterList.printProgress(current->id);
        }

        // Get player choice
        int pick = displayChoices(current);
        Choice& chosen = current->choices[pick - 1];

        // Record in history
        string histEntry = "[Scene " + to_string(current->id) + "] " + chosen.text;
        history.push(histEntry);

        if (chosen.nextSceneId == -1 || !chosen.isCorrect) {
            // Wrong choice — reset
            gs.resetCount++;
            displayReset(gs.resetCount);

            // Restore to initial state
            gs = initialState;
            gs.resetCount = stateStack.getInitial().resetCount; 
            // Actually keep current reset count
            int savedReset = gs.resetCount;
            gs = initialState;
            gs.resetCount = savedReset + 1;
            // Re-save
            stateStack.save(gs);

        } else {
            // Correct — advance
            gs.currentSceneId = chosen.nextSceneId;
            stateStack.save(gs);
        }
    }

    // ── Post-game ─────────────────────────────────────────────
    if (gs.reachedEnding) {
        cout << "\n  Total reset yang dialami: " << gs.resetCount << " kali.\n\n";
        if (gs.resetCount > 0) {
            cout << "  Pilihanmu selama perjalanan:\n";
            history.printAll();
        }
    }

    cout << "\n";
    printSeparator();
    cout << "  [ Program selesai. ]\n";
    printSeparator();
    cout << "\n";

    return 0;
}