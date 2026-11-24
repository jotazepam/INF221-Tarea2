// brute_force.cpp
// Tarea 2  INF221 Algoritmos y Complejidad  2026-2
// Juan Pablo Naylor, 202104614-3
//
// Enumera todos los caminos construyendo la solucion de a un movimiento y
// recorriendo el arbol de estados en profundidad. Las ramas mueren en los
// nodos de fracaso, que aqui son los movimientos que entrarian a una muralla
// o dejarian el blindaje en cero: las dos unicas podas que el enunciado
// permite. No hay memorizacion ni cotas sobre el puntaje acumulado, que seria
// branch-and-bound.
//
// Costo: la cota no es la cantidad de caminos, C(N+M-2, N-1), sino la de
// prefijos, C(N+M, N) - 1, porque el recorrido comparte los prefijos pero
// vuelve a caminar los sufijos. Para 16x16 son 601 millones de nodos contra
// 155 millones de caminos. Tiempo Theta(C(N+M, N)), memoria Theta(N+M) de
// pila. El limite N+M <= 32 lo impone el programa principal, no este archivo.
//
// Dos decisiones de implementacion. Los datos invariantes viven en una clase
// en lugar de pasarse por parametro en cada nivel, que en un arbol de 10^8
// nodos se nota en el reloj. Y se explora el este antes que el sur
// conservando el primer optimo, de modo que los empates se resuelven igual que
// en la programacion dinamica y las dos salidas coinciden caracter por
// caracter.
//
// Fuentes: clase INF-221 "Fuerza Bruta: Backtracking"; Erickson, Algorithms,
// 2019, cap. 3.

#include "algoritmos.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace {

// El recorrido necesita muchos datos que no cambian entre llamadas (la grilla,
// las dimensiones, la penalizacion, el mejor camino hasta ahora). Agruparlos
// en una clase evita pasar ocho parametros por nivel de recursion, que en un
// arbol de 10^8 nodos no es un detalle cosmetico sino tiempo de ejecucion.
class BusquedaExhaustiva {
public:
    BusquedaExhaustiva(int n, int m, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values)
        : n_(n), m_(m), p_(p), t_(types), g_(values) {
        camino_.reserve(static_cast<std::size_t>(n + m));
        mejor_.found = false;
        mejor_.score = 0;
        mejor_.remaining_lives = 0;
    }

    PathResult ejecutar(int vidas) {
        // El enunciado garantiza que la celda de partida es normal, pero no se
        // da por supuesto. Si fuera muralla, o una mina con una sola vida de
        // blindaje, no existiria ningun camino valido.
        if (!transitable(t_[0][0], vidas)) return mejor_;

        // Pisar la celda de partida cuenta igual que pisar cualquier otra: se
        // cobra su ganancia y, si es mina, tambien la vida.
        const int vidasInicio = vidas - (t_[0][0] == kMina ? 1 : 0);
        explorar(0, 0, vidasInicio, gananciaCelda(t_[0][0], g_[0][0], p_));
        return mejor_;
    }

private:
    // Invariante de explorar(): el rover esta parado en (i,j) habiendo
    // recorrido el prefijo camino_, con el puntaje acumulado y las vidas que
    // se indican, y ese prefijo es valido.
    void explorar(int i, int j, int vidas, long long puntaje) {
        if (i == n_ - 1 && j == m_ - 1) {
            // Camino completo. El > estricto conserva el primero encontrado
            // entre los de puntaje maximo, que es el que prefiere 'R'.
            if (!mejor_.found || puntaje > mejor_.score) {
                mejor_.found = true;
                mejor_.score = puntaje;
                mejor_.path = camino_;
                mejor_.remaining_lives = vidas;
            }
            return;
        }

        // Primero el este y despues el sur, para fijar el desempate.
        intentar(i, j + 1, j + 1 < m_, 'R', vidas, puntaje);
        intentar(i + 1, j, i + 1 < n_, 'D', vidas, puntaje);
    }

    // Intenta avanzar a la celda destino (i,j). El parametro dentro dice si el
    // destino cae en la grilla, y se consulta antes de tocar las matrices.
    void intentar(int i, int j, bool dentro, char mov,
                  int vidas, long long puntaje) {
        if (!dentro) return;

        const int tipo = t_[i][j];
        if (!transitable(tipo, vidas)) return;   // muralla, o mina sin blindaje

        const int vidasDestino = vidas - (tipo == kMina ? 1 : 0);

        camino_.push_back(mov);
        explorar(i, j, vidasDestino, puntaje + gananciaCelda(tipo, g_[i][j], p_));
        camino_.pop_back();                      // se deshace el paso al volver
    }

    int n_, m_, p_;
    const std::vector<std::vector<int>>& t_;
    const std::vector<std::vector<long long>>& g_;
    std::string camino_;
    PathResult mejor_;
};

} // namespace

PathResult solveBruteForce(int n, int m, int v, int p,
                           const std::vector<std::vector<int>>& types,
                           const std::vector<std::vector<long long>>& values) {
    PathResult sinSolucion{false, 0, std::string(), 0};
    if (n <= 0 || m <= 0 || v <= 0) return sinSolucion;

    BusquedaExhaustiva busqueda(n, m, p, types, values);
    return busqueda.ejecutar(v);
}
