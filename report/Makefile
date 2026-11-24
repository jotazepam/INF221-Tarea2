# =============================================================================
#  makefile  --  Compilacion del mini-informe
#  Tarea 2 - INF221 Algoritmos y Complejidad - 2026-2
# =============================================================================
#
#     make          compila report.pdf
#     make clean    borra los archivos intermedios (no el PDF)
#     make overleaf arma informe_overleaf.zip para subir a Overleaf
#
#  POR QUE TRES PASADAS DE pdflatex
#  --------------------------------
#  LaTeX es de una sola pasada: cuando encuentra \ref{fig:algo} todavia no sabe
#  que numero le tocara a esa figura, asi que escribe un "??" y anota la
#  pregunta en el archivo .aux. Por eso:
#
#    1a pasada  ->  recoge las etiquetas y citas en .aux/.bcf, imprime "??"
#    biber      ->  lee .bcf, resuelve la bibliografia contra references.bib
#    2a pasada  ->  ya conoce los numeros y la bibliografia, pero insertarla
#                   puede correr las paginas y desajustar las referencias
#    3a pasada  ->  estabiliza la numeracion definitiva
#
#  Si el PDF sale con "??" o "[?]" donde deberian ir numeros, falto una pasada.
#
#  NOTA: se usa biber (no bibtex) porque el preambulo carga biblatex.
# =============================================================================

LATEX  := pdflatex -interaction=nonstopmode -halt-on-error
BIBER  := biber
MAIN   := report

.PHONY: all clean

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex preamble.tex author.tex references.bib \
             $(wildcard sections/*.tex) $(wildcard tables/*.tex)
	$(LATEX) $(MAIN).tex
	-$(BIBER) $(MAIN)
	@test -s $(MAIN).bbl || { \
	  echo ""; \
	  echo "AVISO: biber no genero $(MAIN).bbl y las citas quedaran sin"; \
	  echo "resolver. En MiKTeX sobre Windows biber a veces falla cuando lo"; \
	  echo "lanza make pero funciona desde la consola. Corra a mano:"; \
	  echo "    biber $(MAIN) && make"; \
	  echo ""; }
	$(LATEX) $(MAIN).tex
	$(LATEX) $(MAIN).tex
	@grep -q 'Citation.*undefined' $(MAIN).log \
	  && echo "AVISO: quedaron citas sin resolver, revise el paso de biber." \
	  || echo "Listo: $(MAIN).pdf"

clean:
	-rm -f $(MAIN).aux $(MAIN).log $(MAIN).out $(MAIN).toc \
	       $(MAIN).bbl $(MAIN).bcf $(MAIN).blg $(MAIN).run.xml \
	       sections/*.aux
