#ifndef TRIANGULATION_TABLE_H
#define TRIANGULATION_TABLE_H
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#else
	#if defined(_WIN32)
		#include "glew.h"
	#endif
	#include <GL/gl.h>
#endif

#include "VectorUtils3.h"
#include <stdio.h>
extern int triTable[256][16];
vec3 getVertFromEdge(vec3 vert[], int edge);
vec3 VertexInterp(vec3 p1,vec3 p2);
void getVertList(vec3 vertlist[12], vec3 vert[], int cubeindex);
int getVertIndex(int w, int h, int d, int edge, int dims);

#endif