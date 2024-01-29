#include "ExampleFunction.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "Wrapper.hpp"
// minimize 3*x^2 + 2*x*y + 2*y^2 + 7

ExampleFunction::ExampleFunction(wrapper::Placement& placement)
	:_placement(placement)
{
	beta = 0;
	//chip的寬高
	Width = _placement.boundryRight() - _placement.boundryLeft();
	Height = _placement.boundryTop() - _placement.boundryBottom();
	 
	//r = 500 越大代表更加自信地選擇一個類別，可以使用較小的模型預測更加均勻
	//也就是如果r越小可以讓module分布更均勻
	r = 500;
	binNumPerEdge = 10;  //每個邊bin的數量
	dBinW = Width / binNumPerEdge;  //bin寬
	dBinH = Height / binNumPerEdge;  //bin高
	binNum = binNumPerEdge * binNumPerEdge;  //bin的數量

	g_temp = new double[_placement.numModules() * 2]();
	eExp = new double[_placement.numModules() * 4]();  //exp(Xk/r)

	targetDensity = 0;  //module密度
	for (unsigned i = 0; i < _placement.numModules(); ++i) {
		targetDensity += _placement.module(i).area();
	}
	targetDensity = targetDensity / (Width * Height);
	binDensity.resize(binNum);
}

void ExampleFunction::evaluateFG(const vector<double>& x, double& f, vector<double>& g)
{
	double x1, x2, y1, y2; //summation of each exp 
	double overlapX, overlapY, alphaX, betaX, alphaY, betaY, dX, dY, ABSdX, ABSdY;
	double densityRatio;
	//f is the objective cost function
	fill(g.begin(), g.end(), 0.0);  //梯度向量初始化為0
	f = 0;  //L
	int vectorGSize = g.size();
	int modulesCount = _placement.numModules();

	//calculate exp(Xk/r)
	//r=r
	for (int i = 0; i < modulesCount; i++) {
		if (_placement.module(i).isFixed()) { //如果是fixed module 
			wrapper::Module cur_module = _placement.module(i);
			eExp[4 * i] = exp(cur_module.centerX() / r);
			eExp[4 * i + 1] = exp(cur_module.centerX() * (-1) / r);
			eExp[4 * i + 2] = exp(cur_module.centerY() / r);
			eExp[4 * i + 3] = exp(cur_module.centerY() * (-1) / r);
		}
		else {
			eExp[4 * i] = exp(x[2 * i] / r);
			eExp[4 * i + 1] = exp(-x[2 * i] / r);
			eExp[4 * i + 2] = exp(x[2 * i + 1] / r);
			eExp[4 * i + 3] = exp(-x[2 * i + 1] / r);
		}
	}

	//calculate log-sum-exp and gradient
	for (unsigned i = 0; i < _placement.numNets(); i++) {
		x1 = x2 = y1 = y2 = 0;  //變數宣告
		//calculate inner Σexp(Xk/r) of LSE wirelength
		for (unsigned j = 0; j < _placement.net(i).numPins(); ++j) {  //計算net所有pin連接的module
			int cur_moduleID = _placement.net(i).pin(j).moduleId();
			x1 += eExp[4 * cur_moduleID];  //Σexp(Xk/r)
			x2 += eExp[4 * cur_moduleID + 1];  //Σexp(-Xk/r)
			y1 += eExp[4 * cur_moduleID + 2];  //Σexp(Yk/r)
			y2 += eExp[4 * cur_moduleID + 3];  //Σexp(-Yk/r)
		}
		//calculate r*Σ(ln(Σexp(Xk/r)) + ln(Σexp(-Xk/r) + ln(Σexp(Yk/r) + ln(Σexp(-Yk/r)) of LSE wirelength
		f += r * (log(x1) + log(x2) + log(y1) + log(y2)); //belinda

		//calculate gradient of objective function f
		for (unsigned j = 0; j < _placement.net(i).numPins(); j++) {
			wrapper::Module cur_module = _placement.module(i);
			int cur_moduleID = _placement.net(i).pin(j).moduleId();
			if (cur_module.isFixed()) {
				g[2 * cur_moduleID] = g[2 * cur_moduleID + 1] = 0;
			}
			else {
				g[2 * cur_moduleID] += eExp[4 * cur_moduleID] / (r * x1);
				g[2 * cur_moduleID] -= eExp[4 * cur_moduleID + 1] / (r * x2);
				g[2 * cur_moduleID + 1] += eExp[4 * cur_moduleID + 2] / (r * y1);
				g[2 * cur_moduleID + 1] -= eExp[4 * cur_moduleID + 3] / (r * y2);
			}
		}
	}


	//focus on minimize LSE wirelength, return whenever it is the first iteration 如果是第一次loop就不做後面先return
	if (beta == 0)	return;

	//bin density
	for (int i = 0; i < binNum; i++) {
		binDensity[i] = 0;
	}
	for (int i = 0; i < vectorGSize; i++) {
		g_temp[i] = 0;
	}

	for (int a = 0; a < binNumPerEdge; a++) {
		for (int b = 0; b < binNumPerEdge; b++) {
			for (int i = 0; i < modulesCount; i++) {
				wrapper::Module cur_module = _placement.module(i);
				if (!cur_module.isFixed()) {
					// a=4/(Wb+Wi)(2Wb+Wi) 
					// b=4/(Wb(2Wb+Wi))
					alphaX = 4 / ((dBinW + cur_module.width()) * (2 * dBinW + cur_module.width()));
					betaX = 4 / (dBinW * (2 * dBinW + cur_module.width()));
					alphaY = 4 / ((dBinH + cur_module.height()) * (2 * dBinH + cur_module.height()));
					betaY = 4 / (dBinH * (2 * dBinH + cur_module.height()));

					densityRatio = cur_module.area() / (dBinW * dBinH);

					dX = x[2 * i] - ((a + 0.5) * dBinW + _placement.boundryLeft());
					ABSdX = abs(dX);
					dY = x[2 * i + 1] - ((b + 0.5) * dBinH + _placement.boundryBottom());
					ABSdY = abs(dY);

					//bell-shaped function of bin density smoothing	for large module
					if (ABSdX <= (dBinW / 2 + cur_module.width() / 2) && ABSdX >= 0) {
						overlapX = 1 - alphaX * ABSdX * ABSdX;
					}
					else if (ABSdX <= (dBinW + cur_module.width() / 2)) {
						overlapX = betaX * pow(ABSdX - dBinW - cur_module.width() / 2, 2);
					}
					else {
						overlapX = 0;
					}
					if (ABSdY <= (dBinH / 2 + cur_module.height() / 2) && ABSdY >= 0) {
						overlapY = 1 - alphaY * ABSdY * ABSdY;
					}
					else if (ABSdY <= (dBinH + cur_module.height() / 2)) {
						overlapY = betaY * pow(ABSdY - dBinH - cur_module.height() / 2, 2);
					}
					else {
						overlapY = 0;
					}

					//bell-shaped function for large module differential
					// 1. -2.a.dx
					// 2. 2b*(d-Wb-Wi/2)
					if (ABSdX <= (dBinW / 2 + cur_module.width() / 2)) {
						g_temp[2 * i] = densityRatio * ((-2) * alphaX * dX) * overlapY;
					}
					else if (ABSdX <= (dBinW + cur_module.width() / 2)) {
						//g_temp[2 * i] = densityRatio * 2 * betaX * (dX - dBinW - cur_module.width() / 2) * overlapY;
						if (dX > 0) {
							g_temp[2 * i] = densityRatio * 2 * betaX * (dX - dBinW - cur_module.width() / 2) * overlapY;
						}
						else {
							g_temp[2 * i] = densityRatio * 2 * betaX * (dX + (dBinW + cur_module.width() / 2)) * overlapY;
						}
					}
					else {
						g_temp[2 * i] = 0;
					}

					if (ABSdY <= (dBinH / 2 + cur_module.height() / 2)) {
						g_temp[2 * i + 1] = densityRatio * ((-2) * alphaY * dY) * overlapX;
					}
					else if (ABSdY <= (dBinH + cur_module.height() / 2)) {
						//g_temp[2 * i + 1] = densityRatio * 2 * betaY * (dY - dBinH - cur_module.height() / 2) * overlapX;
						if (dY > 0) {
							g_temp[2 * i + 1] = densityRatio * 2 * betaY * (dY - (dBinH + cur_module.height() / 2)) * overlapX;
						}
						else {
							g_temp[2 * i + 1] = densityRatio * 2 * betaY * (dY + (dBinH + cur_module.height() / 2)) * overlapX;
						}
					}
					else {
						g_temp[2 * i + 1] = 0;
					}
					//calculate overlap length
					binDensity[binNumPerEdge * b + a] += densityRatio * overlapX * overlapY;
				}
			}
			//calculate (beta * Σ((Db(x, y)-Tb)^2)) part in objective function
			f += beta * pow(binDensity[binNumPerEdge * b + a] - targetDensity, 2);

			for (int j = 0; j < modulesCount; ++j) {
				g[2 * j] += beta * 2 * (binDensity[binNumPerEdge * b + a] - targetDensity) * g_temp[2 * j];
				g[2 * j + 1] += beta * 2 * (binDensity[binNumPerEdge * b + a] - targetDensity) * g_temp[2 * j + 1];
			}
		}
	}
}

void ExampleFunction::evaluateF(const vector<double>& x, double& f)
{
	double x1, x2, y1, y2;
	double overlapX, overlapY, alphaX, betaX, alphaY, betaY, dX, dY, ABSdX, ABSdY;
	double densityRatio;
	//f is the objective cost function
	f = 0;
	int modulesCount = _placement.numModules();
	//calculate exp(Xk/r)
	for (int i = 0; i < modulesCount; i++) {
		if (_placement.module(i).isFixed()) {
			wrapper::Module cur_module = _placement.module(i);
			eExp[4 * i] = exp(cur_module.centerX() / r);
			eExp[4 * i + 1] = exp((-1) * cur_module.centerX() / r);
			eExp[4 * i + 2] = exp(cur_module.centerY() / r);
			eExp[4 * i + 3] = exp((-1) * cur_module.centerY() / r);
		}
		else {
			eExp[4 * i] = exp(x[2 * i] / r);
			eExp[4 * i + 1] = exp((-1) * x[2 * i] / r);
			eExp[4 * i + 2] = exp(x[2 * i + 1] / r);
			eExp[4 * i + 3] = exp((-1) * x[2 * i + 1] / r);
		}
	}
	//calculate log-sum-exp and gradient
	for (unsigned i = 0; i < _placement.numNets(); i++) {
		x1 = x2 = y1 = y2 = 0;
		//calculate inner Σexp(Xk/r) of LSE wirelength
		for (unsigned j = 0; j < _placement.net(i).numPins(); ++j) {
			int cur_moduleID = _placement.net(i).pin(j).moduleId();
			x1 += eExp[4 * cur_moduleID];
			x2 += eExp[4 * cur_moduleID + 1];
			y1 += eExp[4 * cur_moduleID + 2];
			y2 += eExp[4 * cur_moduleID + 3];
		}
		//calculate Σ(ln(Σexp(Xk/r)) + ln(Σexp(-Xk/r) + ln(Σexp(Yk/r) + ln(Σexp(-Yk/r)) of LSE wirelength
		f += log(x1) + log(x2) + log(y1) + log(y2);

	}
	//focus on minimize LSE wirelength, return whenever it is the first iteration
	if (beta == 0)	return;

	for (int i = 0; i < binNum; i++) {
		binDensity[i] = 0;
	}
	for (int a = 0; a < binNumPerEdge; a++) {
		for (int b = 0; b < binNumPerEdge; b++) {
			for (int i = 0; i < modulesCount; i++) {
				wrapper::Module cur_module = _placement.module(i);
				if (!cur_module.isFixed()) {
					alphaX = 4 / ((dBinW + cur_module.width()) * (2 * dBinW + cur_module.width()));
					betaX = 4 / (dBinW * (2 * dBinW + cur_module.width()));
					alphaY = 4 / ((dBinH + cur_module.height()) * (2 * dBinH + cur_module.height()));
					betaY = 4 / (dBinH * (2 * dBinH + cur_module.height()));

					densityRatio = cur_module.area() / (dBinW * dBinH);

					dX = x[2 * i] - ((a + 0.5) * dBinW + _placement.boundryLeft());
					ABSdX = abs(dX);
					dY = x[2 * i + 1] - ((b + 0.5) * dBinH + _placement.boundryBottom());
					ABSdY = abs(dY);

					//bell-shaped function of bin density smoothing	
					if (ABSdX <= (dBinW / 2 + cur_module.width() / 2)) {
						overlapX = 1 - alphaX * ABSdX * ABSdX;
					}
					else if (ABSdX <= (dBinW + cur_module.width() / 2)) {
						overlapX = betaX * pow(ABSdX - dBinW - cur_module.width() / 2, 2);
					}
					else {
						overlapX = 0;
					}
					//bell-shaped function of bin density smoothing	
					if (ABSdY <= (dBinH / 2 + cur_module.height() / 2)) {
						overlapY = 1 - alphaY * ABSdY * ABSdY;
					}
					else if (ABSdY <= (dBinH + cur_module.height() / 2)) {
						overlapY = betaY * pow(ABSdY - dBinH - cur_module.height() / 2, 2);
					}
					else {
						overlapY = 0;
					}
					//calculate overlap length
					binDensity[binNumPerEdge * b + a] += densityRatio * overlapX * overlapY;
				}
			}
			//calculate (beta * Σ((Db(x, y)-Tb)^2)) part in objective function
			f += beta * pow(binDensity[binNumPerEdge * b + a] - targetDensity, 2);

		}
	}
}

unsigned ExampleFunction::dimension()
{
	return _placement.numModules() * 2; // num_blocks*2
	// each two dimension represent the X and Y dimensions of each block
}
