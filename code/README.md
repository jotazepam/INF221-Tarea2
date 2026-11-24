# Documentación del código de la Tarea 2 INF221 (2026-2)

Juan Pablo Naylor, 202104614-3

Estudio experimental de tres paradigmas de diseño de algoritmos sobre un mismo
problema: camino de máximo puntaje en una grilla con murallas y minas.

## Entrega

La entrega se realiza vía **aula.usm.cl** en formato `.zip`.

## Estructura

```
code/
├── common/                           infraestructura compartida de medición
│   ├── medicion.hpp                     cronómetro, contador de memoria, CSV
│   └── medicion.cpp                     reemplazo global de operator new/delete
└── grid_pathfinding/
    ├── algorithms/
    │   ├── algoritmos.hpp               firmas, tipos de celda y helpers
    │   ├── brute_force.cpp              backtracking recursivo
    │   ├── dynamic_programming.cpp      tabulación con las vidas en el estado
    │   └── greedy.cpp                   la regla local del enunciado
    ├── scripts/
    │   ├── grid_generator.py            generador de grillas
    │   └── plot_generator.py            gráficos PNG y tablas .tex
    ├── data/
    │   ├── grid_input/                  grillas de entrada
    │   ├── grid_output/                 un archivo por instancia y algoritmo
    │   ├── measurements/                el CSV de mediciones
    │   └── plots/                       las 12 figuras
    ├── grid_pathfinding.cpp             programa principal de mediciones
    └── makefile
```

`common/medicion.{hpp,cpp}` y `algorithms/algoritmos.hpp` no están en la
plantilla original: son archivos auxiliares necesarios para compilar y para
medir tiempo y memoria. No contienen ninguno de los tres algoritmos, que están
cada uno en el archivo que el enunciado exige.

## Requisitos

| Herramienta | Versión usada | Nota |
|---|---|---|
| g++ | 16.2.0 (MSYS2 UCRT64) | requiere C++17 |
| make | 4.4.1 | |
| Python | 3.11 | con `numpy` y `matplotlib`; **no** usa `pandas` |

En Windows, `make` necesita `sh.exe` de MSYS2, y los directorios de MSYS2 van
**al final** del `PATH`, no al principio. Comprobado de las dos formas: al
final, `make data` genera las grillas; al principio, el `python` que alcanza la
receta deja de ser el que tiene los paquetes y falla con
`ModuleNotFoundError: No module named 'numpy'`.

```bash
export PATH="$PATH:/c/msys64/usr/bin:/c/msys64/ucrt64/bin"
```

Si aun así toma otro intérprete, se fuerza con
`make plots PYTHON="ruta/python.exe"`.

## Cómo reproducir los experimentos

Para comprobar que todo funciona, con las grillas hasta N = 16, en dos minutos:

```bash
cd grid_pathfinding && make all-small DATA_ROOT=prueba TABLES_DIR=prueba/tables
```

Los dos parámetros del final no son opcionales si ya existen mediciones
definitivas: sin ellos, el ensayo sobrescribe el CSV y las tablas `.tex` del
informe con los resultados del subconjunto chico.

Para reconstruir exactamente lo que se midió en el informe:

```bash
cd grid_pathfinding
make data-full        # 288 grillas, 421 MB, ~18 s
make run-full         # las 288 instancias, ~3 min
make plots            # 12 figuras PNG y 2 tablas .tex
```

Las grillas de N = 1024 concentran el 94 % de esos 421 MB, así que conviene
generarlas fuera de una carpeta sincronizada. `GRID_ROOT` mueve solo las
entradas; las salidas, las mediciones y los gráficos siguen quedando dentro del
proyecto, que es donde el enunciado los pide:

```bash
make data-full run-full GRID_ROOT=C:/Users/usuario/algoco_datos
```

El `.zip` entregado **no trae las grillas de N = 256 ni N = 1024**, que son el
94 % de los 421 MB, ni sus archivos de salida; sí trae las grillas menores, las
demás salidas, las mediciones completas, los gráficos, el código y el informe.

El generador usa semilla fija, de modo que lo que reconstruye coincide byte a
byte con lo que produjo estas mediciones. Se verificó de dos maneras:
regenerando el conjunto completo en otra ubicación y comparando sumas MD5, con
144 de 144 archivos idénticos, y después comprobando que las 408 mediciones ya
tomadas salieran sin un solo cambio en puntaje, camino ni vidas.

## Grid pathfinding

### Programa principal

`grid_pathfinding.cpp` recorre los casos
`{n} × {m} × {tipo} × {dominio} × {muestra}`, lee cada grilla una vez y fuera
del cronómetro, ejecuta los algoritmos sobre la misma copia por referencia
constante y registra tiempo, memoria auxiliar, puntaje y vidas restantes.

Escribe `data/measurements/measurements_grid_pathfinding.csv` con una fila por
medición y **sin promediar**, porque la mediana se calcula al graficar.

Escribe también `data/grid_output/{n}_{m}_{t}_{d}_{s}_{alg}_out.txt` con el
puntaje, el camino y las vidas restantes, o la única línea `SIN SOLUCION`. Son
**tres archivos por instancia, uno por algoritmo**, salvo en las instancias
donde la fuerza bruta no se ejecuta por la cota `N + M <= 32` que fija el
enunciado: esas tienen dos. No se escribe un archivo para un algoritmo que no
corrió, porque `SIN SOLUCION` significa que no existe camino válido y en esas
grillas sí existe.

Verifica que la fuerza bruta y la programación dinámica coincidan en todas las
instancias donde ambas se ejecutan. La comparación es más estricta de lo que
pide el enunciado: no solo el puntaje, también el camino y las vidas, porque
ambas resuelven los empates con el mismo criterio. Una discrepancia se imprime
y el programa termina con código distinto de cero.

Por último, aplica la cota `N + M <= 32` de la fuerza bruta. Ningún archivo de
algoritmo la impone; es responsabilidad del programa principal.

Opciones: `--data-root`, `--input-dir`, `--sizes`, `--factors`, `--types`,
`--domains`, `--samples`, `--algos`, `--reps`, `--max-bf`, `--no-output`.

### Algoritmos

| Archivo | Paradigma | Tiempo | Memoria aux. |
|---|---|---|---|
| `algorithms/brute_force.cpp` | backtracking recursivo | Θ(C(N+M, N)) | Θ(N+M) en la pila |
| `algorithms/dynamic_programming.cpp` | programación dinámica por tabulación | Θ(N·M·V) | Θ(N·M·V) |
| `algorithms/greedy.cpp` | heurística voraz | Θ(N+M) | Θ(N+M) por la salida |

La cota de la fuerza bruta **no** es la cantidad de caminos, que es
C(N+M-2, N-1). El recorrido visita un nodo por prefijo válido, y la suma de
prefijos sobre toda la grilla vale exactamente C(N+M, N) menos 1: unos 601
millones para una grilla de 16×16, casi cuatro veces la cantidad de caminos. Se
comprobó contando los nodos visitados sobre grillas libres de ocho tamaños
distintos.

Las tres implementaciones comparten firma y devuelven un `PathResult` con
puntaje, camino, vidas restantes y una bandera de éxito. Las decisiones de
diseño de cada una están documentadas en el encabezado de su archivo.

### Scripts

`scripts/grid_generator.py` es el del template, con una sola adición:
`--data-root`, para poder dejar los 421 MB fuera de la carpeta sincronizada. La
función interna ya aceptaba la carpeta como parámetro.

`scripts/plot_generator.py` lee el CSV y produce los PNG de `data/plots/` y
también las tablas `.tex` de `report/tables/`. No usa `pandas`, solo `csv`,
`collections.defaultdict` y `numpy`.

## Notas de implementación que no son obvias

1. **Las vidas son parte del estado de la programación dinámica.** Dos caminos
   que llegan a la misma celda no son comparables solo por puntaje: el que trae
   más blindaje puede cruzar minas que el otro no, y el que trae más puntaje
   puede quedar encerrado más adelante. Sin la tercera coordenada el algoritmo
   responde mal en cuanto hay una mina en juego.
2. **Los empates se resuelven igual en los dos algoritmos exactos**, a favor
   del este. Cuando se implementaron, el criterio elegido fue reportar el
   camino de mayor puntaje con un desempate determinista, sin saber todavía que
   el foro aclararía después que, habiendo varios caminos óptimos, da lo mismo
   cuál se entregue. Se mantuvo igual porque no cuesta nada y permite comparar
   las tres salidas carácter por carácter en vez de solo el puntaje, que es una
   verificación bastante más fuerte.
3. **La memoria de la fuerza bruta aparece en cero y está bien.** El contador
   instrumenta el heap; el backtracking gasta su memoria en la pila de
   recursión, que son a lo más 32 marcos en las instancias permitidas.
4. **`-static` es obligatorio en Windows.** El reemplazo global de
   `operator new` no cruza los límites de una DLL, y con enlace dinámico se
   corrompe el heap al arrancar.
5. **`-fno-builtin-operator-new` y compañía.** Sin estos flags GCC puede
   eliminar pares new/delete y la memoria reportada saldría menor que la real.
6. **Los nombres de algoritmo no llevan comas**, porque van sin escapar al CSV.

## Formato de las mediciones

```
measurements_grid_pathfinding.csv:
algoritmo,n,m,tipo,dominio,muestra,rep,tiempo_ms,memoria_bytes,puntaje,vidas,estado
```

Se guarda **una fila por repetición**, sin promediar; los gráficos calculan la
**mediana**, no la media, porque el ruido de una medición de tiempo es
asimétrico: ninguna interferencia hace que el programa corra más rápido de lo
que puede.

La columna `estado` vale `OK` o `SIN SOLUCION`. En el segundo caso las columnas
`puntaje` y `vidas` quedan vacías, que es lo que el generador de gráficos usa
para calcular la tasa de falla del greedy. Las instancias donde la fuerza bruta
no corre simplemente no aportan filas de ese algoritmo.

En la entrega hay 2088 mediciones sobre 288 instancias y 696 archivos de
salida. Las 120 instancias en que se ejecutan los dos algoritmos exactos
coincidieron en puntaje, camino y vidas, sin una sola discrepancia. El greedy
no alcanzó la meta en 38 de las 288, todas ellas en grillas con murallas o
mixtas.

## Fuentes

Cada archivo fuente lleva en su encabezado sus decisiones de diseño y las
fuentes que le corresponden, como pide el enunciado.

No se usó código de terceros: las cuatro implementaciones y la infraestructura
de medición son propias.
