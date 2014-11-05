#ifdef UNICODE
	#undef UNICODE
#endif
#include <QtGui>
#include <QtOpenGL>
#include "GLWidget.h"
 
GLWidget::GLWidget(QWidget *parent, QGLWidget *shareWidget)
    : QGLWidget(parent, shareWidget)
{
	setFormat(QGLFormat(QGL::DoubleBuffer|QGL::DepthBuffer));
    xRot = 0.0;
	yRot = 0.0;
	zRot = 0.0;
	faceColors[0]=Qt::red;
	faceColors[1]=Qt::green;
	faceColors[2]=Qt::blue;
	faceColors[3]=Qt::yellow;
	faceColors[4]=Qt::magenta;
	faceColors[5]=Qt::cyan;
}

GLWidget::~GLWidget()
{

}

void GLWidget::initializedGL()
{
	qglClearColor(Qt::black);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void GLWidget::resizeGL(int width, int height)
{
  glViewport(0,0,width,height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double x = double(width)/height;
  glFrustum(-x,+x,-1.0,+1.0,4.0,15.0);
  glMatrixMode(GL_MODELVIEW);
}

void GLWidget::paintGL()
{
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw();
}

void GLWidget::draw()
{
	//static const double P1[3]={0.0,-1.0,+2.0};
	//static const double P2[3]={+1.732,-1.0,-1.0};
	//static const double P3[3]={-1.732,-1.0,-1.0};
	//static const double P4[3]={0.0,+2.0,0.0};
	
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0,0.0,-10.0);
	//画x轴 
	glColor3f(1.0f,0.0f,0.0f);  // 颜色改成红色
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(2,0,0); 
	glEnd();
    //画y轴 
	glColor3f(0.0f,1.0f,0.0f);  // 颜色改成绿色
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(0,2,0);
	glEnd();
	//画Z轴 
	glColor3f(0.0f,0.0f,1.0f);  // 颜色改成蓝色
	glBegin(GL_LINES);
	glVertex3f(0,0,0);
	glVertex3f(-1.414,-1.414,0);   
	glEnd();
	
;
	//static const double P1[3]={2.0,0.0,0.0};
	//static const double P2[3]={0.0,2.0,0.0};
	//static const double P3[3]={0.0,0.0,2.0};
	//static const double P4[3]={0.0,0.0,0.0};
    //static const double* const coords[4][3] ={
		//{P1,P2,P3},{P1,P2,P4},{P1,P3,P4},{P2,P3,P4}};
	 static const double coords[6][4][3] = {
        { { +1.5, -0.25, -1 }, { -1.5, -0.25, -1 }, { -1.5, +0.25, -1 }, { +1.5, +0.25, -1 } },
        { { +1.5, +0.25, -1 }, { -1.5, +0.25, -1 }, { -1.5, +0.25, +1 }, { +1.5, +0.25, +1 } },
        { { +1.5, -0.25, +1 }, { +1.5, -0.25, -1 }, { +1.5, +0.25, -1 }, { +1.5, +0.25, +1 } },
        { { -1.5, -0.25, -1 }, { -1.5, -0.25, +1 }, { -1.5, +0.25, +1 }, { -1.5, +0.25, -1 } },
        { { +1.5, -0.25, +1 }, { -1.5, -0.25, +1 }, { -1.5, -0.25, -1 }, { +1.5, -0.25, -1 } },
        { { -1.5, -0.25, +1 }, { +1.5, -0.25, +1 }, { +1.5, +0.25, +1 }, { -1.5, +0.25, +1 } }
    };
	glRotatef(xRot,1.0,0.0,0.0);
	glRotatef(yRot,0.0,1.0,0.0);
	glRotatef(zRot,0.0,0.0,1.0);

	for(int i=0 ;i < 6;++i)
	{
	    
		//glLoadName(i);
		//glBegin(GL_TRIANGLES);
		glBegin(GL_QUADS);
		qglColor(faceColors[i]);     
		for(int j = 0;j<4;++j)
		{
        	glVertex3f(coords[i][j][0],coords[i][j][1],coords[i][j][2]);
		}
		glEnd();
	}
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event ->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    double dx =double(event->x()-lastPos.x())/width();
	double dy =double(event->y()-lastPos.y())/height();
	if (event->buttons()&Qt::LeftButton){
	    xRot += 180*dy;
	    yRot += 180*dx;
	    updateGL();
	}
	else if(event->buttons()& Qt::RightButton){
           xRot += 180*dy;
	       zRot += 180*dx;
	       updateGL();
	}
	lastPos = event->pos();
}
