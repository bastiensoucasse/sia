#include "viewer.h"

#include <imgui.h>
#include <SOIL2.h>

#include "camera.h"
#include "collider.h"

#define DRAW_PARTICLE_SYSTEM false

using namespace Eigen;

#pragma region OpenGL Methods

void Viewer::init(int w, int h)
{
    _winWidth = w;
    _winHeight = h;

    glClearColor(1.f, 1.f, 1.f, 1.f);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loadProgram();

    /// Ground
    _ground = new Mesh();
    _ground->createGrid(2, 2);
    _ground->init();
    _ground->transformationMatrix() = Translation3f(-500.f, 0.f, -500.f) * Scaling(1000.f, 1.f, 1000.f) * AngleAxisf(M_PI / 2.f, Vector3f::UnitX());

    /// Lightning
    _lightPos = Vector4f(1.f, 0.75f, 1.f, 1.f).normalized();
    _lightColor = Vector3f(1.f, 1.f, 1.f);

    /// Girl
    _girl = new Mesh();
    _girl->load(DATA_DIR "/models/girl.obj");
    _girl->init();

    /// Cloth
    _cloth = new Mesh();
    _psys.init();
}

void Viewer::reshape(int w, int h)
{
    _winWidth = w;
    _winHeight = h;
    _cam.setScreenViewport(AlignedBox2f(Vector2f(0.0, 0.0), Vector2f(w, h)));
    glViewport(0, 0, w, h);
}

void Viewer::updateScene()
{
    if (_resetSimu)
    {
        _psys.clear();
        // _psys.makeGrid();
        _cloth->load(DATA_DIR "/models/cloth.obj");
        _cloth->init();
        _cloth->instanciateParticleSystem(_psys);

        _mouseForce = new AnchorForce(nullptr, Vector3d::Zero(), 1000, 1);
        _psys.forces.push_back(_mouseForce);
        _psys.forces.push_back(new DragForce(&_psys, 0.01));
        _psys.forces.push_back(new GravityForce(&_psys, Vector3d(0, -9.807, 0)));

        _psys.colliders.push_back(new PlaneCollider(Vector3d::Zero(), Vector3d::UnitY().normalized(), .5));
        _psys.colliders.push_back(new MeshCollider(_girl, 1.));

        _psys.updateGL(true);

        _cam.setSceneCenter(_psys.boundingBox().center());
        _cam.setSceneRadius(_psys.boundingBox().sizes().maxCoeff());
        _cam.setSceneDistance(_cam.sceneRadius() * 3.f);
        _cam.setMinNear(0.1f);
        _cam.setNearFarOffsets(-_cam.sceneRadius() * 100.f, _cam.sceneRadius() * 100.f);
        _cam.setScreenViewport(AlignedBox2f(Vector2f(0.0, 0.0), Vector2f(_winWidth, _winHeight)));
        _resetSimu = false;
    }

    if (_simulation)
    {
        double currentTime = glfwGetTime();
        while (glfwGetTime() - currentTime < 1.f / 60.f)
            _psys.step(_dt, _integration);
        _psys.updateGL();
    }

    _cloth->updatePos(_psys);
    draw();
}

void Viewer::updateGUI()
{
    ImGui::Checkbox("Simulation", &_simulation);
    if (ImGui::Button("Reset"))
        _resetSimu = true;
    const char *items[] = {"Explicit Euler", "Mid-Point", "Backward Euler"};
    ImGui::Combo("integration", (int *)&_integration, items, 3);
    ImGui::InputFloat("dt", &_dt, 0.0001f, 0.f, "%.4f");
}

void Viewer::loadProgram()
{
    _blinnPrg.loadFromFiles(DATA_DIR "/shaders/blinn.vert", DATA_DIR "/shaders/blinn.frag");
    _simplePrg.loadFromFiles(DATA_DIR "/shaders/simple.vert", DATA_DIR "/shaders/simple.frag");
}

void Viewer::draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (DRAW_PARTICLE_SYSTEM)
    {
        _simplePrg.activate();

        /// Camera
        glUniformMatrix4fv(_simplePrg.getUniformLocation("projection_matrix"), 1, GL_FALSE, _cam.computeProjectionMatrix().data());
        glUniformMatrix4fv(_simplePrg.getUniformLocation("view_matrix"), 1, GL_FALSE, _cam.computeViewMatrix().data());

        /// Particle System
        glUniformMatrix4fv(_simplePrg.getUniformLocation("model_matrix"), 1, GL_FALSE, _psys.transformationMatrix().data());
        glUniform3fv(_simplePrg.getUniformLocation("col"), 1, Vector3f::Zero().eval().data());
        _psys.draw(_simplePrg);

        _simplePrg.deactivate();
    }

    _blinnPrg.activate();

    /// Camera
    glUniformMatrix4fv(_blinnPrg.getUniformLocation("projection_matrix"), 1, GL_FALSE, _cam.computeProjectionMatrix().data());
    glUniformMatrix4fv(_blinnPrg.getUniformLocation("view_matrix"), 1, GL_FALSE, _cam.computeViewMatrix().data());

    /// Lightning
    glUniform4fv(_blinnPrg.getUniformLocation("light_pos_view"), 1, _lightPos.data());
    glUniform3fv(_blinnPrg.getUniformLocation("light_color"), 1, _lightColor.data());
    glUniform1f(_blinnPrg.getUniformLocation("specular_coef"), 0);

    /// Ground
    glUniformMatrix4fv(_blinnPrg.getUniformLocation("model_matrix"), 1, GL_FALSE, _ground->transformationMatrix().data());
    glUniformMatrix3fv(_blinnPrg.getUniformLocation("normal_matrix"), 1, GL_FALSE, Matrix3f((_cam.computeViewMatrix() * _ground->transformationMatrix()).linear().inverse().transpose()).data());
    _ground->draw(_blinnPrg);

    /// Girl
    glUniformMatrix4fv(_blinnPrg.getUniformLocation("model_matrix"), 1, GL_FALSE, _girl->transformationMatrix().data());
    glUniformMatrix3fv(_blinnPrg.getUniformLocation("normal_matrix"), 1, GL_FALSE, Matrix3f((_cam.computeViewMatrix() * _girl->transformationMatrix()).linear().inverse().transpose()).data());
    _girl->draw(_blinnPrg);

    /// Cloth
    glUniformMatrix4fv(_blinnPrg.getUniformLocation("model_matrix"), 1, GL_FALSE, _cloth->transformationMatrix().data());
    glUniformMatrix3fv(_blinnPrg.getUniformLocation("normal_matrix"), 1, GL_FALSE, Matrix3f((_cam.computeViewMatrix() * _cloth->transformationMatrix()).linear().inverse().transpose()).data());
    _cloth->draw(_blinnPrg);

    _blinnPrg.deactivate();
}

#pragma endregion

#pragma region Events

void Viewer::mousePressed(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (mods == GLFW_MOD_CONTROL)
            {
                Matrix4f proj4 = _cam.computeProjectionMatrix();
                Matrix4f view4 = _cam.computeViewMatrix();
                Matrix4f C = view4.inverse();
                Matrix3f proj3;
                proj3 << proj4.topLeftCorner<2, 3>(), proj4.bottomLeftCorner<1, 3>();
                Vector2f proj_pos = Vector2f(2.f * float(_lastMousePos[0] + 0.5f) / float(_winWidth) - 1.0, -(2.f * float(_lastMousePos[1] + 0.5f) / float(_winHeight) - 1.0));
                // find nearest particle
                double dmin = 0.2; // ignore particles farther than 0.2
                for (int i = 0; i < _psys.particles.size(); i++)
                {
                    Particle *p = _psys.particles[i];
                    Vector4f pos;
                    pos << p->x.cast<float>(), 1.f;
                    Vector4f pos_p = (proj4 * view4 * pos);
                    Vector3f dir;
                    dir << pos_p.head<2>() / pos_p[3] - proj_pos, 0;
                    double d = dir.head<2>().norm();
                    if (d < dmin)
                    {
                        _mouseForce->x =
                            p->x - (C.topLeftCorner<3, 3>() * (proj3.inverse() * dir))
                                       .cast<double>();
                        _mouseForce->p = p;
                        dmin = d;
                    }
                }
                _mod = true;
            }
            else
            {
                _cam.startRotation(_lastMousePos);
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            _cam.startTranslation(_lastMousePos);
        }
        _button = button;
    }
    else if (action == GLFW_RELEASE)
    {
        if (!_mod)
        {
            if (_button == GLFW_MOUSE_BUTTON_LEFT)
            {
                _cam.endRotation();
            }
            else if (_button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                _cam.endTranslation();
            }
        }
        _mouseForce->p = nullptr;
        _mod = false;
        _button = -1;
    }
}

void Viewer::mouseMoved(int x, int y)
{
    if (_button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (_mod)
        {
            Matrix4f proj4 = _cam.computeProjectionMatrix();
            Matrix4f view4 = _cam.computeViewMatrix();
            Matrix4f C = view4.inverse();
            Matrix3f proj3;
            proj3 << proj4.topLeftCorner<2, 3>(), proj4.bottomLeftCorner<1, 3>();

            Vector2f proj_pos =
                Vector2f(2.f * float(x + 0.5f) / float(_winWidth) - 1.0, -(2.f * float(y + 0.5f) / float(_winHeight) - 1.0));

            Particle *p = _mouseForce->p;
            if (p)
            {
                Vector4f pos;
                pos << p->x.cast<float>(), 1.f;
                Vector4f pos_p = (proj4 * view4 * pos);
                Vector3f dir;
                dir << pos_p.head<2>() / pos_p[3] - proj_pos.head<2>(), 0;
                _mouseForce->x =
                    p->x -
                    (C.topLeftCorner<3, 3>() * (proj3.inverse() * dir)).cast<double>();
            }
        }
        else
        {
            _cam.dragRotate(Vector2f(x, y));
        }
    }
    else if (_button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        _cam.dragTranslate(Vector2f(x, y));
    }
    _lastMousePos = Vector2f(x, y);
}

void Viewer::mouseScroll(double x, double y)
{
    _cam.zoom((y > 0) ? 1.1 : 1. / 1.1);
}

void Viewer::keyPressed(int key, int action, int mods)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        loadProgram();
    }
    else if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        _simulation = !_simulation;
    }
}

void Viewer::charPressed(int key) {}

#pragma endregion
