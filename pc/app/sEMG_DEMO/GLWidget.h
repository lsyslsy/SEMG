#pragma once

#include <QtGui>
#include <QGLWidget>

class GLWidget : public QGLWidget
{
	Q_OBJECT
public:
	GLWidget(QWidget *parent = 0, QGLWidget *shareWidget = 0);
	~GLWidget();
   protected:
	   void initializedGL();
	   void resizeGL(int width, int height);
	   void paintGL();
	   void mousePressEvent(QMouseEvent *event);
	   void mouseMoveEvent (QMouseEvent *event);
   private:
      void draw();
	  QColor faceColors[6];
     double xRot;
     double yRot;
     double zRot;
	 QPoint lastPos;
};
