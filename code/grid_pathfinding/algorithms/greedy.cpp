// greedy.cpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// La regla la fija el enunciado y aqui se implementa literal. Estando en (i,j)
// con v vidas, son candidatas las celdas contiguas al este y al sur que caen
// dentro de la grilla, que no son murallas y que no son minas si v == 1. La
// ganancia neta de una candidata es su valor, o -P si es mina. Se avanza a la
// de mayor ganancia neta y los empates van al este. Si no hay candidatas, el
// algoritmo falla y la salida es SIN SOLUCION.
//
// La eleccion es factible, localmente optima e irrevocable. De lo ultimo
// vienen tanto su costo lineal como sus dos formas de fallar: perder puntaje
// por miopia, que es la brecha que mide el informe, y quedarse sin candidatas
// en un callejon, que es la tasa de falla.
//
// Descartar minas cuando queda una sola vida coincide con la condicion de
// validez del camino, asi que la regla nunca produce un camino invalido por
// falta de blindaje: o llega, o falla antes.
//
// Costo: Theta(N+M) en tiempo. La memoria adicional es Theta(1) sobre el
// tamano de la salida, que es Theta(N+M).
//
// Fuentes: clase INF-221 "Algoritmos Greedy"; Cormen, Leiserson, Rivest y
// Stein, Introduction to Algorithms, 3a ed., sec. 16.2.

#include "algoritmos.hpp"

#include <cstddef>
#include <string>
#include <vector>

PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values) {
    const PathResult sinSolucion{false, 0, std::string(), 0};
    if (n <= 0 || m <= 0 || v <= 0) return sinSolucion;

    // La partida deberia ser una celda normal. Si no lo es se la trata como
    // cualquier otra, igual que en los otros dos algoritmos.
    if (!transitable(types[0][0], v)) return sinSolucion;

    int i = 0, j = 0;
    int vidas = v - (types[0][0] == kMina ? 1 : 0);
    long long puntaje = gananciaCelda(types[0][0], values[0][0], p);

    std::string camino;
    camino.reserve(static_cast<std::size_t>(n + m));

    // El rover avanza siempre al este o al sur, asi que la distancia que le
    // falta baja en uno por paso y el ciclo termina en n+m-2 iteraciones a lo
    // mas, llegue a la meta o falle antes.
    while (!(i == n - 1 && j == m - 1)) {
        // Evaluacion de las dos candidatas. transitable() aplica las dos
        // exclusiones de la regla: murallas siempre, minas cuando el blindaje
        // esta en su ultima unidad.
        const bool hayEste = (j + 1 < m) && transitable(types[i][j + 1], vidas);
        const bool haySur  = (i + 1 < n) && transitable(types[i + 1][j], vidas);

        if (!hayEste && !haySur) return sinSolucion;   // callejon sin salida

        bool alEste;
        if (!haySur) {
            alEste = true;
        } else if (!hayEste) {
            alEste = false;
        } else {
            const long long ganEste =
                gananciaCelda(types[i][j + 1], values[i][j + 1], p);
            const long long ganSur =
                gananciaCelda(types[i + 1][j], values[i + 1][j], p);
            alEste = (ganEste >= ganSur);              // el empate va al este
        }

        if (alEste) ++j;
        else        ++i;

        camino.push_back(alEste ? 'R' : 'D');

        // Efecto de pisar la celda: la mina detona, cobra una vida y resta P
        // en lugar de aportar su valor, que es lo que calcula gananciaCelda.
        if (types[i][j] == kMina) --vidas;
        puntaje += gananciaCelda(types[i][j], values[i][j], p);
    }

    PathResult res;
    res.found = true;
    res.score = puntaje;
    res.path = camino;
    res.remaining_lives = vidas;
    return res;
}
