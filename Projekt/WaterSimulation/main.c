// Laboration i spelfysik: Biljardbordet
// Av Ingemar Ragnemalm 2010, baserad p� material av Tomas Szabo.
// 2012: Ported to OpenGL 3.2 by Justina Mickonyt� and Ingemar R.
// 2013: Adapted to VectorUtils3 and MicroGlut.

// gcc lab3.c ../common/*.c -lGL -o lab3 -I../common 

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
#define kBallSize 0.1
#define ELASTICITY 0.0
#define FRICTION 1.0
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
  Model* model;
  GLuint textureId;
} ModelTexturePair;

typedef struct
{
  GLuint tex;
  GLfloat mass;

  vec3 X, P, L; // position, linear momentum, angular momentum
  mat4 R; // Rotation

  vec3 F, T; // accumulated force and torque

  //mat3 J, Ji; //We could have these but we can live without them for spheres.
  vec3 omega; // Angular momentum
  vec3 v; // Change in velocity

} Ball;

typedef struct
{
    GLfloat diffColor[4], specColor[4],
    ka, kd, ks, shininess;  // coefficients and specular exponent
} Material;

Material ballMt = { { 0.4, 0.4, 0.9, 1.0 }, { 1.0, 1.0, 1.0, 0.0 },
                    0.1, 0.6, 1.0, 50
                },
        shadowMt = { { 0.0, 0.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0, 0.5 },
                    0.1, 0.6, 1.0, 5.0
                },
        tableMt = { { 0.2, 0.1, 0.0, 1.0 }, { 0.4, 0.2, 0.1, 0.0 },
                    0.1, 0.6, 1.0, 5.0
                },
        tableSurfaceMt = { { 0.1, 0.5, 0.1, 1.0 }, { 0.0, 0.0, 0.0, 0.0 },
                    0.1, 0.6, 1.0, 0.0
                },
        controlableBallMt= { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 },
                    0.1, 0.6, 1.0, 0.0
                };


enum {kNumBalls = 700}; // Change as desired

//------------------------------Globals---------------------------------
ModelTexturePair tableAndLegs, tableSurf;
Model *sphere;
Ball ball[kNumBalls]; // We only use kNumBalls but textures for all 16 are always loaded so they must exist. So don't change here, change above.
Ball ControlableBall;


GLfloat deltaT, currentTime;

vec3 cam, point;

GLuint shader = 0;
GLint lastw = W, lasth = H;  // for resizing
//-----------------------------matrices------------------------------
mat4 projectionMatrix,
        viewMatrix, rotateMatrix, scaleMatrix, transMatrix, tmpMatrix;

//------------------------- lighting--------------------------------
vec3 lightSourcesColorArr[] = { {1.0f, 1.0f, 1.0f} }; // White light
GLfloat specularExponent[] = {50.0};
GLint directional[] = {0};
vec3 lightSourcesDirectionsPositions[] = { {0.0, 10.0, 0.0} };


//----------------------------------Utility functions-----------------------------------

void loadModelTexturePair(ModelTexturePair* modelTexturePair,
			  char* model, char* texture)
{
  modelTexturePair->model = LoadModelPlus(model); // , shader, "in_Position", "in_Normal", "in_TexCoord");
  if (texture)
    LoadTGATextureSimple(texture, &modelTexturePair->textureId);
  else
    modelTexturePair->textureId = 0;
}

void renderModelTexturePair(ModelTexturePair* modelTexturePair)
{
    if(modelTexturePair->textureId)
        glUniform1i(glGetUniformLocation(shader, "objID"), 0);  // use texture
    else
        glUniform1i(glGetUniformLocation(shader, "objID"), 1); // use material color only

    glBindTexture(GL_TEXTURE_2D, modelTexturePair->textureId);
    glUniform1i(glGetUniformLocation(shader, "texUnit"), 0);

    DrawModel(modelTexturePair->model, shader, "in_Position", "in_Normal", NULL);
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
	for (i = 0; i < kNumBalls; i++)
	{
		ball[i].F = SetVector(0,-9.82*ball[i].mass,0);
		ball[i].T = SetVector(0,0,0);
	}

	// Wall tests
    	for (i = 0; i < kNumBalls; i++)
	{
		if (ball[i].X.x < -BOXSIZE + kBallSize)
			ball[i].P.x = abs(ball[i].P.x);
		if (ball[i].X.x > BOXSIZE - kBallSize)
			ball[i].P.x = -abs(ball[i].P.x);
        if (ball[i].X.y < 0 + kBallSize)
            ball[i].P.y = abs(ball[i].P.y);
            //ball[i].F.y = 0;
		if (ball[i].X.z < -BOXSIZE + kBallSize)
			ball[i].P.z = abs(ball[i].P.z);
		if (ball[i].X.z > BOXSIZE - kBallSize)
			ball[i].P.z = -abs(ball[i].P.z);
	}

	// Detect collisions, calculate speed differences, apply forces
	for (i = 0; i < kNumBalls; i++)
        for (j = i+1; j < kNumBalls; j++)
        {            
            if(abs(Norm(VectorSub(ball[i].X, ball[j].X))) <= 0.8*(2*kBallSize)){

		        ball[i].v = ScalarMult(ball[i].P, 1.0/(ball[i].mass));
		        ball[j].v = ScalarMult(ball[j].P, 1.0/(ball[j].mass));
                if(DotProduct(VectorSub(ball[i].X, ball[j].X), VectorSub(ball[i].v, ball[j].v)) < 0.0){
                    vec3 SpeedDifference = VectorSub(ball[i].v , ball[j].v);
                    vec3 CollisionNormal = Normalize(VectorSub(ball[i].X, ball[j].X));
                    float J = ((DotProduct(SpeedDifference, CollisionNormal) * (ELASTICITY + 1))/ (1/ball[i].mass + 1/ball[j].mass))*-1;
                    ball[i].F = VectorAdd(ball[i].F, ScalarMult(ScalarMult(CollisionNormal,J),1/deltaT));
                    ball[j].F = VectorAdd(ball[j].F, ScalarMult(ScalarMult(ScalarMult(CollisionNormal,-1),J),1/deltaT));
                }
                
            }

        }

	// Control rotation here to reflect
	// friction against floor, simplified as well as more correct
    vec3 Up = SetVector(0,1,0);
    
	/*for (i = 0; i < kNumBalls; i++)
	{
      
        //float AngleSpeed = Norm(ball[i].v) * deltaT / kBallSize;
        //vec3 axis = CrossProduct(Up, ball[i].v);
		//ball[i].R = Mult(ArbRotate(axis, AngleSpeed),ball[i].R);
        vec3 velocity = VectorAdd(ball[i].v, CrossProduct(ball[i].omega,ScalarMult(Up,-kBallSize)));
        vec3 friction = ScalarMult(velocity, -FRICTION);
        ball[i].F = VectorAdd(ball[i].F, friction);
        //ball[i].T = VectorAdd(ball[i].T,CrossProduct(ScalarMult(Up,-kBallSize), friction));  
	}*/

// Update state, follows the book closely
	for (i = 0; i < kNumBalls; i++)
	{
		vec3 dX, dP, dL, dO;
		mat4 Rd;

		// Note: omega is not set. How do you calculate it?
        float val = 1.0/(0.3*ball[i].mass * kBallSize * kBallSize);
		ball[i].omega = ScalarMult(ball[i].L,val);
        
        //ball[i].omega = ScalarMult(ball[i].L,scaleValue);
//		v := P * 1/mass

		ball[i].v = ScalarMult(ball[i].P, 1.0/(ball[i].mass));
//		X := X + v*dT
		dX = ScalarMult(ball[i].v, deltaT); // dX := v*dT
		ball[i].X = VectorAdd(ball[i].X, dX); // X := X + dX
//		R := R + Rd*dT
		dO = ScalarMult(ball[i].omega, deltaT); // dO := omega*dT
		Rd = CrossMatrix(dO); // Calc dO, add to R
		Rd = Mult(Rd, ball[i].R); // Rotate the diff (NOTE: This was missing in early versions.)
		ball[i].R = MatrixAdd(ball[i].R, Rd);
//		P := P + F * dT
		dP = ScalarMult(ball[i].F, deltaT); // dP := F*dT
		ball[i].P = VectorAdd(ball[i].P, dP); // P := P + dP
//		L := L + t * dT
		dL = ScalarMult(ball[i].T, deltaT); // dL := T*dT
		ball[i].L = VectorAdd(ball[i].L, dL); // L := L + dL

		OrthoNormalizeMatrix(&ball[i].R);
	}
}

void renderBall(int ballNr)
{
    glBindTexture(GL_TEXTURE_2D, ball[ballNr].tex);

    // Ball with rotation
    transMatrix = T(ball[ballNr].X.x, ball[ballNr].X.y, ball[ballNr].X.z); // position
    tmpMatrix = Mult(transMatrix, ball[ballNr].R); // ball rotation
    tmpMatrix = Mult(S(kBallSize/0.1, kBallSize/0.1, kBallSize/0.1), tmpMatrix);
    tmpMatrix = Mult(viewMatrix, tmpMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
    loadMaterial(ballMt);
    DrawModel(sphere, shader, "in_Position", "in_Normal", NULL);
}
void renderControlableBall()
{
    glBindTexture(GL_TEXTURE_2D, ControlableBall.tex);

    // Ball with rotation
    transMatrix = T(ControlableBall.X.x, ControlableBall.X.y, ControlableBall.X.z); // position
    tmpMatrix = Mult(transMatrix, S(2, 2, 2));
    tmpMatrix = Mult(viewMatrix, tmpMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, tmpMatrix.m);
    loadMaterial(controlableBallMt);
    DrawModel(sphere, shader, "in_Position", "in_Normal", NULL);
}
//-------------------------------------------------------------------------------------

void init()
{
	dumpInfo();  // shader info

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

    loadModelTexturePair(&tableAndLegs, "tableandlegsnosurf.obj", 0);
    loadModelTexturePair(&tableSurf, "tablesurf.obj", "surface.tga");
    sphere = LoadModelPlus("sphere.obj");

    projectionMatrix = perspective(90, 1.0, 0.1, 1000); // It would be silly to upload an uninitialized matrix
    glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

    char *textureStr = malloc(128);
    int i;
    for(i = 0; i < kNumBalls; i++)
    {
        LoadTGATextureSimple("tex/texture.tga", &ball[i].tex);
    }
	free(textureStr);

    // Initialize ball data, positions etc
    float BallMargin = 0.01;
    int BallsPerRow = floor(BOXSIZE/(2*kBallSize + BallMargin));
	for (i = 0; i < kNumBalls; i++)
	{

		ball[i].mass = 1.0;
        //(kBallSize*2+BallMargin)(i/(BallsPerRow*BallsPerRow))
		ball[i].X = SetVector(-BOXSIZE+ (kBallSize*2 + BallMargin) *(i%BallsPerRow),1 +(kBallSize*2 + BallMargin) * (i/(BallsPerRow*BallsPerRow)), -BOXSIZE + (kBallSize*2 + BallMargin) *((i%(BallsPerRow*BallsPerRow))/BallsPerRow));
		ball[i].P = SetVector(((float)(i % 13))/ 50.0, 0.0, ((float)(i % 15))/50.0);
		ball[i].R = IdentityMatrix();
	}


    ControlableBall.X = SetVector(0,2.0,0.0);

    ControlableBall.mass = 1.0;
    cam = SetVector(0, 4, 4);
    point = SetVector(0, 0, 0);
    zprInit(&viewMatrix, cam, point, &ControlableBall.X );  // camera controls

    resetElapsedTime();
}
void drawStrokeText(char*string,int x,int y,int z)
{
	  //char *c;
	  //glPushMatrix();
	  //glTranslatef(x, y+8,z);
	  //glScalef(0.09f,-0.08f,z);
  
	  /*for (c=string; *c != '\0'; c++)
	  {
    		glutStrokeCharacter(GLUT_STROKE_ROMAN , *c);
	  }*/
	  //glPopMatrix();
}
//-------------------------------callback functions------------------------------------------
void display(void)
{
	int i;
    // This function is called whenever it is time to render
    //  a new frame; due to the idle()-function below, this
    //  function will get called several times per second
    
    updateWorld();

    // Clear framebuffer & zbuffer
	glClearColor(0.3, 0.3, 0.3, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    int time = glutGet(GLUT_ELAPSED_TIME);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);

    printError("uploading to shader");
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
	//glColor3f(0,1,0);
    //drawStrokeText("hej", 200,200,0);
    renderControlableBall();
	for (i = 0; i < kNumBalls; i++)
        renderBall(i);

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
