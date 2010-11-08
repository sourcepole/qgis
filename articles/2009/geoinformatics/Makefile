#targets:
#  make           - builds the text
#  make clean     - removed Latex' auxiliary files
#  make ps        - create PS file
#  make pdf       - create PDF file
#  make changelog - creates ChangeLog

FILE=qgis-geoinformatics

all: 
	make pics
	latex $(FILE)
	echo Rerun > $(FILE).log
	while grep Rerun $(FILE).log >/dev/null 2>&1 ; do latex $(FILE).tex ; done

#make PostScript:
ps:
	make all
	dvips $(FILE).dvi -o $(FILE).ps

#make PDF:
pdf:
	make all
	dvipdf $(FILE).dvi

# make pics
pics:
	cd ./images && make

# make clean 
clean:
	rm -f *.log *.aux $(FILE).dvi *.bbl *.blg *.ind *.ilg *.toc *.tof *.lof *.lot *.pdf *.ps *.idx *.brf *.out
	rm -rf ./finalpix
	rm -f ./images/*.eps

# cvs2cl.pl creates a GNU style ChangeLog file:
# http://www.red-bean.com/~kfogel/cvs2cl.shtml
changelog:
	cvs2cl.pl
	@echo "Written ChangeLog"
