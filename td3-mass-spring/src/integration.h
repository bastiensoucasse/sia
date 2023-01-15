#pragma once

#include <Eigen/Dense>

enum Integration
{
    EULER,
    MID_POINT
};

class ODESystem
{
public:
    /// @brief Gets the number of degrees of freedom.
    /// @return The number of degrees of freedom.
    virtual int getDimensions() = 0;

    /// @brief Writes the states into a vector.
    /// @param state The vector to write into.
    virtual void getState(Eigen::VectorXd &states) = 0;

    /// @brief Reads and applies the states from a vector.
    /// @param state The vector to read from.
    virtual void setState(const Eigen::VectorXd &states) = 0;

    /// @brief Writes the state derivatives into a vector.
    /// @param derivatives The vector to write into.
    virtual void getDerivative(Eigen::VectorXd &derivatives) = 0;
};

/// @brief Performs an explicit Euler step.
/// @param system The system to apply it to.
/// @param dt The length of the step.
void explicitEulerStep(ODESystem *system, double dt);

/// @brief Performs an explicit MidPoint step.
/// @param system The system to apply it to.
/// @param dt The length of the step
void midPointStep(ODESystem *system, double dt);
