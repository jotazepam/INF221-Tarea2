# TAREA 2 INF221 2026-2

Juan Pablo Naylor, Rol 202104614-3, Campus San Joaquín

Estudio experimental comparativo de tres paradigmas de diseño de algoritmos
sobre un mismo problema: encontrar en una grilla de N × M el camino de máximo
puntaje desde la esquina superior izquierda hasta la inferior derecha,
moviéndose solo hacia el este y hacia el sur, sorteando murallas intransitables
y minas que cuestan una vida y restan puntos.

Los tres paradigmas son fuerza bruta por backtracking, programación dinámica
por tabulación y una heurística greedy con la regla local que fija el
enunciado. El objetivo no es diseñarlos, que es directo, sino medir su
comportamiento real y contrastarlo con las cotas asintóticas que les
corresponden.

## Resultados en una línea

En el mayor tamaño donde ambas soluciones exactas se ejecutan, la programación
dinámica es unas doscientas mil veces más rápida que la búsqueda exhaustiva
dando exactamente la misma respuesta. La heurística greedy no alcanza la meta
en 38 de las 288 instancias, todas con murallas, y cuando llega pierde una
cantidad constante de puntos por decisión, independiente del tamaño de la
grilla.

## Estructura

```
.
├── assignment_statement    el enunciado y sus fuentes LaTeX
├── code                    implementaciones, mediciones y gráficos
├── report                  el mini informe y sus fuentes LaTeX
└── README.md               este archivo
```

En `code/README.md` está la documentación técnica: requisitos, cómo reproducir
los experimentos, las decisiones de implementación y el formato de las
mediciones. El informe compilado es `report/report.pdf`.

## Cómo reproducirlo

Con las herramientas instaladas, el experimento completo se rehace con tres
comandos desde `code/grid_pathfinding`:

```bash
make data-full        # 288 grillas, 421 MB, ~18 s
make run-full         # las 288 instancias, ~3 min
make plots            # 12 figuras PNG y 2 tablas .tex
```

El generador usa semilla fija, así que las grillas se reconstruyen idénticas a
las que produjeron estas mediciones. Los detalles están en `code/README.md`.

## Entrega

Vía aula.usm.cl en formato `.zip`.
