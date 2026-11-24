#!/usr/bin/env python3
# =============================================================================
#  plot_generator.py
#  Tarea 2  INF221 Algoritmos y Complejidad  2026-2
#  Juan Pablo Naylor, 202104614-3
# =============================================================================
#
#  Lee  <data-root>/measurements/measurements_grid_pathfinding.csv  y escribe
#  los PNG en  <data-root>/plots/, mas las tablas .tex del informe en
#  --tables-dir. Ningun numero se escribe a mano: todo sale del CSV, que es lo
#  que permite confirmar que las figuras del informe vienen de los datos.
#
#  Se usa la mediana para el tiempo porque el ruido de una medicion es
#  asimetrico: una interrupcion del sistema solo puede hacer que el programa
#  parezca mas lento. Para la brecha se reportan mediana y maximo y se dibuja
#  la distribucion, porque ahi los extremos no son ruido sino el resultado.
#
#  Produce: tiempo contra tamano, global y por tipo de grilla; tiempo
#  normalizado por la cota teorica de cada algoritmo; memoria; la distribucion
#  de la brecha por tipo y dominio; la brecha contra el tamano; la brecha por
#  movimiento del camino; y la tasa de falla del greedy.
#
#  Fuentes: clases INF-221 2026-2;
#  https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.boxplot.html
# =============================================================================

import argparse
import csv
import math
import os
from collections import defaultdict

import numpy as np
import matplotlib
matplotlib.use("Agg")          # backend sin ventana: necesario para correr
                               # dentro de un makefile sin entorno grafico
import matplotlib.pyplot as plt

# --- Estilo comun ------------------------------------------------------------
BRUTA = "fuerza bruta"
DP = "programacion dinamica"
GREEDY = "greedy"

COLORES = {BRUTA: "#d62728", DP: "#1f77b4", GREEDY: "#2ca02c"}
MARCAS = {BRUTA: "s", DP: "o", GREEDY: "^"}
ETIQUETAS = {BRUTA: "fuerza bruta", DP: "programación dinámica", GREEDY: "greedy"}

TIPOS = ["libre", "murallas", "minas", "mixta"]
COLOR_TIPO = {"libre": "#4c72b0", "murallas": "#c44e52",
              "minas": "#dd8452", "mixta": "#8172b3"}


def estilo(ax, titulo, xlabel, ylabel, leyenda=True):
    ax.set_title(titulo, fontsize=11, fontweight="bold")
    ax.set_xlabel(xlabel, fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.grid(True, which="both", alpha=0.3, linewidth=0.5)
    if leyenda:
        ax.legend(fontsize=8, framealpha=0.9)


def ejex_tamanos(ax, xs):
    """Marca el eje x solo en los tamanos medidos.

    Con escala logaritmica y un rango estrecho, matplotlib rotula tambien las
    marcas menores y las etiquetas se encabalgan ("3x10^1" encima de
    "4x10^1"). Poniendo las marcas a mano en los valores que de verdad existen
    el eje queda legible y ademas dice cual es cada grilla.
    """
    xs = sorted(set(xs))
    ax.set_xticks(xs)
    # Con el dataset completo son doce tamanos y los ultimos quedan muy juntos
    # en escala logaritmica, asi que a partir de cierta cantidad se rotan.
    if len(xs) > 8:
        ax.set_xticklabels([str(x) for x in xs], fontsize=7,
                           rotation=45, ha="right")
    else:
        ax.set_xticklabels([str(x) for x in xs], fontsize=8)
    ax.minorticks_off()


def leer_csv(ruta):
    if not os.path.exists(ruta):
        return []
    with open(ruta, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def guardar(fig, destino, nombre):
    os.makedirs(destino, exist_ok=True)
    fig.tight_layout()
    fig.savefig(os.path.join(destino, nombre), dpi=150)
    plt.close(fig)
    print(f"  {nombre}")


def agregar(filas, clave_fn, valor_fn):
    """Agrupa por clave y devuelve la mediana del valor en cada grupo."""
    grupos = defaultdict(list)
    for r in filas:
        grupos[clave_fn(r)].append(valor_fn(r))
    return {k: float(np.median(v)) for k, v in grupos.items()}


def celdas(r):
    return int(r["n"]) * int(r["m"])


def instancia(r):
    """Clave que identifica una grilla concreta, sin el algoritmo ni la rep."""
    return (int(r["n"]), int(r["m"]), r["tipo"], r["dominio"], r["muestra"])


def brechas(filas):
    """Lista de (instancia, brecha) y lista de instancias donde greedy fallo.

    La brecha es s(OPT) - s(greedy), con el optimo tomado de la programacion
    dinamica, que es exacta y corre en todas las instancias. Solo se define
    cuando el greedy llego a la meta; cuando falla, el caso cuenta para la tasa
    de falla y no para la brecha, tal como pide el enunciado.
    """
    cero = [r for r in filas if r["rep"] == "0"]
    optimo = {instancia(r): r for r in cero if r["algoritmo"] == DP}

    pares, fallas = [], []
    for r in cero:
        if r["algoritmo"] != GREEDY:
            continue
        k = instancia(r)
        if r["estado"] != "OK":
            fallas.append(k)
            continue
        ref = optimo.get(k)
        if ref is None or ref["estado"] != "OK":
            continue
        pares.append((k, int(ref["puntaje"]) - int(r["puntaje"])))
    return pares, fallas


# =============================================================================
def grafico_tiempo(filas, destino, tipo=None):
    sub = filas if tipo is None else [r for r in filas if r["tipo"] == tipo]
    if not sub:
        return
    med = agregar(sub, lambda r: (r["algoritmo"], celdas(r)),
                  lambda r: float(r["tiempo_ms"]))

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for alg in (BRUTA, DP, GREEDY):
        pts = sorted((c, t) for (a, c), t in med.items() if a == alg)
        if not pts:
            continue
        xs = [p[0] for p in pts]
        ys = [max(p[1], 1e-4) for p in pts]       # evitar log(0)
        ax.plot(xs, ys, marker=MARCAS[alg], color=COLORES[alg],
                label=ETIQUETAS[alg], linewidth=1.6, markersize=5)

    # Recta de pendiente 1 anclada al ultimo punto de la dinamica: en log-log
    # una funcion lineal en N*M se ve como una recta de pendiente 1.
    pts_dp = sorted((c, t) for (a, c), t in med.items() if a == DP)
    if len(pts_dp) >= 2 and pts_dp[-1][1] > 0:
        xs = np.array([p[0] for p in pts_dp], dtype=float)
        guia = pts_dp[-1][1] * xs / pts_dp[-1][0]
        ax.plot(xs, guia, "k--", linewidth=1, alpha=0.6,
                label="referencia lineal en $N\\cdot M$")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ejex_tamanos(ax, [c for _, c in med])
    sufijo = "todas las grillas" if tipo is None else f"grillas tipo {tipo}"
    estilo(ax, f"Tiempo de ejecución vs tamaño  --  {sufijo}",
           "$N \\cdot M$ (escala log)", "tiempo [ms] (escala log)")
    nombre = "tiempo_vs_tamano.png" if tipo is None else f"tiempo_vs_tamano_{tipo}.png"
    guardar(fig, destino, nombre)


def grafico_normalizado(filas, destino):
    """Tiempo dividido por la cota teorica: si la cota sirve, la curva aplana."""
    med = agregar(filas, lambda r: (r["algoritmo"], int(r["n"]), int(r["m"])),
                  lambda r: float(r["tiempo_ms"]))

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.3))

    # El orden importa: set_xscale reinstala el localizador de marcas, asi que
    # las marcas explicitas se ponen despues de fijar la escala.

    # Fuerza bruta: la cota es la cantidad de prefijos del arbol, C(N+M, N) - 1.
    axes[0].set_xscale("log")
    pts = sorted((n * m, t, n, m) for (a, n, m), t in med.items() if a == BRUTA)
    if pts:
        xs = [p[0] for p in pts]
        ys = [p[1] * 1e6 / (math.comb(p[2] + p[3], p[2]) - 1) for p in pts]
        axes[0].plot(xs, ys, marker=MARCAS[BRUTA], color=COLORES[BRUTA],
                     linewidth=1.6, markersize=5, label=ETIQUETAS[BRUTA])
        ejex_tamanos(axes[0], xs)
    estilo(axes[0], "Fuerza bruta normalizada por $C(N+M,\\,N)$\n"
                    "una curva plana confirma la cota",
           "$N \\cdot M$ (escala log)", "ns por nodo del árbol")

    # Dinamica: la cota es la cantidad de estados, N*M*V, con V = 3 fijo.
    axes[1].set_xscale("log")
    pts = sorted((n * m, t) for (a, n, m), t in med.items() if a == DP)
    if pts:
        xs = [p[0] for p in pts]
        ys = [p[1] * 1e6 / (p[0] * 3) for p in pts]
        axes[1].plot(xs, ys, marker=MARCAS[DP], color=COLORES[DP],
                     linewidth=1.6, markersize=5, label=ETIQUETAS[DP])
        ejex_tamanos(axes[1], xs)
    estilo(axes[1], "Dinámica normalizada por $N \\cdot M \\cdot V$\n"
                    "una curva plana confirma la cota",
           "$N \\cdot M$ (escala log)", "ns por estado")

    guardar(fig, destino, "normalizado_cotas.png")


def grafico_memoria(filas, destino):
    med = agregar(filas, lambda r: (r["algoritmo"], celdas(r)),
                  lambda r: float(r["memoria_bytes"]))

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for alg in (BRUTA, DP, GREEDY):
        pts = sorted((c, m) for (a, c), m in med.items() if a == alg)
        if not pts:
            continue
        xs = [p[0] for p in pts]
        ys = [max(p[1], 1.0) for p in pts]        # 1 byte en vez de 0 para el log
        ax.plot(xs, ys, marker=MARCAS[alg], color=COLORES[alg],
                label=ETIQUETAS[alg], linewidth=1.6, markersize=5)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ejex_tamanos(ax, [c for _, c in med])
    estilo(ax, "Memoria auxiliar de heap vs tamaño\n"
               "(la fuerza bruta gasta pila, no heap: se dibuja en 1 byte)",
           "$N \\cdot M$ (escala log)", "bytes (escala log)")
    guardar(fig, destino, "memoria_vs_tamano.png")


def grafico_brecha(filas, destino):
    """Distribucion de la brecha por tipo de grilla, una figura por dominio."""
    pares, _ = brechas(filas)
    if not pares:
        return
    for dom in sorted({k[3] for k, _ in pares}):
        datos = [[b for k, b in pares if k[3] == dom and k[2] == t] for t in TIPOS]
        if not any(datos):
            continue
        fig, ax = plt.subplots(figsize=(7, 4.5))
        bp = ax.boxplot(datos, tick_labels=TIPOS, patch_artist=True, widths=0.55)
        for caja, t in zip(bp["boxes"], TIPOS):
            caja.set_facecolor(COLOR_TIPO[t])
            caja.set_alpha(0.55)
        for mediana in bp["medians"]:
            mediana.set_color("black")
        estilo(ax, f"Brecha del greedy por tipo de grilla  --  dominio {dom}\n"
                   "$\\Delta = s(C^*) - s(C_{greedy})$, solo casos con solución",
               "", "brecha [puntos]", leyenda=False)
        guardar(fig, destino, f"brecha_greedy_{dom}.png")


def grafico_brecha_vs_tamano(filas, destino):
    pares, _ = brechas(filas)
    if not pares:
        return
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.3))
    for ax, dom in zip(axes, sorted({k[3] for k, _ in pares})):
        por_tam = defaultdict(list)
        for k, b in pares:
            if k[3] == dom:
                por_tam[k[0] * k[1]].append(b)
        xs = sorted(por_tam)
        ax.plot(xs, [float(np.median(por_tam[x])) for x in xs],
                marker="o", color="#1f77b4", linewidth=1.6, label="mediana")
        ax.plot(xs, [max(por_tam[x]) for x in xs],
                marker="^", linestyle="--", color="#d62728", linewidth=1.2,
                label="máximo observado")
        ax.set_xscale("log")
        ejex_tamanos(ax, xs)
        estilo(ax, f"Brecha vs tamaño  --  dominio {dom}",
               "$N \\cdot M$ (escala log)", "brecha [puntos]")
    guardar(fig, destino, "brecha_vs_tamano.png")


def grafico_brecha_por_paso(filas, destino):
    """Brecha dividida por la cantidad de movimientos del camino.

    La brecha cruda crece con el tamano, lo que es obvio: un camino mas largo
    da mas oportunidades de equivocarse. La pregunta interesante es si el
    greedy se degrada ademas POR DECISION, y para eso hay que dividir por los
    N+M-2 movimientos. Una curva plana significa que la heuristica pierde una
    cantidad constante por paso y que el deterioro con el tamano es solo el
    acumulado, no un empeoramiento de la regla.
    """
    pares, _ = brechas(filas)
    if not pares:
        return
    fig, ax = plt.subplots(figsize=(7, 4.5))
    todos = []
    for dom in sorted({k[3] for k, _ in pares}):
        por_tam = defaultdict(list)
        for k, b in pares:
            if k[3] == dom:
                por_tam[k[0] * k[1]].append(b / (k[0] + k[1] - 2))
        # En las grillas mas chicas la brecha mediana puede ser cero, porque
        # con tan pocos caminos el greedy suele dar con el optimo. El cero no
        # cabe en escala logaritmica, asi que esos puntos se omiten en vez de
        # dibujar una linea que se va al infinito.
        puntos = [(x, float(np.median(por_tam[x]))) for x in sorted(por_tam)]
        puntos = [(x, y) for x, y in puntos if y > 0]
        if not puntos:
            continue
        todos += [x for x, _ in puntos]
        ax.plot([x for x, _ in puntos], [y for _, y in puntos],
                marker="o", linewidth=1.6, markersize=5, label=f"dominio {dom}")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ejex_tamanos(ax, todos)
    estilo(ax, "Brecha por movimiento del camino\n"
               "$\\Delta / (N+M-2)$: una curva plana significa pérdida "
               "constante por decisión",
           "$N \\cdot M$ (escala log)", "puntos perdidos por paso (escala log)")
    guardar(fig, destino, "brecha_por_paso.png")


def grafico_tasa_falla(filas, destino):
    pares, fallas = brechas(filas)
    total = defaultdict(int)
    malas = defaultdict(int)
    for k, _ in pares:
        total[(k[2], k[3])] += 1
    for k in fallas:
        total[(k[2], k[3])] += 1
        malas[(k[2], k[3])] += 1
    if not total:
        return

    doms = sorted({d for _, d in total})
    x = np.arange(len(TIPOS))
    ancho = 0.8 / max(len(doms), 1)

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for i, dom in enumerate(doms):
        alturas = [100.0 * malas[(t, dom)] / total[(t, dom)] if total[(t, dom)] else 0.0
                   for t in TIPOS]
        pos = x + (i - (len(doms) - 1) / 2) * ancho
        barras = ax.bar(pos, alturas, ancho * 0.9, label=f"dominio {dom}")
        for b, h, t in zip(barras, alturas, TIPOS):
            if total[(t, dom)]:
                ax.text(b.get_x() + b.get_width() / 2, h + 0.6,
                        f"{malas[(t, dom)]}/{total[(t, dom)]}",
                        ha="center", fontsize=7)
    ax.set_xticks(x)
    ax.set_xticklabels(TIPOS)
    ax.set_ylim(0, max(5.0, ax.get_ylim()[1] * 1.15))
    estilo(ax, "Tasa de falla del greedy\n"
               "instancias en que no alcanza la meta pese a existir camino válido",
           "", "% de instancias sin solución")
    guardar(fig, destino, "tasa_falla_greedy.png")


# =============================================================================
def tex_escapar(s):
    return s.replace("_", "\\_")


def generar_tablas_latex(filas, destino_tex):
    """Emite las tablas del informe como .tex, directo desde las mediciones.

    El enunciado exige poder confirmar que lo presentado en el informe proviene
    de los datos generados por los algoritmos. Con los graficos eso ya se
    cumple; generando TAMBIEN las tablas se cierra el circulo: ningun numero
    del informe se escribe a mano.
    """
    if not filas:
        return
    os.makedirs(destino_tex, exist_ok=True)

    # --- Tabla 1: tiempo y memoria por tamano ---
    t = agregar(filas, lambda r: (r["algoritmo"], int(r["n"]), int(r["m"])),
                lambda r: float(r["tiempo_ms"]))
    mem = agregar(filas, lambda r: (r["algoritmo"], int(r["n"]), int(r["m"])),
                  lambda r: float(r["memoria_bytes"]))
    formas = sorted({(n, m) for _, n, m in t})

    lineas = [
        "% Generado automaticamente por grid_pathfinding/scripts/plot_generator.py",
        "% NO EDITAR A MANO: se sobrescribe al correr 'make plots'.",
        "\\begin{table}[H]", "\\centering", "\\small",
        "\\begin{tabular}{lrrrr}", "\\hline",
        "Grilla & Fuerza bruta & Dinámica & Greedy & Memoria DP \\\\",
        "{} & [ms] & [ms] & [ms] & [KB] \\\\", "\\hline",
    ]
    for (n, m) in formas:
        def celda(alg):
            v = t.get((alg, n, m))
            return "--" if v is None else f"{v:.4f}"
        kb = mem.get((DP, n, m), 0.0) / 1024.0
        lineas.append(f"${n} \\times {m}$ & {celda(BRUTA)} & {celda(DP)} & "
                      f"{celda(GREEDY)} & {kb:.1f} \\\\")
    lineas += [
        "\\hline", "\\end{tabular}",
        "\\caption{Tiempo (mediana de todas las repeticiones e instancias de "
        "cada tamaño) y memoria auxiliar de la programación dinámica. El guion "
        "indica que la fuerza bruta no se ejecuta por la cota $N+M \\leq 32$.}",
        "\\label{tab:grid_tiempos}", "\\end{table}",
    ]
    ruta = os.path.join(destino_tex, "tabla_grid_tiempos.tex")
    with open(ruta, "w", encoding="utf-8") as f:
        f.write("\n".join(lineas) + "\n")
    print("  [tex] tabla_grid_tiempos.tex")

    # --- Tabla 2: calidad del greedy ---
    pares, fallas = brechas(filas)
    total = defaultdict(int)
    malas = defaultdict(int)
    valores = defaultdict(list)
    for k, b in pares:
        total[(k[2], k[3])] += 1
        valores[(k[2], k[3])].append(b)
    for k in fallas:
        total[(k[2], k[3])] += 1
        malas[(k[2], k[3])] += 1

    doms = sorted({d for _, d in total})
    lineas = [
        "% Generado automaticamente por grid_pathfinding/scripts/plot_generator.py",
        "% NO EDITAR A MANO: se sobrescribe al correr 'make plots'.",
        "\\begin{table}[H]", "\\centering", "\\small",
        "\\begin{tabular}{llrrrr}", "\\hline",
        "Tipo & Dominio & Instancias & Sin solución & $\\Delta$ mediana & "
        "$\\Delta$ máxima \\\\", "\\hline",
    ]
    for tipo in TIPOS:
        for dom in doms:
            if not total[(tipo, dom)]:
                continue
            v = valores[(tipo, dom)]
            med = f"{float(np.median(v)):.0f}" if v else "--"
            mx = f"{max(v)}" if v else "--"
            lineas.append(f"{tex_escapar(tipo)} & {dom} & {total[(tipo, dom)]} & "
                          f"{malas[(tipo, dom)]} & {med} & {mx} \\\\")
    lineas += [
        "\\hline", "\\end{tabular}",
        "\\caption{Calidad de la solución greedy. La brecha "
        "$\\Delta = s(C^*) - s(C_{\\text{greedy}})$ se calcula solo sobre las "
        "instancias en que la heurística alcanza la meta.}",
        "\\label{tab:grid_calidad}", "\\end{table}",
    ]
    ruta = os.path.join(destino_tex, "tabla_grid_calidad.tex")
    with open(ruta, "w", encoding="utf-8") as f:
        f.write("\n".join(lineas) + "\n")
    print("  [tex] tabla_grid_calidad.tex")


# =============================================================================
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data-root", default="../data")
    ap.add_argument("--tables-dir", default="../../../report/tables",
                    help="donde escribir las tablas .tex del informe")
    args = ap.parse_args()

    med_dir = os.path.join(args.data_root, "measurements")
    destino = os.path.join(args.data_root, "plots")

    filas = leer_csv(os.path.join(med_dir, "measurements_grid_pathfinding.csv"))
    if not filas:
        print("No hay mediciones. Corra primero:  make run")
        return

    print(f"Graficos de grid pathfinding en {destino}:")
    grafico_tiempo(filas, destino)
    for tipo in TIPOS:
        grafico_tiempo(filas, destino, tipo)
    grafico_normalizado(filas, destino)
    grafico_memoria(filas, destino)
    grafico_brecha(filas, destino)
    grafico_brecha_vs_tamano(filas, destino)
    grafico_brecha_por_paso(filas, destino)
    grafico_tasa_falla(filas, destino)
    generar_tablas_latex(filas, args.tables_dir)


if __name__ == "__main__":
    main()
