
all: pdf

pdf:
	pdflatex proposal.tex
	bibtex proposal
	pdflatex proposal.tex

clean:
	rm -rf *.aux *.log *.dvi *.blg