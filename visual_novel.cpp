/*
=============================================================
  LAYAR LAIN: Kisah di Balik Panel
  Visual Novel Terminal - C++
  
  Struktur Data yang digunakan:
  - Array        : pilihan dialog, inventory log
  - Struct       : karakter, scene, dialog, diary entry
  - Pointer      : navigasi scene, linked list node
  - Single LL    : rantai chapter/scene
  - Double LL    : diary entry (bisa traversal dua arah)
  - Stack        : history pilihan per chapter (untuk reset)
  - Queue        : antrian dialog dalam scene
  - Tree         : decision tree pilihan cerita
=============================================================
*/

#include <iostream>
#include <string>
#include <stack>
#include <queue>
#include <vector>
#include <limits>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif

using namespace std;

// ============================================================
// STRUCT DEFINITIONS
// ============================================================

struct Karakter {
    string nama;
    string deskripsi;
    int relationship;  // 0-100
    bool terungkap;    // apakah identitas femboy sudah terungkap
};

struct DialogNode {
    string teks;
    string speaker;   // "" = narasi
    DialogNode* next;
};

struct PilihanNode {
    string teks;
    bool benar;       // apakah pilihan ini yang "canon"
    int childLeft;    // index scene jika salah
    int childRight;   // index scene jika benar
    PilihanNode* next;
};

// Tree node untuk decision branching
struct TreeNode {
    string pertanyaan;
    string pilihan[4];
    bool benar[4];
    int jumlahPilihan;
    int sceneBenar;   // index scene selanjutnya jika benar
    int sceneSalah;   // index scene jika salah (reset)
    TreeNode* children[4];
    
    TreeNode() {
        jumlahPilihan = 0;
        sceneBenar = -1;
        sceneSalah = -1;
        for(int i = 0; i < 4; i++) children[i] = nullptr;
        for(int i = 0; i < 4; i++) benar[i] = false;
    }
};

// Single Linked List node untuk chapter
struct ChapterNode {
    int id;
    string judul;
    string deskripsiSingkat;
    bool sudahClear;
    ChapterNode* next;
    
    ChapterNode(int _id, string _judul, string _desk) {
        id = _id;
        judul = _judul;
        deskripsiSingkat = _desk;
        sudahClear = false;
        next = nullptr;
    }
};

// Double Linked List node untuk diary
struct DiaryNode {
    int id;
    string tanggal;
    string judul;
    string isi;
    DiaryNode* prev;
    DiaryNode* next;
    
    DiaryNode(int _id, string _tgl, string _judul, string _isi) {
        id = _id;
        tanggal = _tgl;
        judul = _judul;
        isi = _isi;
        prev = nullptr;
        next = nullptr;
    }
};

// ============================================================
// GLOBAL STATE
// ============================================================

Karakter mc;
Karakter femboy;
int chapterSaatIni = 0;
int totalChapterClear = 0;
bool gameOver = false;
bool gameWin = false;

// Single Linked List - chain of chapters
ChapterNode* headChapter = nullptr;

// Double Linked List - diary
DiaryNode* headDiary = nullptr;
DiaryNode* tailDiary = nullptr;
int diaryCount = 0;

// Stack - history pilihan dalam chapter (untuk reset)
stack<string> historyPilihan;

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

void clearScreen() {
    system(CLEAR);
}

void pauseScreen() {
    cout << "\n\033[90m[ Tekan ENTER untuk melanjutkan... ]\033[0m";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void printGaris(char c = '=', int panjang = 60) {
    for(int i = 0; i < panjang; i++) cout << c;
    cout << "\n";
}

void printJudul(string teks) {
    clearScreen();
    printGaris('=');
    int spasi = (60 - teks.length()) / 2;
    for(int i = 0; i < spasi; i++) cout << " ";
    cout << "\033[1;93m" << teks << "\033[0m\n";
    printGaris('=');
    cout << "\n";
}

void printNarasi(string teks) {
    cout << "\033[3;37m  " << teks << "\033[0m\n";
}

void printDialog(string speaker, string teks) {
    if(speaker == "") {
        printNarasi(teks);
    } else {
        cout << "\033[1;96m" << speaker << "\033[0m";
        cout << ": \033[97m\"" << teks << "\"\033[0m\n";
    }
}

void printSistem(string teks) {
    cout << "\n\033[1;91m[ " << teks << " ]\033[0m\n";
}

void printInfo(string teks) {
    cout << "\033[1;92m>> " << teks << "\033[0m\n";
}

void animasiKetik(string teks, int delay = 30) {
    for(char c : teks) {
        cout << c;
        cout.flush();
#ifdef _WIN32
        Sleep(delay);
#else
        // simple delay loop
        for(volatile long i = 0; i < delay * 1000L; i++);
#endif
    }
    cout << "\n";
}

void printTruk() {
    cout << "\033[1;91m\n";
    cout << "        _____________\n";
    cout << "       |  TRUCK-KUN  |\n";
    cout << "       |_____________|\n";
    cout << "    __|_____________|__\n";
    cout << "   |  ___       ___   |\n";
    cout << "   | |   |     |   |  |\n";
    cout << "   |_|___|_____|___|__|\n";
    cout << "     (O)           (O)\n";
    cout << "\033[0m\n";
}

void printStar() {
    cout << "\033[1;93m";
    cout << "    ✦ ✧ ✦ ✧ ✦ ✧ ✦ ✧ ✦ ✧ ✦ ✧ ✦ ✧ ✦ \n";
    cout << "\033[0m";
}

// ============================================================
// QUEUE: Dialog System
// ============================================================

class DialogQueue {
private:
    queue<pair<string,string>> q; // <speaker, teks>
public:
    void tambah(string speaker, string teks) {
        q.push({speaker, teks});
    }
    
    void jalankan() {
        while(!q.empty()) {
            auto [speaker, teks] = q.front();
            q.pop();
            cout << "\n";
            printDialog(speaker, teks);
            if(!q.empty()) {
                cout << "\033[90m  ...\033[0m\n";
                pauseScreen();
            }
        }
    }
    
    bool kosong() { return q.empty(); }
};

// ============================================================
// SINGLE LINKED LIST: Chapter Management
// ============================================================

void tambahChapter(int id, string judul, string desk) {
    ChapterNode* baru = new ChapterNode(id, judul, desk);
    if(!headChapter) {
        headChapter = baru;
        return;
    }
    ChapterNode* cur = headChapter;
    while(cur->next) cur = cur->next;
    cur->next = baru;
}

ChapterNode* cariChapter(int id) {
    ChapterNode* cur = headChapter;
    while(cur) {
        if(cur->id == id) return cur;
        cur = cur->next;
    }
    return nullptr;
}

void tampilProgressChapter() {
    printJudul("PROGRESS CERITA");
    ChapterNode* cur = headChapter;
    while(cur) {
        if(cur->sudahClear) {
            cout << "  \033[1;92m[CLEAR]\033[0m ";
        } else if(cur->id == chapterSaatIni) {
            cout << "  \033[1;93m[AKTIF]\033[0m ";
        } else {
            cout << "  \033[90m[KUNCI]\033[0m ";
        }
        cout << "Chapter " << cur->id << ": " << cur->judul << "\n";
        if(cur->sudahClear || cur->id == chapterSaatIni) {
            cout << "           \033[90m" << cur->deskripsiSingkat << "\033[0m\n";
        }
        cur = cur->next;
    }
    cout << "\n";
    pauseScreen();
}

// ============================================================
// DOUBLE LINKED LIST: Diary System
// ============================================================

void tambahDiary(string tgl, string judul, string isi) {
    diaryCount++;
    DiaryNode* baru = new DiaryNode(diaryCount, tgl, judul, isi);
    if(!headDiary) {
        headDiary = tailDiary = baru;
        return;
    }
    tailDiary->next = baru;
    baru->prev = tailDiary;
    tailDiary = baru;
    printInfo("Entry diary berhasil ditambahkan!");
}

void tampilSemuaDiary() {
    printJudul("DIARY " + mc.nama);
    if(!headDiary) {
        cout << "  Belum ada entry diary.\n";
        pauseScreen();
        return;
    }
    DiaryNode* cur = headDiary;
    while(cur) {
        cout << "\033[1;93m[" << cur->id << "] " << cur->judul << "\033[0m";
        cout << " \033[90m(" << cur->tanggal << ")\033[0m\n";
        cout << "  \033[37m" << cur->isi << "\033[0m\n\n";
        cur = cur->next;
    }
    pauseScreen();
}

void tampilDiaryTerbalik() {
    printJudul("DIARY (Terbalik - Terbaru ke Lama)");
    if(!tailDiary) {
        cout << "  Belum ada entry diary.\n";
        pauseScreen();
        return;
    }
    DiaryNode* cur = tailDiary;
    while(cur) {
        cout << "\033[1;93m[" << cur->id << "] " << cur->judul << "\033[0m";
        cout << " \033[90m(" << cur->tanggal << ")\033[0m\n";
        cout << "  \033[37m" << cur->isi << "\033[0m\n\n";
        cur = cur->prev;
    }
    pauseScreen();
}

void ubahDiary(int id) {
    DiaryNode* cur = headDiary;
    while(cur) {
        if(cur->id == id) {
            cout << "  Judul baru (kosongkan untuk tidak diubah): ";
            string jBaru;
            cin.ignore();
            getline(cin, jBaru);
            if(jBaru != "") cur->judul = jBaru;
            
            cout << "  Isi baru (kosongkan untuk tidak diubah): ";
            string iBaru;
            getline(cin, iBaru);
            if(iBaru != "") cur->isi = iBaru;
            
            printInfo("Diary berhasil diubah!");
            return;
        }
        cur = cur->next;
    }
    printSistem("Entry tidak ditemukan!");
}

void hapusDiary(int id) {
    DiaryNode* cur = headDiary;
    while(cur) {
        if(cur->id == id) {
            if(cur->prev) cur->prev->next = cur->next;
            else headDiary = cur->next;
            
            if(cur->next) cur->next->prev = cur->prev;
            else tailDiary = cur->prev;
            
            delete cur;
            printInfo("Entry diary berhasil dihapus!");
            return;
        }
        cur = cur->next;
    }
    printSistem("Entry tidak ditemukan!");
}

void menuDiary() {
    while(true) {
        printJudul("MENU DIARY");
        cout << "  1. Lihat semua diary\n";
        cout << "  2. Lihat diary (terbaru ke lama)\n";
        cout << "  3. Tambah entry baru\n";
        cout << "  4. Ubah entry\n";
        cout << "  5. Hapus entry\n";
        cout << "  0. Kembali\n\n";
        cout << "  Pilih: ";
        
        int pilih;
        cin >> pilih;
        
        switch(pilih) {
            case 1: tampilSemuaDiary(); break;
            case 2: tampilDiaryTerbalik(); break;
            case 3: {
                clearScreen();
                printJudul("TULIS DIARY BARU");
                cin.ignore();
                cout << "  Tanggal: ";
                string tgl; getline(cin, tgl);
                cout << "  Judul: ";
                string judul; getline(cin, judul);
                cout << "  Isi: ";
                string isi; getline(cin, isi);
                tambahDiary(tgl, judul, isi);
                pauseScreen();
                break;
            }
            case 4: {
                tampilSemuaDiary();
                cout << "  ID entry yang ingin diubah: ";
                int id; cin >> id;
                ubahDiary(id);
                pauseScreen();
                break;
            }
            case 5: {
                tampilSemuaDiary();
                cout << "  ID entry yang ingin dihapus: ";
                int id; cin >> id;
                hapusDiary(id);
                pauseScreen();
                break;
            }
            case 0: return;
            default: printSistem("Pilihan tidak valid!"); pauseScreen();
        }
    }
}

// ============================================================
// STACK: History Pilihan
// ============================================================

void simpanHistory(string pilihan) {
    historyPilihan.push(pilihan);
}

void resetHistory() {
    while(!historyPilihan.empty()) historyPilihan.pop();
    printSistem("Waktu berbalik... kamu kembali ke awal chapter.");
}

void tampilHistory() {
    if(historyPilihan.empty()) {
        cout << "  Belum ada history pilihan.\n";
        return;
    }
    stack<string> temp = historyPilihan;
    stack<string> reversed;
    while(!temp.empty()) {
        reversed.push(temp.top());
        temp.pop();
    }
    cout << "  \033[90mHistory pilihan kamu:\033[0m\n";
    int i = 1;
    while(!reversed.empty()) {
        cout << "    " << i++ << ". " << reversed.top() << "\n";
        reversed.pop();
    }
}

// ============================================================
// TREE: Decision Tree untuk pilihan story
// ============================================================

TreeNode* buatTreeNode(string pertanyaan, int sceneBenar, int sceneSalah) {
    TreeNode* node = new TreeNode();
    node->pertanyaan = pertanyaan;
    node->sceneBenar = sceneBenar;
    node->sceneSalah = sceneSalah;
    return node;
}

void tambahPilihanTree(TreeNode* node, string pilihan, bool benar) {
    node->pilihan[node->jumlahPilihan] = pilihan;
    node->benar[node->jumlahPilihan] = benar;
    node->jumlahPilihan++;
}

// Returns true jika jawaban benar
bool prosesTree(TreeNode* node) {
    if(!node) return true;
    
    cout << "\n\033[1;95m? " << node->pertanyaan << "\033[0m\n\n";
    for(int i = 0; i < node->jumlahPilihan; i++) {
        cout << "  " << (i+1) << ". " << node->pilihan[i] << "\n";
    }
    cout << "\n  Pilih (1-" << node->jumlahPilihan << "): ";
    
    int pilih;
    cin >> pilih;
    
    if(pilih < 1 || pilih > node->jumlahPilihan) {
        printSistem("Pilihan tidak valid!");
        return prosesTree(node);
    }
    
    string pilihanTeks = node->pilihan[pilih-1];
    simpanHistory(pilihanTeks);
    
    if(node->benar[pilih-1]) {
        printInfo("Sesuatu dalam benakmu terasa... benar.");
        return true;
    } else {
        printSistem("Kamu merasa sesuatu yang salah... dunia mulai berputar.");
        return false;
    }
}

// ============================================================
// INPUT NAMA KARAKTER
// ============================================================

void inputNamaKarakter() {
    printJudul("INISIALISASI KARAKTER");
    
    cout << "  Sebelum cerita dimulai, siapakah namamu?\n\n";
    cout << "  Nama karakter utama (MC): ";
    cin.ignore();
    getline(cin, mc.nama);
    if(mc.nama.empty()) mc.nama = "Ryo";
    
    cout << "\n  Dan siapakah nama tetangga kamarmu di kost?\n";
    cout << "  (Karakter yang akan kamu temui di dunia lain)\n\n";
    cout << "  Nama karakter femboy: ";
    getline(cin, femboy.nama);
    if(femboy.nama.empty()) femboy.nama = "Haru";
    
    mc.deskripsi = "Seorang mahasiswa baru, otaku sejati pecinta manga.";
    mc.relationship = 0;
    
    femboy.deskripsi = "Berambut panjang, kulit mulus, style androgynous yang unik.";
    femboy.relationship = 0;
    femboy.terungkap = false;
    
    printInfo("Karakter berhasil dibuat!");
    cout << "\n  MC     : \033[1;96m" << mc.nama << "\033[0m\n";
    cout << "  Femboy : \033[1;95m" << femboy.nama << "\033[0m\n\n";
    pauseScreen();
}

// ============================================================
// PROLOG: DUNIA NYATA
// ============================================================

void prolog() {
    printJudul("PROLOG - Dunia yang Biasa-Biasa Saja");
    
    DialogQueue dq;
    dq.tambah("", "Hari Sabtu. Langit cerah, angin sepoi-sepoi.");
    dq.tambah("", "Toko manga langgananmu sudah buka. Kau sudah menunggu rilisan terbaru ini sejak tiga minggu lalu.");
    dq.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq2;
    dq2.tambah(mc.nama, "Akhirnya... edisi spesial! Langsung beli dua sekalian.");
    dq2.tambah("Penjaga Toko", "Hati-hati di jalan, dek. Lagi banyak motor ngebut.");
    dq2.tambah(mc.nama, "Iya, makasih!");
    dq2.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Kamu berjalan pulang sambil sesekali mengintip cover manga barumu.");
    printNarasi("Judulnya: 'Layar Lain' - kisah slice of life tentang seorang maba dan tetangga kostnya.");
    cout << "\n";
    
    DialogQueue dq3;
    dq3.tambah(mc.nama, "*bergumam* Wah, covernya bagus banget... karakternya juga...");
    dq3.tambah("", "Kau terlalu fokus pada manga di tanganmu.");
    dq3.tambah("", "Terlalu fokus, sampai tidak mendengar suara klakson dari tikungan.");
    dq3.jalankan();
    pauseScreen();
    
    clearScreen();
    printTruk();
    
    cout << "\033[1;91m";
    animasiKetik("                 BRAAAAKK!!!");
    cout << "\033[0m\n";
    
    DialogQueue dq4;
    dq4.tambah("", "Rasa sakit yang luar biasa. Manga-mu terlempar jauh.");
    dq4.tambah("", "Pandanganmu menggelap...");
    dq4.tambah("", "Dan yang terakhir kau ingat adalah wajah di cover manga itu.");
    dq4.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris('*');
    printStar();
    cout << "\n";
    printNarasi("                    G E L A P");
    cout << "\n";
    printNarasi("        kemudian... sebuah suara...");
    cout << "\n";
    printStar();
    printGaris('*');
    pauseScreen();
    
    clearScreen();
    printJudul("SELAMAT DATANG DI DUNIA LAIN");
    
    DialogQueue dq5;
    dq5.tambah("???", "...Hei. Hei! Kamu baik-baik aja?");
    dq5.tambah("", "Kamu membuka mata perlahan.");
    dq5.tambah("", "Langit-langit yang asing. Bau kayu dan kain yang tidak familiar.");
    dq5.tambah("", "Dan di atasmu... wajah seseorang.");
    dq5.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Rambut panjang tergerai. Mata bening menatapmu khawatir.");
    printNarasi("Wajah itu... familiar. Sangat familiar.");
    printNarasi("Seperti yang pernah kau lihat di suatu tempat...");
    cout << "\n";
    
    DialogQueue dq6;
    dq6.tambah(femboy.nama, "Eh, syukurlah. Kamu pingsan di depan kamarku tadi.");
    dq6.tambah(mc.nama, "*dalam hati* Ini... ini 'kan " + femboy.nama + "! Karakter di manga!");
    dq6.tambah(mc.nama, "*dalam hati* Berarti aku... masuk ke dalam cerita itu?!");
    dq6.tambah(femboy.nama, "Hei, kamu kenapa malah diem? Kepalamu kebentur ya?");
    dq6.tambah(mc.nama, "A-ah, nggak apa-apa! Aku... aku cuma sedikit pusing.");
    dq6.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Kamu mencoba mengingat-ingat isi manga 'Layar Lain' yang baru saja kamu beli.");
    printNarasi("Tapi kamu belum sempat membacanya sampai habis...");
    printNarasi("Kamu hanya ingat garis besar ceritanya.");
    cout << "\n";
    
    printSistem("PERHATIAN DARI KESADARAN DALAM DIRIMU");
    cout << "\n";
    cout << "  \033[93mKamu berada di dalam manga 'Layar Lain'.\033[0m\n";
    cout << "  \033[93mKamu harus menjalankan cerita sesuai alur yang benar.\033[0m\n";
    cout << "  \033[93mJika kamu salah memilih, waktu akan berbalik ke awal chapter.\033[0m\n";
    cout << "  \033[93mIngatlah setiap pilihanmu. Kamu pasti bisa.\033[0m\n\n";
    
    pauseScreen();
    
    // Tambah diary otomatis pertama
    tambahDiary("Hari 1", "Apa yang terjadi padaku?", 
        "Aku tertabrak truk dan entah bagaimana aku masuk ke dalam manga. Aku harus menjalankan cerita ini dengan benar. " + femboy.nama + " ada di sini. Dia... persis seperti di cover.");
}

// ============================================================
// CHAPTER 1
// ============================================================

bool chapter1() {
    resetHistory();
    chapterSaatIni = 1;
    
    printJudul("CHAPTER 1 - Tetangga Kamar");
    
    DialogQueue dq;
    dq.tambah("", "Kamu 'resmi' menjadi penghuni kamar 7, tepat di sebelah kamar " + femboy.nama + ".");
    dq.tambah("", "Hari pertama kuliah. Kamu berjalan menuju kampus bersamanya karena kalian satu jurusan.");
    dq.tambah(femboy.nama, "Eh, kamu jurusan apa?");
    dq.tambah(mc.nama, "Informatika. Kamu?");
    dq.tambah(femboy.nama, "Sama dong! Wah, lumayan ada yang bisa diajak nebeng ngerjain tugas.");
    dq.tambah(mc.nama, "*dalam hati* Dia... ramah banget. Nggak nyangka.");
    dq.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Di tengah perjalanan, " + femboy.nama + " tersandung.");
    cout << "\n";
    
    // DECISION TREE - Chapter 1
    TreeNode* root = buatTreeNode(
        "Apa yang kamu lakukan saat " + femboy.nama + " tersandung?",
        2, 1
    );
    tambahPilihanTree(root, "Langsung membantu dan menanyakan apakah ia baik-baik saja", true);
    tambahPilihanTree(root, "Berpura-pura tidak melihat dan terus berjalan", false);
    tambahPilihanTree(root, "Tertawa dan mengatakan ia ceroboh", false);
    
    bool benar = prosesTree(root);
    delete root;
    
    if(!benar) {
        cout << "\n";
        printNarasi(femboy.nama + " menatapmu dengan ekspresi terluka.");
        printNarasi("Kamu merasa sesuatu yang salah. Ini bukan bagaimana ceritanya seharusnya berjalan...");
        cout << "\n";
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq2;
    dq2.tambah(mc.nama, "Eh, kamu baik-baik aja? Hati-hati.");
    dq2.tambah(femboy.nama, "Aduh... iya, makasih. Kamu baik ya.");
    dq2.tambah("", femboy.nama + " tersenyum tipis. Ada sesuatu yang hangat dari senyum itu.");
    dq2.tambah(mc.nama, "*dalam hati* Tunggu... manga ini alurnya benar. Aku ingat scene ini!");
    dq2.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Sesampainya di kampus, kalian berkenalan lebih jauh.");
    cout << "\n";
    
    TreeNode* root2 = buatTreeNode(
        "Teman sekelas bertanya: 'Siapa temenmu yang cantik itu?'. Kamu menjawab...",
        2, 1
    );
    tambahPilihanTree(root2, "Memperkenalkan " + femboy.nama + " dengan sopan", true);
    tambahPilihanTree(root2, "Bilang 'Nggak kenal' lalu pergi", false);
    tambahPilihanTree(root2, "Jawab dengan malu-malu tanpa memperkenalkan", false);
    
    bool benar2 = prosesTree(root2);
    delete root2;
    
    if(!benar2) {
        clearScreen();
        printNarasi(femboy.nama + " tampak sedikit kecewa mendengar responmu.");
        printNarasi("Sesuatu terasa tidak pas. Waktu berputar kembali...");
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq3;
    dq3.tambah(mc.nama, "Ini " + femboy.nama + ", teman sekost sekaligus teman sekelas.");
    dq3.tambah(femboy.nama, "Hai~ Makasih ya udah diperkenalkan.");
    dq3.tambah("", "Hari pertama berjalan lancar. Kamu dan " + femboy.nama + " pulang bersama.");
    dq3.tambah(femboy.nama, "Eh, tadi kenapa kamu mau bantuin aku pas jatuh? Biasanya orang lewat aja.");
    dq3.tambah(mc.nama, "Ya... wajar aja kan bantu orang yang kesusahan.");
    dq3.tambah(femboy.nama, "Hehe... kamu aneh. Tapi anehnya yang menyenangkan.");
    dq3.jalankan();
    
    femboy.relationship += 20;
    mc.relationship += 10;
    
    cout << "\n";
    printInfo("Hubunganmu dengan " + femboy.nama + " semakin dekat! [Relationship: " + to_string(femboy.relationship) + "/100]");
    cout << "\n";
    
    tambahDiary("Hari 3", "Dia lebih menarik dari yang kukira",
        femboy.nama + " orangnya ternyata ramah. Cara dia senyum itu... aneh, tapi aku suka. Bukan dalam artian apa-apa. Cuma... menyenangkan aja.");
    
    pauseScreen();
    
    ChapterNode* ch = cariChapter(1);
    if(ch) ch->sudahClear = true;
    totalChapterClear++;
    
    printInfo("CHAPTER 1 CLEAR!");
    pauseScreen();
    return true;
}

// ============================================================
// CHAPTER 2
// ============================================================

bool chapter2() {
    resetHistory();
    chapterSaatIni = 2;
    
    printJudul("CHAPTER 2 - Hujan dan Rahasia Kecil");
    
    DialogQueue dq;
    dq.tambah("", "Dua minggu berlalu. Kamu dan " + femboy.nama + " sudah cukup sering menghabiskan waktu bersama.");
    dq.tambah("", "Hari ini hujan deras. Kamu lupa bawa payung.");
    dq.tambah(femboy.nama, "Eh, kamu nggak bawa payung?");
    dq.tambah(mc.nama, "Lupa...");
    dq.tambah(femboy.nama, "Sini, bareng aku aja. Payungku cukup buat berdua.");
    dq.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Di bawah satu payung, kalian berjalan berdampingan.");
    printNarasi("Jarak yang sangat dekat. Kamu bisa mencium aroma shampoo-nya.");
    cout << "\n";
    
    TreeNode* root = buatTreeNode(
        "Di bawah payung yang sempit, kamu merasa canggung. Kamu...",
        3, 2
    );
    tambahPilihanTree(root, "Berjalan natural dan mengajak mengobrol supaya tidak canggung", true);
    tambahPilihanTree(root, "Menjaga jarak ekstra sampai kamu malah hampir basah", false);
    tambahPilihanTree(root, "Tiba-tiba lari meninggalkan " + femboy.nama + " sendirian", false);
    
    bool benar = prosesTree(root);
    delete root;
    
    if(!benar) {
        clearScreen();
        printNarasi("Situasi menjadi canggung. " + femboy.nama + " terlihat sedikit kecewa.");
        printNarasi("Ini bukan alur yang benar. Waktu berbalik...");
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq2;
    dq2.tambah(mc.nama, "Eh, ngomong-ngomong, kamu suka musik apa?");
    dq2.tambah(femboy.nama, "Hmm... banyak sih. Tapi aku paling suka lo-fi sama jazz.");
    dq2.tambah(mc.nama, "Oh serius? Aku juga!");
    dq2.tambah(femboy.nama, "Haha, cocok dong kita.");
    dq2.tambah("", "Kalian mengobrol ringan sepanjang jalan. Tanpa terasa, sudah sampai di kost.");
    dq2.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    printNarasi("Di depan kamar masing-masing, " + femboy.nama + " berhenti.");
    cout << "\n";
    
    DialogQueue dq3;
    dq3.tambah(femboy.nama, "Eh... boleh aku tanya sesuatu?");
    dq3.tambah(mc.nama, "Boleh, apa?");
    dq3.tambah(femboy.nama, "Kamu... nggak aneh ya sama aku? Maksudnya, cara aku berpakaian, cara aku ngomong...");
    dq3.tambah("", "Ada nada ragu dalam suaranya. Pertanyaan yang serius.");
    dq3.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    TreeNode* root2 = buatTreeNode(
        femboy.nama + " bertanya apakah kamu merasa aneh dengannya. Jawabanmu...",
        3, 2
    );
    tambahPilihanTree(root2, "Bilang jujur bahwa kamu tidak merasa aneh, dia menyenangkan", true);
    tambahPilihanTree(root2, "Bilang 'Sedikit sih...' dengan nada ragu", false);
    tambahPilihanTree(root2, "Ganti topik pembicaraan tanpa menjawab", false);
    
    bool benar2 = prosesTree(root2);
    delete root2;
    
    if(!benar2) {
        clearScreen();
        printNarasi(femboy.nama + " mengangguk pelan. Tapi matanya tampak sedikit redup.");
        printNarasi("Kamu merasa sesuatu yang salah... waktu berputar kembali.");
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq4;
    dq4.tambah(mc.nama, "Aneh gimana? Kamu biasa aja. Malah menyenangkan.");
    dq4.tambah(femboy.nama, "...");
    dq4.tambah(femboy.nama, "Makasih. Serius.");
    dq4.tambah("", "Dia tersenyum. Kali ini lebih lebar dari biasanya. Dan ada sesuatu di matanya.");
    dq4.tambah("", "Kelegaan? Atau mungkin... sesuatu yang lain.");
    dq4.jalankan();
    
    femboy.relationship += 25;
    
    cout << "\n";
    printInfo("Hubunganmu dengan " + femboy.nama + " semakin dalam! [Relationship: " + to_string(femboy.relationship) + "/100]");
    cout << "\n";
    
    tambahDiary("Hari 17", "Pertanyaan yang tidak kuduga",
        femboy.nama + " tanya apakah aku merasa aneh dengannya. Kenapa dia tanya itu? Ada sesuatu yang dia sembunyikan? Tapi... aku jawab jujur. Karena memang itu kebenarannya.");
    
    pauseScreen();
    
    ChapterNode* ch = cariChapter(2);
    if(ch) ch->sudahClear = true;
    totalChapterClear++;
    
    printInfo("CHAPTER 2 CLEAR!");
    pauseScreen();
    return true;
}

// ============================================================
// CHAPTER 3
// ============================================================

bool chapter3() {
    resetHistory();
    chapterSaatIni = 3;
    
    printJudul("CHAPTER 3 - Yang Tersembunyi");
    
    DialogQueue dq;
    dq.tambah("", "Satu bulan di kost. Kamu dan " + femboy.nama + " sudah seperti sahabat lama.");
    dq.tambah("", "Suatu sore, kamu mendengar suara tangis pelan dari kamar sebelah.");
    dq.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    TreeNode* root = buatTreeNode(
        "Kamu mendengar suara tangis dari kamar " + femboy.nama + ". Kamu...",
        4, 3
    );
    tambahPilihanTree(root, "Mengetuk pintunya dengan lembut dan bertanya apakah dia baik-baik saja", true);
    tambahPilihanTree(root, "Berpura-pura tidak mendengar dan masuk ke kamarmu", false);
    tambahPilihanTree(root, "Langsung membuka pintu tanpa mengetuk", false);
    
    bool benar = prosesTree(root);
    delete root;
    
    if(!benar) {
        clearScreen();
        printNarasi("Malam itu berlalu tanpa kamu berbuat apa-apa.");
        printNarasi("Keesokan harinya, " + femboy.nama + " tampak menjaga jarak.");
        printNarasi("Ini bukan alur yang benar... waktu berputar.");
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq2;
    dq2.tambah("", "Hening sejenak. Lalu suara tangis berhenti.");
    dq2.tambah(femboy.nama, "*suara serak* ...Masuk aja.");
    dq2.tambah("", "Kamu membuka pintu perlahan.");
    dq2.tambah("", femboy.nama + " duduk di sudut kasur, lutut dipeluk, mata sedikit merah.");
    dq2.tambah(mc.nama, "Eh, kenapa? Ada yang terjadi?");
    dq2.tambah(femboy.nama, "...Keluarga. Mereka nelpon. Seperti biasa.");
    dq2.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq3;
    dq3.tambah(femboy.nama, "Mereka... nggak terlalu suka cara aku jadi diri sendiri.");
    dq3.tambah("", "Ada jeda panjang.");
    dq3.tambah(femboy.nama, "Kamu pasti tau kan, aku ini... bukan perempuan.");
    dq3.tambah("", "Kamu membeku.");
    dq3.tambah("", "*dalam hati* Ini... moment pengungkapan. Aku ingat scene ini di manga.");
    dq3.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    femboy.terungkap = true;
    
    TreeNode* root2 = buatTreeNode(
        femboy.nama + " mengungkapkan identitasnya padamu. Reaksimu...",
        4, 3
    );
    tambahPilihanTree(root2, "Mengangguk pelan dan bilang 'Aku tau. Dan itu nggak mengubah apapun.'", true);
    tambahPilihanTree(root2, "Terkejut berlebihan dan langsung berdiri", false);
    tambahPilihanTree(root2, "Terdiam lama tanpa berkata apa-apa sampai dia salah paham", false);
    
    bool benar2 = prosesTree(root2);
    delete root2;
    
    if(!benar2) {
        clearScreen();
        printNarasi(femboy.nama + " menunduk. Ekspresinya... hancur.");
        printNarasi("Seharusnya bukan ini reaksimu. Waktu berputar kembali...");
        pauseScreen();
        return false;
    }
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq4;
    dq4.tambah(mc.nama, "Aku tau. Dan itu nggak mengubah apapun.");
    dq4.tambah(femboy.nama, "...Kamu nggak kaget?");
    dq4.tambah(mc.nama, "Kaget sih. Tapi... kamu tetap " + femboy.nama + " yang sama. Yang nemenin aku nebeng payung. Yang suka lo-fi.");
    dq4.tambah(femboy.nama, "...");
    dq4.tambah(femboy.nama, "*berbisik* Bego banget sih kamu. Kok bisa segampang itu nerima.");
    dq4.tambah(mc.nama, "Emang kenapa? Susah ya?");
    dq4.tambah(femboy.nama, "*ketawa kecil di tengah tangis* Idiot.");
    dq4.jalankan();
    
    femboy.relationship += 30;
    
    cout << "\n";
    printInfo("Kepercayaan " + femboy.nama + " padamu mencapai puncaknya! [Relationship: " + to_string(femboy.relationship) + "/100]");
    cout << "\n";
    
    tambahDiary("Hari 32", "Pengakuan",
        femboy.nama + " akhirnya bilang. Dia bukan perempuan. Aku... aku nggak tau harus merasa apa. Tapi yang aku tau, aku nggak mau kehilangan dia sebagai teman. Atau lebih dari itu?");
    
    pauseScreen();
    
    ChapterNode* ch = cariChapter(3);
    if(ch) ch->sudahClear = true;
    totalChapterClear++;
    
    printInfo("CHAPTER 3 CLEAR!");
    pauseScreen();
    return true;
}

// ============================================================
// CHAPTER 4 - FINAL
// ============================================================

bool chapter4() {
    resetHistory();
    chapterSaatIni = 4;
    
    printJudul("CHAPTER 4 - Pilihan");
    
    DialogQueue dq;
    dq.tambah("", "Seminggu setelah malam pengakuan itu.");
    dq.tambah("", "Kamu sudah menyelesaikan cerita 'Layar Lain' sampai di sini.");
    dq.tambah("", "Tapi ada satu hal yang tersisa. Satu pilihan terakhir.");
    dq.tambah(femboy.nama, "Eh... boleh aku tanya sesuatu lagi?");
    dq.tambah(mc.nama, "Boleh, apa?");
    dq.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    DialogQueue dq2;
    dq2.tambah(femboy.nama, "Kamu... nggak nyesel kan, udah kenal aku?");
    dq2.tambah("", "Ada keraguan di matanya. Tapi juga harapan.");
    dq2.tambah("", "Kamu tahu ini adalah pertanyaan terakhir dalam manga.");
    dq2.tambah("", "Dan jawabanmu akan menentukan ending cerita ini.");
    dq2.jalankan();
    pauseScreen();
    
    clearScreen();
    printGaris();
    cout << "\n";
    
    cout << "\033[1;95mINI ADALAH PILIHAN TERAKHIR.\033[0m\n\n";
    cout << "  " << femboy.nama << " menunggumu menjawab.\n\n";
    cout << "  1. \033[92m\"Nggak nyesel. Sama sekali nggak.\"\033[0m\n";
    cout << "     \033[90m(True Ending - Tetap bersama)\033[0m\n\n";
    cout << "  2. \033[93m\"Aku... butuh waktu untuk mikir.\"\033[0m\n";
    cout << "     \033[90m(Normal Ending - Perlu jarak)\033[0m\n\n";
    cout << "  3. \033[91m\"Maaf, aku rasa aku harus pindah kost.\"\033[0m\n";
    cout << "     \033[90m(Sad Ending - Pergi)\033[0m\n\n";
    cout << "  Pilihanmu (1-3): ";
    
    int pilih;
    cin >> pilih;
    
    simpanHistory("Pilihan akhir: " + to_string(pilih));
    
    clearScreen();
    
    if(pilih == 1) {
        // TRUE ENDING
        printJudul("TRUE ENDING - Tetap di Sini");
        printStar();
        
        DialogQueue de;
        de.tambah(mc.nama, "Nggak nyesel. Sama sekali nggak.");
        de.tambah(femboy.nama, "...");
        de.tambah(mc.nama, "Justru aku seneng bisa kenal kamu. Dan aku harap kita terus kayak gini.");
        de.tambah(femboy.nama, "Kayak gini gimana?");
        de.tambah(mc.nama, "Ya... temen. Temen yang baik. Yang bisa diandalkan. Yang nemenin jalan di bawah hujan.");
        de.tambah(femboy.nama, "*tertawa* Kamu romantis banget deh ngomonginnya.");
        de.tambah(mc.nama, "Apanya yang romantis, itu fakta.");
        de.tambah(femboy.nama, "Idiot.");
        de.tambah("", "Tapi kali ini, 'idiot' itu terdengar seperti hal yang paling hangat di dunia.");
        de.jalankan();
        pauseScreen();
        
        clearScreen();
        printGaris('*');
        printStar();
        cout << "\n";
        printNarasi("  Dan kamu merasa sesuatu menarikmu pulang.");
        printNarasi("  Seperti benang yang menghubungkan dua dunia.");
        printNarasi("  Kamu menutup matamu...");
        cout << "\n";
        printStar();
        printGaris('*');
        pauseScreen();
        
        clearScreen();
        printJudul("EPILOG - DUNIA NYATA");
        
        DialogQueue ep;
        ep.tambah("", "Rumah sakit. Bau antiseptik. Langit-langit putih.");
        ep.tambah("", "Kamu membuka mata.");
        ep.tambah("Perawat", "Oh! Dia sadar! Dokter, cepat!");
        ep.tambah("", "Kamu selamat dari kecelakaan itu.");
        ep.tambah("", "Di meja samping ranjangmu... manga 'Layar Lain' tergeletak.");
        ep.tambah("", "Kamu meraihnya dengan tangan yang masih lemah.");
        ep.tambah("", "Dan di halaman terakhir, ada panel yang belum pernah kamu baca.");
        ep.tambah("", "Dua karakter. Berdampingan. Dengan kalimat sederhana:");
        ep.jalankan();
        pauseScreen();
        
        clearScreen();
        cout << "\n\n";
        cout << "\033[1;93m";
        cout << "  ╔══════════════════════════════════════════╗\n";
        cout << "  ║                                          ║\n";
        cout << "  ║   'Yang penting bukan kamu siapa.        ║\n";
        cout << "  ║    Yang penting adalah kamu ada.'        ║\n";
        cout << "  ║                                          ║\n";
        cout << "  ╚══════════════════════════════════════════╝\n";
        cout << "\033[0m\n\n";
        pauseScreen();
        
        gameWin = true;
        
    } else if(pilih == 2) {
        // NORMAL ENDING
        printJudul("NORMAL ENDING - Jarak yang Jujur");
        
        DialogQueue de;
        de.tambah(mc.nama, "Aku... butuh waktu untuk mikir.");
        de.tambah(femboy.nama, "..Oh.");
        de.tambah(mc.nama, "Bukan berarti aku nyesel kenal kamu. Tapi ada banyak hal yang aku perlu proses.");
        de.tambah(femboy.nama, "Aku ngerti. Wajar.");
        de.tambah("", femboy.nama + " tersenyum. Tapi lebih kecil dari biasanya.");
        de.tambah(femboy.nama, "Aku tunggu ya, kalau kamu sudah siap ngobrol lagi.");
        de.jalankan();
        pauseScreen();
        
        clearScreen();
        printNarasi("Waktu berlalu. Kalian masih berteman.");
        printNarasi("Tidak sedekat dulu, tapi tidak juga menjauh.");
        printNarasi("Kadang jarak itu bukan penghalang. Tapi ruang untuk tumbuh.");
        cout << "\n";
        
        tambahDiary("Hari 40", "Jarak",
            "Aku bilang butuh waktu. Dan " + femboy.nama + " mengerti. Mungkin itu yang aku suka darinya. Dia tidak memaksakan apapun.");
        
        pauseScreen();
        gameWin = true;
        
    } else if(pilih == 3) {
        // SAD ENDING
        printJudul("SAD ENDING - Selamat Tinggal");
        
        DialogQueue de;
        de.tambah(mc.nama, "Maaf... aku rasa aku harus pindah kost.");
        de.tambah(femboy.nama, "...");
        de.tambah(femboy.nama, "Oh. Oke.");
        de.tambah("", "Hanya dua kata. Tapi matanya berkata ribuan hal.");
        de.tambah(mc.nama, "Bukan karena kamu—");
        de.tambah(femboy.nama, "Nggak usah dijelasin. Aku ngerti.");
        de.tambah("", "Dan senyum itu. Senyum yang terbiasa menyembunyikan rasa sakit.");
        de.jalankan();
        pauseScreen();
        
        clearScreen();
        printNarasi("Seminggu kemudian, kamu pindah.");
        printNarasi("Kadang kamu masih memikirkan suara tawa di balik dinding tipis itu.");
        printNarasi("Kadang orang memilih jarak bukan karena benci.");
        printNarasi("Tapi karena belum siap.");
        cout << "\n";
        
        pauseScreen();
        gameWin = true;
        
    } else {
        cout << "Pilihan tidak valid. Coba lagi.\n";
        pauseScreen();
        return chapter4();
    }
    
    ChapterNode* ch = cariChapter(4);
    if(ch) ch->sudahClear = true;
    totalChapterClear++;
    
    return true;
}

// ============================================================
// INISIALISASI DATA
// ============================================================

void inisialisasiChapter() {
    tambahChapter(1, "Tetangga Kamar", "Hari pertama kuliah dan pertemuan awal.");
    tambahChapter(2, "Hujan dan Rahasia Kecil", "Satu payung, dua orang, satu pertanyaan.");
    tambahChapter(3, "Yang Tersembunyi", "Malam pengakuan yang mengubah segalanya.");
    tambahChapter(4, "Pilihan", "Satu pertanyaan terakhir. Satu jawaban yang menentukan.");
}

// ============================================================
// MAIN MENU
// ============================================================

void tampilCredits() {
    printJudul("CREDITS");
    cout << "  \033[1;93mLAYAR LAIN: Kisah di Balik Panel\033[0m\n\n";
    cout << "  Sebuah visual novel terminal berbasis C++\n\n";
    cout << "  \033[90mStruktur Data yang diimplementasikan:\033[0m\n";
    cout << "  • Array         - Pilihan dialog & opsi jawaban\n";
    cout << "  • Struct        - Karakter, Scene, Dialog, Diary\n";
    cout << "  • Pointer       - Navigasi node & linked list\n";
    cout << "  • Single LL     - Rantai chapter cerita\n";
    cout << "  • Double LL     - Sistem diary (traversal dua arah)\n";
    cout << "  • Stack         - History pilihan per chapter\n";
    cout << "  • Queue         - Antrian dialog dalam scene\n";
    cout << "  • Tree          - Decision tree pilihan cerita\n\n";
    pauseScreen();
}

void mainMenu() {
    printJudul("LAYAR LAIN: Kisah di Balik Panel");
    printStar();
    cout << "\n";
    cout << "  \033[93mSeorang otaku. Sebuah manga. Sebuah kecelakaan.\033[0m\n";
    cout << "  \033[93mDan sebuah dunia yang harus ia selesaikan.\033[0m\n\n";
    printStar();
    cout << "\n";
    cout << "  1. Mulai Perjalanan Baru\n";
    cout << "  2. Lihat Progress Chapter\n";
    cout << "  3. Buka Diary\n";
    cout << "  4. Lihat History Pilihan\n";
    cout << "  5. Credits\n";
    cout << "  0. Keluar\n\n";
    cout << "  Pilih: ";
}

// ============================================================
// MAIN
// ============================================================

int main() {
    clearScreen();
    
    // Opening screen
    printGaris('*');
    printStar();
    cout << "\n";
    cout << "\033[1;93m";
    cout << "     ██╗      █████╗ ██╗   ██╗ █████╗ ██████╗ \n";
    cout << "     ██║     ██╔══██╗╚██╗ ██╔╝██╔══██╗██╔══██╗\n";
    cout << "     ██║     ███████║ ╚████╔╝ ███████║██████╔╝\n";
    cout << "     ██║     ██╔══██║  ╚██╔╝  ██╔══██║██╔══██╗\n";
    cout << "     ███████╗██║  ██║   ██║   ██║  ██║██║  ██║\n";
    cout << "     ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝\n";
    cout << "\033[0m\n";
    cout << "          \033[1;95mL A I N\033[0m\n\n";
    cout << "     \033[90mKisah di Balik Panel\033[0m\n\n";
    printStar();
    printGaris('*');
    
    pauseScreen();
    
    // Input nama karakter
    inputNamaKarakter();
    
    // Inisialisasi chapter list
    inisialisasiChapter();
    
    // Prolog
    prolog();
    
    // Main game loop
    while(!gameOver && !gameWin) {
        clearScreen();
        mainMenu();
        
        int pilih;
        cin >> pilih;
        
        switch(pilih) {
            case 1: {
                // Lanjutkan cerita
                clearScreen();
                bool berhasil = true;
                
                if(chapterSaatIni == 0 || (cariChapter(1) && !cariChapter(1)->sudahClear)) {
                    do {
                        berhasil = chapter1();
                        if(!berhasil) {
                            clearScreen();
                            printSistem("RESET - Kamu kembali ke awal Chapter 1");
                            pauseScreen();
                        }
                    } while(!berhasil);
                }
                
                if(!gameWin && cariChapter(1)->sudahClear && 
                   cariChapter(2) && !cariChapter(2)->sudahClear) {
                    do {
                        berhasil = chapter2();
                        if(!berhasil) {
                            clearScreen();
                            printSistem("RESET - Kamu kembali ke awal Chapter 2");
                            pauseScreen();
                        }
                    } while(!berhasil);
                }
                
                if(!gameWin && cariChapter(2)->sudahClear && 
                   cariChapter(3) && !cariChapter(3)->sudahClear) {
                    do {
                        berhasil = chapter3();
                        if(!berhasil) {
                            clearScreen();
                            printSistem("RESET - Kamu kembali ke awal Chapter 3");
                            pauseScreen();
                        }
                    } while(!berhasil);
                }
                
                if(!gameWin && cariChapter(3)->sudahClear && 
                   cariChapter(4) && !cariChapter(4)->sudahClear) {
                    chapter4();
                }
                
                if(gameWin) {
                    clearScreen();
                    printJudul("TAMAT");
                    printStar();
                    cout << "\n";
                    cout << "  Terima kasih sudah memainkan \033[1;93mLayar Lain\033[0m.\n\n";
                    cout << "  \033[90mCerita sudah selesai. Tapi diary-mu masih terbuka.\033[0m\n\n";
                    printStar();
                    cout << "\n";
                    pauseScreen();
                    gameOver = true;
                }
                break;
            }
            case 2: tampilProgressChapter(); break;
            case 3: menuDiary(); break;
            case 4: {
                clearScreen();
                printJudul("HISTORY PILIHAN");
                tampilHistory();
                cout << "\n";
                pauseScreen();
                break;
            }
            case 5: tampilCredits(); break;
            case 0: 
                clearScreen();
                cout << "\n  Sampai jumpa...\n\n";
                gameOver = true;
                break;
            default:
                printSistem("Pilihan tidak valid!");
                pauseScreen();
        }
    }
    
    // Cleanup
    ChapterNode* cur = headChapter;
    while(cur) {
        ChapterNode* next = cur->next;
        delete cur;
        cur = next;
    }
    
    DiaryNode* dcur = headDiary;
    while(dcur) {
        DiaryNode* next = dcur->next;
        delete dcur;
        dcur = next;
    }
    
    return 0;
}