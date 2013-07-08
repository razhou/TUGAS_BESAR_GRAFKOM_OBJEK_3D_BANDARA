#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <math.h>

#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"

GLuint _textureId;


GLuint loadTexture(Image* Foto) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Foto->width, Foto->height, 0, GL_RGB, GL_UNSIGNED_BYTE, Foto->pixels);
	return textureId;
}

void initRendering1() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	
	Image* Foto = loadBMP("wood.bmp");
	_textureId = loadTexture(Foto);
	delete Foto;
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

void papan() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glTranslatef(0.0f, 0.0f, -20.0f);
	
	GLfloat ambientLight[] = {0.3f, 0.3f, 0.3f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);
	
	GLfloat lightColor[] = {0.7f, 0.7f, 0.7f, 1.0f};

	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glutSwapBuffers();
}


static GLfloat spin, spin2 = 0.0;
float angle = 0;
GLuint mentari;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = 10;           /////////////// script awal penglihatan /////////
static int viewy = 90;
static int viewz = 300;

float rot = 0;

//train 2D
//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class



void initRendering(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;



const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;

}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
	 glTranslatef(0.0f, 0.0f, -10.0f);
	 glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
	 glRotatef(-_angle, 0.0f, 1.0f, 0.0f);

	 GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	 glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	 GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	 GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	 glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	 glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	 */
	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

unsigned int LoadTextureFromBmpFile(char *filename);


void matahari(void)
{
    glPushMatrix();
    glTranslatef(-120,120,-100);
    glColor3ub(255, 253, 116);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidSphere(20, 60, 60);
    glPopMatrix();
    glEndList();
}


void awan(void){
     glPushMatrix(); 
     glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3ub(153, 223, 255);
     glTranslatef(60,100,-2);
     glutSolidSphere(10, 50, 50);
     glPopMatrix();

     glPushMatrix();
     glTranslatef(50,99,-2);
     glutSolidSphere(9, 50, 50);
     glPopMatrix();   
     
     glPushMatrix();
     glTranslatef(70,98,-2);
     glutSolidSphere(9, 50, 50);
     glPopMatrix();     
     
     glPushMatrix();
     glTranslatef(43,97,-2);
     glutSolidSphere(7, 50, 50);
     glPopMatrix();
     
     glPushMatrix();
     glTranslatef(50,104,-8);
     glutSolidSphere(10, 50, 50);
     glPopMatrix();
}     

void lampu(void) {
     ////////////////////////////////////////////////////////////LAMPU PEMBATAS DI LANDASAN/////////////////////////////////////////
     glPushMatrix();
     glTranslatef(1,0.6,2);
     glutSolidSphere(5, 50, 50);
     glPopMatrix(); 
} 

void markajalan(void) {
     ////////////////////////////////////////////////////////////GARIS PUTUS - PUTUS DI ASPAL.BMP/////////////////////////////////////////
    glPushMatrix();
    glScaled(1, 0.05,0.3);
    glTranslatef(2.4,2.5,67); 
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
} 


/////////////////////////////////////////POHON////////////////////////////////
void pohon(void){
    //batang
    GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);
 
    glPushMatrix();
    glColor3ub(104,70,14);
    glRotatef(270,1,0,0);
    gluCylinder(pObj, 4, 0.7, 30, 25, 25);
    glPopMatrix();
}
 
//ranting
void ranting(void){
    GLUquadricObj *pObj;
    pObj =gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);
    glPushMatrix();
    glColor3ub(104,70,14);
    glTranslatef(0,27,0);
    glRotatef(330,1,0,0);
    gluCylinder(pObj, 0.6, 0.1, 15, 25, 25);
    glPopMatrix();
 
    //daun
    glPushMatrix();
    glColor3ub(18,118,13);
    glScaled(5, 5, 5);
    glTranslatef(0,7,3);
    glutSolidDodecahedron();
    glPopMatrix();
}


void bangunan(void){
	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	//drawSceneTanah(_terrainPapan, 0.0f, 0.2f, 0.5f);
	glPopMatrix();     
     
glPushMatrix();
glTranslatef(30, 30, -120);
glScalef(10,5,5);
glColor3f(0.3402, 0.3412, 0.3117);
glutSolidCube(20);
glPopMatrix(); 
//atap
glPushMatrix();

glTranslatef(0, 100, -100);
glScalef(20, 10, 20);
glRotatef(5, 0, 1, 0);
glColor3f(0, 1, 0);
glutSolidOctahedron();

glPopMatrix(); 

//pagar
glPushMatrix();
glColor3f(0, 1, 0);
glTranslatef(40,70, -69);
glScalef(10,2,1);
glutSolidCube(15);
glPopMatrix();

//jendela1
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(-45,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(-25,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(-5,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(15,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(35,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(55,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(75,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(95,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//jendela2
glPushMatrix();
glColor3f(1, 1, 1);
glTranslatef(115,43, -69);
glScaled(6,10,1);
glutSolidCube(2);
glPopMatrix();

//pintu
 glPushMatrix();
 glColor3f(0.0980, 0.0608, 0.0077);
glTranslatef(25,15, -70);
glScaled(15,10,1);
glutSolidCube(2);
glPopMatrix();

//garis atas pintu
 glPushMatrix();
 glColor3f(0.0980, 0.0608, 0.0077);
 glTranslatef(25,27, -70);
glScaled(15,1,1);
glutSolidCube(1);
glPopMatrix();
     }
void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);

	glPushMatrix();


bangunan();

///////////////////////////////////////POHON////////////////////////////////////
//Pohon 1
glPushMatrix();
        glTranslatef(-180,15,-105);
        glScalef(1, 1, 1);
        glRotatef(10,0,1,0);
        pohon();
 
        //ranting1
        ranting();
 
        //ranting2
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glTranslatef(0,25,25);
        glRotatef(250,1,0,0);
        ranting();
        glPopMatrix();
 
        //ranting3
        glPushMatrix();
        glScalef(1.8, 1.8, 1.8);
        glTranslatef(0,-6,21.5);
        glRotatef(-55,1,0,0);
        ranting();
        glPopMatrix();
glPopMatrix();
//Pohon 2
glPushMatrix();
        glTranslatef(-150,15,-105);
        glScalef(1, 1, 1);
        glRotatef(170,0,1,0);
        pohon();
 
        //ranting1
        ranting();
 
        //ranting2
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glTranslatef(0,25,25);
        glRotatef(250,1,0,0);
        ranting();
        glPopMatrix();
 
        //ranting3
        glPushMatrix();
        glScalef(1.8, 1.8, 1.8);
        glTranslatef(0,-6,21.5);
        glRotatef(-55,1,0,0);
        ranting();
        glPopMatrix();
glPopMatrix();
//Pohon 3
glPushMatrix();
        glTranslatef(-105,15,-105);
        glScalef(1, 1, 1);
        glRotatef(90,0,1,0);
        pohon();
 
        //ranting1
        ranting();
 
        //ranting2
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glTranslatef(0,25,25);
        glRotatef(250,1,0,0);
        ranting();
        glPopMatrix();
 
        //ranting3
        glPushMatrix();
        glScalef(1.8, 1.8, 1.8);
        glTranslatef(0,-6,21.5);
        glRotatef(-55,1,0,0);
        ranting();
        glPopMatrix();
glPopMatrix();
//Pohon 4
glPushMatrix();
        glTranslatef(165,15,-105);
        glScalef(1, 1, 1);
        glRotatef(90,0,1,0);
        pohon();
 
        //ranting1
        ranting();
 
        //ranting2
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glTranslatef(0,25,25);
        glRotatef(250,1,0,0);
        ranting();
        glPopMatrix();
 
        //ranting3
        glPushMatrix();
        glScalef(1.8, 1.8, 1.8);
        glTranslatef(0,-6,21.5);
        glRotatef(-55,1,0,0);
        ranting();
        glPopMatrix();
glPopMatrix();
//Pohon 5
glPushMatrix();
        glTranslatef(215,15,-105);
        glScalef(1, 1, 1);
        glRotatef(110,0,1,0);
        pohon();
 
        //ranting1
        ranting();
 
        //ranting2
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glTranslatef(0,25,25);
        glRotatef(250,1,0,0);
        ranting();
        glPopMatrix();
 
        //ranting3
        glPushMatrix();
        glScalef(1.8, 1.8, 1.8);
        glTranslatef(0,-6,21.5);
        glRotatef(-55,1,0,0);
        ranting();
        glPopMatrix();
glPopMatrix();



///////////////////////////////////////// MATAHARIKU ///////////////////////////////////
    glPushMatrix();
    glTranslatef(5,90,20); 
    glScalef(1.25, 1.0, 0.20);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    matahari();
    glPopMatrix();
/////////////////////////////////////////////////////////// AHIRNYA MATAHARINYA BERES ///////////////

////////////////////////////////// LAMPU - LAMPU PEMBATAS LANDASAN ////////////////////////////////////////////
     glPushMatrix();
     glTranslatef(-220,6,30); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix(); 
     
     glPushMatrix();
     glTranslatef(-186,6,30); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix(); 
     
     glPushMatrix();
     glTranslatef(-185,6,65); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix();    
                                   glPushMatrix();
                                   glTranslatef(-150,6,65); 
                                   glScalef(0.5,0.5,0.5);
                                   //glBindTexture(GL_TEXTURE_2D, texture[0]);
                                   lampu();
                                   glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-150,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-120,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-90,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-60,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-30,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(30,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(60,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(90,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(120,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(150,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(180,6,30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(205,6,53); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(245,6,63); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    
    ////////////////////////// LAMPU KANAN ////////////////////////////////////////////
    
     glPushMatrix();
     glTranslatef(-220,6,-30); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix(); 
     
     glPushMatrix();
     glTranslatef(-186,6,-30); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix(); 
     
     glPushMatrix();
     glTranslatef(-185,6,-65); 
     glScalef(0.5,0.5,0.5);
     //glBindTexture(GL_TEXTURE_2D, texture[0]);
     lampu();
     glPopMatrix();    
                                   glPushMatrix();
                                   glTranslatef(-150,6,-65); 
                                   glScalef(0.5,0.5,0.5);
                                   //glBindTexture(GL_TEXTURE_2D, texture[0]);
                                   lampu();
                                   glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-150,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-120,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-90,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-60,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-30,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(30,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(60,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(90,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(120,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(150,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(180,6,-30); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(205,6,-53); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(245,6,-63); 
    glScalef(0.5,0.5,0.5);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    lampu();
    glPopMatrix();
//////////////////////////////////////////////////////////// AHIRNYA LAMPUNYA HABIS///////////////////////////

////////////////////////////////////////////// Awan - Awan Dilangit ////////////////////////////////////
    glPushMatrix();
    glTranslatef(5,90,20); 
    glScalef(1.25, 1.0, 0.20);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    awan();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(60,40,20); 
    glScalef(1.25, 1.0, 0.20);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    awan();
    glPopMatrix();	
    
    glPushMatrix();
    glTranslatef(-130,85,20); 
    glScalef(1.25, 1.0, 0.20);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    awan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-110,75,10); 
    glScalef(1.25, 1.0, 0.20);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    awan();
    glPopMatrix();

///////////////////////////////////////////// AWAN _ AWAN BIRU DI LANGIT //////////////////////////////////////////////


    ////////////////////////////////////////////////////////////GARIS PUTUS - PUTUS DI ASPAL.BMP/////////////////////////////////////////
    glPushMatrix();
    glTranslatef(220,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(200,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(180,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(160,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(140,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(120,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(100,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(80,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(60,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(40,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(20,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.20,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    
     glPushMatrix();
    glTranslatef(-20,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-40,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-60,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-80,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-100,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-120,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-140,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-160,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-180,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
                  glPushMatrix();
                  glTranslatef(-180,5,0.0); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
    
                  glPushMatrix();
                  glTranslatef(-180,5,0.-10); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                        
                  glPushMatrix();
                  glTranslatef(-180,5,0.-20); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5,0.-30); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5,0.-40); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                        
                  glPushMatrix();
                  glTranslatef(-180,5,0.-50); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5, -70); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
    
                  glPushMatrix();
                  glTranslatef(-180,5,0.-80); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                        
                  glPushMatrix();
                  glTranslatef(-180,5,0.-90); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5,0.-100); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5,0.-110); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();
                  
                  glPushMatrix();
                  glTranslatef(-180,5, -120); 
                  glScalef(3, 3, 3);
                  //glBindTexture(GL_TEXTURE_2D, texture[0]);
                  markajalan();
                  glPopMatrix();              
                  
    glPushMatrix();
    glTranslatef(-200,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    
    glPushMatrix();
    glTranslatef(-220,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-240,5,0.-60); 
    glScalef(3, 3, 3);
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    markajalan();
    glPopMatrix();
    

//////////////////////////////////////////////////////////// AHIR DARI GARIS PUTUS - PUTUS ////////////////////////////////////////////////  

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.3f, 0.9f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah,0.4613, 0.4627, 0.4174); ////////////////// WARNA ASPAL.BMP//////////////////////////////////////////////
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();
	
	
	

	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;
	


}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	_terrain = loadTerrain("jalan.bmp", 20);
	_terrainTanah = loadTerrain("aspal.bmp", 20);
	_terrainAir = loadTerrain("heightmapAir.bmp", 20);
   
	//binding texture

}

static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;

	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;

	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {

		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}





int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Sample Terain");
	init();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);


	glutKeyboardFunc(keyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
    
    
	glutMainLoop();
	return 0;
}
