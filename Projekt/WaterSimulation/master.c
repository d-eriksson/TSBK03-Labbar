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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// initial width and heights
#define W 600
#define H 600

#define NEAR 1.0
#define FAR 100.0

#define NUM_LIGHTS 1
#define kParticleSize 0.05
#define controlParticleSize 0.5
#define ELASTICITY 1
#define BOXSIZE 0.7
#define RENDERBOXSIZE 1.0
#define RENDERBALLS false
#define LOG false

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
enum {GridPointsPerDim = 16}; // Number of actual point GridPointsPerDim^3
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
  vec3 X; // position, 
  vec3 PX; // previous position
  vec3 a; // Acceleration
} Particle;

typedef struct
{
    GLfloat diffColor[4], specColor[4],
    ka, kd, ks, shininess;  // coefficients and specular exponent
} Material;

Material particleMt = { { 0.3, 0.5, 0.9, 1.0 }, { 1.0, 1.0, 1.0, 0.0 },
                    0.1, 0.6, 1.0, 50
                },
        ControlableParticleMt= { { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 },
                    0.1, 0.6, 1.0, 0.0
                };


enum {kNumParticles = 100}; // Change as desired

//------------------------------Globals---------------------------------
Model *sphere;
Particle particles[kNumParticles]; // We only use kNumParticles but textures for all 16 are always loaded so they must exist. So don't change here, change above.
Particle ControlableParticle;

GRID waterGrid;
int tempVertices[16];
GLfloat deltaT, currentTime;
GLfloat* Vertex_Array;
GLfloat* Vertex_Array_Buffer_Water;
GLfloat* Normal_Array;
GLfloat* Normal_Array_Buffer_Water;
GLuint* Index_Array_Buffer_Water;
GLuint* Index_Array;
int vertices;
Model* Water;
Model* skybox;

vec3 cam, point;

GLuint shader = 0, skyboxShader = 0. waterShader = 0;
GLint lastw = W, lasth = H;  // for resizing
//-----------------------------matrices------------------------------
mat4 projectionMatrix,
        viewMatrix, transMatrix, tmpMatrix;

//------------------------- lighting--------------------------------
vec3 lightSourcesColorArr[] = { {1.0f, 1.0f, 1.0f} }; // White light
GLfloat specularExponent[] = {50.0};
GLint directional[] = {0};
vec3 lightSourcesDirectionsPositions[] = { {0.0, 10.0, 0.0} };

//-----------------------------------skybox-------------
float skyboxVert[] = {
	-2.0f, -2.0f, -2.0f,
	2.0f, -2.0f, -2.0f,
	2.0f, -2.0f, 2.0f,
	-2.0f, -2.0f, 2.0f,
	-2.0f, 2.0f, -2.0f,
	2.0f, 2.0f, -2.0f,
	2.0f, 2.0f, 2.0f,
	-2.0f, 2.0f, 2.0f

};

unsigned int skyboxIndices[] = {
	0,1,5,
	0,5,4,
	0,4,7,
    0,7,3,
	0,3,2,
	0,2,1,
    6,4,5,
	6,7,4,
	6,2,3,
	6,3,7,
	6,5,1,
	6,1,2,
	
};

unsigned int cubemapTexture;

unsigned int loadCubemap(char faces[6][15])
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; i++)
    {
        unsigned char *data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}  



//----------------------------------Utility functions-----------------------------------
void resetBuffers(){
    Index_Array_Buffer_Water = (GLuint*) malloc(2*4*pow(GridPointsPerDim,3)*sizeof(GLuint));
    Normal_Array_Buffer_Water = (GLfloat*) malloc(2*12*pow(GridPointsPerDim,3)*sizeof(GLfloat));
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
	int i, j;

    // Add gravity
	for (i = 0; i < kNumParticles; i++)
	{
        // accelerate according to gravity
        particles[i].X = VectorAdd(particles[i].X, ScalarMult(SetVector(0,-3.52,0), deltaT * deltaT));

	}

    // Collision test
    for (i = 0; i < kNumParticles; i++)
    {
        for (j = i+1; j < kNumParticles; j++)
        {    
            vec3 colNormal = VectorSub(particles[i].X, particles[j].X);
            float slength = DotProduct(colNormal,colNormal);
            float length = sqrt(slength);
            float target = 2 * kParticleSize;

            if(length < target){
                

                // resolve overlapping conflict
                float factor = (length - target)/length;
                particles[i].X = VectorSub(particles[i].X,ScalarMult(colNormal,factor * 0.5));
                particles[j].X = VectorAdd(particles[j].X,ScalarMult(colNormal,factor * 0.5));

            }      
        }
        vec3 colNormal = VectorSub(particles[i].X, ControlableParticle.X);
            float slength = DotProduct(colNormal,colNormal);
            float length = sqrt(slength);
            float target = kParticleSize + controlParticleSize ;

            if(length < target){
                // resolve overlapping conflict
                float factor = (length - target)/length;
                particles[i].X = VectorSub(particles[i].X,ScalarMult(colNormal,factor));
            } 

    }

	// Wall tests + inertia
    for (i = 0; i < kNumParticles; i++)
	{
		if (particles[i].X.x < -BOXSIZE + kParticleSize) {
            particles[i].X.x = -BOXSIZE + kParticleSize;
            particles[i].PX.x = particles[i].X.x ;
        }

		if (particles[i].X.x > BOXSIZE - kParticleSize){
            particles[i].X.x = BOXSIZE - kParticleSize;
            particles[i].PX.x = particles[i].X.x ;
        }

        if (particles[i].X.y < 0.0 + kParticleSize){
            particles[i].X.y = kParticleSize;
            particles[i].PX.y = particles[i].X.y ;
        }

		if (particles[i].X.z < -BOXSIZE + kParticleSize){
            particles[i].X.z = -BOXSIZE + kParticleSize;
            particles[i].PX.z = particles[i].X.z ;
        }
			
		if (particles[i].X.z > BOXSIZE - kParticleSize){
            particles[i].X.z = BOXSIZE - kParticleSize;
            particles[i].PX.z = particles[i].X.z ;
        }



       // Inertia
        vec3 dX = VectorSub(ScalarMult(particles[i].X,2),particles[i].PX);
        particles[i].PX = particles[i].X;
        particles[i].X = dX;
	}

	// Collision test keep energy
    for (i = 0; i < kNumParticles; i++)
    {
        for (j = i+1; j < kNumParticles; j++)
        {    
            vec3 colNormal = VectorSub(particles[i].X, particles[j].X);
            float slength = DotProduct(colNormal,colNormal);
            float length = sqrt(slength);
            float target = 2 * kParticleSize;

            if(length < target){
                // prevois velocity
                vec3 vi = VectorSub(particles[i].X, particles[i].PX);
                vec3 vj = VectorSub(particles[j].X, particles[j].PX);

                // resolve overlapping conflict
                float factor = (length - target)/length;
                particles[i].X = VectorSub(particles[i].X,ScalarMult(colNormal,factor * 0.5));
                particles[j].X = VectorAdd(particles[j].X,ScalarMult(colNormal,factor * 0.5));

               // Compute the projected component factors
                float fi = ELASTICITY * DotProduct(colNormal,vi) / slength ;
                float fj = ELASTICITY * DotProduct(colNormal,vj) / slength ;

                // Swap the projected components
                vi = VectorAdd(vi,VectorSub(ScalarMult(colNormal,fj),ScalarMult(colNormal,fi)));
                vj = VectorAdd(vj,VectorSub(ScalarMult(colNormal,fi),ScalarMult(colNormal,fj)));

                // Adjust the previos pos
                particles[i].PX = VectorSub(particles[i].X, vi);
                particles[j].PX = VectorSub(particles[j].X, vj);

            }
        }
        vec3 colNormal = VectorSub(particles[i].X, ControlableParticle.X);
        float slength = DotProduct(colNormal,colNormal);
        float length = sqrt(slength);
        float target = kParticleSize + controlParticleSize;

        if(length < target){
            // prevois velocity
            vec3 vi = VectorSub(particles[i].X, particles[i].PX);
            vec3 vj = VectorSub(ControlableParticle.X, ControlableParticle.PX);

            // resolve overlapping conflict
            float factor = (length - target)/length;
            particles[i].X = VectorSub(particles[i].X,ScalarMult(colNormal,factor));

            // Compute the projected component factors
            float fi = ELASTICITY * DotProduct(colNormal,vi) / slength ;
            float fj = ELASTICITY * DotProduct(colNormal,vj) / slength ;

            // Swap the projected components
            vi = VectorAdd(vi,VectorSub(ScalarMult(colNormal,fj),ScalarMult(colNormal,fi)));

            // Adjust the previos pos
            particles[i].PX = VectorSub(particles[i].X, vi);
                
        }

    }
    // Wall tests keep energy
    for (i = 0; i < kNumParticles; i++)
	{
		if (particles[i].X.x < -BOXSIZE + kParticleSize) {
            particles[i].X.x = -BOXSIZE + kParticleSize;
        }

		if (particles[i].X.x > BOXSIZE - kParticleSize){
            particles[i].X.x = BOXSIZE - kParticleSize;
        }

        if (particles[i].X.y < 0 + kParticleSize){
            particles[i].X.y = kParticleSize;
        }

		if (particles[i].X.z < -BOXSIZE + kParticleSize){
            particles[i].X.z = -BOXSIZE + kParticleSize;
        }
			
		if (particles[i].X.z > BOXSIZE - kParticleSize){
            particles[i].X.z = BOXSIZE - kParticleSize;
        }		
	}

    ControlableParticle.PX = ControlableParticle.X;
			
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
    int subSet = ceil(2+ kParticleSize/pointSpace);
    resetGrid();
    for(int i = 0; i < kNumParticles; i++){
        Nw = floor((particles[i].X.x + RENDERBOXSIZE)/pointSpace);
        Nh = floor((particles[i].X.y + RENDERBOXSIZE)/pointSpace);
        Nd = floor((particles[i].X.z + RENDERBOXSIZE)/pointSpace);
        if(Nw >= 0 && Nw < GridPointsPerDim && Nh >= 0 && Nh < GridPointsPerDim && Nd >= 0 && Nd < GridPointsPerDim){
            for(int w = Nw-subSet; w <= Nw+subSet; w++){
                for(int h = Nh-subSet; h <= Nh+subSet; h++){
                    for(int d = Nd-subSet; d <= Nd+subSet; d++){
                        if(w < GridPointsPerDim &&  w >= 0  && h < GridPointsPerDim && h >= 0 &&  d < GridPointsPerDim &&  d >= 0){
                            if(abs(Norm(VectorSub(waterGrid.points[w][h][d].position , particles[i].X))) <= (2.2*kParticleSize)){
                                waterGrid.points[w][h][d].value = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

void renderParticle(vec3 position, Material m, float scale)
{
    transMatrix = T(position.x, position.y, position.z); // position
    tmpMatrix = Mult(transMatrix, S(scale, scale, scale));
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
void preAllocateVertices(){
    int index = 0;
    vec3 temp1;
    vec3 temp2;
    vec3 temp3;

    Vertex_Array = (GLfloat*) malloc(9*pow(GridPointsPerDim,3)*sizeof(GLfloat));
    Normal_Array = (GLfloat*) malloc(9*pow(GridPointsPerDim,3)*sizeof(GLfloat));
    for(int w = 0; w < GridPointsPerDim; w++ ){
        for(int h = 0; h < GridPointsPerDim; h++ ){
            for(int d = 0; d < GridPointsPerDim; d++ ){
                if(w >= GridPointsPerDim-1){ // Last point in width
                    temp1 = SetVector(0,0,0);
                }
                else{
                    temp1 = VertexInterp(waterGrid.points[w][h][d].position,waterGrid.points[w+1][h][d].position);
                }
                if(h >= GridPointsPerDim-1){ // Last point in height
                    temp2 = SetVector(0,0,0);
                }
                else{
                    temp2 = VertexInterp(waterGrid.points[w][h][d].position,waterGrid.points[w][h+1][d].position);
                }
                if(d >= GridPointsPerDim-1){
                    temp3 = SetVector(0,0,0);
                }
                else{
                    temp3 = VertexInterp(waterGrid.points[w][h][d].position,waterGrid.points[w][h][d+1].position);
                }
                
                Vertex_Array[index ] = temp1.x;
                Vertex_Array[index +1] = temp1.y;
                Vertex_Array[index +2] = temp1.z;

                Vertex_Array[index +3] = temp2.x;
                Vertex_Array[index +4] = temp2.y;
                Vertex_Array[index +5] = temp2.z;

                Vertex_Array[index +6] = temp3.x;
                Vertex_Array[index +7] = temp3.y;
                Vertex_Array[index +8] = temp3.z;

                Normal_Array[index ] = 0.0f;
                Normal_Array[index +1] = 0.0f;
                Normal_Array[index +2] = 0.0f;

                Normal_Array[index +3] =  0.0f;
                Normal_Array[index +4] =  0.0f;
                Normal_Array[index +5] =  0.0f;

                Normal_Array[index +6] =  0.0f;
                Normal_Array[index +7] =  0.0f;
                Normal_Array[index +8] =  0.0f;


                index +=9;

            }
        }
    }
}
void march(){
    int cubeindex;
    int tempIdx1;
    int tempIdx2;
    int tempIdx3;

    vec3 temp1;
    vec3 temp2;
    vec3 temp3;

    Index_Array = (GLuint*) malloc(2*4*pow(GridPointsPerDim,3)*sizeof(GLuint));
    vertices= 0;
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

                cubeindex = 0;
                if(P0.value) cubeindex |= 1;
                if(P1.value) cubeindex |= 2;
                if(P2.value) cubeindex |= 4;
                if(P3.value) cubeindex |= 8;
                if(P4.value) cubeindex |= 16;
                if(P5.value) cubeindex |= 32;
                if(P6.value) cubeindex |= 64;
                if(P7.value) cubeindex |= 128;
                
                for(int i = 0; triTable[cubeindex][i] != -1; i+= 3){

                    tempIdx1 = getVertIndex(w, h, d, triTable[cubeindex][i],GridPointsPerDim);
                    tempIdx2 = getVertIndex(w, h, d, triTable[cubeindex][i+1], GridPointsPerDim);
                    tempIdx3 =  getVertIndex(w, h, d, triTable[cubeindex][i+2], GridPointsPerDim);

                    temp1 = SetVector(Vertex_Array[3*tempIdx1],Vertex_Array[3*tempIdx1+1],Vertex_Array[3*tempIdx1+2]);
                    temp2 = SetVector(Vertex_Array[3*tempIdx2],Vertex_Array[3*tempIdx2+1],Vertex_Array[3*tempIdx2+2]);
                    temp3 = SetVector(Vertex_Array[3*tempIdx3],Vertex_Array[3*tempIdx3+1],Vertex_Array[3*tempIdx3+2]);

                    vec3 Normal = CalcNormalVector(temp1,temp2,temp3);
  

                    Normal_Array[3*tempIdx1] += Normal.x;
                    Normal_Array[3*tempIdx1 +1] += Normal.y;
                    Normal_Array[3*tempIdx1 +2] += Normal.z;

                    Normal_Array[3*tempIdx2] += Normal.x;
                    Normal_Array[3*tempIdx2 +1] += Normal.y;
                    Normal_Array[3*tempIdx2 +2] += Normal.z;

                    Normal_Array[3*tempIdx3] += Normal.x;
                    Normal_Array[3*tempIdx3 +1] += Normal.y;
                    Normal_Array[3*tempIdx3 +2] += Normal.z;
                    
                    Index_Array[vertices] = tempIdx1;
                    Index_Array[vertices+1] = tempIdx2;
                    Index_Array[vertices+2] = tempIdx3;
                    
                    vertices+=3;
                }
            }
        }
    }
    int vertCounter = 0;
    Vertex_Array_Buffer_Water = realloc(Vertex_Array_Buffer_Water,vertices*3*sizeof(GLfloat) );
    Index_Array_Buffer_Water = realloc(Index_Array_Buffer_Water,vertices*sizeof(GLuint) );
    Normal_Array_Buffer_Water = realloc(Normal_Array_Buffer_Water, vertices*3*sizeof(GLfloat));
    for(int i = 0; i < vertices; i+=3){
        Vertex_Array_Buffer_Water[vertCounter] = Vertex_Array[3*Index_Array[i]];
        Vertex_Array_Buffer_Water[vertCounter + 1] = Vertex_Array[3*Index_Array[i]+1];
        Vertex_Array_Buffer_Water[vertCounter + 2] = Vertex_Array[3*Index_Array[i]+2];

        Vertex_Array_Buffer_Water[vertCounter + 3] = Vertex_Array[3*Index_Array[i+1]];
        Vertex_Array_Buffer_Water[vertCounter + 4] = Vertex_Array[3*Index_Array[i+1]+1];
        Vertex_Array_Buffer_Water[vertCounter + 5] = Vertex_Array[3*Index_Array[i+1]+2];

        Vertex_Array_Buffer_Water[vertCounter + 6] = Vertex_Array[3*Index_Array[i+2]];
        Vertex_Array_Buffer_Water[vertCounter + 7] = Vertex_Array[3*Index_Array[i+2]+1];
        Vertex_Array_Buffer_Water[vertCounter + 8] = Vertex_Array[3*Index_Array[i+2]+2];

        Index_Array_Buffer_Water[i] = i;
        Index_Array_Buffer_Water[i+1] = i+1;
        Index_Array_Buffer_Water[i+2] = i+2;

        vec3 Normal;
        Normal = SetVector(Normal_Array[3*Index_Array[i]],Normal_Array[3*Index_Array[i]+1],Normal_Array[3*Index_Array[i]+2]);
        
        Normal = Normalize(Normal);
        Normal_Array_Buffer_Water[vertCounter] =  Normal.x;
        Normal_Array_Buffer_Water[vertCounter+1] = Normal.y;
        Normal_Array_Buffer_Water[vertCounter+2] = Normal.z;

        Normal = SetVector(Normal_Array[3*Index_Array[i+1]],Normal_Array[3*Index_Array[i+1]+1],Normal_Array[3*Index_Array[i+1]+2]);
        Normal = Normalize(Normal);
        Normal_Array_Buffer_Water[vertCounter+3] =  Normal.x;
        Normal_Array_Buffer_Water[vertCounter+4] = Normal.y;
        Normal_Array_Buffer_Water[vertCounter+5] = Normal.z;

        Normal = SetVector(Normal_Array[3*Index_Array[i+2]],Normal_Array[3*Index_Array[i+2]+1],Normal_Array[3*Index_Array[i+2]+2]);
        Normal = Normalize(Normal);
        Normal_Array_Buffer_Water[vertCounter+6] =  Normal.x;
        Normal_Array_Buffer_Water[vertCounter+7] = Normal.y;
        Normal_Array_Buffer_Water[vertCounter+8] = Normal.z;
        
        vertCounter+=9;
    }
    
    if(LOG){
    printf("START\n");
    for(int i = 0; i < vertices; i+=3){
        printf("Triangle (%u): ", i/3);
        printf(" %u ", Index_Array_Buffer_Water[i]);
        printf(" %u ", Index_Array_Buffer_Water[i+1]);
        printf(" %u \n", Index_Array_Buffer_Water[i+2]);
    }
    for(int i = 0; i < vertices*3; i+=3){
        printf("Vertex (%u): ", i /3);
        printf(" %f ", Vertex_Array_Buffer_Water[i]);
        printf(" %f ", Vertex_Array_Buffer_Water[i+1]);
        printf(" %f \n", Vertex_Array_Buffer_Water[i+2]);
        if((i/3 +1 )%3 ==0 && i != 0){
            printf("\n");
        }
    }
    printf("END\n %u  \n", vertices);
    }
}

void init()
{
	dumpInfo();
	// GL initss
    resetBuffers();
	glClearColor(0.4, 0.4, 0.4, 0);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");
    createGrid();
    // Load shader
    skyboxShader = loadShaders("shaders/skybox.vert", "shaders/skybox.frag");
    shader = loadShaders("shaders/lab3.vert", "shaders/lab3.frag");
    printError("init shader");
    sphere = LoadModelPlus("sphere.obj");
    
    preAllocateVertices();
    projectionMatrix = perspective(90, 1.0, 0.1, 1000); // It would be silly to upload an uninitialized matrix
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
    glUseProgram(skyboxShader);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

    // Initialize ball data, positions etc
    float ParticleMargin = 0.0001;
    float ballSize = 2 * kParticleSize + ParticleMargin;
    int ParticlesPerRow = floor(BOXSIZE/kParticleSize + 2*ParticleMargin);
	for (int i = 0; i < kNumParticles; i++)
	{
		particles[i].X = SetVector((-BOXSIZE+kParticleSize) + ballSize *(i%ParticlesPerRow), 4 + kParticleSize +  ballSize * floor(i/(ParticlesPerRow*ParticlesPerRow)), (-BOXSIZE+kParticleSize) + ballSize * ((int)(i/ParticlesPerRow)%ParticlesPerRow));
		particles[i].PX = particles[i].X;
        particles[i].a = SetVector(0.0,0.0,0.0);
        //particles[i].P = SetVector(((float)(i % 13))/ 50.0, 0.0, ((float)(i % 15))/50.0);
	}


    ControlableParticle.X = SetVector(0,2.0,0.0);

    ControlableParticle.mass = 1.0;
    cam = SetVector(0, 4, 4);
    point = SetVector(0, 0, 0);
    zprInit(&viewMatrix, cam, point, &ControlableParticle.X );  // camera controls

    char faces[6][15] =
    {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg"
    };
    cubemapTexture = loadCubemap(faces);  

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
    //glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
    if(!RENDERBALLS){
        evaluateGrid();
        march();
        Water = LoadDataToModel(Vertex_Array_Buffer_Water,Normal_Array_Buffer_Water,NULL,NULL,Index_Array_Buffer_Water,vertices,vertices);
        transMatrix = T(0.0, 0.0, 0.0); // position
        tmpMatrix = Mult(viewMatrix, transMatrix);
        glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
        loadMaterial(particleMt);
        DrawModel(Water, shader, "in_Position", "in_Normal", NULL);
    }
    printError("uploading to shader");

    renderParticle(ControlableParticle.X, ControlableParticleMt, controlParticleSize/0.1);
    
    if(RENDERBALLS){
        for (int i = 0; i < kNumParticles; i++){
            renderParticle(particles[i].X, particleMt, kParticleSize/0.1);
        }
    }

    glUseProgram(skyboxShader);
    glDepthFunc(GL_LEQUAL);
    skybox = LoadDataToModel(skyboxVert,NULL,NULL,NULL,skyboxIndices,8,36);
    transMatrix = S(1.0, 1.0, 1.0); // position
    tmpMatrix = Mult(viewMatrix, transMatrix);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
    DrawModel(skybox, skyboxShader, "in_Position", NULL, NULL);
    glDepthFunc(GL_LESS);
	

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