
mac:
	g++ Main.cpp GLSL_helper.cpp MStackHelp.cpp GeometryCreator.cpp -DGL_GLEXT_PROTOTYPES -framework OpenGL -framework GLUT -o Final

clean:
	rm -f *~
	rm -f Final
	rm -f *.o

