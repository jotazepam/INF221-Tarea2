# =============================================================================
#  makefile
#  Tarea 2  INF221 Algoritmos y Complejidad  2026-2
#  Juan Pablo Naylor, 202104614-3
# =============================================================================
#
#  Uso rapido:
#     make            compila
#     make data       genera las grillas con N hasta 16
#     make run        mide con N = 4, 8, 16   (incluye toda la fuerza bruta)
#     make plots      genera los graficos PNG
#     make all-small  data + run + plots  (el flujo completo, rapido)
#
#     make data-full  genera TAMBIEN N = 64, 256 y 1024
#     make run-full   mide TAMBIEN esos tamanos
#
#  Los tiempos y tamanos reales de cada objetivo estan anotados mas abajo, en
#  la seccion correspondiente.
#
#  GRID_ROOT elige donde viven las GRILLAS DE ENTRADA, que son lo unico
#  pesado: el conjunto completo son unos 420 MB. Conviene sacarlas de una
#  carpeta sincronizada por OneDrive:
#     make data-full run-full GRID_ROOT=C:/Users/usuario/algoco_datos
#  Las salidas, las mediciones y los graficos siguen quedando bajo DATA_ROOT,
#  o sea dentro del proyecto, porque son livianos y van en la entrega.
#
#  SEED fija la semilla del generador. No cambiarla entre data y run: las
#  mediciones del informe corresponden a las grillas de esta semilla.
# =============================================================================

CXX      := g++
# Los -fno-builtin son imprescindibles: sin ellos GCC elide pares new/delete y
# asume que free() recibe el puntero que devolvio new, lo que rompe el contador
# de memoria de medicion.cpp. Mismos flags que exigen tcmalloc y jemalloc.
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra \
            -fno-builtin-operator-new -fno-builtin-operator-delete \
            -fno-builtin-malloc -fno-builtin-free

# -static es imprescindible en Windows: el reemplazo de operator new no cruza
# los limites de una DLL y el enlace dinamico corrompe el heap al arrancar.
LDFLAGS   := -static
PYTHON    := python
DATA_ROOT := data
GRID_ROOT := $(DATA_ROOT)
SEED      := 2026

# Donde el generador de graficos deja las tablas .tex del informe.
TABLES_DIR := ../../report/tables

# En Windows los ejecutables llevan .exe; en Linux/Mac no.
ifeq ($(OS),Windows_NT)
  EXE := .exe
  RM  := del /Q /F
else
  EXE :=
  RM  := rm -f
endif

TARGET := grid_pathfinding$(EXE)

SRCS := grid_pathfinding.cpp \
        algorithms/brute_force.cpp \
        algorithms/dynamic_programming.cpp \
        algorithms/greedy.cpp \
        ../common/medicion.cpp

OBJS := $(SRCS:.cpp=.o)

# -----------------------------------------------------------------------------
.PHONY: all data data-full run run-full plots all-small clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Depender de los headers hace que un cambio en ellos recompile lo que toca.
%.o: %.cpp algorithms/algoritmos.hpp ../common/medicion.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Generacion de datos -----------------------------------------------------
# data:      144 grillas, 0,13 MB en disco, instantaneo.
# data-full: 288 grillas, 421 MB, unos 18 segundos. El 94 por ciento de ese
#            peso son las grillas de N = 1024: una sola de 1024x2048 en dominio
#            D1000 ocupa 12,8 MB. Conviene usar GRID_ROOT para sacarlas de
#            OneDrive.
data:
	$(PYTHON) scripts/grid_generator.py --data-root $(GRID_ROOT) --max-n 16 --seed $(SEED)

data-full:
	$(PYTHON) scripts/grid_generator.py --data-root $(GRID_ROOT) --seed $(SEED)

# --- Mediciones --------------------------------------------------------------
# run: los tres algoritmos sobre N = 4, 8 y 16, unos 2 minutos. Cubre todas
# las instancias en que corre la fuerza bruta, porque el enunciado la limita a
# N + M <= 32 y el mayor de esos casos es 16x16. Es la corrida que sostiene la
# comparacion entre los tres paradigmas. Casi todo ese tiempo se va en las 24
# instancias de 16x16, donde la busqueda exhaustiva tarda unos 2 segundos cada
# una y los otros dos algoritmos no llegan a la centesima de milisegundo.
run: $(TARGET)
	./$(TARGET) --data-root $(DATA_ROOT) --input-dir $(GRID_ROOT)/grid_input --sizes 4,8,16 --reps 3

# run-full: agrega N = 64, 256 y 1024, donde solo corren dinamica y greedy.
# Es la corrida que muestra como escalan esos dos. Unos 3 minutos: el calculo
# de la dinamica sobre los tamanos grandes son solo 6 segundos, el resto se va
# en leer los 421 MB de grillas y en la fuerza bruta de los tamanos chicos.
# Memoria pico alrededor de 90 MB, contando la grilla ademas de la tabla.
run-full: $(TARGET)
	./$(TARGET) --data-root $(DATA_ROOT) --input-dir $(GRID_ROOT)/grid_input --reps 3

# --- Graficos ----------------------------------------------------------------
plots:
	$(PYTHON) scripts/plot_generator.py --data-root $(DATA_ROOT) --tables-dir $(TABLES_DIR)

all-small: $(TARGET) data run plots

clean:
	-$(RM) $(TARGET) 2>/dev/null || true
	-rm -f $(OBJS) 2>/dev/null || true
