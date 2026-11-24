// medicion.cpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// Reemplaza globalmente operator new/delete para contabilizar la memoria
// auxiliar de cada algoritmo. Cada bloque lleva una cabecera de 16 bytes con
// su tamano y un numero magico, de modo que delete sepa cuanto descontar y
// pueda ignorar los punteros que no vengan de este new. Requiere enlace
// estatico: el reemplazo no cruza los limites de una DLL.
//
// Fuentes: https://en.cppreference.com/w/cpp/memory/new/operator_new ;
// https://en.cppreference.com/w/cpp/chrono/steady_clock

#include "medicion.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <new>

// Variables se inicializan en tiempo de carga, antes de que corra
// cualquier constructor, para que ninguna asignacion temprana las encuentre
// sin inicializar.
namespace {
    size_t g_live    = 0;
    size_t g_peak    = 0;
    size_t g_base    = 0;
    bool   g_enabled = true;

    constexpr size_t kHeader = 16;

    // Numero magico de validacion. Ver la explicacion en operator delete.
    constexpr size_t kMagic = 0x494E463232314131ULL;

    size_t g_ajenos = 0;
}

// Cuantos bloques ajenos se detectaron (deberia ser 0 con enlace estatico).
size_t medicion_bloques_ajenos() { return g_ajenos; }

// noinline a proposito: si GCC inlinea esto reporta un falso -Warray-bounds y
__attribute__((noinline))
void* operator new(size_t sz) {
    void* raw = std::malloc(sz + kHeader);
    if (!raw) throw std::bad_alloc();

    // Cabecera de 16 bytes: [ tamano | magico ]
    static_cast<size_t*>(raw)[0] = sz;
    static_cast<size_t*>(raw)[1] = kMagic;

    if (g_enabled) {
        g_live += sz;
        if (g_live > g_peak) g_peak = g_live;
    }
    return static_cast<char*>(raw) + kHeader;
}

__attribute__((noinline))
void operator delete(void* p) noexcept {
    if (!p) return;

    char* raw = static_cast<char*>(p) - kHeader;
    const size_t sz    = reinterpret_cast<size_t*>(raw)[0];
    const size_t magia = reinterpret_cast<size_t*>(raw)[1];

    // Red de seguridad por si llega un bloque asignado por otro operator new
    // (ocurre con enlace dinamico; por eso se compila con -static).
    if (magia != kMagic) {
        ++g_ajenos;
        std::free(p);
        return;
    }

    if (g_enabled) {
        // Si el contador se apago entre el new y el delete, saturar en 0.
        g_live = (g_live >= sz) ? g_live - sz : 0;
    }
    std::free(raw);
}


void operator delete(void* p, size_t) noexcept { operator delete(p); }

namespace medicion {

void   mem_reiniciar()   { g_base = g_live; g_peak = g_live; }
size_t mem_pico()    { return g_peak > g_base ? g_peak - g_base : 0; }
size_t mem_actual() { return g_live; }
void mem_habilitar(bool on) { g_enabled = on; }

// Plazo de ejecucion. Con el plazo apagado la comprobacion es leer un bool.
namespace {
    bool g_plazo_activo = false;
    std::chrono::steady_clock::time_point g_plazo_fin;
}

void plazo_iniciar(double ms) {
    if (ms <= 0) { g_plazo_activo = false; return; }
    g_plazo_activo = true;
    g_plazo_fin = std::chrono::steady_clock::now() +
                  std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                      std::chrono::duration<double, std::milli>(ms));
}

void plazo_quitar() { g_plazo_activo = false; }

bool plazo_vencido() {
    return g_plazo_activo && std::chrono::steady_clock::now() >= g_plazo_fin;
}

// Una sola lectura del archivo completo a RAM y despues parseo caracter a
// caracter: saltar lo que no sea digito y acumular con x = x*10 + d.
bool leer_enteros(const std::string& path, std::vector<int>& out) {
    MemPausa pausa;

    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;

    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz < 0) { std::fclose(f); return false; }

    std::vector<char> buf(static_cast<size_t>(sz) + 1);
    size_t got = std::fread(buf.data(), 1, static_cast<size_t>(sz), f);
    buf[got] = '\0';
    std::fclose(f);

    out.clear();

    out.reserve(got / 2 + 1);

    const char* p = buf.data();
    while (*p) {
        while (*p && (*p < '0' || *p > '9') && *p != '-') ++p;
        if (!*p) break;
        bool neg = false;
        if (*p == '-') { neg = true; ++p; }
        long long x = 0;
        while (*p >= '0' && *p <= '9') x = x * 10 + (*p++ - '0');
        out.push_back(static_cast<int>(neg ? -x : x));
    }
    return true;
}


static inline void append_int(std::vector<char>& b, long long x) {
    if (x == 0) { b.push_back('0'); return; }
    if (x < 0)  { b.push_back('-'); x = -x; }
    char tmp[24];
    int k = 0;
    while (x > 0) { tmp[k++] = char('0' + x % 10); x /= 10; }
    while (k > 0) b.push_back(tmp[--k]);   // los digitos salen al reves
}

bool escribir_enteros(const std::string& path, const std::vector<int>& v) {
    MemPausa pausa;
    std::vector<char> buf;
    buf.reserve(v.size() * 9 + 2);
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) buf.push_back(' ');
        append_int(buf, v[i]);
    }
    buf.push_back('\n');

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(buf.data(), 1, buf.size(), f);
    std::fclose(f);
    return true;
}

bool leer_matriz(const std::string& path, std::vector<long long>& out, int& n) {
    MemPausa pausa;
    std::vector<int> flat;
    if (!leer_enteros(path, flat)) return false;

    size_t total = flat.size();
    n = 0;
    while (static_cast<size_t>(n + 1) * (n + 1) <= total) ++n;
    if (static_cast<size_t>(n) * n != total) return false;  // no es cuadrada

    out.assign(flat.begin(), flat.end());
    return true;
}

bool escribir_matriz(const std::string& path, const std::vector<long long>& m, int n) {
    MemPausa pausa;
    std::vector<char> buf;
    buf.reserve(static_cast<size_t>(n) * n * 8 + n + 2);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (j) buf.push_back(' ');
            append_int(buf, m[static_cast<size_t>(i) * n + j]);
        }
        buf.push_back('\n');
    }
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(buf.data(), 1, buf.size(), f);
    std::fclose(f);
    return true;
}

bool EscritorCsv::abrir(const std::string& path, const std::string& header) {
    f_ = std::fopen(path.c_str(), "wb");
    if (!f_) return false;
    std::fprintf(f_, "%s\n", header.c_str());
    return true;
}
void EscritorCsv::fila(const std::string& line) {
    if (f_) { std::fprintf(f_, "%s\n", line.c_str()); std::fflush(f_); }
}
void EscritorCsv::cerrar() { if (f_) { std::fclose(f_); f_ = nullptr; } }
EscritorCsv::~EscritorCsv() { cerrar(); }

void asegurar_dir(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

}
