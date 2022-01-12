todo: spinningcube_withlight_SKEL

LDLIBS=-lGL -lGLEW -lglfw -lm 

spinningcube_withlight: spinningcube_withlight_SKEL.cpp
	gcc $^ -lGL -lGLEW -lglfw -lm -o $@

clean:
	rm -f *.o *~

cleanall: clean
	rm -f spinningcube_withlight_SKEL
