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

// initial width and heights
#define W 600
#define H 600

#define NEAR 1.0
#define FAR 100.0

#define NUM_LIGHTS 1
#define kParticleSize 0.1
#define controlParticleSize 0.5
#define ELASTICITY 1
#define BOXSIZE 0.7

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


GLfloat deltaT, currentTime;

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
        particles[i].X = VectorAdd(particles[i].X, ScalarMult(SetVector(0,-1.52,0), deltaT * deltaT));

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

        if (particles[i].X.y < 0 + kParticleSize){
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

void init()
{
	dumpInfo();
	// GL inits
	glClearColor(0.4, 0.4, 0.4, 0);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    // Load shader
    shader = loadShaders("shaders/lab3.vert", "shaders/lab3.frag");
    printError("init shader");
    sphere = LoadModelPlus("sphere.obj");

    projectionMatrix = perspective(90, 1.0, 0.1, 1000); // It would be silly to upload an uninitialized matrix
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

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

    printError("uploading to shader");
    renderParticle(ControlableParticle.X, ControlableParticleMt,controlParticleSize/0.1);
	for (int i = 0; i < kNumParticles; i++)
        renderParticle(particles[i].X, particleMt, kParticleSize/ 0.1);

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