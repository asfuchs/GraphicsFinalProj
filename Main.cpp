/*
 *  CPE 474 lab 0 - modern graphics test bed
 *  draws a partial cube using a VBO and IBO 
 *  glut/OpenGL/GLSL application   
 *  Uses glm and local matrix stack
 *  to handle matrix transforms for a view matrix, projection matrix and
 *  model transform matrix
 *
 *  zwood 9/12 
 *  Copyright 2012 Cal Poly. All rights reserved.
 *
 *  Modified by Annamarie Fuchs for CPE471 Final Project 6/2013
 *
 *****************************************************************************/

#ifdef __APPLE__
#include "GLUT/glut.h"
#include <OPENGL/gl.h>
#endif

#ifdef __unix__
#include <GL/glut.h>
#endif

#ifdef _WIN32
#include <GL/glew.h>
#include <GL/glut.h>

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "freeglut.lib")
#endif

#include <iostream>
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GLSL_helper.h"
#include "MStackHelp.h"

#include "GeometryCreator.h"
#define _USE_MATH_DEFINES
#include <math.h>
using namespace std;
using namespace glm;

/*data structure for the image used for  texture mapping */
typedef struct Image {
  unsigned long sizeX;
  unsigned long sizeY;
  char *data;
} Image;

Image *TextureImage;

typedef struct RGB {
  GLubyte r;
  GLubyte g; 
  GLubyte b;
} RGB;

RGB myimage[64][64];
RGB* g_pixel;

//forward declaration of image loading and texture set-up code
int ImageLoad(char *filename, Image *image);
GLvoid LoadTexture(char* image_file, int tex_id);
////////////////
// Globals :( //
////////////////

// Parameters
unsigned int const StepSize = 10;
unsigned int WindowWidth = 1000, WindowHeight = 1000;

// Meshes
unsigned int const MeshCount = 14;
Mesh * Meshes[MeshCount];
unsigned int CurrentMesh;

// Variable Handles
GLuint aPosition;
GLuint aNormal;
GLuint uModelMatrix;
GLuint uNormalMatrix;
GLuint uViewMatrix;
GLuint uProjMatrix;
GLuint uColor;
GLuint uShiny;
GLuint uSpecular;
GLuint aTexCoord, uTexUnit;
GLuint GrndBuffObj, GIndxBuffObj, GNormalBuffObj, TexBuffObj;
int g_GiboLen;

// Shader Handle
GLuint ShadeProg;


// Program Variables
float Accumulator;
float CameraHeight;
float px, py;
vec3 lookpt = vec3(0.f, 0.f, 0.f);
vec3 eye = vec3(0.f, 5, 5.f);
vec3 viewV = vec3(lookpt - eye), u = vec3(1, 0, 0), v = vec3(0, 1, 0), w = vec3(0, 0, 1);
float curX, curY, mouseMoving = 1;
static const float g_groundY = -.1;      // y coordinate of the ground
static const float g_groundSize = 50;   // half the ground length
float b = 0.6;
float phi = -M_PI /2 , theta = M_PI * 1.5, radius;
float handRotate = 0, goalRotation = 90;
vec3 handTranslate = vec3(-3, 0, 2);
float twristAng = 30, tknuckleAng = 15, tAng = 15;
float iwristAng = 30, iknuckleAng = 15, iAng = 15, itAng = 15;
float mwristAng = 30, mknuckleAng = 15, mAng = 15, mtAng = 15;
float rwristAng = 30, rknuckleAng = 15, rAng = 15, rtAng = 15;
float pwristAng = 30, pknuckleAng = 15, pAng = 15, ptAng = 15;
float Px = 0, Py = .05, Pz = 0;
vec3 inter = vec3(3, 0, 2);
vec3 path = vec3(1, 0, 0);

int tupDown = 0, iupDown = 0, mupDown = 0, rupDown = 0, pupDown = 0, walkCount = 1, check = 0;

RenderingHelper ModelTrans;

void SetProjectionMatrix()
{
    glm::mat4 Projection = glm::perspective(80.0f, ((float) WindowWidth)/ ((float)WindowHeight), 0.1f, 100.f);
    safe_glUniformMatrix4fv(uProjMatrix, glm::value_ptr(Projection));
}

void SetView()
{
    glm::mat4 View = glm::lookAt(eye, lookpt, vec3(0.f, 1.f, 0.f));
    safe_glUniformMatrix4fv(uViewMatrix, glm::value_ptr(View));
}

void SetModel()
{
    safe_glUniformMatrix4fv(uModelMatrix, glm::value_ptr(ModelTrans.modelViewMatrix));
    safe_glUniformMatrix4fv(uNormalMatrix, glm::value_ptr(glm::transpose(glm::inverse(ModelTrans.modelViewMatrix))));
}

bool InstallShader(std::string const & vShaderName, std::string const & fShaderName)
{
    GLuint VS; // handles to shader object
    GLuint FS; // handles to frag shader object
    GLint vCompiled, fCompiled, linked; // status of shader

    VS = glCreateShader(GL_VERTEX_SHADER);
    FS = glCreateShader(GL_FRAGMENT_SHADER);

    // load the source
    char const * vSource = textFileRead(vShaderName);
    char const * fSource = textFileRead(fShaderName);
    glShaderSource(VS, 1, & vSource, NULL);
    glShaderSource(FS, 1, & fSource, NULL);

    // compile shader and print log
    glCompileShader(VS);
    printOpenGLError();
    glGetShaderiv(VS, GL_COMPILE_STATUS, & vCompiled);
    printShaderInfoLog(VS);

    // compile shader and print log
    glCompileShader(FS);
    printOpenGLError();
    glGetShaderiv(FS, GL_COMPILE_STATUS, & fCompiled);
    printShaderInfoLog(FS);

    if (! vCompiled || ! fCompiled)
    {
        std::cerr << "Error compiling either shader " << vShaderName << " or " << fShaderName << std::endl;
        return false;
    }

    // create a program object and attach the compiled shader
    ShadeProg = glCreateProgram();
    glAttachShader(ShadeProg, VS);
    glAttachShader(ShadeProg, FS);

    glLinkProgram(ShadeProg);

    // check shader status requires helper functions
    printOpenGLError();
    glGetProgramiv(ShadeProg, GL_LINK_STATUS, &linked);
    printProgramInfoLog(ShadeProg);

    glUseProgram(ShadeProg);

    // get handles to attribute data
    aPosition   = safe_glGetAttribLocation(ShadeProg, "aPosition");
    aNormal     = safe_glGetAttribLocation(ShadeProg, "aNormal");
    
    uColor          = safe_glGetUniformLocation(ShadeProg, "uColor");
    uProjMatrix     = safe_glGetUniformLocation(ShadeProg, "uProjMatrix");
    uViewMatrix     = safe_glGetUniformLocation(ShadeProg, "uViewMatrix");
    uModelMatrix    = safe_glGetUniformLocation(ShadeProg, "uModelMatrix");
    uNormalMatrix   = safe_glGetUniformLocation(ShadeProg, "uNormalMatrix");

    uShiny          = safe_glGetUniformLocation(ShadeProg,  "uShiny");
    uSpecular       = safe_glGetUniformLocation(ShadeProg,  "uSpecular");
    
    aTexCoord = safe_glGetAttribLocation(ShadeProg,  "aTexCoord");
    uTexUnit = safe_glGetUniformLocation(ShadeProg, "uTexUnit");
  
    std::cout << "Sucessfully installed shader " << ShadeProg << std::endl;
    return true;
}

void Initialize()
{
    glClearColor(0.8f, 0.8f, 1.0f, 1.0f);

    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    ModelTrans.useModelViewMatrix();
    ModelTrans.loadIdentity();

}
static void initGround() {
  
  // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
  float GrndPos[] = {
    -g_groundSize, g_groundY, -g_groundSize,
    -g_groundSize, g_groundY,  g_groundSize,
    g_groundSize, g_groundY,  g_groundSize,
    g_groundSize, g_groundY, -g_groundSize
  };
  
  float GrndNormal[] = {
    0, 1, 0,
    0, 1, 0,
    0, 1, 0,
    0, 1, 0
  };
  static GLfloat GrndTex[] = {
    0, 1,
    0, 0,
    1, 1,
    1, 0
  };
    
  unsigned short idx[] = {0, 1, 2, 0, 2, 3};
  
  g_GiboLen = 6;
  glGenBuffers(1, &GrndBuffObj);
  glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);
  
  glGenBuffers(1, &GIndxBuffObj);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
  
  glGenBuffers(1, &GNormalBuffObj);
  glBindBuffer(GL_ARRAY_BUFFER, GNormalBuffObj);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNormal), GrndNormal, GL_STATIC_DRAW);
  
  glGenBuffers(1, &TexBuffObj);
  glBindBuffer(GL_ARRAY_BUFFER, TexBuffObj);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);
}

void drawCyl(int r, int g, int b, float sca) {
  
  glUniform3f(uColor, .4, .4, .5);
  glUniform1f(uShiny, 200);
  glUniform3f(uSpecular, .6, 1, .95);
  
  ModelTrans.scale(1, 1, sca);
  SetModel();
  
  glDrawElements(GL_TRIANGLES, Meshes[2]->IndexBufferLength, GL_UNSIGNED_SHORT, 0);
  
}

void DrawThumb(float Tx, float Ty, float Tz, float wristAng, float knuckleAng, float tAng) {
  float scale1 = .6, scale2 = .47, scale3 = .37;
  
  //                    P          Q
  // Bicep       -1 [ -.75    0   .75 ] 1
  // Elbow       -1 [ -.75    0   .75 ] 1
  //                    Q
  
  //ModelTrans.loadIdentity();
  
  //Hierarchical Transformations
  ModelTrans.pushMatrix();
    ModelTrans.translate (vec3(Tx, Ty, Tz));
    ModelTrans.rotate(55, vec3(0, 0, 1));
    ModelTrans.rotate(360 - wristAng, vec3(1, 0, 0));
    ModelTrans.translate (vec3(-Px, -Py, -Pz));
    ModelTrans.pushMatrix();
  // Draw finger
      ModelTrans.translate (vec3(0, .05, scale1));
      ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
      ModelTrans.translate (vec3(-Px, -Py, -Pz));    
      ModelTrans.pushMatrix();
        ModelTrans.translate(vec3(0, .05, scale2));
        ModelTrans.rotate(tAng, vec3(1, 0, 0));
        ModelTrans.translate (vec3(-Px, -Py, -Pz)); 
        drawCyl(0, 1, 0, scale3);
      ModelTrans.popMatrix();
      drawCyl(0, 0, 1, scale2);
    ModelTrans.popMatrix();
  // Draw palm
    drawCyl(1, 0, 0, scale1);
  ModelTrans.popMatrix();
}

void DrawIndexFinger(float Tx, float Ty, float Tz, float wristAng, float knuckleAng, float Ang, float tAng) {
  float scale1 = 1, scale2 = .5, scale3 = .33, scale4 = .3;
    
    //ModelTrans.loadIdentity();
    
    //Hierarchical Transformations
    ModelTrans.pushMatrix();
      ModelTrans.translate (vec3(Tx, Ty, Tz));
      ModelTrans.rotate(5, vec3(0, 0, 1));
      ModelTrans.rotate(360 - wristAng, vec3(1, 0, 0));
      ModelTrans.translate (vec3(-Px, -Py, -Pz));
      ModelTrans.pushMatrix();
    // Draw finger
        ModelTrans.translate (vec3(0, .05, scale1));
        ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
        ModelTrans.translate (vec3(-Px, -Py, -Pz));
          ModelTrans.pushMatrix();
    // Draw middle
          ModelTrans.translate (vec3(0, .05, scale2));
          ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
          ModelTrans.translate (vec3(-Px, -Py, -Pz));
            ModelTrans.pushMatrix();
    // Draw tip
            ModelTrans.translate (vec3(0, .05, scale3));
            ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
            ModelTrans.translate (vec3(-Px, -Py, -Pz));
            drawCyl(1, 0, 1, scale4);
          ModelTrans.popMatrix();
          drawCyl(0, 1, 0, scale3);
        ModelTrans.popMatrix();
        drawCyl(0, 0, 1, scale2);
      ModelTrans.popMatrix();
    // Draw palm
      drawCyl(1, 0, 0, scale1);
    ModelTrans.popMatrix();
}
void DrawMiddleFinger(float Tx, float Ty, float Tz, float wristAng, float knuckleAng, float Ang, float tAng) {
  float scale1 = 1, scale2 = .53, scale3 = .37, scale4 = .3;
  
  //ModelTrans.loadIdentity();
  
  //Hierarchical Transformations
  ModelTrans.pushMatrix();
    ModelTrans.translate (vec3(Tx, Ty, Tz));
    ModelTrans.rotate(350, vec3(0, 0, 1));
    ModelTrans.rotate(360 - wristAng, vec3(1, 0, 0));
    ModelTrans.translate (vec3(-Px, -Py, -Pz));
    ModelTrans.pushMatrix();
  // Draw finger
      ModelTrans.translate (vec3(0, .05, scale1));
      ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
      ModelTrans.translate (vec3(-Px, -Py, -Pz));           // Move
      ModelTrans.pushMatrix();
  // Draw middle
        ModelTrans.translate (vec3(0, .05, scale2));
        ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
        ModelTrans.translate (vec3(-Px, -Py, -Pz));
        ModelTrans.pushMatrix();
  // Draw tip
          ModelTrans.translate (vec3(0, .05, scale3));
          ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
          ModelTrans.translate (vec3(-Px, -Py, -Pz));
          drawCyl(1, 0, 1, scale4);
        ModelTrans.popMatrix();
        drawCyl(0, 1, 0, scale3);
      ModelTrans.popMatrix();
      drawCyl(0, 0, 1, scale2);
    ModelTrans.popMatrix();
  // Draw palm
    drawCyl(1, 0, 0, scale1);
  ModelTrans.popMatrix();
}
void DrawRingFinger(float Tx, float Ty, float Tz, float wristAng, float knuckleAng, float Ang, float tAng) {
  float scale1 = .9, scale2 = .53, scale3 = .33, scale4 = .3;
  
  //ModelTrans.loadIdentity();
  
  //Hierarchical Transformations
  ModelTrans.pushMatrix();
    ModelTrans.translate (vec3(Tx, Ty, Tz));
    ModelTrans.rotate(340, vec3(0, 0, 1));
    ModelTrans.rotate(360 - wristAng, vec3(1, 0, 0));
    ModelTrans.translate (vec3(-Px, -Py, -Pz));
    ModelTrans.pushMatrix();
  // Draw finger
      ModelTrans.translate (vec3(0, .05, scale1));
      ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
      ModelTrans.translate (vec3(-Px, -Py, -Pz));           // Move
      ModelTrans.pushMatrix();
  // Draw middle
        ModelTrans.translate (vec3(0, .05, scale2));
        ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
        ModelTrans.translate (vec3(-Px, -Py, -Pz));
        ModelTrans.pushMatrix();
  // Draw tip
          ModelTrans.translate (vec3(0, .05, scale3));
          ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
          ModelTrans.translate (vec3(-Px, -Py, -Pz));
          drawCyl(1, 0, 1, scale4);
        ModelTrans.popMatrix();
        drawCyl(0, 1, 0, scale3);
      ModelTrans.popMatrix();
      drawCyl(0, 0, 1, scale2);
    ModelTrans.popMatrix();
  // Draw palm
    drawCyl(1, 0, 0, scale1);
  ModelTrans.popMatrix();
}
void DrawPinkyFinger(float Tx, float Ty, float Tz, float wristAng, float knuckleAng, float Ang, float tAng) {
  float scale1 = .87, scale2 = .37, scale3 = .23, scale4 = .27;
  
  //ModelTrans.loadIdentity();
  
  //Hierarchical Transformations
  ModelTrans.pushMatrix();
    ModelTrans.translate (vec3(Tx, Ty, Tz));
    ModelTrans.rotate(330, vec3(0, 0, 1));
    ModelTrans.rotate(360 - wristAng, vec3(1, 0, 0));
    ModelTrans.translate (vec3(-Px, -Py, -Pz));
    ModelTrans.pushMatrix();
  // Draw finger
      ModelTrans.translate (vec3(0, .05, scale1));
      ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
      ModelTrans.translate (vec3(-Px, -Py, -Pz));           // Move
      ModelTrans.pushMatrix();
  // Draw middle
        ModelTrans.translate (vec3(0, .05, scale2));
        ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
        ModelTrans.translate (vec3(-Px, -Py, -Pz));
        ModelTrans.pushMatrix();
  // Draw tip
          ModelTrans.translate (vec3(0, .05, scale3));
          ModelTrans.rotate (knuckleAng, vec3(1, 0, 0));
          ModelTrans.translate (vec3(-Px, -Py, -Pz));
          drawCyl(1, 0, 1, scale4);
        ModelTrans.popMatrix();
        drawCyl(0, 1, 0, scale3);
      ModelTrans.popMatrix();
      drawCyl(0, 0, 1, scale2);
    ModelTrans.popMatrix();
  // Draw palm
    drawCyl(1, 0, 0, scale1);
  ModelTrans.popMatrix();
} 

void WalkForward() {
  //printf("upDown %d %d %d %d %d\n", tupDown, iupDown, mupDown, rupDown, pupDown);
  //printf("HandTranslate %f %f inter %f %f\n", handTranslate.x, handTranslate.z, inter.x, inter.z);
  if ((handTranslate.x + 1) >= inter.x && (handTranslate.x - 1) <= inter.x && (handTranslate.z + 1) >= inter.z && (handTranslate.z - 1) <= inter.z) {
    walkCount = 2;
  }
  if (pknuckleAng < 60 && pupDown == 0) {
    pknuckleAng += 1;
    pAng += 1;
    ptAng += 1;
    pwristAng += .2;
    handTranslate.x += .02 * cos((90 - handRotate) * M_PI / 180);
    handTranslate.z += .02 * cos(handRotate * M_PI / 180);
  }
  else if(pknuckleAng > 15 && pupDown == 1) {
    pknuckleAng -= 1;
    pAng -= 1;
    ptAng -= 1;
    pwristAng -= .2;
  }
  if (rknuckleAng < 60 && pupDown == 1 && rupDown == 0) {
    rknuckleAng += 1;
    rAng += 1;
    rtAng += 1;
    rwristAng += .2;
    handTranslate.x += .02 * cos((90 - handRotate) * M_PI / 180);
    handTranslate.z += .02 * cos(handRotate * M_PI / 180);
  }
  else if(rknuckleAng > 15 && rupDown == 1) {
    rknuckleAng -= 1;
    rAng -= 1;
    rtAng -= 1;
    rwristAng -= .2;
  }
  if (mknuckleAng < 60 && rupDown == 1 && mupDown == 0) {
    mknuckleAng += 1;
    mAng += 1;
    mtAng += 1;
    mwristAng += .2;
    handTranslate.x += .02 * cos((90 - handRotate) * M_PI / 180);
    handTranslate.z += .02 * cos(handRotate * M_PI / 180);
  }
  else if(mknuckleAng > 15 && mupDown == 1) {
    mknuckleAng -= 1;
    mAng -= 1;
    mtAng -= 1;
    mwristAng -= .2;
  }
  if (iknuckleAng < 60 && mupDown == 1 && iupDown == 0) {
    iknuckleAng += 1;
    iAng += 1;
    itAng += 1;
    iwristAng += .2;
    handTranslate.x += .02 * cos((90 - handRotate) * M_PI / 180);
    handTranslate.z += .02 * cos(handRotate * M_PI / 180);
  }
  else if(iknuckleAng > 15 && iupDown == 1) {
    iknuckleAng -= 1;
    iAng -= 1;
    itAng -= 1;
    iwristAng -= .2;
  }
  if (tknuckleAng < 60 && iupDown == 1 && tupDown == 0) {
    tknuckleAng += 1;
    tAng += 1;
    twristAng += .2;
    handTranslate.x += .02 * cos((90 - handRotate) * M_PI / 180);
    handTranslate.z += .02 * cos(handRotate * M_PI / 180);
  }
  else if(tknuckleAng > 15 && tupDown == 1) {
    tknuckleAng -= 1;
    tAng -= 1;
    twristAng -= .2;
  }
  if (pknuckleAng == 60) {
    pupDown = 1;
    rupDown = 0;
  }
  if (rknuckleAng == 60) {
    rupDown = 1;
    mupDown = 0;
  }
  if (mknuckleAng == 60) {
    mupDown = 1;
    iupDown = 0;
  }
  if (iknuckleAng == 60) {
    iupDown = 1;
    tupDown = 0;
  }
  if (tknuckleAng == 60) {
    tupDown = 1;
    pupDown = 0;
  }
}
void TurnHand() {
  if (pknuckleAng < 60) {
    pknuckleAng += 1;
    pAng += 1;
    ptAng += 1;
    pwristAng += .2;
  }
  if (rknuckleAng < 60) {
    rknuckleAng += 1;
    rAng += 1;
    rtAng += 1;
    rwristAng += .2;
  }
  if (mknuckleAng < 60) {
    mknuckleAng += 1;
    mAng += 1;
    mtAng += 1;
    mwristAng += .2;    
  }
  if (iknuckleAng < 60) {
    iknuckleAng += 1;
    iAng += 1;
    itAng += 1;
    iwristAng += .2;
  }
  if (tknuckleAng < 60 && tupDown == 0) {
    tknuckleAng += 1;
    tAng += 1;
    twristAng += .2;
    if (goalRotation < handRotate) {
      handRotate -= 1;
    }

  }
  else if(tknuckleAng > 30 && tupDown == 1) {
    tknuckleAng -= 1;
    tAng -= 1;
    twristAng -= .2;
    if (goalRotation > handRotate) {
      handRotate += 1;
    }
  }
  if (tknuckleAng >= 60) {
    tupDown = 1;
  }
  if (tknuckleAng <= 30 && tupDown == 1) {
    tupDown = 0;
  }
  if (handRotate <= (goalRotation + 1) && handRotate >= (goalRotation - 1)) {
    handRotate = goalRotation;
    walkCount = 0;
    pupDown = 0;
  }
  //printf("goalRotation %f handRotate %f\n", goalRotation, handRotate);

}
void CurlHand() {
  if (pknuckleAng < 60) {
    pknuckleAng += 1;
    pAng += 1;
    ptAng += 1;
    pwristAng += .2;
  }
  if (rknuckleAng < 60) {
    rknuckleAng += 1;
    rAng += 1;
    rtAng += 1;
    rwristAng += .2;
  }
  if (mknuckleAng < 60) {
    mknuckleAng += 1;
    mAng += 1;
    mtAng += 1;
    mwristAng += .2;    
  }
  if (iknuckleAng < 60) {
    iknuckleAng += 1;
    iAng += 1;
    itAng += 1;
    iwristAng += .2;
  }
  if (tknuckleAng < 60 && tupDown == 0) {
    tknuckleAng += 1;
    tAng += 1;
    twristAng += .2;
  }
  
}

void Draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ShadeProg);

    SetProjectionMatrix();
    SetView();
    
    ModelTrans.loadIdentity();
    SetModel();
  //set up the texture unit
  glEnable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, 2);
  safe_glUniform1i(uTexUnit, 2);

  safe_glEnableVertexAttribArray(aPosition);
  
  // bind vbo
  glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  safe_glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
  // bind ibo
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  
  safe_glEnableVertexAttribArray(aNormal);
  glBindBuffer(GL_ARRAY_BUFFER, GNormalBuffObj);
  safe_glVertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  safe_glEnableVertexAttribArray(aTexCoord);
  glBindBuffer(GL_ARRAY_BUFFER, TexBuffObj);
  safe_glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0); 
  
  glUniform3f(uColor, 0.5, 1, 0.3);
  glUniform1f(uShiny, 100);
  glUniform3f(uSpecular, .8, .8, .8);
  
  // draw!
  glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
  
  // Disable the attributes used by our shader
  safe_glDisableVertexAttribArray(aPosition);
  safe_glDisableVertexAttribArray(aNormal); 
  glDisable(GL_TEXTURE_2D);
  
        safe_glEnableVertexAttribArray(aPosition);
        glBindBuffer(GL_ARRAY_BUFFER, Meshes[0]->PositionHandle);
        safe_glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

        safe_glEnableVertexAttribArray(aNormal);
        glBindBuffer(GL_ARRAY_BUFFER, Meshes[0]->NormalHandle);
        safe_glVertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
 
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Meshes[0]->IndexHandle);

        glUniform3f(uColor, 0.1f, 0.78f, 0.9f);
        glUniform1f(uShiny, 10);
        glUniform3f(uSpecular, .5, .2, .2);
  
  ModelTrans.pushMatrix();
    ModelTrans.translate(inter);
    ModelTrans.rotate(90, vec3(.5, 0, 0));
    ModelTrans.scale(2.5, 2.5, .002);
    glUniform3f(uColor, 1, .2, 0);
    glUniform1f(uShiny, 200);
    glUniform3f(uSpecular, .95, .75, .75);
    SetModel();
    glDrawElements(GL_TRIANGLES, Meshes[2]->IndexBufferLength, GL_UNSIGNED_SHORT, 0);
  ModelTrans.popMatrix();
  
  if (walkCount == 0) {
    WalkForward();
  }
  else if (walkCount == 1) {
    TurnHand();
  }
  else {
    CurlHand();
  } 

  //printf("walkCount %d\n", walkCount);
  
  ModelTrans.pushMatrix();
    ModelTrans.translate (handTranslate);
    ModelTrans.rotate(handRotate, vec3(0, 1, 0));    
    //ModelTrans.translate (vec3(-Px, -Py, -Pz));
    DrawThumb(-.1, 0, 0, twristAng, tknuckleAng, tAng);
    DrawIndexFinger(0, 0, 0, iwristAng, iknuckleAng, iAng, itAng);
    DrawMiddleFinger(.15, 0, 0, mwristAng, mknuckleAng, mAng, mtAng);
    DrawRingFinger(.3, 0, 0, rwristAng, rknuckleAng, rAng, rtAng);
    DrawPinkyFinger(.45, 0, 0, pwristAng, pknuckleAng, pAng, ptAng);
    ModelTrans.translate(vec3(-.15, 0, .05));
    ModelTrans.rotate(90, vec3(0, 1, 0));
    drawCyl(1, 0, 0, .63);
  ModelTrans.popMatrix();


  safe_glDisableVertexAttribArray(aPosition);
  safe_glDisableVertexAttribArray(aNormal);
    glUseProgram(0);
    glutSwapBuffers();
    glutPostRedisplay();
    printOpenGLError();
}

void Reshape(int width, int height)
{
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

void Keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    // Camera up/down
      case 'w':
        lookpt = vec3(lookpt - (vec3(w.x * b, w.y * b, w.z * b)));
        eye = vec3(eye - (vec3(w.x * b, w.y * b, w.z * b)));
        break;
      case 's':
        lookpt = vec3(lookpt + (vec3(w.x * b, w.y * b, w.z * b)));
        eye = vec3(eye + (vec3(w.x * b, w.y * b, w.z * b)));
        break;
      case 'a':
        lookpt = vec3(lookpt - (vec3(u.x * b, u.y * b, u.z * b)));
        eye = vec3(eye - (vec3(u.x * b, u.y * b, u.z * b)));
        break;
      case 'd':
        lookpt = vec3(lookpt + (vec3(u.x * b, u.y * b, u.z * b)));
        eye = vec3(eye + (vec3(u.x * b, u.y * b, u.z * b)));
        break;
      //Camera angle
      case 'i':
        phi += .1;
        break;
      case 'j':
        theta -= .1;
        break;
      case 'k':
        phi -= .1;
        break;
      case 'l':
        theta += .1;
        break;
    // Toggle wireframe
    case 'n':
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        break;
    case 'm':
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
        break;

    // Quit program
    case 'q': case 'Q' :
        exit( EXIT_SUCCESS );
        break;
    
    }
  lookpt.x = cos(theta) + eye.x;
  lookpt.y = sin(phi) + eye.y;
  lookpt.z = cos(M_PI/2 - theta) + eye.z;
}

void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      float xx = x, yy = y;
      
      viewV = normalize(vec3(lookpt - eye));
      w = -viewV;
      u = normalize(cross(vec3(0, 1, 0), w));
      v = normalize(cross(u, viewV));
      yy = WindowHeight - yy - 1;
      
      vec3 ray = unProject(vec3(xx, yy, 1.0f), lookAt(eye, lookpt, vec3(0.f, 1.f, 0.f)), 
                glm::perspective(80.0f, ((float) WindowWidth)/ ((float)WindowHeight), 0.1f, 100.f),
                vec4(0, 0, WindowWidth, WindowHeight));
      ray = normalize(ray);

      float rad = phi * M_PI / 180;
      float vLength = tan(rad / 2) * 0.1f;
      float uLength = vLength * (WindowWidth / WindowHeight);

      v = vLength * v;
      u = uLength * u;
      
      xx -= WindowWidth / 2;
      yy -= WindowHeight / 2;
      
      xx /= (WindowHeight / 2);
      yy /= (WindowWidth / 2);
      vec3 posn = eye + viewV * 0.1f + xx*u + yy*v;
      vec3 posf = eye + viewV * 100.f + xx*u + yy*v;
      
      vec3 norm = vec3(0, 1, 0);
      
      float np = dot(norm, posn);
      float nd = dot(norm, ray);
      
      vec3 oldinter = inter;
      inter = posn + (((-0.1f - np)/nd) * ray);
      printf("onldinter %f %f inter %f %f hand %f %f\n", oldinter.x, oldinter.z, inter.x, inter.z, handTranslate.x, handTranslate.z);
      
      vec3 h = handTranslate - oldinter;
      path = handTranslate - inter;
      h = normalize(h);
      path = normalize(path);
      if (h.z >= 0 && h.x >= 0) {
        goalRotation = -acos(dot(h, path)) * 180/M_PI + goalRotation;
      }
      else if (h.z >= 0 && h.x < 0) {
        goalRotation = acos(dot(h, path)) * 180/M_PI + goalRotation;
      }
      else if (h.z < 0 && h.x < 0) {
        goalRotation = -acos(dot(h, path)) * 180/M_PI + goalRotation;
      }
      else {
        goalRotation = acos(dot(h, path)) * 180/M_PI + goalRotation;
      }
      check = 0;
      printf("acos %f unchecked goalRotation %f\n", acos(dot(h, path)), goalRotation);
      //goalRotation = acos(dot(h, n) / (sqrt(n.x*n.x + n.y*n.y + n.z*n.z) * sqrt(h.x*h.x + h.y*h.y + h.z*h.z))) * 180/M_PI + goalRotation;
      /*while (goalRotation > 180 || goalRotation < -180) {
        if (goalRotation > 180) {
          goalRotation = goalRotation - 360;
        }
        else if (goalRotation < -180) {
          goalRotation = goalRotation + 360;
        }
        check++;
        check = check % 2;
        printf("check\n");
      }*/
      
      //printf("walkCount %d\n", walkCount);
      walkCount = 1;
      printf("goalRotation %f handRotate %f\n", goalRotation, handRotate);
    }
  }
  glutPostRedisplay();
}
void mouseMove(int x, int y) {

  glutPostRedisplay();
}
void Timer(int param)
{
    Accumulator += StepSize * 0.001f;
    glutTimerFunc(StepSize, Timer, 1);
}

int main(int argc, char *argv[])
{
    // Initialize Global Variables
    Accumulator = 0.f;
    CameraHeight = 5.f;
    CurrentMesh = 0;

    // Glut Setup
    glutInit(& argc, argv);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("The Hand that Creeps");
    glutDisplayFunc(Draw);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc( mouse );
    glutMotionFunc( mouseMove );
    //glutTimerFunc(StepSize, Timer, 1);

    // GLEW Setup (Windows only)
#ifdef _WIN32
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Error initializing glew! " << glewGetErrorString(err) << std::endl;
        return 1;
    }
#endif

    // OpenGL Setup
    Initialize();
    LoadTexture((char *)"dirt.bmp", 0);
  
  
    initGround();
    getGLversion();

    // Shader Setup
    if (! InstallShader("Phong.vert", "Phong.frag"))
    {
        printf("Error installing shader!\n");
        return 1;
    }
    
    Meshes[0] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 1.0f, 8, 8);
    Meshes[1] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[2] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 1.0f, 8, 8);
    Meshes[3] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[4] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[5] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[6] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[7] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[8] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[9] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[10] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[11] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[12] = GeometryCreator::CreateCylinder(0.05f, 0.05f, 0.8f, 8, 8);
    Meshes[13] = GeometryCreator::CreateTorus(2.3f, 2.5f, 48, 64);
    glutMainLoop();

    return 0;
}
//routines to load in a bmp files - must be 2^nx2^m and a 24bit bmp
GLvoid LoadTexture(char* image_file, int texID) { 
  
  TextureImage = (Image *) malloc(sizeof(Image));
  if (TextureImage == NULL) {
    printf("Error allocating space for image");
    exit(1);
  }
  cout << "trying to load " << image_file << endl;
  if (!ImageLoad(image_file, TextureImage)) {
    exit(1);
  }  
  /*  2d texture, level of detail 0 (normal), 3 components (red, green, blue),            */
  /*  x size from image, y size from image,                                              */    
  /*  border 0 (normal), rgb color data, unsigned byte data, data  */ 
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, 3,
               TextureImage->sizeX, TextureImage->sizeY,
               0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage->data);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); /*  cheap scaling when image bigger than texture */    
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); /*  cheap scaling when image smalled than texture*/
  
}


/* BMP file loader loads a 24-bit bmp file only */

/*
 * getint and getshort are help functions to load the bitmap byte by byte
 */
static unsigned int getint(FILE *fp) {
  int c, c1, c2, c3;
  
  /*  get 4 bytes */ 
  c = getc(fp);  
  c1 = getc(fp);  
  c2 = getc(fp);  
  c3 = getc(fp);
  
  return ((unsigned int) c) +   
  (((unsigned int) c1) << 8) + 
  (((unsigned int) c2) << 16) +
  (((unsigned int) c3) << 24);
}

static unsigned int getshort(FILE *fp){
  int c, c1;
  
  /* get 2 bytes*/
  c = getc(fp);  
  c1 = getc(fp);
  
  return ((unsigned int) c) + (((unsigned int) c1) << 8);
}

/*  quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.  */

int ImageLoad(char *filename, Image *image) {
  FILE *file;
  unsigned long size;                 /*  size of the image in bytes. */
  unsigned long i;                    /*  standard counter. */
  unsigned short int planes;          /*  number of planes in image (must be 1)  */
  unsigned short int bpp;             /*  number of bits per pixel (must be 24) */
  char temp;                          /*  used to convert bgr to rgb color. */
  
  /*  make sure the file is there. */
  if ((file = fopen(filename, "rb"))==NULL) {
    printf("File Not Found : %s\n",filename);
    return 0;
  }
  
  /*  seek through the bmp header, up to the width height: */
  fseek(file, 18, SEEK_CUR);
  
  /*  No 100% errorchecking anymore!!! */
  
  /*  read the width */    image->sizeX = getint (file);
  
  /*  read the height */ 
  image->sizeY = getint (file);
  
  /*  calculate the size (assuming 24 bits or 3 bytes per pixel). */
  size = image->sizeX * image->sizeY * 3;
  
  /*  read the planes */    
  planes = getshort(file);
  if (planes != 1) {
    printf("Planes from %s is not 1: %u\n", filename, planes);
    return 0;
  }
  
  /*  read the bpp */    
  bpp = getshort(file);
  if (bpp != 24) {
    printf("Bpp from %s is not 24: %u\n", filename, bpp);
    return 0;
  }
  
  /*  seek past the rest of the bitmap header. */
  fseek(file, 24, SEEK_CUR);
  
  /*  read the data.  */
  image->data = (char *) malloc(size);
  if (image->data == NULL) {
    printf("Error allocating memory for color-corrected image data");
    return 0; 
  }
  
  if ((i = fread(image->data, size, 1, file)) != 1) {
    printf("Error reading image data from %s.\n", filename);
    return 0;
  }
  
  for (i=0;i<size;i+=3) { /*  reverse all of the colors. (bgr -> rgb) */
    temp = image->data[i];
    image->data[i] = image->data[i+2];
    image->data[i+2] = temp;
  }
  
  fclose(file); /* Close the file and release the filedes */
  
  /*  we're done. */
  return 1;
}
