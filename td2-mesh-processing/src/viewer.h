#pragma once

#include "camera.h"
#include "mesh.h"
#include "opengl.h"
#include "shader.h"

#include <iostream>

struct Object3D {
  Object3D(Mesh *m, Shader *s) : mesh(m), prg(s) {}
  Mesh *mesh = 0;
  Eigen::Matrix4f transformation = Eigen::Matrix4f::Identity();
  Shader *prg = 0;
  bool display = true;
};

class Viewer {
public:
  //! Constructor
  Viewer();
  virtual ~Viewer();

  // gl stuff
  virtual void init(int w, int h, int argc, char **argv);
  virtual void updateScene();
  virtual void updateGUI();
  void reshape(int w, int h);

  // events
  virtual void mousePressed(GLFWwindow *window, int button, int action, int mods);
  virtual void mouseMoved(int x, int y);
  virtual void mouseScroll(double x, double y);
  virtual void keyPressed(int key, int action, int mods);
  virtual void charPressed(int key);

protected:
  void loadProgram();
  void draw();
  void setObjectMatrix(const Eigen::Matrix4f &M, Shader &prg) const;

  int _winWidth, _winHeight;

  Camera _cam;

  // the default shader program
  Shader _main_shader;
  int _texid;

  // list of meshes to display
  std::vector<Object3D> _meshes;

  bool _disp_selection{true};
  bool _wireframe{false};
  bool _texturing{false};
  bool _coldwarm_shading{true};

  // mouse parameters
  Eigen::Vector2i _lastMousePos;
  int _button = -1;
  bool _mod{false};
};
