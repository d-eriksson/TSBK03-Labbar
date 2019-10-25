// Demo of heavily simplified sprite engine
// by Ingemar Ragnemalm 2009
// used as base for lab 4 in TSBK03.
// OpenGL 3 conversion 2013.

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	#include "MicroGlut.h"
	// uses framework Cocoa
#else
	#include <GL/gl.h>
	#include "MicroGlut.h"
#endif

#include <stdlib.h>
#include "LoadTGA.h"
#include "SpriteLight.h"
#include "GL_utilities.h"
#include <math.h>

// L�gg till egna globaler h�r efter behov.
float pointabs(FPoint X){
	return sqrt((X.v)*(X.v) + (X.h)*(X.h));
}
float pointdist(FPoint X, FPoint Y){
	return sqrt((X.v-Y.v)*(X.v-Y.v) + (X.h-Y.h)*(X.h-Y.h));
}
FPoint pointdiff(FPoint X, FPoint Y){
	FPoint temp;
	temp.h = X.h - Y.h;
	temp.v = X.v - Y.v;
	return temp;
}
FPoint CalcAvoidance(FPoint P, float r){
	FPoint temp;
	temp.v = 0;
	temp.h = 0;
	if(pointabs(P) < r){
		temp.v = P.v;
		temp.h = P.h;
	}
	return temp;
}
FPoint pointNormalize(FPoint f){
	FPoint temp;
	temp.h = 0;
	temp.v = 0;
	if(pointabs(f) > 0.00001){
		temp.h = f.h/pointabs(f);
		temp.v = f.v/pointabs(f);
	}
	return temp;
}
bool inView(SpritePtr sp, SpritePtr sp2, float rad){
	FPoint X = pointdiff(sp2->position, sp->position);
	X = pointNormalize(X);
	FPoint Y =pointNormalize(sp->speed); 
	float dot = X.h * Y.h + X.v * Y.v;
	return dot > rad;
}

void SpriteBehavior() // Din kod!
{
// L�gg till din labbkod h�r. Det g�r bra att �ndra var som helst i
// koden i �vrigt, men mycket kan samlas h�r. Du kan utg� fr�n den
// globala listroten, gSpriteRoot, f�r att kontrollera alla sprites
// hastigheter och positioner, eller arbeta fr�n egna globaler.

	// Loop though all sprites. (Several loops in real engine.)
	float radius = 150.0;
	float minRadius = 50.0;
	float kAlignmentWeight = 0.01;
	float kCohesionWeight = 0.008;
	float kAvoidanceWeight = 0.1;
	SpritePtr sp;
	SpritePtr sp2;
	sp = gSpriteRoot;
	do
	{
		sp2 = gSpriteRoot;
		int count = 0;
		sp->speedDiff.h = 0;
		sp->speedDiff.v = 0;
		sp->averagePosition.h = 0;
		sp->averagePosition.v = 0;
		sp->avoidanceVector.h = 0;
		sp->avoidanceVector.v = 0;

		do{
			if(sp != sp2){
				if(pointdist(sp->position, sp2->position) < radius && inView(sp,sp2,0.0)){
					FPoint diffSpeed = pointdiff(sp2->speed, sp->speed);
					FPoint avoidVec = CalcAvoidance(pointdiff(sp->position, sp2->position), minRadius);
					sp->speedDiff.h += diffSpeed.h;
					sp->speedDiff.v += diffSpeed.v;
					sp->averagePosition.h += sp2->position.h;
					sp->averagePosition.v += sp2->position.v;
					sp->avoidanceVector.h += avoidVec.h;
					sp->avoidanceVector.v += avoidVec.v;
					count++;
				}
			}
			sp2 = sp2->next;
		}while(sp2 != NULL);
		if(count > 0){
			
			sp->speedDiff.h /= count;
			sp->speedDiff.v /= count;
			//sp->speedDiff = pointNormalize(sp->speedDiff);
			
			sp->averagePosition.h /= count;
			sp->averagePosition.v /= count;
			//sp->averagePosition = pointNormalize(sp->averagePosition);

			sp->avoidanceVector.h /= count;
			sp->avoidanceVector.v /= count;
			//sp->avoidanceVector = pointNormalize(sp->avoidanceVector);
			
		}
		sp = sp->next;
	} while (sp != NULL);
	sp =gSpriteRoot;
	do{
		FPoint cohesionVector;
		cohesionVector.h = 0;
		cohesionVector.v =0;
		if(pointabs(sp->averagePosition) > 0.0000001){
			cohesionVector = pointdiff(sp->averagePosition, sp->position);
		}
		
		sp->speed.h += sp->speedDiff.h * kAlignmentWeight + cohesionVector.h * kCohesionWeight + sp->avoidanceVector.h * kAvoidanceWeight;
		sp->speed.v += sp->speedDiff.v * kAlignmentWeight + cohesionVector.v * kCohesionWeight + sp->avoidanceVector.v * kAvoidanceWeight;
		sp->speed = pointNormalize(sp->speed);
		float random = rand() % (6 + 1 -3) + 3;
		sp->speed.h *= 3 + random*sp->craziness;
		sp->speed.v *= 3 + random*sp->craziness;
		
		sp = sp->next;
	}while(sp != NULL);
	
}

// Drawing routine
void Display()
{
	SpritePtr sp;
	
	glClearColor(0, 0, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT+GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	DrawBackground();
	
	SpriteBehavior(); // Din kod!
	
// Loop though all sprites. (Several loops in real engine.)
	sp = gSpriteRoot;
	do
	{
		HandleSprite(sp); // Callback in a real engine
		DrawSprite(sp);
		sp = sp->next;
	} while (sp != NULL);
	
	glutSwapBuffers();
}

void Reshape(int h, int v)
{
	glViewport(0, 0, h, v);
	gWidth = h;
	gHeight = v;
}

void Timer(int value)
{
	glutTimerFunc(20, Timer, 0);
	glutPostRedisplay();
}

// Example of user controllable parameter
float someValue = 0.0;

void Key(unsigned char key,
         __attribute__((unused)) int x,
         __attribute__((unused)) int y)
{
  switch (key)
  {
    case '+':
    	someValue += 0.1;
    	printf("someValue = %f\n", someValue);
    	break;
    case '-':
    	someValue -= 0.1;
    	printf("someValue = %f\n", someValue);
    	break;
    case 0x1b:
      exit(0);
  }
}

void Init()
{
	TextureData *sheepFace, *blackFace, *dogFace, *foodFace;
	
	LoadTGATextureSimple("bilder/leaves.tga", &backgroundTexID); // Bakgrund
	
	sheepFace = GetFace("bilder/sheep.tga"); // Ett f�r
	blackFace = GetFace("bilder/blackie.tga"); // Ett svart f�r
	//dogFace = GetFace("bilder/dog.tga"); // En hund
	//foodFace = GetFace("bilder/mat.tga"); // Mat
	
	NewSprite(sheepFace, 100, 200, -1, 1,0);
	NewSprite(sheepFace, 120, 100, -1, 1.2,0);
	NewSprite(sheepFace, 340, 300, -1, 1.5,0);
	NewSprite(sheepFace, 460, 50, 1, 1.9,0);
	NewSprite(sheepFace, 500, 100, -1.5, -1,0);
	NewSprite(sheepFace, 420, 110, -1.5, -1,0);
	NewSprite(sheepFace, 510, 100, -1.5, -1,0);
	NewSprite(sheepFace, 500, 120, -1.5, -1,0);
	
	NewSprite(sheepFace, 250, 200, -1, 1.5,0);
	NewSprite(sheepFace, 150, 100, -1, 1.5,0);
	NewSprite(blackFace, 400, 100, -1, 1.5,1.0);
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(800, 600);
	glutInitContextVersion(3, 2);
	glutCreateWindow("SpriteLight demo / Flocking");
	
	glutDisplayFunc(Display);
	glutTimerFunc(20, Timer, 0); // Should match the screen synch
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Key);
	
	InitSpriteLight();
	Init();
	
	glutMainLoop();
	return 0;
}
