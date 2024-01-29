#ifndef EXAMPLEFUNCTION_H
#define EXAMPLEFUNCTION_H

#include "NumericalOptimizerInterface.h"
#include "Wrapper.hpp"

class ExampleFunction : public NumericalOptimizerInterface
{
public:
    //ExampleFunction();
    ExampleFunction(wrapper::Placement& placement);

    void evaluateFG(const vector<double> &x, double &f, vector<double> &g);
    void evaluateF(const vector<double> &x, double &f);
    unsigned dimension();

	int beta;
	int binNumPerEdge;
	int binNum;
	double r;
	double Width;
	double Height;
	double dBinW;
	double dBinH;
	double targetDensity;
	double* g_temp;
	double* eExp;
	vector<double> binDensity;
private:
	wrapper::Placement& _placement;
};

#endif // EXAMPLEFUNCTION_H
