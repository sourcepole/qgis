# makefile to convert figures into eps format for the book

# Comments from Michel Wurtz:
# You will notice that the list given is the list of targets.  "all" just
# refer to this list.  The most important rule is the next rule, that give
# the generic rule for transforming a .fig into a .eps file.  Note the use
# of % (define the couple target:source) and the use of
# $< (source postfix, here .fig) and $@ (target postfix, here .eps)

# with .eps extension:
#EPS=datenbank_vect.eps

PNG=community-map.eps\
    python-plugin-installer.eps\
    grass-plugin.eps\
    qgis10.eps\
    spit.eps\
   
##### TARGETS #####

all:  $(PNG)
	if [ ! -d ../finalpix/ ]; then mkdir ../finalpix; fi
	#bitte nicht mv machen:
	cp *.eps ../finalpix/

$(EPS):	%.eps: %.fig
	fig2dev -L eps $< > $@

$(FIGPNG): %.png: %.fig
	fig2dev -L png -m 2 $< > $@

$(PNG): %.eps: %.png
	# Workaround for PDF
	# convert -geometry 800x600 -depth 1 $< $@
	convert $< $@ 
clean:
	rm -f *.bak *~ *.pdf *.eps $(PNG) $(EPS)
