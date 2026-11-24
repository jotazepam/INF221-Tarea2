// grid_pathfinding.cpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// Recorre los casos {n} x {m} x {tipo} x {dominio} x {muestra}, ejecuta los
// tres algoritmos sobre cada grilla midiendo tiempo y memoria, escribe un
// archivo de salida por algoritmo y vuelca las mediciones en un CSV.
//
// Decisiones de medicion: la grilla se lee una vez por caso y fuera del
// cronometro, porque leer dos millones de enteros no es parte de ningun
// algoritmo; los tres la reciben por referencia constante, asi que no hay
// copias que falseen la comparacion; el contador de memoria se reinicia justo
// antes de cada ejecucion, de modo que reporta la memoria auxiliar del
// algoritmo y no la que ya ocupaba la grilla; y se escribe una fila por
// repeticion, sin promediar, para poder calcular la mediana despues.
//
// La columna de memoria mide heap. La busqueda exhaustiva gasta la suya en la
// pila de recursion, que no pasa por ahi, asi que sale en cero: no es un error
// de medicion sino su costo real, Theta(N+M) de pila.
//
// Verificacion exigida por el enunciado: cuando fuerza bruta y programacion
// dinamica corren sobre la misma instancia se comparan puntaje, camino y vidas
// restantes. Comparar solo el puntaje bastaria, pero ambas resuelven los
// empates igual, asi que comparar las tres salidas es una prueba mas fuerte.
// Una discrepancia se informa y el programa termina con codigo distinto de
// cero.
//
// Fuentes: Bryant y O'Hallaron, Computer Systems: A Programmer's Perspective,
// 3a ed., cap. 5 (metodologia de medicion de tiempos);
// https://en.cppreference.com/w/cpp/chrono/steady_clock

#include "../common/medicion.hpp"
#include "algorithms/algoritmos.hpp"

#include <cstdio>
#include <string>
#include <vector>

// --- Utilidades de linea de comandos -----------------------------------------
static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static std::string opcion(int argc, char** argv, const std::string& nombre,
                          const std::string& porDefecto) {
    for (int i = 1; i + 1 < argc; ++i)
        if (nombre == argv[i]) return argv[i + 1];
    return porDefecto;
}

static bool bandera(int argc, char** argv, const std::string& nombre) {
    for (int i = 1; i < argc; ++i)
        if (nombre == argv[i]) return true;
    return false;
}

// --- Entrada y salida ---------------------------------------------------------
// Formato de entrada: N M V P, luego N filas de M tipos, luego N filas de M
// valores. Se usa fscanf y no iostreams porque en la grilla mas grande son 4,2
// millones de enteros y la diferencia se nota, aunque la lectura quede fuera
// del cronometro.
static bool leerGrilla(const std::string& ruta, int& n, int& m, int& v, int& p,
                       std::vector<std::vector<int>>& tipos,
                       std::vector<std::vector<long long>>& valores) {
    std::FILE* f = std::fopen(ruta.c_str(), "r");
    if (!f) return false;

    if (std::fscanf(f, "%d %d %d %d", &n, &m, &v, &p) != 4 ||
        n <= 0 || m <= 0 || v <= 0) {
        std::fclose(f);
        return false;
    }

    tipos.assign(n, std::vector<int>(m));
    valores.assign(n, std::vector<long long>(m));

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            if (std::fscanf(f, "%d", &tipos[i][j]) != 1) { std::fclose(f); return false; }

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            if (std::fscanf(f, "%lld", &valores[i][j]) != 1) { std::fclose(f); return false; }

    std::fclose(f);
    return true;
}

// Formato de salida del enunciado: puntaje, camino y vidas restantes en tres
// lineas, o la unica linea SIN SOLUCION.
static bool escribirSalida(const std::string& ruta, const PathResult& r) {
    std::FILE* f = std::fopen(ruta.c_str(), "w");
    if (!f) return false;
    if (r.found)
        std::fprintf(f, "%lld\n%s\n%d\n", r.score, r.path.c_str(), r.remaining_lives);
    else
        std::fprintf(f, "SIN SOLUCION\n");
    std::fclose(f);
    return true;
}

// --- Tabla de algoritmos ------------------------------------------------------
// La clave es la que el enunciado exige en el nombre del archivo de salida.
using FnAlgoritmo = PathResult (*)(int, int, int, int,
                                   const std::vector<std::vector<int>>&,
                                   const std::vector<std::vector<long long>>&);

struct Algoritmo {
    const char* clave;
    const char* nombre;
    FnAlgoritmo fn;
};

static const Algoritmo kAlgoritmos[] = {
    {"bruteforce", "fuerza bruta",          solveBruteForce},
    {"dp",         "programacion dinamica", solveDP},
    {"greedy",     "greedy",                solveGreedy},
};

// --- MAIN ---------------------------------------------------------------------
int main(int argc, char** argv) {
    const std::string dataRoot = opcion(argc, argv, "--data-root", "data");
    const int reps = std::stoi(opcion(argc, argv, "--reps", "3"));

    const auto sizes   = split(opcion(argc, argv, "--sizes",   "4,8,16,64,256,1024"), ',');
    const auto factors = split(opcion(argc, argv, "--factors", "1,2"), ',');
    const auto types   = split(opcion(argc, argv, "--types",   "libre,murallas,minas,mixta"), ',');
    const auto domains = split(opcion(argc, argv, "--domains", "D10,D1000"), ',');
    const auto samples = split(opcion(argc, argv, "--samples", "a,b,c"), ',');
    const auto algos   = split(opcion(argc, argv, "--algos",   "bruteforce,dp,greedy"), ',');
    const bool escribirSalidas = !bandera(argc, argv, "--no-output");

    // Cota del enunciado para la fuerza bruta. Ningun archivo de algoritmo la
    // impone: es responsabilidad de este programa.
    const int maxBf = std::stoi(opcion(argc, argv, "--max-bf", "32"));

    // Solo las grillas de entrada son pesadas: el conjunto completo son unos
    // 420 MB, de los cuales el 94 por ciento esta en los casos de N = 1024.
    // Con --input-dir se las puede dejar fuera de la carpeta sincronizada sin
    // mover las salidas ni las mediciones, que son livianas y si forman parte
    // de la entrega en la estructura que exige el enunciado.
    const std::string dirIn  = opcion(argc, argv, "--input-dir", dataRoot + "/grid_input");
    const std::string dirOut = dataRoot + "/grid_output";
    const std::string dirMed = dataRoot + "/measurements";
    medicion::asegurar_dir(dirOut);
    medicion::asegurar_dir(dirMed);

    const std::string rutaCsv = dirMed + "/measurements_grid_pathfinding.csv";
    medicion::EscritorCsv csv;
    if (!csv.abrir(rutaCsv,
                   "algoritmo,n,m,tipo,dominio,muestra,rep,"
                   "tiempo_ms,memoria_bytes,puntaje,vidas,estado")) {
        std::fprintf(stderr, "ERROR: no se pudo escribir %s\n", rutaCsv.c_str());
        return 1;
    }

    std::fprintf(stderr, "%-22s %-11s %-9s %-6s %-3s %12s %12s %12s\n",
                 "algoritmo", "grilla", "tipo", "dom", "mue",
                 "tiempo_ms", "memoria_KB", "puntaje");
    std::fprintf(stderr, "%s\n", std::string(95, '-').c_str());

    int casosLeidos = 0, casosFaltantes = 0;
    int bfOmitidos = 0, verificados = 0, discrepancias = 0, greedySinSolucion = 0;

    for (const auto& sn : sizes) {
        for (const auto& sf : factors) {
            const int n = std::stoi(sn);
            const int m = n * std::stoi(sf);

            for (const auto& t : types) {
                for (const auto& d : domains) {
                    for (const auto& s : samples) {
                        const std::string base = std::to_string(n) + "_" + std::to_string(m) +
                                                 "_" + t + "_" + d + "_" + s;
                        const std::string rutaIn = dirIn + "/" + base + ".txt";

                        int gn = 0, gm = 0, v = 0, p = 0;
                        std::vector<std::vector<int>> tipos;
                        std::vector<std::vector<long long>> valores;
                        if (!leerGrilla(rutaIn, gn, gm, v, p, tipos, valores)) {
                            ++casosFaltantes;
                            continue;
                        }
                        ++casosLeidos;

                        // Resultados de la repeticion 0, para la verificacion
                        // cruzada entre fuerza bruta y programacion dinamica.
                        PathResult resBf{false, 0, std::string(), 0};
                        PathResult resDp{false, 0, std::string(), 0};
                        bool corrioBf = false, corrioDp = false;

                        for (const auto& clave : algos) {
                            const Algoritmo* alg = nullptr;
                            for (const auto& a : kAlgoritmos)
                                if (clave == a.clave) { alg = &a; break; }
                            if (!alg) {
                                std::fprintf(stderr, "AVISO: algoritmo desconocido '%s'\n",
                                             clave.c_str());
                                continue;
                            }

                            // El costo de la busqueda exhaustiva crece como
                            // C(n+m, n), asi que fuera de la cota no se ejecuta.
                            if (clave == "bruteforce" && gn + gm > maxBf) {
                                ++bfOmitidos;
                                continue;
                            }

                            for (int r = 0; r < reps; ++r) {
                                medicion::mem_reiniciar();
                                medicion::Cronometro timer;
                                timer.iniciar();
                                PathResult res = alg->fn(gn, gm, v, p, tipos, valores);
                                const double ms = timer.detener_ms();
                                const size_t mem = medicion::mem_pico();

                                // Fuera del cronometro y del contador: el resto
                                // del cuerpo solo registra lo ya medido.
                                medicion::MemPausa pausa;

                                const char* estado = res.found ? "OK" : "SIN SOLUCION";
                                csv.fila(std::string(alg->nombre) + "," +
                                         std::to_string(gn) + "," + std::to_string(gm) + "," +
                                         t + "," + d + "," + s + "," + std::to_string(r) + "," +
                                         std::to_string(ms) + "," + std::to_string(mem) + "," +
                                         (res.found ? std::to_string(res.score) : std::string()) + "," +
                                         (res.found ? std::to_string(res.remaining_lives) : std::string()) + "," +
                                         estado);

                                if (r == 0) {
                                    std::fprintf(stderr, "%-22s %5dx%-5d %-9s %-6s %-3s %12.3f %12.1f %12s\n",
                                                 alg->nombre, gn, gm, t.c_str(), d.c_str(), s.c_str(),
                                                 ms, mem / 1024.0,
                                                 res.found ? std::to_string(res.score).c_str()
                                                           : "SIN SOLUCION");

                                    if (clave == "bruteforce") { resBf = res; corrioBf = true; }
                                    if (clave == "dp")         { resDp = res; corrioDp = true; }
                                    if (clave == "greedy" && !res.found) ++greedySinSolucion;

                                    if (escribirSalidas &&
                                        !escribirSalida(dirOut + "/" + base + "_" + clave + "_out.txt", res)) {
                                        std::fprintf(stderr, "AVISO: no se pudo escribir la salida de %s en %s\n",
                                                     clave.c_str(), base.c_str());
                                    }
                                }
                            }
                        }

                        // Verificacion cruzada exigida por el enunciado.
                        if (corrioBf && corrioDp) {
                            ++verificados;
                            const bool iguales = resBf.found == resDp.found &&
                                                 resBf.score == resDp.score &&
                                                 resBf.path == resDp.path &&
                                                 resBf.remaining_lives == resDp.remaining_lives;
                            if (!iguales) {
                                ++discrepancias;
                                std::fprintf(stderr,
                                    "  !! DISCREPANCIA en %s\n"
                                    "     fuerza bruta: %s puntaje=%lld camino=%s vidas=%d\n"
                                    "     dinamica:     %s puntaje=%lld camino=%s vidas=%d\n",
                                    base.c_str(),
                                    resBf.found ? "OK" : "SIN SOLUCION",
                                    resBf.score, resBf.path.c_str(), resBf.remaining_lives,
                                    resDp.found ? "OK" : "SIN SOLUCION",
                                    resDp.score, resDp.path.c_str(), resDp.remaining_lives);
                            }
                        }
                    }
                }
            }
        }
    }

    csv.cerrar();

    std::fprintf(stderr,
                 "\nCasos medidos: %d   |   No encontrados: %d   |   "
                 "Fuerza bruta omitida por n+m > %d: %d\n",
                 casosLeidos, casosFaltantes, maxBf, bfOmitidos);
    std::fprintf(stderr,
                 "Verificacion bruta contra dinamica: %d instancias, %d discrepancias\n",
                 verificados, discrepancias);
    std::fprintf(stderr, "Greedy sin solucion: %d de %d instancias\n",
                 greedySinSolucion, casosLeidos);

    if (casosLeidos == 0) {
        std::fprintf(stderr,
            "\nNo se encontro ninguna grilla en %s\n"
            "Genere los datos primero:  python scripts/grid_generator.py\n",
            dirIn.c_str());
        return 1;
    }
    std::fprintf(stderr, "Mediciones escritas en: %s\n", rutaCsv.c_str());

    return discrepancias == 0 ? 0 : 1;
}
