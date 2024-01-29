#include "GlobalPlacer.h"
#include "ExampleFunction.h"
#include "NumericalOptimizer.h"
#include <cstdlib>
#include <iostream>

GlobalPlacer::GlobalPlacer(wrapper::Placement& placement)
    : _placement(placement)
{
}

vector<double> coordinates;

void GlobalPlacer::randomPlace()
{
    double coreWidth = _placement.boundryRight() - _placement.boundryLeft();
    double coreHeight = _placement.boundryTop() - _placement.boundryBottom();
    for (size_t i = 0; i < _placement.numModules(); ++i)
    {
        if (_placement.module(i).isFixed())
            continue;

        double width = _placement.module(i).width();
        double height = _placement.module(i).height();
        double x = rand() % static_cast<int>(coreWidth - width) + _placement.boundryLeft();
        double y = rand() % static_cast<int>(coreHeight - height) + _placement.boundryBottom();
        coordinates[2 * i] = x;
        coordinates[2 * i + 1] = y;
        _placement.module(i).setPosition(x, y);
    }
}

void GlobalPlacer::place()
{
    ///////////////////////////////////////////////////////////////////
    // The following example is only for analytical methods.
    // if you use other methods, you can skip and delete it directly.
    //////////////////////////////////////////////////////////////////
    ExampleFunction ef(_placement); // require to define the object function and gradient function
    coordinates.resize(ef.dimension()); // solution vector, size: num_blocks*2 
    NumericalOptimizer no(ef);// each 2 variables represent the X and Y dimensions of a block

    srand(time(NULL));
    randomPlace();
    no.setNumIteration(150);
    for (int j = 0; j < 6; j++) {
        ef.beta += 3000;
        no.setX(coordinates);//set initial solution
        no.setStepSizeBound((_placement.boundryRight() - _placement.boundryLeft()) * 0.9); // user-specified parameter
        no.solve(); // Conjugate Gradient solver
        int numModulesCount = _placement.numModules();
        for (int i = 0; i < numModulesCount; i++) {
            double moduleX = no.x(2 * i);
            double moduleY = no.x(2 * i + 1);if (!_placement.module(i).isFixed())
            {
                if (moduleX + _placement.module(i).width() > _placement.boundryRight()) {
                    moduleX = _placement.boundryRight() - _placement.module(i).width();
                }
                else if (moduleX - _placement.module(i).width() < _placement.boundryLeft()) {
                    moduleX = _placement.boundryLeft();
                }
            }
            else
            {
                moduleX = _placement.module(i).x();
            }

            if (!_placement.module(i).isFixed())
            {
                if (moduleY + _placement.module(i).height() > _placement.boundryTop()) {
                    moduleY = _placement.boundryTop() - _placement.module(i).height();
                }
                else if (moduleY - _placement.module(i).height() < _placement.boundryBottom()) {
                    moduleY = _placement.boundryBottom();
                }
            }
            else
            {
                moduleY = _placement.module(i).x();
            }


            _placement.module(i).setPosition(moduleX, moduleY);

            coordinates[2 * i] = moduleX;
            coordinates[2 * i + 1] = moduleY;
        }
    }
    ////////////////////////////////////////////////////////////////

    // An example of random placement implemented by TA.
    // If you want to use it, please uncomment the folllwing 1 line.


    /* @@@ TODO
     * 1. Understand above example and modify ExampleFunction.cpp to implement the analytical placement
     * 2. You can choose LSE or WA as the wirelength model, the former is easier to calculate the gradient
     * 3. For the bin density model, you could refer to the lecture notes
     * 4. You should first calculate the form of wirelength model and bin density model and the forms of their gradients ON YOUR OWN
     * 5. Replace the value of f in evaluateF() by the form like "f = alpha*WL() + beta*BinDensity()"
     * 6. Replace the form of g[] in evaluateG() by the form like "g = grad(WL()) + grad(BinDensity())"
     * 7. Set the initial vector x in place(), set step size, set #iteration, and call the solver like above example
     * */
}
