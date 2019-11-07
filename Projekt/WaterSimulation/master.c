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
#define ELASTICITY 0.0
#define BOXSIZE 1.0

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


enum {kNumParticles = 700}; // Change as desired

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
            if(abs(Norm(VectorSub(particles[i].X, particles[j].X))) <= 0.8*(2*kParticleSize)){
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

    printError("uploading to shader");
    renderParticle(ControlableParticle.X, ControlableParticleMt);
	for (int i = 0; i < kNumParticles; i++)
        renderParticle(particles[i].X, particleMt);

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