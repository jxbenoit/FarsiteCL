CC=g++
CFLAGS=-c -Wall -O3 -DOS_X
SOURCES=main.cpp PerimeterPoint.cpp Point.cpp burnupw.cpp crossthread.cpp dbfopen.cpp Farsite.cpp fcopy.cpp fsairatk.cpp fsglbvar.cpp fsxpfront.cpp fsxw.cpp  fsxwatm.cpp fsxwattk.cpp fsxwbar.cpp fsxwburn4.cpp fsxwcrwn.cpp fsxwenvt2.cpp fsxwfms2.cpp fsxwfotp.cpp fsxwignt.cpp fsxwinpt.cpp fsxwmech.cpp fsxwrast.cpp fsxwshap.cpp fsxwspot.cpp fsxwutil.cpp fsxwvect.cpp globals.cpp gridthem.cpp newclip.cpp newfms.cpp portablestrings.cpp rand2.cpp shpopen.cpp LCPAnalyzer.cpp Perimeter.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=farsite.x

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
