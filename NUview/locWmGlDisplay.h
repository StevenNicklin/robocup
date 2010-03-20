#ifndef LOCWMGLDISPLAY_H
#define LOCWMGLDISPLAY_H

#include <QGLWidget>
#include <Vector>
#include <cmath>

struct point3d
{
    float x;
    float y;
    float z;
};

class locWmGlDisplay : public QGLWidget
{
Q_OBJECT
public:
    locWmGlDisplay(QWidget *parent);
    ~locWmGlDisplay();

    //! Returns the minimum desired size for the window
    QSize minimumSizeHint() const;
    //! Returns the most desired size for the window
    QSize sizeHint() const;
    void restoreState(const QByteArray & state);
    QByteArray saveState() const;

protected:
        void keyPressEvent ( QKeyEvent * e );
        void mousePressEvent ( QMouseEvent * event );
        void mouseMoveEvent ( QMouseEvent * event );
        void wheelEvent ( QWheelEvent * event );
        void LoadModels();
        void drawModel();
        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void drawField();

        bool loadTexture(QString fileName, GLuint* textureId);

        void drawGoal(QColor colour, float x, float y, float facing);
        void drawBall(QColor colour, float x, float y);
        void drawRobot(QColor colour, float x, float y, float theta);

        GLuint fieldLineTexture;
        GLuint grassTexture;
        GLuint robotTexture;
        GLuint robotBackTexture;
        GLUquadric* quadratic;
        float viewTranslation[3];
        float viewOrientation[3];
        QPoint dragStartPosition;
        QPoint prevDragPos;

        bool light;

        std::vector<point3d> points;
        std::vector<point3d> normals;
        std::vector<int> drawOrder;

        point3d NormaliseVector(point3d vector)
        {									// To A Unit Normal Vector With A Length Of One.
                float length;							// Holds Unit Length
                // Calculates The Length Of The Vector
                length = (float)sqrt((vector.x*vector.x) + (vector.y*vector.y) + (vector.z*vector.z));

                if(length == 0.0f)						// Prevents Divide By 0 Error By Providing
                        length = 1.0f;						// An Acceptable Value For Vectors To Close To 0.

                vector.x /= length;						// Dividing Each Element By
                vector.y /= length;						// The Length Results In A
                vector.z /= length;						// Unit Normal Vector.
                return vector;
        }


        point3d calculateNormal(point3d a, point3d b, point3d c)
        {
            point3d v1, v2, result;
            v1.x = b.x - a.x;
            v1.y = b.y - a.y;
            v1.z = b.z - a.z;

            v2.x = c.x - a.x;
            v2.y = c.y - a.y;
            v2.z = c.z - a.z;

            v1 = NormaliseVector(v1);
            v2 = NormaliseVector(v2);
            result.x = v1.y*v2.z - v1.z*v2.y;
            result.y = v1.z*v2.x - v1.x*v2.z;
            result.z = v1.x*v2.y - v1.y*v2.x;

            return NormaliseVector(result);
        }

};

#endif // LOCWMGLDISPLAY_H
