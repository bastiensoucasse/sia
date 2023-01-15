#include "mesh_processing_app.h"

#include <imgui.h>

#include "laplacian.h"
#include "neighborhood.h"

using namespace Eigen;
using namespace pmp;

MeshProcessingApp::MeshProcessingApp() : Viewer(), _mesh(0) {}

MeshProcessingApp::~MeshProcessingApp() { delete _mesh; }

void MeshProcessingApp::init(int w, int h, int argc, char **argv)
{
    std::cout << "Usage: " << argv[0] << " [file]" << std::endl;
    std::cout << "  where must be a mesh file (e.g. .off) " << std::endl
              << std::endl;
    std::cout << "**************************************************" << std::endl;
    std::cout << "Selection:" << std::endl;
    std::cout << "  ctrl+left+drag: selection" << std::endl;
    std::cout << "  c:    clear the selection" << std::endl;
    std::cout << "  s:    enable/disable display of selected vertices" << std::endl;
    std::cout << "Mesh processing:" << std::endl;
    std::cout << "  y/Y:  decrease/increase selection brush size" << std::endl
              << std::endl;
    std::cout << "  1:    perform harmonic interpolation" << std::endl;
    std::cout << "  2:    perform bi-harmonic interpolation" << std::endl;
    std::cout << "  3:    perform tri-harmonic interpolation" << std::endl;

    _mesh = new Mesh();
    if (argc > 1)
        _mesh->load(argv[1]);
    else
    {
        // _mesh->load(DATA_DIR"/models/sphere.off");
        _mesh->load(DATA_DIR "/models/bunny70k.off");
        // _mesh->load(DATA_DIR "/models/colorfull.off");
        // _mesh->load(DATA_DIR "/models/bunny_70k_color.off");
    }
    _mesh->init();
    _meshes.push_back(Object3D(_mesh, &_main_shader));
    int n = _mesh->n_vertices();
    L = SparseMatrix<float>(n, n);
    create_laplacian_matrix(*_mesh, L, true);

    _pickingRadius = 0.03 * _mesh->boundingBox().sizes().maxCoeff();

    Viewer::init(w, h, argc, argv);
}

void MeshProcessingApp::updateGUI()
{
    Viewer::updateGUI();
    ImGui::Separator();
    ImGui::Checkbox("Draw selection", &_disp_selection);
    ImGui::InputFloat("radius", &_pickingRadius, 0.5f);
    if (ImGui::Button("Clear selection"))
    {
        _mesh->masks().setZero();
        _mesh->updateVBO();
    }
    ImGui::Separator();
    const char *items[] = {"harmonic", "bi-harmonic", "tri-harmonic"};
    ImGui::Combo("interp.", (int *)&_interpolation, items, 3);
    if (ImGui::Button("Process"))
    {
        poly_harmonic_interpolation(*_mesh, L, perm, _mesh->positions(), nb_unknowns, _interpolation + 1);
        _mesh->updateAll();
    }
}

bool MeshProcessingApp::pickAt(const Eigen::Vector2f &p, Hit &hit) const
{
    Matrix4f proj4 = _cam.computeProjectionMatrix();
    Matrix3f proj3;
    proj3 << proj4.topLeftCorner<2, 3>(), proj4.bottomLeftCorner<1, 3>();
    Matrix4f C = _cam.computeViewMatrix().inverse();

    Vector3f q((2.0 * float(p.x() + 0.5f) / float(_winWidth) - 1.), -(2.0 * float(p.y() + 0.5f) / float(_winHeight) - 1.), 1);

    Ray ray;
    ray.origin = C.col(3).head<3>();
    ray.direction = C.topLeftCorner<3, 3>() * (proj3.inverse() * q);
    return _mesh->intersect(ray, hit);
}

bool MeshProcessingApp::selectAround(const Eigen::Vector2f &p) const
{
    Hit hit;
    if (pickAt(p, hit))
    {
        int clostest_id = 0;
        hit.baryCoords().maxCoeff(&clostest_id);
        pmp::Vertex closest_v(_mesh->faceIndices()(clostest_id, hit.faceId()));
        std::vector<pmp::Vertex> neighbors =
            select_neighbors(*_mesh, closest_v, _pickingRadius);
        auto masks = _mesh->get_vertex_property<int>("v:mask");
        for (auto v : neighbors)
            masks[v] = 1;
        return true;
    }
    return false;
}

void MeshProcessingApp::mousePressed(GLFWwindow *window, int button, int action, int mods)
{
    if ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL)
    {
        if (action == GLFW_PRESS)
        {
            _pickingMode = true;
            _disp_selection = true;
            if (selectAround(_lastMousePos.cast<float>()))
                _mesh->updateVBO();
        }
        else
        {
            _pickingMode = false;
            nb_unknowns = create_permutation(*_mesh, perm);
        }
    }
    else if ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT)
    {
        if (action == GLFW_PRESS)
        {
            Hit hit;
            if (pickAt(_lastMousePos.cast<float>(), hit))
            {
                int clostest_id = 0;
                hit.baryCoords().maxCoeff(&clostest_id);
                picked_v = pmp::Vertex(_mesh->faceIndices()(clostest_id, hit.faceId()));
                auto masks = _mesh->get_vertex_property<int>("v:mask");
                if (masks[picked_v] != 1)
                    picked_v = pmp::Vertex();
                masks[picked_v] = 0;
                std::cout << "Picked: " << picked_v << std::endl;
            }
        }
    }
    else
        Viewer::mousePressed(window, button, action, mods);
}

void MeshProcessingApp::mouseMoved(int x, int y)
{
    if (_pickingMode)
    {
        if (selectAround(Vector2f(x, y)))
            _mesh->updateVBO();
    }
    else
        Viewer::mouseMoved(x, y);
    _lastMousePos = Vector2i(x, y);
}

void MeshProcessingApp::updateScene() { Viewer::updateScene(); }

void MeshProcessingApp::charPressed(int key)
{
    if (key == GLFW_KEY_Y)
    {
        _pickingRadius *= 1.2;
    }
    else if (key == 'y')
    {
        _pickingRadius /= 1.2;
    }
    else if (key == 'c')
    {
        _mesh->masks().setZero();
        _mesh->updateVBO();
    }
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_3)
    {
        int k = key - GLFW_KEY_1 + 1;
        poly_harmonic_interpolation(*_mesh, L, perm, _mesh->positions(), nb_unknowns, k);
        // poly_harmonic_interpolation(*_mesh, L, perm, _mesh->colors(), nb_unknowns, k);
        _mesh->updateAll();
    }
    else if (key == '-' || key == '+')
    {
        if (picked_v.idx() >= 0)
        {
            if (key == '-') _mesh->shifts()[picked_v.idx()] -= 10.f;
            if (key == '+') _mesh->shifts()[picked_v.idx()] += 10.f;
            poly_harmonic_interpolation(*_mesh, L, perm, _mesh->shifts(), nb_unknowns, 2);
            _mesh->updateAll();
        }
    }
    else
        Viewer::charPressed(key);
}
