// Includes vary a bit with platforms.
// MS Windows needs GLEW or glee.
// For Mac, I used MicroGlut and Lightweight IDE.
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	//#include "MicroGlut.h"
	// uses framework Cocoa
#else
	#include <GL/glut.h>//#include "MicroGlut.h" // 
	#include <GL/gl.h>
#endif

#include <sys/time.h>
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#include "LoadTGA.h"
#include "zpr.h"
#include "TriangulationTable.h"

// initial width and heights
#define W 600
#define H 600

#define NEAR 1.0
#define FAR 100.0

#define NUM_LIGHTS 1
#define kParticleSize 0.1
#define ELASTICITY 0.0
#define BOXSIZE 0.5
#define RENDERBOXSIZE 1.0
#define RENDERBALLS false

#define abs(x) (x > 0.0? x: -x)

void onTimer(int value);

static double startTime = 0;

void resetElapsedTime()
{
  struct timeval timeVal;
  gettimeofday(&timeVal, 0);
  startTime = (double) timeVal.tv_sec + (double) timeVal.tv_usec * 0.000001;
}

float getElapsedTime()
{
  struct timeval timeVal;
  gettimeofday(&timeVal, 0);
  double currentTime = (double) timeVal.tv_sec
    + (double) timeVal.tv_usec * 0.000001;

  return currentTime - startTime;
}
enum {GridPointsPerDim = 64}; // Number of actual point GridPointsPerDim^3
typedef struct{
    vec3 points[3];
}TRIANGLE;
typedef struct{
    vec3 position;
    bool value;
}GRIDPOINT;
typedef struct{
    GRIDPOINT points[GridPointsPerDim][GridPointsPerDim][GridPointsPerDim];
}GRID;
typedef struct
{
  GLfloat mass;
  vec3 X, P; // position, linear momentum, angular momentum
  vec3 F;
  vec3 v; // Change in velocity
} Particle;

typedef struct
{
    GLfloat diffColor[4], specColor[4],
    ka, kd, ks, shininess;  // coefficients and specular exponent
} Material;

Material particleMt = { { 0.4, 0.4, 0.9, 1.0 }, { 1.0, 1.0, 1.0, 0.0 },
                    0.1, 0.6, 1.0, 50
                },
        ControlableParticleMt= { { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 },
                    0.1, 0.6, 1.0, 0.0
                };


enum {kNumParticles = 200}; // Change as desired

//------------------------------Globals---------------------------------
Model *sphere;
Particle particles[kNumParticles]; // We only use kNumParticles but textures for all 16 are always loaded so they must exist. So don't change here, change above.
Particle ControlableParticle;

GRID waterGrid;
int tempVertices[16];
GLfloat deltaT, currentTime;
GLfloat* Vertex_Array_Buffer_Water;
GLuint* Index_Array_Buffer_Water;
int vertices;
Model* Water;

vec3 cam, point;

GLuint shader = 0;
GLint lastw = W, lasth = H;  // for resizing
//-----------------------------matrices------------------------------
mat4 projectionMatrix,
        viewMatrix, transMatrix, tmpMatrix;

//------------------------- lighting--------------------------------
vec3 lightSourcesColorArr[] = { {1.0f, 1.0f, 1.0f} }; // White light
GLfloat specularExponent[] = {50.0};
GLint directional[] = {0};
vec3 lightSourcesDirectionsPositions[] = { {0.0, 10.0, 0.0} };


//----------------------------------Utility functions-----------------------------------
void resetBuffers(){
    Vertex_Array_Buffer_Water = (GLfloat*) malloc(2*12*pow(GridPointsPerDim,3)*sizeof(GLfloat));
    Index_Array_Buffer_Water = (GLuint*) malloc(2*4*pow(GridPointsPerDim,3)*sizeof(GLuint));
    vertices = 0;
}
void loadMaterial(Material mt)
{
    glUniform4fv(glGetUniformLocation(shader, "diffColor"), 1, &mt.diffColor[0]);
    glUniform1fv(glGetUniformLocation(shader, "shininess"), 1, &mt.shininess);
}

//---------------------------------- physics update and billiard table rendering ----------------------------------
void updateWorld()
{
	// Zero forces
	int i, j;
	for (i = 0; i < kNumParticles; i++)
	{
		particles[i].F = SetVector(0,-9.82*particles[i].mass,0);
	}
	// Wall tests
    for (i = 0; i < kNumParticles; i++)
	{
		if (particles[i].X.x < -BOXSIZE + kParticleSize)
			particles[i].P.x = abs(particles[i].P.x);
		if (particles[i].X.x > BOXSIZE - kParticleSize)
			particles[i].P.x = -abs(particles[i].P.x);
        if (particles[i].X.y < 0 + kParticleSize)
            particles[i].P.y = abs(particles[i].P.y);
		if (particles[i].X.z < -BOXSIZE + kParticleSize)
			particles[i].P.z = abs(particles[i].P.z);
		if (particles[i].X.z > BOXSIZE - kParticleSize)
			particles[i].P.z = -abs(particles[i].P.z);
	}

	// Detect collisions, calculate speed differences, apply forces
	for (i = 0; i < kNumParticles; i++)
    {
        for (j = i+1; j < kNumParticles; j++)
        {            
            if(abs(Norm(VectorSub(particles[i].X, particles[j].X))) <= (2*kParticleSize)){
		        particles[i].v = ScalarMult(particles[i].P, 1.0/(particles[i].mass));
		        particles[j].v = ScalarMult(particles[j].P, 1.0/(particles[j].mass));
                if(DotProduct(VectorSub(particles[i].X, particles[j].X), VectorSub(particles[i].v, particles[j].v)) < 0.0){
                    vec3 SpeedDifference = VectorSub(particles[i].v , particles[j].v);
                    vec3 CollisionNormal = Normalize(VectorSub(particles[i].X, particles[j].X));
                    float J = ((DotProduct(SpeedDifference, CollisionNormal) * (ELASTICITY + 1))/ (1/particles[i].mass + 1/particles[j].mass))*-1;
                    particles[i].F = VectorAdd(particles[i].F, ScalarMult(ScalarMult(CollisionNormal,J),1/deltaT));
                    particles[j].F = VectorAdd(particles[j].F, ScalarMult(ScalarMult(ScalarMult(CollisionNormal,-1),J),1/deltaT));
                }
            }
                
        }

    }
    // Update state, follows the book closely
	for (i = 0; i < kNumParticles; i++)
	{
		vec3 dX, dP;
        //v := P * 1/mass
		particles[i].v = ScalarMult(particles[i].P, 1.0/(particles[i].mass));
        //X := X + v*dT
		dX = ScalarMult(particles[i].v, deltaT); // dX := v*dT
		particles[i].X = VectorAdd(particles[i].X, dX); // X := X + dX
        //P := P + F * dT
		dP = ScalarMult(particles[i].F, deltaT); // dP := F*dT
		particles[i].P = VectorAdd(particles[i].P, dP); // P := P + dP
	}
}
void resetGrid(){
    for(int w = 0; w < GridPointsPerDim; w++ ){
        for(int h = 0; h < GridPointsPerDim; h++ ){
            for(int d = 0; d < GridPointsPerDim; d++ ){
                waterGrid.points[w][h][d].value = false;
            }
        }
    }
}
void evaluateGrid(){
    int Nw;
    int Nh;
    int Nd;
    float pointSpace = 2*RENDERBOXSIZE/(GridPointsPerDim-1);
    int subSet = ceil(kParticleSize/pointSpace)+1;
    resetGrid();
    for(int i = 0; i < kNumParticles; i++){
        Nw = floor((particles[i].X.x + RENDERBOXSIZE)/pointSpace);
        Nh = floor((particles[i].X.y + RENDERBOXSIZE)/pointSpace);
        Nd = floor((particles[i].X.z + RENDERBOXSIZE)/pointSpace);
        if(Nw >= 0 && Nw < GridPointsPerDim && Nh >= 0 && Nh < GridPointsPerDim && Nd >= 0 && Nd < GridPointsPerDim){
            for(int w = Nw-subSet; w <= Nw+subSet; w++){
                for(int h = Nh-subSet; h <= Nh+subSet; h++){
                    for(int d = Nd-subSet; d <= Nd+subSet; d++){
                        if(w < GridPointsPerDim &&  w > 0  && h < GridPointsPerDim && h > 0 &&  d < GridPointsPerDim &&  d > 0){
                            if(abs(Norm(VectorSub(waterGrid.points[w][h][d].position , particles[i].X))) <= (2*kParticleSize)){
                                waterGrid.points[w][h][d].value = true;
                            }
                        }

                    }
                }
            }
        }
    }
}
void renderParticle(vec3 position, Material m)
{
    transMatrix = T(position.x, position.y, position.z); // position
    tmpMatrix = Mult(S(kParticleSize/0.1, kParticleSize/0.1, kParticleSize/0.1), transMatrix);
    tmpMatrix = Mult(viewMatrix, tmpMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
    loadMaterial(m);
    DrawModel(sphere, shader, "in_Position", "in_Normal", NULL);
}
//-------------------------------------------------------------------------------------
void createGrid(){
    float pointSpace = 2*RENDERBOXSIZE/(GridPointsPerDim-1);
    for(int w = 0; w < GridPointsPerDim; w++ ){
        for(int h = 0; h < GridPointsPerDim; h++ ){
            for(int d = 0; d < GridPointsPerDim; d++ ){
                waterGrid.points[w][h][d].position = SetVector(-RENDERBOXSIZE + w*pointSpace , -RENDERBOXSIZE + h*pointSpace, -RENDERBOXSIZE + d*pointSpace);
                waterGrid.points[w][h][d].value = false;
            }
        }
    }
}
void march(){
    int cubeindex;
    resetBuffers();
    vec3 Temp[8];
    vec3 t;
    vec3 t1;
    vec3 t2;
    for(int w = 0; w < GridPointsPerDim-1; w++ ){
        for(int h = 0; h < GridPointsPerDim-1; h++ ){
            for(int d = 0; d < GridPointsPerDim-1; d++ ){

                GRIDPOINT P0 = waterGrid.points[w][h][d];
                GRIDPOINT P1 = waterGrid.points[w+1][h][d];
                GRIDPOINT P2 = waterGrid.points[w+1][h][d+1];
                GRIDPOINT P3 = waterGrid.points[w][h][d+1];
                GRIDPOINT P4 = waterGrid.points[w][h+1][d];
                GRIDPOINT P5 = waterGrid.points[w+1][h+1][d];
                GRIDPOINT P6 = waterGrid.points[w+1][h+1][d+1];
                GRIDPOINT P7 = waterGrid.points[w][h+1][d+1];
                
                Temp[0] = P0.position;
                Temp[1] = P1.position;
                Temp[2] = P2.position;
                Temp[3] = P3.position;
                Temp[4] = P4.position;
                Temp[5] = P5.position;
                Temp[6] = P6.position;
                Temp[7] = P7.position;
                cubeindex = 0;
                if(P0.value) cubeindex += 1;
                if(P1.value) cubeindex += 2;
                if(P2.value) cubeindex += 4;
                if(P3.value) cubeindex += 8;
                if(P4.value) cubeindex += 16;
                if(P5.value) cubeindex += 32;
                if(P6.value) cubeindex += 64;
                if(P7.value) cubeindex += 128;

                
                //printf(" %u \n", cubeindex);
                for(int i = 0; triTable[cubeindex][i] != -1; i+= 3){
                    /* THIS IS CAUSING THE ACTUAL ISSUE*/
                     //printf(" %u \n", 1);
                    t = getVertFromEdge(Temp, triTable[cubeindex][i]);
                    t1 = getVertFromEdge(Temp, triTable[cubeindex][i+1]);
                    t2 = getVertFromEdge(Temp, triTable[cubeindex][i+2]);
                    if(t.x == 0.0){
                         printf(" %u ZERO \n", vertices);
                    }
                    Vertex_Array_Buffer_Water[vertices*3+i] = t.x;
                    Vertex_Array_Buffer_Water[vertices*3+i+1] = t.y;
                    Vertex_Array_Buffer_Water[vertices*3+i+2] = t.z;

                    Vertex_Array_Buffer_Water[(vertices+1)*3+i] = t1.x;
                    Vertex_Array_Buffer_Water[(vertices+1)*3+i+1] = t1.y;
                    Vertex_Array_Buffer_Water[(vertices+1)*3+i+2] = t1.z;

                    Vertex_Array_Buffer_Water[(vertices+2)*3+i] = t2.x;
                    Vertex_Array_Buffer_Water[(vertices+2)*3+i+1] = t2.y;
                    Vertex_Array_Buffer_Water[(vertices+2)*3+i+2] = t2.z;
                    
                    Index_Array_Buffer_Water[vertices]= vertices;
                    Index_Array_Buffer_Water[vertices+1]= vertices+1;
                    Index_Array_Buffer_Water[vertices+2]= vertices+2;
                    
                    vertices+=3;
                }
            }
        }
    }
}

void init()
{
	dumpInfo();
	// GL inits
    resetBuffers();
	glClearColor(0.4, 0.4, 0.4, 0);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");
    createGrid();
    // Load shader
    shader = loadShaders("shaders/lab3.vert", "shaders/lab3.frag");
    printError("init shader");
    sphere = LoadModelPlus("sphere.obj");
    
    projectionMatrix = perspective(90, 1.0, 0.1, 1000); // It would be silly to upload an uninitialized matrix
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

    // Initialize ball data, positions etc
    float ParticleMargin = 0.01;
    int ParticlesPerRow = floor(BOXSIZE/(2*kParticleSize + ParticleMargin));
	for (int i = 0; i < kNumParticles; i++)
	{
		particles[i].mass = 1.0;
		particles[i].X = SetVector(-BOXSIZE+ (kParticleSize*2 + ParticleMargin) *(i%ParticlesPerRow),1 +(kParticleSize*2 + ParticleMargin) * (i/(ParticlesPerRow*ParticlesPerRow)), -BOXSIZE + (kParticleSize*2 + ParticleMargin) *((i%(ParticlesPerRow*ParticlesPerRow))/ParticlesPerRow));
		particles[i].P = SetVector(((float)(i % 13))/ 50.0, 0.0, ((float)(i % 15))/50.0);
	}


    ControlableParticle.X = SetVector(0,2.0,0.0);

    ControlableParticle.mass = 1.0;
    cam = SetVector(0, 4, 4);
    point = SetVector(0, 0, 0);
    zprInit(&viewMatrix, cam, point, &ControlableParticle.X );  // camera controls

    resetElapsedTime();
}


//---------------------------callback functions------------------------------------------
void display(void)
{
    updateWorld();

    // Clear framebuffer & zbuffer
	glClearColor(0.3, 0.3, 0.3, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);
    evaluateGrid();// Causes segmentation fault, probably a array index out of bounds error.
    march();
    printError("uploading to shader");


    Water = LoadDataToModel(Vertex_Array_Buffer_Water,NULL,NULL,NULL,Index_Array_Buffer_Water,vertices,vertices);
    transMatrix = T(0.0, 0.0, 0.0); // position
    tmpMatrix = Mult(viewMatrix, transMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
    loadMaterial(ControlableParticleMt);
    DrawModel(Water, shader, "in_Position", "in_Normal", NULL);

    renderParticle(ControlableParticle.X, ControlableParticleMt);
    if(RENDERBALLS){
        for (int i = 0; i < kNumParticles; i++)
            renderParticle(particles[i].X, particleMt);
    }
	

    printError("rendering");

	glutSwapBuffers();

}

void onTimer(int value)
{
    glutPostRedisplay();
    deltaT = getElapsedTime() - currentTime;
    currentTime = getElapsedTime();
    glutTimerFunc(20, &onTimer, value);
}

void reshape(GLsizei w, GLsizei h)
{
	lastw = w;
	lasth = h;

    glViewport(0, 0, w, h);
    GLfloat ratio = (GLfloat) w / (GLfloat) h;
    projectionMatrix = perspective(90, ratio, 0.1, 1000);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
}

//-----------------------------main-----------------------------------------------
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(W, H);
	glutInitContextVersion(3, 2);
	glutCreateWindow ("Water Simulator 2000 Xtreme Edition Ulitmate 2020 limited");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
    glutTimerFunc(20, &onTimer, 0);

	init();

	glutMainLoop();
	exit(0);
}