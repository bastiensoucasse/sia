#pragma once

#include <Eigen/SparseCholesky>

#include "viewer.h"

typedef Eigen::SparseMatrix<float> SpMat;
typedef Eigen::PermutationMatrix<Eigen::Dynamic> Permutation;

class MeshProcessingApp : public Viewer
{
public:
    MeshProcessingApp();
    ~MeshProcessingApp();

    void init(int w, int h, int argc, char **argv);
    void updateScene();
    void updateGUI();

    void mousePressed(GLFWwindow *window, int button, int action, int mods);
    void mouseMoved(int x, int y);
    void charPressed(int key);

protected:
    bool pickAt(const Eigen::Vector2f &p, Hit &hit) const;
    bool selectAround(const Eigen::Vector2f &p) const;

    Mesh *_mesh;

    float _pickingRadius = 0.1;
    bool _pickingMode = false;
    int _interpolation{0};

    pmp::Vertex picked_v;

    SpMat L;
    Permutation perm;
    int nb_unknowns;
    // Eigen::SimplicialLDLT<SpMat> solver;
};
