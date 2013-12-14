
all: pdf

pdf:
	pdflatex quiltdb.tex
	bibtex quiltdb
	pdflatex quiltdb.tex

clean:
	rm -rf *.aux *.log *.dvi *.blg