#pragma once

#include <iostream>

#include "camera.h"
#include "forces.h"
#include "mesh.h"
#include "opengl.h"
#include "particles.h"
#include "shader.h"

class Viewer
{
public:
    inline Viewer() {}
    inline virtual ~Viewer() {}

    void init(int w, int h);
    void reshape(int w, int h);
    void updateScene();
    void updateGUI();

    void mousePressed(GLFWwindow *window, int button, int action, int mods);
    void mouseMoved(int x, int y);
    void mouseScroll(double x, double y);
    void keyPressed(int key, int action, int mods);
    void charPressed(int key);

protected:
    void loadProgram();
    void draw();

private:
    int _winWidth, _winHeight;

    Camera _cam;
    Shader _blinnPrg, _simplePrg;
    Eigen::Vector4f _lightPos;
    Eigen::Vector3f _lightColor;

    Mesh *_ground;
    Mesh *_girl;
    Mesh *_cloth;

    ParticleSystem _psys;
    AnchorForce *_mouseForce;
    bool _simulation{true}, _resetSimu{true};
    Integration _integration{EULER};
    float _dt{0.0001f};

    Eigen::Vector2f _lastMousePos;
    int _button = -1;
    bool _mod{false};
};
