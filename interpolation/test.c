/* interpolation/test.c
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Gerard Jungman
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


/* Author:  G. Jungman
 */
#include <config.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gsl/gsl_test.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_ieee_utils.h>

int
test_bsearch(void)
{
  double x_array[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  size_t index_result;
  int status = 0;
  int s;

  /* check an interior point */
  index_result = gsl_interp_bsearch(x_array, 1.5, 0, 4);
  s = (index_result != 1);
  status += s;
  gsl_test (s, "simple bsearch");

  /* check that we get the last interval if x == last value */
  index_result = gsl_interp_bsearch(x_array, 4.0, 0, 4);
  s = (index_result != 3);
  status += s;
  gsl_test (s, "upper endpoint bsearch");

  /* check that we get the first interval if x == first value */
  index_result = gsl_interp_bsearch(x_array, 0.0, 0, 4);
  s = (index_result != 0);
  status += s;
  gsl_test (s, "lower endpoint bsearch");

  /* check that we get correct interior boundary behaviour */
  index_result = gsl_interp_bsearch(x_array, 2.0, 0, 4);
  s = (index_result != 2);
  status += s;
  gsl_test (s, "degenerate bsearch");

  /* check out of bounds above */
  index_result = gsl_interp_bsearch(x_array, 10.0, 0, 4);
  s = (index_result != 3);
  status += s;
  gsl_test (s, "out of bounds bsearch +");

  /* check out of bounds below */
  index_result = gsl_interp_bsearch(x_array, -10.0, 0, 4);
  s = (index_result != 0);
  status += s;
  gsl_test (s, "out of bounds bsearch -");

  /* Test the accelerator */

  {
    size_t i, j, k1 = 0, k2 = 0;
    int t = 0;
    double x = 0;
    double r[16] = { -0.2, 0.0, 0.1, 0.7, 
                     1.0, 1.3, 1.9, 
                     2.0, 2.2, 2.7,
                     3.0, 3.1, 3.6,
                     4.0, 4.1, 4.9 };

    gsl_interp_accel *a = gsl_interp_accel_alloc ();
    
    /* Run through all the pairs of points */

    while (k1 < 16 && k2 < 16) {
      
      x = r[t ? k1 : k2];
      t = !t;
      
      if (t == 0) { 
        k1 = (k1 + 1) % 16; 
        if (k1 == 0) k2++;
      };

      i = gsl_interp_accel_find(a, x_array, 5, x);
      j = gsl_interp_bsearch(x_array, x, 0, 4);
      gsl_test(i != j, "(%u,%u) accelerated lookup vs bsearch (x = %g)", i, j, x);
    }

    gsl_interp_accel_free(a);
  }

  return status;
}



typedef double TEST_FUNC (double);
typedef struct _xy_table xy_table;

struct _xy_table
  {
    double * x;
    double * y;
    size_t n;
  };

xy_table make_xy_table (double x[], double y[], size_t n);

xy_table
make_xy_table (double x[], double y[], size_t n)
{
  xy_table t;
  t.x = x;
  t.y = y;
  t.n = n;
  return t;
}

static int
test_interp (
  const xy_table * data_table,
  const gsl_interp_type * T,
  xy_table * test_table,
  xy_table * test_d_table,
  xy_table * test_i_table
  )
{
  int status = 0, s1, s2, s3;
  size_t i;

  gsl_interp_accel *a = gsl_interp_accel_alloc ();
  gsl_interp *interp = gsl_interp_alloc (T, data_table->n);

  unsigned int min_size = gsl_interp_type_min_size(T);

  gsl_test_int (min_size, T->min_size, "gsl_interp_type_min_size on %s", gsl_interp_name(interp));

  gsl_interp_init (interp, data_table->x, data_table->y, data_table->n);

  for (i = 0; i < test_table->n; i++)
    {
      double x = test_table->x[i];
      double y;
      double deriv;
      double integ;
      double diff_y, diff_deriv, diff_integ;
      s1 = gsl_interp_eval_e (interp, data_table->x, data_table->y, x, a, &y);
      s2 = gsl_interp_eval_deriv_e (interp, data_table->x, data_table->y, x, a, &deriv);
      s3 = gsl_interp_eval_integ_e (interp, data_table->x, data_table->y, test_table->x[0], x, a, &integ);

      gsl_test (s1, "gsl_interp_eval_e %s", gsl_interp_name(interp));
      gsl_test (s2, "gsl_interp_eval_deriv_e %s", gsl_interp_name(interp));
      gsl_test (s3, "gsl_interp_eval_integ_e %s", gsl_interp_name(interp));

      gsl_test_abs (y, test_table->y[i], 1e-10, "%s %d", gsl_interp_name(interp), i);
      gsl_test_abs (deriv, test_d_table->y[i], 1e-10, "%s deriv %d", gsl_interp_name(interp), i);
      gsl_test_abs (integ, test_i_table->y[i], 1e-10, "%s integ %d", gsl_interp_name(interp), i);

      diff_y = y - test_table->y[i];
      diff_deriv = deriv - test_d_table->y[i];
      diff_integ = integ - test_i_table->y[i];
      if (fabs (diff_y) > 1.e-10 || fabs(diff_deriv) > 1.0e-10 || fabs(diff_integ) > 1.0e-10) {
        status++;
      }
    }

  gsl_interp_accel_free (a);
  gsl_interp_free (interp);

  return status;
}

static int
test_linear (void)
{
  int s;

  double data_x[4] = { 0.0, 1.0, 2.0, 3.0 };
  double data_y[4] = { 0.0, 1.0, 2.0, 3.0 };
  double test_x[6] = { 0.0, 0.5, 1.0, 1.5, 2.5, 3.0 };
  double test_y[6] = { 0.0, 0.5, 1.0, 1.5, 2.5, 3.0 };
  double test_dy[6] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
  double test_iy[6] = { 0.0, 0.125, 0.5, 9.0/8.0, 25.0/8.0, 9.0/2.0 };

  xy_table data_table = make_xy_table(data_x, data_y, 4);
  xy_table test_table = make_xy_table(test_x, test_y, 6);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 6);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 6);

  s = test_interp (&data_table, gsl_interp_linear, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "linear interpolation");
  return s;
}

static int
test_polynomial (void)
{
  int s;

  double data_x[4] = { 0.0, 1.0, 2.0, 3.0 };
  double data_y[4] = { 0.0, 1.0, 2.0, 3.0 };
  double test_x[6] = { 0.0, 0.5, 1.0, 1.5, 2.5, 3.0 };
  double test_y[6] = { 0.0, 0.5, 1.0, 1.5, 2.5, 3.0 };
  double test_dy[6] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
  double test_iy[6] = { 0.0, 0.125, 0.5, 9.0/8.0, 25.0/8.0, 9.0/2.0 };

  xy_table data_table = make_xy_table(data_x, data_y, 4);
  xy_table test_table = make_xy_table(test_x, test_y, 6);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 6);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 6);

  s = test_interp (&data_table, gsl_interp_polynomial, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "polynomial interpolation");
  return s;
}


static int
test_cspline (void)
{
  int s;

  double data_x[3] = { 0.0, 1.0, 2.0 };
  double data_y[3] = { 0.0, 1.0, 2.0 };
  double test_x[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_y[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_dy[4] = { 1.0, 1.0, 1.0, 1.0 };
  double test_iy[4] = { 0.0, 0.125, 0.5, 2.0 };

  xy_table data_table = make_xy_table(data_x, data_y, 3);
  xy_table test_table = make_xy_table(test_x, test_y, 4);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 4);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 4);

  s = test_interp (&data_table, gsl_interp_cspline, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "cspline interpolation");
  return s;
}

static int
test_cspline2 (void)
{
  /* Test taken from Young & Gregory, A Survey of Numerical
     Mathematics, Vol 1 Chapter 6.8 */
  
  int s;

  double data_x[6] = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 };

  double data_y[6] = { 1.0, 
                       0.961538461538461, 0.862068965517241, 
                       0.735294117647059, 0.609756097560976, 
                       0.500000000000000 } ;

  double test_x[50] = {  
    0.00, 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14, 0.16, 0.18, 
    0.20, 0.22, 0.24, 0.26, 0.28, 0.30, 0.32, 0.34, 0.36, 0.38, 
    0.40, 0.42, 0.44, 0.46, 0.48, 0.50, 0.52, 0.54, 0.56, 0.58, 
    0.60, 0.62, 0.64, 0.66, 0.68, 0.70, 0.72, 0.74, 0.76, 0.78,
    0.80, 0.82, 0.84, 0.86, 0.88, 0.90, 0.92, 0.94, 0.96, 0.98 };

  double test_y[50] = { 
    1.000000000000000, 0.997583282975581, 0.995079933416512, 
    0.992403318788142, 0.989466806555819, 0.986183764184894, 
    0.982467559140716, 0.978231558888635, 0.973389130893999, 
    0.967853642622158, 0.961538461538461, 0.954382579685350, 
    0.946427487413627, 0.937740299651188, 0.928388131325928, 
    0.918438097365742, 0.907957312698524, 0.897012892252170, 
    0.885671950954575, 0.874001603733634, 0.862068965517241, 
    0.849933363488199, 0.837622973848936, 0.825158185056786, 
    0.812559385569085, 0.799846963843167, 0.787041308336369, 
    0.774162807506023, 0.761231849809467, 0.748268823704033, 
    0.735294117647059, 0.722328486073082, 0.709394147325463, 
    0.696513685724764, 0.683709685591549, 0.671004731246381, 
    0.658421407009825, 0.645982297202442, 0.633709986144797, 
    0.621627058157454, 0.609756097560976, 0.598112015427308, 
    0.586679029833925, 0.575433685609685, 0.564352527583445, 
    0.553412100584061, 0.542588949440392, 0.531859618981294, 
    0.521200654035625, 0.510588599432241};

  double test_dy[50] = { 
    -0.120113913432180, -0.122279726798445, -0.128777166897241,
    -0.139606233728568, -0.154766927292426, -0.174259247588814,
    -0.198083194617734, -0.226238768379184, -0.258725968873165,
    -0.295544796099676, -0.336695250058719, -0.378333644186652,
    -0.416616291919835, -0.451543193258270, -0.483114348201955,
    -0.511329756750890, -0.536189418905076, -0.557693334664512,
    -0.575841504029200, -0.590633926999137, -0.602070603574326,
    -0.611319695518765, -0.619549364596455, -0.626759610807396,
    -0.632950434151589, -0.638121834629033, -0.642273812239728,
    -0.645406366983674, -0.647519498860871, -0.648613207871319,
    -0.648687494015019, -0.647687460711257, -0.645558211379322,
    -0.642299746019212, -0.637912064630930, -0.632395167214473,
    -0.625749053769843, -0.617973724297039, -0.609069178796061,
    -0.599035417266910, -0.587872439709585, -0.576731233416743,
    -0.566762785681043, -0.557967096502484, -0.550344165881066,
    -0.543893993816790, -0.538616580309654, -0.534511925359660,
    -0.531580028966807, -0.529820891131095};

  double test_iy[50] = {
    0.000000000000000, 0.019975905023535, 0.039902753768792, 
    0.059777947259733, 0.079597153869625, 0.099354309321042, 
    0.119041616685866, 0.138649546385285, 0.158166836189794, 
    0.177580491219196, 0.196875783942601, 0.216036382301310,
    0.235045759060558, 0.253888601161251, 0.272550937842853,
    0.291020140643388, 0.309284923399436, 0.327335342246135,
    0.345162795617181, 0.362760024244829, 0.380121111159890,
    0.397241442753010, 0.414117280448683, 0.430745332379281,
    0.447122714446318, 0.463246950320456, 0.479115971441505,
    0.494728117018421, 0.510082134029305, 0.525177177221407,
    0.540012809111123, 0.554589001813881, 0.568906157172889,
    0.582965126887879, 0.596767214344995, 0.610314174616794,
    0.623608214462242, 0.636651992326715, 0.649448618342004,
    0.662001654326309, 0.674315113784241, 0.686393423540581,
    0.698241001711602, 0.709861835676399, 0.721259443710643,
    0.732436874986582, 0.743396709573044, 0.754141058435429,
    0.764671563435718, 0.774989397332469 };

  xy_table data_table = make_xy_table(data_x, data_y, 6);
  xy_table test_table = make_xy_table(test_x, test_y, 50);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 50);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 50);

  s = test_interp (&data_table, gsl_interp_cspline, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "cspline 1/(1+x^2) interpolation");
  return s;
}

static int
test_cspline3 (void)
{
  /* This data has been chosen to be random (uneven spacing in x and y)
     and the exact cubic spine solution computed using Octave */
  
  int s;
  
  double data_x[7] = {   
    -1.2139767065644265,
    -0.792590494453907,
    -0.250954683125019,
    0.665867809951305,
    0.735655088722706,
    0.827622053027153,
    1.426592227816582  
  };

  double data_y[7] = {   
    -0.00453877449035645,
    0.49763182550668716,
    0.17805472016334534,
    0.40514493733644485,
    -0.21595209836959839,
    0.47405586764216423,
    0.46561462432146072
  } ;

  double test_x[19] = {
    -1.2139767065644265,
    -1.0735146358609200,
    -0.9330525651574135,
    -0.7925904944539071,
    -0.6120452240109444,
    -0.4314999535679818,
    -0.2509546831250191,
    0.0546528145670890,
    0.3602603122591972,
    0.6658678099513053,
    0.6891302362084388,
    0.7123926624655723,
    0.7356550887227058,
    0.7663107434908548,
    0.7969663982590039,
    0.8276220530271530,
    1.0272787779569625,
    1.2269355028867721,
    1.4265922278165817,
  };

  double test_y[19] = { 
    -0.00453877449035645,
    0.25816917628390590,
    0.44938881397673230,
    0.49763182550668716,
    0.31389980410075147,
    0.09948951681196887,
    0.17805472016334534,
    1.27633142487980233,
    2.04936553432792001,
    0.40514493733644485,
    0.13322324792901385,
    -0.09656315924697809,
    -0.21595209836959839,
    -0.13551147728045118,
    0.13466779030061801,
    0.47405586764216423,
    1.68064089899304370,
    1.43594739539458649,
    0.46561462432146072
  };

  double test_dy[19] = { 
    1.955137555965937,
    1.700662049790549,
    0.937235531264386,
    -0.335141999612553,
    -1.401385073563169,
    -0.674982149482761,
    1.844066772628670,
    4.202528085784793,
    -0.284432022227558,
    -11.616813551408383,
    -11.272731243226174,
    -7.994209291156876,
    -1.781247695200491,
    6.373970868827501,
    10.597456848997197,
    10.889210245308570,
    1.803124267866902,
    -3.648527318598099,
    -5.465744514086432,
  };

  double test_iy[19] = {
    0.000000000000000,
    0.018231117234914,
    0.069178822023139,
    0.137781019634897,
    0.213936442847744,
    0.249280997744777,
    0.267492946016120,
    0.471372708120518,
    1.014473660088477,
    1.477731933018837,
    1.483978291717981,
    1.484256847945450,
    1.480341742628893,
    1.474315901028282,
    1.473972210647307,
    1.483279773396950,
    1.728562698140330,
    2.057796448999396,
    2.253662886537457,
  };

  xy_table data_table = make_xy_table(data_x, data_y, 7);
  xy_table test_table = make_xy_table(test_x, test_y, 19);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 19);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 19);

  s = test_interp (&data_table, gsl_interp_cspline, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "cspline arbitrary data interpolation");
  return s;
}

static int
test_csplinep (void)
{
  /* This data has been chosen to be random (uneven spacing in x and y)
     and the exact cubic spine solution computed using Octave */
  
  int s;
  
  double data_x[11] = {   
    0.000000000000000,
    0.130153674349869,
    0.164545962312740,
    0.227375461261537,
    0.256465324353657,
    0.372545206874658,
    0.520820016781720,
    0.647654717733075,
    0.753429306654340,
    0.900873984827658,
    1.000000000000000,
  };

  double data_y[11] = {   
    0.000000000000000,
    0.729629261832041,
    0.859286331568207,
    0.989913099419008,
    0.999175006262120,
    0.717928599519215,
    -0.130443237213363,
    -0.800267961158980,
    -0.999767873040527,
    -0.583333769240853,
    -0.000000000000000,
  } ;

  double test_x[31] = {
    0.000000000000000,
    0.043384558116623,
    0.086769116233246,
    0.130153674349869,
    0.141617770337492,
    0.153081866325116,
    0.164545962312740,
    0.185489128629005,
    0.206432294945271,
    0.227375461261537,
    0.237072082292243,
    0.246768703322951,
    0.256465324353657,
    0.295158618527324,
    0.333851912700991,
    0.372545206874658,
    0.421970143510346,
    0.471395080146033,
    0.520820016781720,
    0.563098250432172,
    0.605376484082623,
    0.647654717733075,
    0.682912914040164,
    0.718171110347252,
    0.753429306654340,
    0.802577532712113,
    0.851725758769885,
    0.900873984827658,
    0.933915989885105,
    0.966957994942553,
    1.000000000000000
  };

  double test_y[31] = { 
    0.000000000000000,
    0.268657574670719,
    0.517940878523929,
    0.729629261832041,
    0.777012551497867,
    0.820298314554859,
    0.859286331568207,
    0.918833991960315,
    0.962624749226346,
    0.989913099419008,
    0.996756196601349,
    0.999858105635752,
    0.999175006262120,
    0.959248551766306,
    0.863713527741856,
    0.717928599519215,
    0.470065187871106,
    0.177694938589523,
    -0.130443237213363,
    -0.385093922365765,
    -0.613840011545983,
    -0.800267961158980,
    -0.912498361131651,
    -0.980219217412290,
    -0.999767873040528,
    -0.943635958253643,
    -0.800314354800596,
    -0.583333769240853,
    -0.403689914131666,
    -0.206151346799382,
    -0.000000000000000
  };

  double test_dy[31] = { 
    6.275761975917336,
    6.039181349093287,
    5.382620652895025,
    4.306079887322550,
    3.957389777282752,
    3.591234754612763,
    3.207614819312586,
    2.473048186927024,
    1.702885029353488,
    0.897125346591982,
    0.513561009477969,
    0.125477545550710,
    -0.267125045189792,
    -1.773533414873785,
    -3.141450982859891,
    -4.370877749148106,
    -5.562104227060234,
    -6.171864888961167,
    -6.200159734850907,
    -5.781556055213110,
    -4.974725570010514,
    -3.779668279243120,
    -2.569220222282655,
    -1.254891157670244,
    0.163318914594122,
    2.074985916277011,
    3.711348850147548,
    5.072407716205733,
    5.754438923510391,
    6.155557010080926,
    6.275761975917336
  };

  double test_iy[31] = {
    0.000000000000000,
    0.005864903144198,
    0.023030998930796,
    0.050262495763030,
    0.058902457844100,
    0.068062330564832,
    0.077693991819461,
    0.096340576055474,
    0.116070578226521,
    0.136546192283223,
    0.146181187290769,
    0.155864434185569,
    0.165559443629720,
    0.203636318965633,
    0.239075190181586,
    0.269828050745236,
    0.299428805999600,
    0.315560685785616,
    0.316734151903742,
    0.305773798930857,
    0.284537037103156,
    0.254466034797342,
    0.224146112780097,
    0.190643050847000,
    0.155590744566140,
    0.107448508851745,
    0.064263083957312,
    0.029987183298960,
    0.013618510529526,
    0.003506827320794,
    0.000090064010007,
  };

  xy_table data_table = make_xy_table(data_x, data_y, 11);
  xy_table test_table = make_xy_table(test_x, test_y, 31);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 31);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 31);

  s = test_interp (&data_table, gsl_interp_cspline_periodic, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "cspline periodic sine interpolation");
  return s;
}

static int
test_csplinep2 (void)
{
  /* This data tests the periodic case n=3 */
  
  int s;
  
  double data_x[3] = {   
    0.123,
    0.423,
    1.123
  };

  double data_y[3] = {   
    0.456000000000000,
    1.407056516295154,
    0.456000000000000
  } ;

  double test_x[61] = {
    0.123000000000000,
    0.133000000000000,
    0.143000000000000,
    0.153000000000000,
    0.163000000000000,
    0.173000000000000,
    0.183000000000000,
    0.193000000000000,
    0.203000000000000,
    0.213000000000000,
    0.223000000000000,
    0.233000000000000,
    0.243000000000000,
    0.253000000000000,
    0.263000000000000,
    0.273000000000000,
    0.283000000000000,
    0.293000000000000,
    0.303000000000000,
    0.313000000000000,
    0.323000000000000,
    0.333000000000000,
    0.343000000000000,
    0.353000000000000,
    0.363000000000000,
    0.373000000000000,
    0.383000000000000,
    0.393000000000000,
    0.403000000000000,
    0.413000000000000,
    0.423000000000000,
    0.446333333333333,
    0.469666666666667,
    0.493000000000000,
    0.516333333333333,
    0.539666666666667,
    0.563000000000000,
    0.586333333333333,
    0.609666666666667,
    0.633000000000000,
    0.656333333333333,
    0.679666666666667,
    0.703000000000000,
    0.726333333333333,
    0.749666666666667,
    0.773000000000000,
    0.796333333333333,
    0.819666666666667,
    0.843000000000000,
    0.866333333333333,
    0.889666666666667,
    0.913000000000000,
    0.936333333333333,
    0.959666666666667,
    0.983000000000000,
    1.006333333333333,
    1.029666666666667,
    1.053000000000000,
    1.076333333333333,
    1.099666666666667,
    1.123000000000000
  };

  double test_y[61] = { 
    0.456000000000000,
    0.475443822110923,
    0.497423794931967,
    0.521758764840979,
    0.548267578215809,
    0.576769081434305,
    0.607082120874316,
    0.639025542913690,
    0.672418193930275,
    0.707078920301921,
    0.742826568406475,
    0.779479984621787,
    0.816858015325704,
    0.854779506896076,
    0.893063305710751,
    0.931528258147577,
    0.969993210584403,
    1.008277009399078,
    1.046198500969450,
    1.083576531673367,
    1.120229947888679,
    1.155977595993233,
    1.190638322364879,
    1.224030973381464,
    1.255974395420838,
    1.286287434860848,
    1.314788938079344,
    1.341297751454174,
    1.365632721363187,
    1.387612694184230,
    1.407056516295154,
    1.442092968697928,
    1.463321489456714,
    1.471728359403224,
    1.468299859369172,
    1.454022270186272,
    1.429881872686237,
    1.396864947700781,
    1.355957776061616,
    1.308146638600458,
    1.254417816149018,
    1.195757589539010,
    1.133152239602149,
    1.067588047170148,
    1.000051293074719,
    0.931528258147577,
    0.863005223220435,
    0.795468469125006,
    0.729904276693004,
    0.667298926756143,
    0.608638700146136,
    0.554909877694696,
    0.507098740233537,
    0.466191568594372,
    0.433174643608916,
    0.409034246108881,
    0.394756656925981,
    0.391328156891929,
    0.399735026838439,
    0.420963547597225,
    0.456000000000000
  };

  double test_dy[61] = { 
    1.8115362215145774,
    2.0742089736341924,
    2.3187663635386602,
    2.5452083912279826,
    2.7535350567021584,
    2.9437463599611897,
    3.1158423010050744,
    3.2698228798338147,
    3.4056880964474079,
    3.5234379508458549,
    3.6230724430291570,
    3.7045915729973125,
    3.7679953407503231,
    3.8132837462881874,
    3.8404567896109061,
    3.8495144707184790,
    3.8404567896109061,
    3.8132837462881874,
    3.7679953407503231,
    3.7045915729973125,
    3.6230724430291565,
    3.5234379508458549,
    3.4056880964474074,
    3.2698228798338147,
    3.1158423010050749,
    2.9437463599611897,
    2.7535350567021584,
    2.5452083912279830,
    2.3187663635386597,
    2.0742089736341924,
    1.8115362215145772,
    1.1986331332354823,
    0.6279992234583869,
    0.0996344921833026,
    -0.3864610605897765,
    -0.8302874348608467,
    -1.2318446306299125,
    -1.5911326478969707,
    -1.9081514866620208,
    -2.1829011469250670,
    -2.4153816286861041,
    -2.6055929319451341,
    -2.7535350567021584,
    -2.8592080029571765,
    -2.9226117707101862,
    -2.9437463599611893,
    -2.9226117707101862,
    -2.8592080029571760,
    -2.7535350567021593,
    -2.6055929319451354,
    -2.4153816286861045,
    -2.1829011469250656,
    -1.9081514866620242,
    -1.5911326478969716,
    -1.2318446306299160,
    -0.8302874348608498,
    -0.3864610605897769,
    0.0996344921832995,
    0.6279992234583824,
    1.1986331332354769,
    1.8115362215145772
  };

  double test_iy[61] = {
    0.000000000000000,
    0.004655030170954,
    0.009517330277919,
    0.014611356059886,
    0.019959751719625,
    0.025583349923681,
    0.031501171802382,
    0.037730426949832,
    0.044286513423914,
    0.051183017746288,
    0.058431714902395,
    0.066042568341453,
    0.074023729976459,
    0.082381540184189,
    0.091120527805195,
    0.100243410143811,
    0.109751092968147,
    0.119642670510092,
    0.129915425465314,
    0.140564828993259,
    0.151584540717153,
    0.162966408723997,
    0.174700469564574,
    0.186774948253444,
    0.199176258268946,
    0.211889001553197,
    0.224895968512091,
    0.238178138015305,
    0.251714677396289,
    0.265482942452275,
    0.279458477444273,
    0.312726362409309,
    0.346648754292945,
    0.380914974633193,
    0.415237358187469,
    0.449351252932597,
    0.483015020064806,
    0.516010033999735,
    0.548140682372425,
    0.579234366037328,
    0.609141499068300,
    0.637735508758604,
    0.664912835620912,
    0.690592933387298,
    0.714718269009247,
    0.737254322657649,
    0.758189587722801,
    0.777535570814405,
    0.795326791761572,
    0.811620783612819,
    0.826498092636068,
    0.840062278318649,
    0.852439913367300,
    0.863780583708163,
    0.874256888486789,
    0.884064440068133,
    0.893421864036559,
    0.902570799195836,
    0.911775897569142,
    0.921324824399059,
    0.931528258147577
  };

  xy_table data_table = make_xy_table(data_x, data_y, 3);
  xy_table test_table = make_xy_table(test_x, test_y, 61);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 61);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 61);

  s = test_interp (&data_table, gsl_interp_cspline_periodic, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "cspline periodic 3pt interpolation");
  return s;
}



static int
test_akima (void)
{
  int s;

  double data_x[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  double data_y[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  double test_x[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_y[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_dy[4] = { 1.0, 1.0, 1.0, 1.0 };
  double test_iy[4] = { 0.0, 0.125, 0.5, 2.0 };

  xy_table data_table = make_xy_table(data_x, data_y, 5);
  xy_table test_table = make_xy_table(test_x, test_y, 4);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 4);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 4);

  s = test_interp (&data_table, gsl_interp_akima, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "akima interpolation");
  return s;
}

static int
test_steffen (void)
{
  int s;

  double data_x[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  double data_y[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  double test_x[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_y[4] = { 0.0, 0.5, 1.0, 2.0 };
  double test_dy[4] = { 1.0, 1.0, 1.0, 1.0 };
  double test_iy[4] = { 0.0, 0.125, 0.5, 2.0 };

  xy_table data_table = make_xy_table(data_x, data_y, 5);
  xy_table test_table = make_xy_table(test_x, test_y, 4);
  xy_table test_d_table = make_xy_table(test_x, test_dy, 4);
  xy_table test_i_table = make_xy_table(test_x, test_iy, 4);

  s = test_interp (&data_table, gsl_interp_steffen, &test_table, &test_d_table, &test_i_table);
  gsl_test (s, "steffen interpolation");
  return s;
}

int 
main (int argc, char **argv)
{
  int status = 0;

  gsl_ieee_env_setup ();

  argc = 0;    /* prevent warnings about unused parameters */
  argv = 0;

  status += test_bsearch();
  status += test_linear();
  status += test_polynomial();
  status += test_cspline();
  status += test_cspline2();
  status += test_cspline3();
  status += test_csplinep();
  status += test_csplinep2();
  status += test_akima();
  status += test_steffen();

  exit (gsl_test_summary());
}
