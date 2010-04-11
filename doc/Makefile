TEX2PDF = @pdflatex > /dev/null
ECHO = @echo
RM = -@rm -f

.PHONY: all clean doc doc.tex

all: doc

doc: doc.pdf
	$(RM) *.log *.aux *.out

%.pdf: %.tex
	$(ECHO) 'TEX2PDF  $<'
	$(TEX2PDF) $<

clean:
	$(RM) *.pdf *.log *.aux *.out *.toc
