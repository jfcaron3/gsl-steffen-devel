#include <config.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include <gsl_errno.h>
#include <gsl_complex.h>

#include <gsl_fft_complex.h>

#define BASE_DOUBLE
#include "templates_on.h"
#include "fft_complex.h"
#include "c_init.c"
#include "c_main.c"
#include "c_pass_2.c"
#include "c_pass_3.c"
#include "c_pass_4.c"
#include "c_pass_5.c"
#include "c_pass_6.c"
#include "c_pass_7.c"
#include "c_pass_n.c"
#include "templates_off.h"
#undef  BASE_DOUBLE
