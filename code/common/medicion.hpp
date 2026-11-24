// medicion.hpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// Cronometro, contador de memoria auxiliar, entrada/salida rapida, plazo de
// ejecucion y escritura de CSV. La implementacion esta en medicion.cpp.
//
// Fuentes: https://en.cppreference.com/w/cpp/chrono/steady_clock ; Bryant y
// O'Hallaron, Computer Systems, 3a ed., cap. 5 (por que no usar clock() para
// medir tiempos).

#ifndef MEDICION_HPP
#define MEDICION_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace medicion {


class Cronometro {
public:
    void iniciar() { t0_ = std::chrono::steady_clock::now(); }
    double detener_ms() const {
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0_).count();
    }
private:
    std::chrono::steady_clock::time_point t0_;
};


void   mem_reiniciar();
size_t mem_pico();
size_t mem_actual();
void   mem_habilitar(bool on);

struct MemPausa {
    MemPausa()  { mem_habilitar(false); }
    ~MemPausa() { mem_habilitar(true);  }
};


void plazo_iniciar(double ms);   // ms <= 0 desactiva el plazo
void plazo_quitar();
bool plazo_vencido();


bool leer_enteros(const std::string& path, std::vector<int>& out);
bool escribir_enteros(const std::string& path, const std::vector<int>& v);

bool leer_matriz(const std::string& path, std::vector<long long>& out, int& n);
bool escribir_matriz(const std::string& path, const std::vector<long long>& m, int n);

// Una linea por (algoritmo, caso, repeticion), sin promediar: los datos crudos
// permiten calcular mediana o descartar outliers sin volver a medir.
class EscritorCsv {
public:
    bool abrir(const std::string& path, const std::string& header);
    void fila(const std::string& line);
    void cerrar();
    ~EscritorCsv();
private:
    std::FILE* f_ = nullptr;
};

void asegurar_dir(const std::string& path);

}

#endif
