#include <math.h>
#include <gsl_math.h>
#include <gsl_rng.h>
#include <gsl_randist.h>

double
gsl_ran_gaussian (const gsl_rng * r)
{
  double x, y, r2;

  do
    {
      /* choose x,y in uniform square (-1,-1) to (+1,+1) */

      x = -1 + 2 * gsl_rng_uniform (r);
      y = -1 + 2 * gsl_rng_uniform (r);

      /* see if it is in the unit circle */
      r2 = x * x + y * y;
    }
  while (r2 > 1.0 || r2 == 0);

  return y * sqrt (-2.0 * log (r2) / r2);	/* Box-Muller transform */
}

double
gsl_ran_gaussian_pdf (const double x)
{
  double p = (1 / sqrt (2 * M_PI)) * exp (-x * x / 2);
  return p;
}

void
gsl_ran_bivariate_gaussian (const gsl_rng * r, double *x, double *y)
{
  double u, v, r2, scale;

  do
    {
      /* choose x,y in uniform square (-1,-1) to (+1,+1) */

      u = -1 + 2 * gsl_rng_uniform (r);
      v = -1 + 2 * gsl_rng_uniform (r);

      /* see if it is in the unit circle */
      r2 = u * u + v * v;
    }
  while (r2 > 1.0 || r2 == 0);

  scale = sqrt (-2.0 * log (r2) / r2);

  *x = u * scale;
  *y = v * scale;
}

double
gsl_ran_bivariate_gaussian_pdf (const double x, const double y)
{
  double p = (1 / (2 * M_PI)) * exp (-(x * x + y * y) / 2);
  return p;
}
