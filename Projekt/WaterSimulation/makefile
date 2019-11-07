all :  main

main : main.c ../common/GL_utilities.c ../common/VectorUtils3.c ../common/LoadTGA.c ../common/loadobj.c ../common/zpr.c 
	gcc -Wall -o main -DGL_GLEXT_PROTOTYPES main.c ../common/GL_utilities.c ../common/VectorUtils3.c ../common/LoadTGA.c ../common/loadobj.c ../common/zpr.c  -I../common -I -lXt -lX11 -lm -lGL -lglut -lGLU 

clean :
	rm main

