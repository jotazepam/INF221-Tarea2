// dynamic_programming.cpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// M[i][j][r] es el mejor puntaje alcanzable desde (i,j) hasta la meta,
// incluyendo la ganancia de (i,j), sabiendo que al pisarla quedaron r vidas.
// Con g(i,j) la ganancia de entrar a una celda, y r' = r-1 si el destino es
// mina y r' = r en otro caso:
//
//                 | g(i,j)                          si (i,j) = (n-1, m-1);
//   M[i][j][r] =  | g(i,j) + max { M[i][j+1][r'],
//                 |                M[i+1][j][r'] }  en otro caso;
//                 | inalcanzable                    si no queda destino.
//
// Un destino se descarta si cae fuera de la grilla, si es muralla, si r'
// llegaria a cero o si desde el no existe camino a la meta. La respuesta es
// M[0][0][V].
//
// Las vidas tienen que ser parte del estado: dos caminos que llegan a la misma
// celda no son comparables solo por puntaje, porque el que trae mas blindaje
// puede cruzar minas que el otro no. Esa tercera coordenada cumple el papel de
// la capacidad en la mochila 0-1.
//
// Costo: n*m*v subproblemas de costo Theta(1), o sea Theta(n*m*v) en tiempo y
// en memoria. Para 1024x2048 con V=3 son 6.291.456 estados, unos 56 MB.
//
// Tres decisiones de implementacion. Se tabula de atras hacia adelante en vez
// de memorizar, porque la version top-down abriria millones de marcos de pila
// en las grillas grandes. Las tablas son planas y el indice se calcula a mano,
// por localidad de memoria. Y se guarda la decision de cada estado en lugar de
// recomparar valores al reconstruir, porque ese byte hace ademas de bandera de
// estado inalcanzable y evita un centinela de puntaje, que en este problema
// podria confundirse con un puntaje legitimo muy negativo.
//
// Fuentes: clases INF-221 "Programacion Dinamica" y "DP Knapsack"; Cormen,
// Leiserson, Rivest y Stein, Introduction to Algorithms, 3a ed., secs. 15.1
// a 15.3.

#include "algoritmos.hpp"

#include <cstddef>
#include <string>
#include <vector>

PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values) {
    PathResult res{false, 0, std::string(), 0};
    if (n <= 0 || m <= 0 || v <= 0) return res;

    // Misma salvedad que en los otros dos algoritmos: la partida deberia ser
    // una celda normal, y si no lo es se trata como cualquier otra celda.
    if (!transitable(types[0][0], v)) return res;

    const std::size_t estados =
        static_cast<std::size_t>(n) * static_cast<std::size_t>(m) *
        static_cast<std::size_t>(v);

    // Dos tablas planas en lugar de un vector de vectores de vectores: el
    // indice se calcula a mano y las lecturas de los dos destinos quedan
    // cerca en memoria, lo que importa cuando la tabla pesa decenas de MB.
    std::vector<long long> mejor(estados, 0);

    // decision[e] guarda que movimiento conviene desde el estado e: 'R' al
    // este, 'D' al sur, 'F' si el estado es la meta y 0 si el estado no
    // alcanza la meta. El 0 hace de bandera de inalcanzable, asi no hace falta
    // un valor centinela de puntaje que se podria confundir con un puntaje
    // legitimo muy negativo.
    std::vector<char> decision(estados, 0);

    // r se guarda en r-1 porque un estado con cero vidas es invalido por
    // definicion y no merece una capa de la tabla.
    auto idx = [m, v](int i, int j, int r) -> std::size_t {
        return (static_cast<std::size_t>(i) * static_cast<std::size_t>(m) +
                static_cast<std::size_t>(j)) *
               static_cast<std::size_t>(v) +
               static_cast<std::size_t>(r - 1);
    };

    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            // Ningun estado de una muralla es valido: se dejan en 0, es decir
            // inalcanzables, y los vecinos los descartan solos.
            if (types[i][j] == kMuralla) continue;

            const long long gan = gananciaCelda(types[i][j], values[i][j], p);
            const bool esMeta = (i == n - 1 && j == m - 1);

            for (int r = 1; r <= v; ++r) {
                const std::size_t aqui = idx(i, j, r);

                if (esMeta) {
                    mejor[aqui] = gan;
                    decision[aqui] = 'F';
                    continue;
                }

                long long valor = 0;
                char mov = 0;

                // Candidato del este. Se evalua primero para que gane los
                // empates.
                if (j + 1 < m && transitable(types[i][j + 1], r)) {
                    const int rDestino = r - (types[i][j + 1] == kMina ? 1 : 0);
                    const std::size_t destino = idx(i, j + 1, rDestino);
                    if (decision[destino] != 0) {
                        valor = mejor[destino];
                        mov = 'R';
                    }
                }

                // Candidato del sur. Solo desplaza al este con > estricto.
                if (i + 1 < n && transitable(types[i + 1][j], r)) {
                    const int rDestino = r - (types[i + 1][j] == kMina ? 1 : 0);
                    const std::size_t destino = idx(i + 1, j, rDestino);
                    if (decision[destino] != 0 &&
                        (mov == 0 || mejor[destino] > valor)) {
                        valor = mejor[destino];
                        mov = 'D';
                    }
                }

                if (mov == 0) continue;          // callejon sin salida

                mejor[aqui] = gan + valor;
                decision[aqui] = mov;
            }
        }
    }

    // El estado inicial es (0,0) con las vidas que quedan despues de pisar la
    // partida, que son todas salvo que la partida sea una mina.
    const int rInicio = v - (types[0][0] == kMina ? 1 : 0);
    const std::size_t inicio = idx(0, 0, rInicio);
    if (decision[inicio] == 0) return res;       // SIN SOLUCION

    // Reconstruccion. Se baja por las decisiones desde el estado inicial
    // siguiendo las mismas reglas con que se llenaron: cada mina que se pisa
    // descuenta una vida, de modo que al llegar a la meta la variable r vale
    // exactamente el blindaje restante.
    res.found = true;
    res.score = mejor[inicio];
    res.path.reserve(static_cast<std::size_t>(n + m));

    int i = 0, j = 0, r = rInicio;
    while (decision[idx(i, j, r)] != 'F') {
        const char mov = decision[idx(i, j, r)];

        // Un estado alcanzable siempre apunta a otro alcanzable, asi que esto
        // no deberia ocurrir nunca. Se verifica igual porque sin la guarda un
        // error en el llenado se transformaria en una lectura fuera de rango
        // en lugar de un resultado visiblemente incorrecto.
        if (mov != 'R' && mov != 'D') return PathResult{false, 0, std::string(), 0};

        res.path.push_back(mov);
        if (mov == 'R') ++j;
        else            ++i;
        if (types[i][j] == kMina) --r;
    }
    res.remaining_lives = r;

    return res;
}
