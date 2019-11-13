all :  master

master : master.c ../common/GL_utilities.c ../common/VectorUtils3.c ../common/LoadTGA.c ../common/loadobj.c ../common/zpr.c ../common/TriangulationTable.c
	gcc -Wall -o master -DGL_GLEXT_PROTOTYPES master.c ../common/GL_utilities.c ../common/VectorUtils3.c ../common/LoadTGA.c ../common/loadobj.c ../common/zpr.c ../common/TriangulationTable.c -I../common -I -lXt -lX11 -lm -lGL -lglut -lGLU 

clean :
	rm master

