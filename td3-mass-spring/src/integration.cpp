#include "integration.h"

using namespace Eigen;

void explicitEulerStep(ODESystem *system, double dt)
{
    int d = system->getDimensions();
    VectorXd state(d), deriv(d);
    system->getState(state);
    system->getDerivative(deriv);
    system->setState(state + dt * deriv);
}

void midPointStep(ODESystem *system, double dt)
{
    int d = system->getDimensions();
    VectorXd state(d), deriv(d);
    system->getState(state);
    system->getDerivative(deriv);
    system->setState(state + (dt * deriv) / 2);
    system->getDerivative(deriv);
    system->setState(state + dt * deriv);
}
