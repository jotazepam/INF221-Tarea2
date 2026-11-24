// algoritmos.hpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// Los tres comparten firma: reciben la grilla con los tipos y los valores en
// matrices separadas, tal como los entrega el archivo de entrada, y devuelven
// puntaje, camino y vidas restantes.
//
// Dos decisiones comunes a los tres. Los indices son 0-based, mientras que el
// enunciado numera desde (1,1); el camino de salida no depende del convenio
// porque es una secuencia de movimientos. Y ninguno supone que la celda de
// partida sea normal, aunque el enunciado lo garantice: si fuera mina cobran
// ahi mismo la vida y la penalizacion, porque darlo por supuesto haria que los
// tres se equivocaran igual y la verificacion cruzada no lo detectaria.
//
// Fuentes: clases INF-221 2026-2. El detalle por algoritmo esta en el
// encabezado de cada .cpp y en code/README.md.

#ifndef GRID_ALGORITMOS_HPP
#define GRID_ALGORITMOS_HPP

#include <string>
#include <vector>

// Identificadores de tipo de celda, con los mismos valores que usa el archivo
// de entrada y el generador de grillas.
constexpr int kNormal  =  1;
constexpr int kMuralla =  0;
constexpr int kMina    = -1;


// Se conserva la guarda original PATH_RESULT_H para que, si alguien reintroduce
// el bloque del enunciado en grid_pathfinding.cpp, siga habiendo una sola
// definicion y el programa compile igual.
#ifndef PATH_RESULT_H
#define PATH_RESULT_H
struct PathResult {
    bool found;               // false equivale a SIN SOLUCION
    long long score;
    std::string path;         // N+M-2 caracteres, cada uno 'R' o 'D'
    int remaining_lives;
};
#endif

// Ganancia neta de entrar a una celda. Una celda normal aporta su valor
// cientifico; una mina ignora ese valor y cobra la penalizacion P. Las
// murallas no tienen ganancia definida porque nunca se entra en ellas, y por
// eso ningun algoritmo debe llamar a esta funcion sobre una muralla.
inline long long gananciaCelda(int tipo, long long valor, int p) {
    return tipo == kMina ? -static_cast<long long>(p) : valor;
}

// Una celda es transitable si no es muralla. Entrar a una mina ademas cuesta
// una vida, asi que solo es posible si queda mas de una.
inline bool transitable(int tipo, int vidas) {
    if (tipo == kMuralla) return false;
    if (tipo == kMina && vidas <= 1) return false;
    return true;
}

// Busqueda exhaustiva. Theta(C(n+m, n)) en tiempo, Theta(n+m) en memoria. La
// cota no es la cantidad de caminos sino la de prefijos, que es mayor; el
// detalle esta en brute_force.cpp.
PathResult solveBruteForce(int n, int m, int v, int p,
                           const std::vector<std::vector<int>>& types,
                           const std::vector<std::vector<long long>>& values);

// Programacion dinamica. Theta(n*m*v) en tiempo y en memoria.
PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values);

// Heuristica voraz con la regla local del enunciado. Theta(n+m) en tiempo y
// en memoria por el camino de salida. No garantiza el optimo y puede fallar
// aunque exista un camino valido.
PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values);

#endif // GRID_ALGORITMOS_HPP
