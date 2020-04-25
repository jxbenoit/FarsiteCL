CC=g++
CFLAGS=-c -Wall -O3 -DOS_X
SOURCES=batch.cpp PerimeterPoint.cpp PerimeterPoint.h Point.cpp Point.h burnupw.cpp burnupw.h crossthread.cpp dbfopen.cpp farsite4.cpp farsite4.h fcopy.cpp fcopy.h fsairatk.cpp fsairatk.h fsglbvar.cpp fsglbvar.h fsx4.h fsxlandt.h fsxpfront.cpp fsxpfront.h fsxw.cpp fsxw.h fsxwatm.cpp fsxwatm.h fsxwattk.cpp fsxwattk.h fsxwbar.cpp fsxwbar.h fsxwburn4.cpp fsxwcrwn.cpp fsxwenvt2.cpp fsxwfms2.cpp fsxwfotp.cpp fsxwignt.cpp fsxwignt.h fsxwinpt.cpp fsxwinpt.h fsxwmech.cpp fsxwrast.cpp fsxwshap.cpp fsxwshap.h fsxwspot.cpp fsxwutil.cpp fsxwvect.cpp globals.cpp globals.h gridthem.cpp newclip.cpp newfms.cpp newfms.h portablestrings.cpp portablestrings.h rand2.cpp rand2.h shapefil.h shpopen.cpp themes.h LCPAnalyzer.h LCPAnalyzer.cpp Perimeter.cpp Perimeter.h
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=farsite.x

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
