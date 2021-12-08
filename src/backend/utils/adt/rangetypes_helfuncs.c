#include "postgres.h"
#include <math.h>

//----------------------------------------------------
//
// METHOD:  polyfit
//
// INPUTS:  dependentValues[0..(countOfElements-1)]
//          independentValues[0...(countOfElements-1)]
//          countOfElements
//          order - Order of the polynomial fitting
//
// OUTPUTS: coefficients[0..order] - indexed by term
//               (the (coef*x^3) is coefficients[3])
//
//----------------------------------------------------
int polyfit(
	const double* const dependentValues,
	const double* const independentValues,
	unsigned int        countOfElements,
	unsigned int        order,
	double* coefficients)
{
	// Declarations...
	// ----------------------------------
	enum { maxOrder = 100 };

	double B[maxOrder + 1] = { 0.0f };
	double P[((maxOrder + 1) * 2) + 1] = { 0.0f };
	double A[(maxOrder + 1) * 2 * (maxOrder + 1)] = { 0.0f };

	double x, y, powx;

	unsigned int ii, jj, kk;

	// Verify initial conditions....
	// ----------------------------------

	// This method requires that the countOfElements > 
	// (order+1) 
	if (countOfElements <= order)
		return -1;

	// This method has imposed an arbitrary bound of
	// order <= maxOrder.  Increase maxOrder if necessary.
	if (order > maxOrder)
		return -1;

	// Begin Code...
	// ----------------------------------

	// Identify the column vector
	for (ii = 0; ii < countOfElements; ii++)
	{
		x = dependentValues[ii];
		y = independentValues[ii];
		powx = 1;

		for (jj = 0; jj < (order + 1); jj++)
		{
			B[jj] = B[jj] + (y * powx);
			powx = powx * x;
		}
	}

	// Initialize the PowX array
	P[0] = countOfElements;

	// Compute the sum of the Powers of X
	for (ii = 0; ii < countOfElements; ii++)
	{
		x = dependentValues[ii];
		powx = dependentValues[ii];

		for (jj = 1; jj < ((2 * (order + 1)) + 1); jj++)
		{
			P[jj] = P[jj] + powx;
			powx = powx * x;
		}
	}

	// Initialize the reduction matrix
	//
	for (ii = 0; ii < (order + 1); ii++)
	{
		for (jj = 0; jj < (order + 1); jj++)
		{
			A[(ii * (2 * (order + 1))) + jj] = P[ii + jj];
		}

		A[(ii * (2 * (order + 1))) + (ii + (order + 1))] = 1;
	}

	// Move the Identity matrix portion of the redux matrix
	// to the left side (find the inverse of the left side
	// of the redux matrix
	for (ii = 0; ii < (order + 1); ii++)
	{
		x = A[(ii * (2 * (order + 1))) + ii];
		if (x != 0)
		{
			for (kk = 0; kk < (2 * (order + 1)); kk++)
			{
				A[(ii * (2 * (order + 1))) + kk] =
					A[(ii * (2 * (order + 1))) + kk] / x;
			}

			for (jj = 0; jj < (order + 1); jj++)
			{
				if ((jj - ii) != 0)
				{
					y = A[(jj * (2 * (order + 1))) + ii];
					for (kk = 0; kk < (2 * (order + 1)); kk++)
					{
						A[(jj * (2 * (order + 1))) + kk] =
							A[(jj * (2 * (order + 1))) + kk] -
							y * A[(ii * (2 * (order + 1))) + kk];
					}
				}
			}
		}
		else
		{
			// Cannot work with singular matrices
			return -1;
		}
	}

	// Calculate and Identify the coefficients
	for (ii = 0; ii < (order + 1); ii++)
	{
		for (jj = 0; jj < (order + 1); jj++)
		{
			x = 0;
			for (kk = 0; kk < (order + 1); kk++)
			{
				x = x + (A[(ii * (2 * (order + 1))) + (kk + (order + 1))] *
					B[kk]);
			}
			coefficients[ii] = x;
		}
	}

	return 0;
}

// Calculate the polynomial function
double function_poly(double* coefficients, double x, int order) {
	double s = 0;

	for (int i = 0;i < order + 1;i++) {
		s = s + (pow(x, i) * coefficients[i]);
	}

	return s;
}

// Calcualte the Trapezoidal integral
double trapezoidal(double* coefficients, int order, double a, double b, double n)
{
	// Grid spacing
	double h = (b - a) / n;

	// Computing sum of first and last terms
	// in above formula
	double s = function_poly(coefficients, a, order) + function_poly(coefficients, b, order);

	// Adding middle terms in above formula
	for (int i = 1; i < n; i++)
		s += 2 * function_poly(coefficients, a + i * h, order);

	// h/2 indicates (b-a)/2n. Multiplying h/2
	// with s.
	return (h / 2) * s;
}


double calculate_range_join_overlap_fraction(
	double* xa,
	double* ya,
	double min_a,
	double max_a,
	double* xb,
	double* yb,
	double min_b,
	double max_b,
	unsigned int countOfElements
) {
	const unsigned int order = 5;
	int result;


	double final_result;

	double integ_squares = 100;

	double coefficients_a[6]; // resulting array of coefs
	double coefficients_b[6]; // resulting array of coefs


	// Perform the polyfit
	int a_function = polyfit(xa, ya, countOfElements, order, coefficients_a);
	int b_function = polyfit(xb, yb, countOfElements, order, coefficients_b);


	// // fprintf(stderr, "%f\n", function_poly(coefficients, 4));

	double integ_a = trapezoidal(coefficients_a, order, min_a, max_a, integ_squares);
	double integ_b = trapezoidal(coefficients_b, order, min_b, max_b, integ_squares);
	double intersect_a;
	double intersect_b;

	double start = Max(min_b, min_a);
	double end = Min(max_b, max_a);

	fprintf(stderr, "Start: %f\n", start);
	fprintf(stderr, "End: %f\n", end);

	if (start < end) {
		intersect_a = trapezoidal(coefficients_a, order, start, end, integ_squares);
		intersect_b = trapezoidal(coefficients_b, order, start, end, integ_squares);
	}
	else {
		fprintf(stderr, "No intersections\n");
		return 0;
	}
	final_result = (intersect_a / integ_a) * (intersect_b / integ_b);

	fprintf(stderr, "integ_a:\t%f\n", integ_a);
	fprintf(stderr, "integ_b:\t%f\n", integ_b);
	fprintf(stderr, "intersect_a:\t%f\n", intersect_a);
	fprintf(stderr, "intersect_b:\t%f\n", intersect_b);
	fprintf(stderr, "final:\t%f\n", final_result);

	return final_result;
}

double calculate_range_left_of_fraction(
	double* xa,
	double* ya,
	double min_a,
	double max_a,
	unsigned int countOfElements,
	double const_lower
) {
	const unsigned int order = 5;
	double integ_squares = 100;

	double coefficients_a[6]; // resulting array of coefs
	
	// Don't waste time in trivial cases
	if (const_lower < min_a) return 0.0;
	else if (const_lower > max_a) return 1.0;
	// Perform the polyfit
	fprintf(stderr, "Calculating [%f,%f] / [%f,%f]\n", min_a, const_lower, min_a, max_a);
	int a_function = polyfit(xa, ya, countOfElements, order, coefficients_a);
	double integ_a = trapezoidal(coefficients_a, order, min_a, max_a, integ_squares);
	double intersect_a = trapezoidal(coefficients_a, order, min_a, const_lower, integ_squares);

	return intersect_a / integ_a;
}

double calculate_range_overlap_fraction(
	double* xa,
	double* ya,
	double min_a,
	double max_a,
	unsigned int countOfElements,
	double const_lower,
	double const_upper
) {
	const unsigned int order = 5;
	double integ_squares = 100;

	double coefficients_a[6]; // resulting array of coefs


	// Don't waste time in trivial cases
	if (const_upper < min_a) return 0.0;
	else if (const_lower > max_a) return 0.0;
	else if (const_lower < min_a && const_upper > max_a) return 1.0;
	double lower = Max(min_a, const_lower);
	double upper = Min(max_a, const_upper);
	fprintf(stderr, "Calculating [%f,%f] / [%f,%f]\n", lower, upper, min_a, max_a);
	// Perform the polyfit
	int a_function = polyfit(xa, ya, countOfElements, order, coefficients_a);
	double integ_a = trapezoidal(coefficients_a, order, min_a, max_a, integ_squares);
	double intersect = trapezoidal(coefficients_a, order, lower, upper, integ_squares);

	return intersect / integ_a;
}

