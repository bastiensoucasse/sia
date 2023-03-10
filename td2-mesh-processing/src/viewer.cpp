#include "viewer.h"
#include "camera.h"

#include <SOIL2.h>
#include <imgui.h>

using namespace Eigen;

Viewer::Viewer() {}

Viewer::~Viewer() {}

////////////////////////////////////////////////////////////////////////////////
// GL stuff

// initialize OpenGL context
void Viewer::init(int w, int h, int argc, char **argv) {
  _winWidth = w;
  _winHeight = h;

  std::cout << "Display:" << std::endl;
  std::cout << "  r:    reload shaders" << std::endl;
  std::cout << "  w:    enable/disable wireframe" << std::endl;
  std::cout << "  t:    enable/disable texturing" << std::endl;
  std::cout << "  g:    enable/disable cold-warm shading" << std::endl;
  std::cout << "Camera:" << std::endl;
  std::cout << "  mouse left+drag:      camera rotation" << std::endl;
  std::cout << "**************************************************" << std::endl
            << std::endl;

  // set the background color, i.e., the color used
  // to fill the screen when calling glClear(GL_COLOR_BUFFER_BIT)
  glClearColor(1.f, 1.f, 1.f, 1.f);

  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  loadProgram();

  _texid =
      SOIL_load_OGL_texture(DATA_DIR "/textures/rainbow.png", SOIL_LOAD_AUTO,
                            SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
  if (_texid == 0)
    std::cerr << "SOIL loading error: " << SOIL_last_result() << std::endl;

  AlignedBox3f aabb;
  for (auto obj : _meshes)
    aabb.extend(obj.mesh->boundingBox());

  reshape(w, h);
  _cam.setSceneCenter(aabb.center());
  _cam.setSceneRadius(aabb.sizes().maxCoeff());
  _cam.setSceneDistance(_cam.sceneRadius() * 3.f);
  _cam.setMinNear(0.1f);
  _cam.setNearFarOffsets(-_cam.sceneRadius() * 100.f,
                          _cam.sceneRadius() * 100.f);
}

void Viewer::reshape(int w, int h) {
  _winWidth = w;
  _winHeight = h;
  _cam.setScreenViewport(AlignedBox2f(Vector2f(0.0, 0.0), Vector2f(w, h)));
  glViewport(0, 0, w, h);
}

void Viewer::draw() {
  glViewport(0, 0, _winWidth, _winHeight);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Shader &prg = _main_shader;

  prg.activate();

  glUniform1i(prg.getUniformLocation("wireframe"), 0);
  glUniformMatrix4fv(prg.getUniformLocation("view_mat"), 1, GL_FALSE,
                     _cam.computeViewMatrix().data());
  glUniformMatrix4fv(prg.getUniformLocation("proj_mat"), 1, GL_FALSE,
                     _cam.computeProjectionMatrix().data());

  glUniform1i(prg.getUniformLocation("disp_selection"),
              _disp_selection ? 1 : 0);
  glUniform1i(prg.getUniformLocation("wireframe"), 0);

  glBindTexture(GL_TEXTURE_2D, _texid);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(prg.getUniformLocation("colormap"), 0);

  if (_texturing)
    glUniform1f(prg.getUniformLocation("tex_blend_factor"), 1.0);
  else
    glUniform1f(prg.getUniformLocation("tex_blend_factor"), 0.0);

  if (_coldwarm_shading) {
    glUniform3f(prg.getUniformLocation("CoolColor"), 0.15, 0.23, 0.3);
    glUniform3f(prg.getUniformLocation("WarmColor"), 0.6, 0.6, 0.5);
  } else {
    glUniform3f(prg.getUniformLocation("CoolColor"), 0.3, 0.3, 0.3);
    glUniform3f(prg.getUniformLocation("WarmColor"), 0.7, 0.7, 0.7);
  }

  for (auto obj : _meshes) {
    if (obj.display) {
      setObjectMatrix(obj.transformation, prg);
      obj.mesh->draw(prg);
    }
  }

  if (_wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_LINE_SMOOTH);
    glDepthFunc(GL_LEQUAL);
    glUniform1i(prg.getUniformLocation("wireframe"), 1);

    glUniform3fv(prg.getUniformLocation("color"), 1,
                 Vector3f(0.4, 0.4, 0.8).data());

    for (auto obj : _meshes) {
      if (obj.display) {
        setObjectMatrix(obj.transformation, prg);
        obj.mesh->draw(prg);
      }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUniform1i(prg.getUniformLocation("wireframe"), 0);
  }

  prg.deactivate();
}

void Viewer::setObjectMatrix(const Matrix4f &M, Shader &prg) const {
  glUniformMatrix4fv(prg.getUniformLocation("obj_mat"), 1, GL_FALSE, M.data());
  Matrix4f matLocal2Cam = _cam.computeViewMatrix() * M;
  Matrix3f matN = matLocal2Cam.topLeftCorner<3, 3>().inverse().transpose();
  glUniformMatrix3fv(prg.getUniformLocation("normal_mat"), 1, GL_FALSE,
                     matN.data());
}

void Viewer::updateScene() { draw(); }

void Viewer::loadProgram() {
  _main_shader.loadFromFiles(DATA_DIR "/shaders/main.vert",
                             DATA_DIR "/shaders/main.frag");
}

void Viewer::updateGUI() {
  ImGui::Checkbox("Wireframe", &_wireframe);
  ImGui::Checkbox("Texturing", &_texturing);
  ImGui::Checkbox("Cold-Warm", &_coldwarm_shading);
}

////////////////////////////////////////////////////////////////////////////////
// Events

/*
   callback to manage mouse : called when user press or release mouse button
   You can change in this function the way the user
   interact with the system.
 */
void Viewer::mousePressed(GLFWwindow *window, int button, int action,
                          int mods) {
  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      _cam.startRotation(_lastMousePos.cast<float>());
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      _cam.startTranslation(_lastMousePos.cast<float>());
    }
    _button = button;
  } else if (action == GLFW_RELEASE) {
    if (_button == GLFW_MOUSE_BUTTON_LEFT) {
      _cam.endRotation();
    } else if (_button == GLFW_MOUSE_BUTTON_RIGHT) {
      _cam.endTranslation();
    }
    _button = -1;
  }
}

/*
   callback to manage mouse : called when user move mouse with button pressed
   You can change in this function the way the user
   interact with the system.
 */
void Viewer::mouseMoved(int x, int y) {
  if (_button == GLFW_MOUSE_BUTTON_LEFT) {
    _cam.dragRotate(Vector2f(x, y));
  } else if (_button == GLFW_MOUSE_BUTTON_RIGHT) {
    _cam.dragTranslate(Vector2f(x, y));
  }
  _lastMousePos = Vector2i(x, y);
}

void Viewer::mouseScroll(double x, double y) {
  _cam.zoom((y > 0) ? 1.1 : 1. / 1.1);
}

/*
   callback to manage keyboard interactions
   You can change in this function the way the user
   interact with the system.
 */
void Viewer::keyPressed(int key, int action, int mods) {
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    loadProgram();
  }
}

void Viewer::charPressed(int key) {
  if (key == GLFW_KEY_S || key == 's') {
    _disp_selection = !_disp_selection;
    std::cout << "Display selection: "
              << (_disp_selection ? "enabled" : "disabled") << std::endl;
  } else if (key == GLFW_KEY_W || key == 'w') {
    _wireframe = !_wireframe;
    std::cout << "Wireframe: " << (_wireframe ? "enabled" : "disabled")
              << std::endl;
  } else if (key == GLFW_KEY_G || key == 'g') {
    _coldwarm_shading = !_coldwarm_shading;
    std::cout << "Cold-warm shading: "
              << (_coldwarm_shading ? "enabled" : "disabled") << std::endl;
  } else if (key == GLFW_KEY_T || key == 't') {
    _texturing = !_texturing;
    std::cout << "Texturing: " << (_texturing ? "enabled" : "disabled")
              << std::endl;
  }
}
