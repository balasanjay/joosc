LATEX_BASE_NAME = a4
LC = pdflatex

LATEX_SRC = ${LATEX_BASE_NAME}.tex
LATEX_OUTDIR = .latex_build/${LATEX_BASE_NAME}_out
LATEX_TARG = ${LATEX_BASE_NAME}.pdf

all: ${LATEX_TARG}

clean:
	rm -rf ${LATEX_TARG} ${LATEX_OUTDIR}

${LATEX_TARG}: ${LATEX_SRC}
	# ensure we don't use the partially completed build of a previous
	# compile
	rm -rf ${LATEX_OUTDIR};
	mkdir -p ${LATEX_OUTDIR};
	${LC} -output-directory ${LATEX_OUTDIR} ${LATEX_SRC};
	${LC} -output-directory ${LATEX_OUTDIR} ${LATEX_SRC};
	${LC} -output-directory ${LATEX_OUTDIR} ${LATEX_SRC};
	cp ${LATEX_OUTDIR}/${LATEX_TARG} ${LATEX_TARG};
	rm -r ${LATEX_OUTDIR};
