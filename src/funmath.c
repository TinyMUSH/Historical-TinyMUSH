/* funmath.c - math and logic functions */
/* $Id$ */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"

#include <math.h>

#include "alloc.h"	/* required by mudconf */
#include "flags.h"	/* required by mudconf */
#include "htab.h"	/* required by mudconf */
#include "mudconf.h"	/* required by code */

#include "db.h"		/* required by externs */
#include "externs.h"	/* required by code */

#include "functions.h"	/* required by code */

/* ---------------------------------------------------------------------------
 * fval: copy the floating point value into a buffer and make it presentable
 */

#ifdef FLOATING_POINTS
#define FP_SIZE ((sizeof(double) + sizeof(unsigned int) - 1) / sizeof(unsigned int))
#define FP_EXP_WEIRD	0x1
#define FP_EXP_ZERO	0x2

typedef union {
    double d;
    unsigned int u[FP_SIZE];
} fp_union_uint;

static unsigned int fp_check_weird(buff, bufc, result)
char *buff, **bufc;
double result;
{
	static fp_union_uint fp_sign_mask, fp_exp_mask, fp_mant_mask, fp_val;
	static const double d_zero = 0.0;
	static int fp_initted = 0;
	unsigned int fp_sign, fp_exp, fp_mant;
	int i;

	if (!fp_initted) {
		memset(fp_sign_mask.u, 0, sizeof(fp_sign_mask));
		memset(fp_exp_mask.u, 0, sizeof(fp_exp_mask));
		memset(fp_val.u, 0, sizeof(fp_val));
		fp_exp_mask.d = 1.0 / d_zero;
		fp_sign_mask.d = -1.0 / fp_exp_mask.d;
		for (i = 0; i < FP_SIZE; i++) {
			fp_mant_mask.u[i] = ~(fp_exp_mask.u[i] | fp_sign_mask.u[i]);
		}
		fp_initted = 1;
	}

	fp_val.d = result;

	fp_sign = fp_mant = 0;
	fp_exp = FP_EXP_ZERO | FP_EXP_WEIRD;

	for (i = 0; (i < FP_SIZE) && fp_exp; i++) {
		if (fp_exp_mask.u[i]) {
			unsigned int x = (fp_exp_mask.u[i] & fp_val.u[i]);
			if (x == fp_exp_mask.u[i]) {
				/* these bits are all set. can't be zero
				 * exponent, but could still be max (weird)
				 * exponent.
				 */
				fp_exp &= ~FP_EXP_ZERO;
			} else if (x == 0) {
				/* none of these bits are set. can't be max
				 * exponent, but could still be zero exponent.
				 */
				fp_exp &= ~FP_EXP_WEIRD;
			} else {
				/* some bits were set but not others. can't
				 * be either zero exponent or max exponent.
				 */
				fp_exp = 0;
			}
		}

		fp_sign |= (fp_sign_mask.u[i] & fp_val.u[i]);
		fp_mant |= (fp_mant_mask.u[i] & fp_val.u[i]);
	}
	if (fp_exp == FP_EXP_WEIRD) {
		if (fp_sign) {
			safe_chr('-', buff, bufc);
		}
		if (fp_mant) {
			safe_known_str("NaN", 3, buff, bufc);
	  	} else {
			safe_known_str("Inf", 3, buff, bufc);
		}
	}
	return fp_exp;
}

static void fval(buff, bufc, result)
char *buff, **bufc;
double result;
{
	char *p, *buf1;

	switch (fp_check_weird(buff, bufc, result)) {
	case FP_EXP_WEIRD:	return;
	case FP_EXP_ZERO:	result = 0.0; /* FALLTHRU */
	default:		break;
	}

	buf1 = *bufc;
	safe_tprintf_str(buff, bufc, "%.6f", result);	/* get double val
							 * into buffer 
							 */
	**bufc = '\0';
	p = strrchr(buf1, '0');
	if (p == NULL) {	/* remove useless trailing 0's */
		return;
	} else if (*(p + 1) == '\0') {
		while (*p == '0') {
			*p-- = '\0';
		}
		*bufc = p + 1;
	}
	p = strrchr(buf1, '.');	/* take care of dangling '.' */

	if ((p != NULL) && (*(p + 1) == '\0')) {
			*p = '\0';
		*bufc = p;
	}
	/* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

	if (!strcmp(buf1, "-0")) {
		*buf1 = '0';
		*bufc = buf1 + 1;
	}
}
#else
#define fval(b,p,n)  safe_ltos(b,p,n)
#endif

/* ---------------------------------------------------------------------------
 * fun_ncomp: numerical compare.
 */

FUNCTION(fun_ncomp)
{
    NVAL x, y;

    x = aton(fargs[0]);
    y = aton(fargs[1]);

    if (x == y) {
	safe_chr('0', buff, bufc);
    } else if (x < y) {
	safe_str("-1", buff, bufc);
    } else {
	safe_chr('1', buff, bufc);
    }
}

/*-------------------------------------------------------------------------
 * Comparison functions.
 */

FUNCTION(fun_gt)
{
	safe_bool(buff, bufc, (aton(fargs[0]) > aton(fargs[1])));
}
FUNCTION(fun_gte)
{
	safe_bool(buff, bufc, (aton(fargs[0]) >= aton(fargs[1])));
}
FUNCTION(fun_lt)
{
	safe_bool(buff, bufc, (aton(fargs[0]) < aton(fargs[1])));
}
FUNCTION(fun_lte)
{
	safe_bool(buff, bufc, (aton(fargs[0]) <= aton(fargs[1])));
}
FUNCTION(fun_eq)
{
	safe_bool(buff, bufc, (aton(fargs[0]) == aton(fargs[1])));
}
FUNCTION(fun_neq)
{
	safe_bool(buff, bufc, (aton(fargs[0]) != aton(fargs[1])));
}

FUNCTION(fun_and)
{
	int i;
	
	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		for (i = 0; (i < nfargs) && atoi(fargs[i]); i++) ;
		safe_bool(buff, bufc, i == nfargs);
	}
}

FUNCTION(fun_or)
{
	int i;
	
	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		for (i = 0; (i < nfargs) && !atoi(fargs[i]); i++) ;
		safe_bool(buff, bufc, i != nfargs);
	}
}

FUNCTION(fun_xor)
{
	int i, val;

	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		val = atoi(fargs[0]);
		for (i = 1; i < nfargs; i++) {
			if (val) {
				val = !atoi(fargs[i]);
			} else {
				val = atoi(fargs[i]);
			}
		}
		safe_bool(buff, bufc, val);
	}
	return;
}

FUNCTION(fun_not)
{
	safe_bool(buff, bufc, !atoi(fargs[0]));
}

/*-------------------------------------------------------------------------
 * List-based numeric functions.
 */

FUNCTION(fun_ladd)
{
    NVAL sum;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LADD", 2);

    sum = 0;
    cp = trim_space_sep(fargs[0], isep.c);
    while (cp) {
	curr = split_token(&cp, isep.c);
	sum += aton(curr);
    }
    fval(buff, bufc, sum);
}

FUNCTION(fun_lor)
{
    int i;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LOR", 2);

    i = 0;
    cp = trim_space_sep(fargs[0], isep.c);
    while (cp && !i) {
	curr = split_token(&cp, isep.c);
	i = atoi(curr);
    }
    safe_bool(buff, bufc, (i != 0));
}

FUNCTION(fun_land)
{
    int i;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LAND", 2);

    i = 1;
    cp = trim_space_sep(fargs[0], isep.c);
    while (cp && i) {
	curr = split_token(&cp, isep.c);
	i = atoi(curr);
    }
    safe_bool(buff, bufc, (i != 0));
}

FUNCTION(fun_lorbool)
{
    int i;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LORBOOL", 2);

    i = 0;
    cp = trim_space_sep(fargs[0], isep.c);
    while (cp && !i) {
	curr = split_token(&cp, isep.c);
	i = xlate(curr);
    }
    safe_bool(buff, bufc, (i != 0));
}

FUNCTION(fun_landbool)
{
    int i;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LANDBOOL", 2);

    i = 1;
    cp = trim_space_sep(fargs[0], isep.c);
    while (cp && i) {
	curr = split_token(&cp, isep.c);
	i = xlate(curr);
    }
    safe_bool(buff, bufc, (i != 0));
}

FUNCTION(fun_lmax)
{
    NVAL max, val;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LMAX", 2);

    cp = trim_space_sep(fargs[0], isep.c);
    if (cp) {
	curr = split_token(&cp, isep.c);
	max = aton(curr);
	while (cp) {
	    curr = split_token(&cp, isep.c);
	    val = aton(curr);
	    if (max < val)
		max = val;
	}
	fval(buff, bufc, max);
    }
}

FUNCTION(fun_lmin)
{
    NVAL min, val;
    char *cp, *curr;
    Delim isep;

    varargs_preamble("LMIN", 2);

    cp = trim_space_sep(fargs[0], isep.c);
    if (cp) {
	curr = split_token(&cp, isep.c);
	min = aton(curr);
	while (cp) {
	    curr = split_token(&cp, isep.c);
	    val = aton(curr);
	    if (min > val)
		min = val;
	}
	fval(buff, bufc, min);
    }
}

/*-------------------------------------------------------------------------
 *  True boolean functions.
 */

FUNCTION(fun_andbool)
{
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	for (i = 0; (i < nfargs) && xlate(fargs[i]); i++)
	    ;
	safe_bool(buff, bufc, i == nfargs);
    }
    return;
}

FUNCTION(fun_orbool)
{
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	for (i = 0; (i < nfargs) && !xlate(fargs[i]); i++)
	    ;
	safe_bool(buff, bufc, i != nfargs);
    }
    return;
}

FUNCTION(fun_xorbool)
{
    int i, val;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	val = xlate(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    if (val) {
		val = !xlate(fargs[i]);
	    } else {
		val = xlate(fargs[i]);
	    }
	}
	safe_bool(buff, bufc, val);
    }
    return;
}

FUNCTION(fun_notbool)
{
	safe_bool(buff, bufc, !xlate(fargs[0]));
}

FUNCTION(fun_t)
{
	safe_bool(buff, bufc, xlate(fargs[0]));
}

/*-------------------------------------------------------------------------
 * Short-circuit conditional-evaluation boolean functions.
 */

#define CLOGIC_OR 0x1
#define CLOGIC_BOOL 0x2

static void handle_clogic(player, caller, cause, fargs, nfargs,
			  cargs, ncargs, buff, bufc, flag)
    dbref player, caller, cause;
    char *fargs[], *cargs[], *buff, **bufc;
    int nfargs, ncargs, flag;
{
    int i, val;
    char *str, *tbuf, *bp;
	
    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	tbuf = alloc_lbuf("fun_cand");
	for (i = 0; i < nfargs; i++) { 
	    str = fargs[i];
	    bp = tbuf;
	    exec(tbuf, &bp, player, caller, cause,
		 EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
	    *bp = '\0';
	    val = (flag & CLOGIC_BOOL) ? xlate(tbuf) : atoi(tbuf);
	    if ((flag & CLOGIC_OR) ? val : !val)
		break;
	}
	free_lbuf(tbuf);
	safe_bool(buff, bufc,
		  ((flag & CLOGIC_OR) ? (i != nfargs) : (i == nfargs)));
    }
}

FUNCTION(fun_cand)
{
    handle_clogic(player, caller, cause, fargs, nfargs, cargs, ncargs,
		  buff, bufc, 0);
}

FUNCTION(fun_cor)
{
    handle_clogic(player, caller, cause, fargs, nfargs, cargs, ncargs,
		  buff, bufc, CLOGIC_OR);
} 

FUNCTION(fun_candbool)
{
    handle_clogic(player, caller, cause, fargs, nfargs, cargs, ncargs,
		  buff, bufc, CLOGIC_BOOL);
}

FUNCTION(fun_corbool)
{
    handle_clogic(player, caller, cause, fargs, nfargs, cargs, ncargs,
		  buff, bufc, CLOGIC_OR | CLOGIC_BOOL);
}

/*-------------------------------------------------------------------------
 * Mathematical functions.
 */

FUNCTION(fun_sqrt)
{
	NVAL val;

	val = aton(fargs[0]);
	if (val < 0) {
		safe_str("#-1 SQUARE ROOT OF NEGATIVE", buff, bufc);
	} else if (val == 0) {
		safe_chr('0', buff, bufc);
	} else {
		fval(buff, bufc, sqrt((double)val));
	}
}

FUNCTION(fun_add)
{
	NVAL sum;
	int i;

	if (nfargs < 2) {
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
	} else {
		sum = aton(fargs[0]);
		for (i = 1; i < nfargs; i++) {
			sum += aton(fargs[i]);
		}
		fval(buff, bufc, sum);
	}
	return;
}

FUNCTION(fun_sub)
{
	fval(buff, bufc, aton(fargs[0]) - aton(fargs[1]));
}

FUNCTION(fun_mul)
{
    NVAL prod;
    int i;

    if (nfargs < 2) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	prod = aton(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    prod *= aton(fargs[i]);
	}
	fval(buff, bufc, prod);
    }
    return;
}

FUNCTION(fun_floor)
{
#ifdef FLOATING_POINTS
	char *oldp;
	NVAL x;

	oldp = *bufc;
	x = floor(aton(fargs[0]));
	switch (fp_check_weird(buff, bufc, x)) {
	case FP_EXP_WEIRD:	return;
	case FP_EXP_ZERO:	x = 0.0; /* FALLTHRU */
	default:		break;
	}
	safe_tprintf_str(buff, bufc, "%.0f", x);
	/* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

	if (!strcmp(oldp, "-0")) {
		*oldp = '0';
		*bufc = oldp + 1;
	}
#else
	fval(buff, bufc, aton(fargs[0]));
#endif
}
FUNCTION(fun_ceil)
{
#ifdef FLOATING_POINTS
	char *oldp;
	NVAL x;

	oldp = *bufc;
	x = ceil(aton(fargs[0]));
	switch (fp_check_weird(buff, bufc, x)) {
	case FP_EXP_WEIRD:	return;
	case FP_EXP_ZERO:	x = 0.0; /* FALLTHRU */
	default:		break;
	}
	safe_tprintf_str(buff, bufc, "%.0f", x);
	/* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

	if (!strcmp(oldp, "-0")) {
		*oldp = '0';
		*bufc = oldp + 1;
	}
#else
	fval(buff, bufc, aton(fargs[0]));
#endif
}
FUNCTION(fun_round)
{
#ifdef FLOATING_POINTS
	const char *fstr;
	char *oldp;
	NVAL x;

	oldp = *bufc;
	
	switch (atoi(fargs[1])) {
	case 1:
		fstr = "%.1f";
		break;
	case 2:
		fstr = "%.2f";
		break;
	case 3:
		fstr = "%.3f";
		break;
	case 4:
		fstr = "%.4f";
		break;
	case 5:
		fstr = "%.5f";
		break;
	case 6:
		fstr = "%.6f";
		break;
	default:
		fstr = "%.0f";
		break;
	}
	x = aton(fargs[0]);
	switch (fp_check_weird(buff, bufc, x)) {
	case FP_EXP_WEIRD:	return;
	case FP_EXP_ZERO:	x = 0.0; /* FALLTHRU */
	default:		break;
	}
	safe_tprintf_str(buff, bufc, (char *)fstr, x);

	/* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

	if (!strcmp(oldp, "-0")) {
		*oldp = '0';
		*bufc = oldp + 1;
	}
#else
	fval(buff, bufc, aton(fargs[0]));
#endif
}

FUNCTION(fun_trunc)
{
#ifdef FLOATING_POINTS
	NVAL x;

	x = aton(fargs[0]);
	x = (x >= 0) ? floor(x) : ceil(x);
	switch (fp_check_weird(buff, bufc, x)) {
	case FP_EXP_WEIRD:	return;
	case FP_EXP_ZERO:	x = 0.0; /* FALLTHRU */
	default:		break;
	}
	fval(buff, bufc, x);
#else
	fval(buff, bufc, aton(fargs[0]));
#endif
}

FUNCTION(fun_div)
{
	int top, bot;

	/* The C / operator is only fully specified for non-negative
	 * operands, so we try not to give it negative operands here
	 */

	top = atoi(fargs[0]);
	bot = atoi(fargs[1]);
	if (bot == 0) {
		safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
		return;
	}

	if (top < 0) {
		if (bot < 0)
			top = (-top) / (-bot);
		else
			top = -((-top) / bot);
	} else {
		if (bot < 0)
			top = -(top / (-bot));
		else
			top = top / bot;
	}
	safe_ltos(buff, bufc, top);
}

FUNCTION(fun_floordiv)
{
	int top, bot, res;

	/* The C / operator is only fully specified for non-negative
	 * operands, so we try not to give it negative operands here
	 */

	top = atoi(fargs[0]);
	bot = atoi(fargs[1]);
	if (bot == 0) {
		safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
		return;
	}

	if (top < 0) {
		if (bot < 0) {
			res = (-top) / (-bot);
		} else {
			res = -((-top) / bot);
			if (top % bot)
				res--;
		}
	} else {
		if (bot < 0) {
			res = -(top / (-bot));
			if (top % bot)
				res--;
		} else {
			res = top / bot;
		}
	}
	safe_ltos(buff, bufc, res);
}

FUNCTION(fun_fdiv)
{
	NVAL bot;

	bot = aton(fargs[1]);
	if (bot == 0) {
		safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
	} else {
		fval(buff, bufc, (aton(fargs[0]) / bot));
	}
}

FUNCTION(fun_modulo)
{
	int top, bot;

	/* The C % operator is only fully specified for non-negative
	 * operands, so we try not to give it negative operands here
	 */

	top = atoi(fargs[0]);
	bot = atoi(fargs[1]);
	if (bot == 0)
		bot = 1;
	if (top < 0) {
		if (bot < 0)
			top = -((-top) % (-bot));
		else
			top = (bot - ((-top) % bot)) % bot;
	} else {
		if (bot < 0)
			top = -(((-bot) - (top % (-bot))) % (-bot));
		else
			top = top % bot;
	}
	safe_ltos(buff, bufc, top); 
}

FUNCTION(fun_remainder)
{
	int top, bot;

	/* The C % operator is only fully specified for non-negative
	 * operands, so we try not to give it negative operands here
	 */

	top = atoi(fargs[0]);
	bot = atoi(fargs[1]);
	if (bot == 0)
		bot = 1;
	if (top < 0) {
		if (bot < 0)
			top = -((-top) % (-bot));
		else
			top = -((-top) % bot);
	} else {
		if (bot < 0)
			top = top % (-bot);
		else
			top = top % bot;
	}
	safe_ltos(buff, bufc, top); 
}

FUNCTION(fun_pi)
{
	safe_known_str("3.141592654", 11, buff, bufc);
}
FUNCTION(fun_e)
{
	safe_known_str("2.718281828", 11, buff, bufc);
}

FUNCTION(fun_sin)
{
	fval(buff, bufc, sin(aton(fargs[0])));
}
FUNCTION(fun_cos)
{
	fval(buff, bufc, cos(aton(fargs[0])));
}
FUNCTION(fun_tan)
{
	fval(buff, bufc, tan(aton(fargs[0])));
}

FUNCTION(fun_exp)
{
	fval(buff, bufc, exp(aton(fargs[0])));
}

FUNCTION(fun_power)
{
	NVAL val1, val2;

	val1 = aton(fargs[0]);
	val2 = aton(fargs[1]);
	if (val1 < 0) {
		safe_str("#-1 POWER OF NEGATIVE", buff, bufc);
	} else {
		fval(buff, bufc, pow(val1, val2));
	}
}

FUNCTION(fun_ln)
{
	NVAL val;

	val = aton(fargs[0]);
	if (val > 0)
		fval(buff, bufc, log(val));
	else
		safe_str("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
}

FUNCTION(fun_log)
{
	NVAL val, base;

	if (!fn_range_check("LOG", nfargs, 1, 2, buff, bufc))
	    return;

	val = aton(fargs[0]);

	if (nfargs == 2)
	    base = aton(fargs[1]);
	else
	    base = 10;

	if ((val <= 0) || (base <= 0))
	    safe_str("#-1 LOG OF NEGATIVE OR ZERO", buff, bufc);
	else if (base == 1)
	    safe_str("#-1 DIVISION BY ZERO", buff, bufc);
	else
	    fval(buff, bufc, log(val) / log(base));
}


FUNCTION(fun_asin)
{
	NVAL val;

	val = aton(fargs[0]);
	if ((val < -1) || (val > 1)) {
		safe_str("#-1 ASIN ARGUMENT OUT OF RANGE", buff, bufc);
	} else {
		fval(buff, bufc, asin(val));
	}
}

FUNCTION(fun_acos)
{
	NVAL val;

	val = aton(fargs[0]);
	if ((val < -1) || (val > 1)) {
		safe_str("#-1 ACOS ARGUMENT OUT OF RANGE", buff, bufc);
	} else {
		fval(buff, bufc, acos(val));
	}
}

FUNCTION(fun_atan)
{
	fval(buff, bufc, atan(aton(fargs[0])));
}

FUNCTION(fun_dist2d)
{
	int d;
	double r;

	d = atoi(fargs[0]) - atoi(fargs[2]);
	r = (double)(d * d);
	d = atoi(fargs[1]) - atoi(fargs[3]);
	r += (double)(d * d);
	d = (int)(sqrt(r) + 0.5);
	safe_ltos(buff, bufc, d);
}

FUNCTION(fun_dist3d)
{
	int d;
	double r;

	d = atoi(fargs[0]) - atoi(fargs[3]);
	r = (double)(d * d);
	d = atoi(fargs[1]) - atoi(fargs[4]);
	r += (double)(d * d);
	d = atoi(fargs[2]) - atoi(fargs[5]);
	r += (double)(d * d);
	d = (int)(sqrt(r) + 0.5);
	safe_ltos(buff, bufc, d);
}

/* ------------------------------------------------------------------------
 * Vector functions: VADD, VSUB, VMUL, VCROSS, VMAG, VUNIT, VDIM
 * Vectors are space-separated numbers.
 */

#define VADD_F 0
#define VSUB_F 1
#define VMUL_F 2
#define VDOT_F 3
#define VCROSS_F 4

static void handle_vectors(vecarg1, vecarg2, buff, bufc,
			   sep, osep, osep_len, flag)
    char *vecarg1;
    char *vecarg2;
    char *buff;
    char **bufc;
    char sep;
    Delim osep;
    int osep_len, flag;
{
    char *v1[LBUF_SIZE], *v2[LBUF_SIZE];
    NVAL scalar;
    int n, m, i;

    /*
     * split the list up, or return if the list is empty 
     */
    if (!vecarg1 || !*vecarg1 || !vecarg2 || !*vecarg2) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, vecarg1, sep);
    m = list2arr(v2, LBUF_SIZE, vecarg2, sep);

    /* It's okay to have vmul() be passed a scalar first or second arg,
     * but everything else has to be same-dimensional.
     */
    if ((n != m) &&
	!((flag == VMUL_F) && ((n == 1) || (m == 1)))) {
	safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc);
	return;
    }

    switch (flag) {
	case VADD_F:
	    fval(buff, bufc, aton(v1[0]) + aton(v2[0]));
	    for (i = 1; i < n; i++) {
		print_sep(osep, osep_len, buff, bufc);
		fval(buff, bufc, aton(v1[i]) + aton(v2[i]));
	    }
	    return;
	    /* NOTREACHED */
	case VSUB_F:
	    fval(buff, bufc, aton(v1[0]) - aton(v2[0]));
	    for (i = 1; i < n; i++) {
		print_sep(osep, osep_len, buff, bufc);
		fval(buff, bufc, aton(v1[i]) - aton(v2[i]));
	    }
	    return;
	    /* NOTREACHED */
	case VMUL_F:
	    /* if n or m is 1, this is scalar multiplication.
	     * otherwise, multiply elementwise.
	     */
	    if (n == 1) {
		scalar = aton(v1[0]);
		fval(buff, bufc, aton(v2[0]) * scalar);
		for (i = 1; i < m; i++) {
		    print_sep(osep, osep_len, buff, bufc);
		    fval(buff, bufc, aton(v2[i]) * scalar);
		}
	    } else if (m == 1) {
		scalar = aton(v2[0]);
		fval(buff, bufc, aton(v1[0]) * scalar);
		for (i = 1; i < n; i++) {
		    print_sep(osep, osep_len, buff, bufc);
		    fval(buff, bufc, aton(v1[i]) * scalar);
		}
	    } else {
		/* vector elementwise product.
		 *
		 * Note this is a departure from TinyMUX, but an imitation
		 * of the PennMUSH behavior: the documentation in Penn
		 * claims it's a dot product, but the actual behavior
		 * isn't. We implement dot product separately!
		 */
		fval(buff, bufc, aton(v1[0]) * aton(v2[0]));
		for (i = 1; i < n; i++) {
		    print_sep(osep, osep_len, buff, bufc);
		    fval(buff, bufc, aton(v1[i]) * aton(v2[i]));
		}
	    }
	    return;
	    /* NOTREACHED */
	case VDOT_F:
	    scalar = 0;
	    for (i = 0; i < n; i++) {
		scalar += aton(v1[i]) * aton(v2[i]);
	    }
	    fval(buff, bufc, scalar);
	    return;
	    /* NOTREACHED */
	default:
	    /* If we reached this, we're in trouble. */
	    safe_str("#-1 UNIMPLEMENTED", buff, bufc);
    }
}

FUNCTION(fun_vadd)
{
    Delim isep, osep;
    int osep_len;

    svarargs_preamble("VADD", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc,
		   isep.c, osep, osep_len, VADD_F);
}

FUNCTION(fun_vsub)
{
    Delim isep, osep;
    int osep_len;

    svarargs_preamble("VSUB", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc,
		   isep.c, osep, osep_len, VSUB_F);
}

FUNCTION(fun_vmul)
{
    Delim isep, osep;
    int osep_len;

    svarargs_preamble("VMUL", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc,
		   isep.c, osep, osep_len, VMUL_F);
}

FUNCTION(fun_vdot)
{
    /* dot product: (a,b,c) . (d,e,f) = ad + be + cf
     * 
     * no cross product implementation yet: it would be
     * (a,b,c) x (d,e,f) = (bf - ce, cd - af, ae - bd)
     */

    Delim isep, osep;
    int osep_len;

    svarargs_preamble("VDOT", 4);
    handle_vectors(fargs[0], fargs[1], buff, bufc,
		   isep.c, osep, osep_len, VDOT_F);
}

FUNCTION(fun_vmag)
{
    char *v1[LBUF_SIZE];
    int n, i;
    NVAL tmp, res = 0;
    Delim isep;

    varargs_preamble("VMAG", 2);

    /*
     * split the list up, or return if the list is empty 
     */
    if (!fargs[0] || !*fargs[0]) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, fargs[0], isep.c);

    /*
     * calculate the magnitude 
     */
    for (i = 0; i < n; i++) {
	tmp = aton(v1[i]);
	res += tmp * tmp;
    }

    if (res > 0) {
	fval(buff, bufc, sqrt(res));
    } else {
	safe_chr('0', buff, bufc);
    }
}

FUNCTION(fun_vunit)
{
    char *v1[LBUF_SIZE];
    int n, i, osep_len;
    NVAL tmp, res = 0;
    Delim isep, osep;

    svarargs_preamble("VUNIT", 3);

    /*
     * split the list up, or return if the list is empty 
     */
    if (!fargs[0] || !*fargs[0]) {
	return;
    }
    n = list2arr(v1, LBUF_SIZE, fargs[0], isep.c);

    /*
     * calculate the magnitude 
     */
    for (i = 0; i < n; i++) {
	tmp = aton(v1[i]);
	res += tmp * tmp;
    }

    if (res <= 0) {
	safe_str("#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR",
		 buff, bufc);
	return;
    }
    res = sqrt(res);
    fval(buff, bufc, aton(v1[0]) / res);
    for (i = 1; i < n; i++) {
	print_sep(osep, osep_len, buff, bufc);
	fval(buff, bufc, aton(v1[i]) / res);
    }
}

FUNCTION(fun_vdim)
{
    Delim isep;

    if (nfargs == 0) {
	safe_chr('0', buff, bufc);
    } else {
	varargs_preamble("VDIM", 2);
	safe_ltos(buff, bufc, countwords(fargs[0], isep.c));
    }
}

/* ---------------------------------------------------------------------------
 * fun_abs: Returns the absolute value of its argument.
 */

FUNCTION(fun_abs)
{
	NVAL num;

	num = aton(fargs[0]);
	if (num == 0) {
		safe_chr('0', buff, bufc);
	} else if (num < 0) {
		fval(buff, bufc, -num);
	} else {
		fval(buff, bufc, num);
	}
}

/* ---------------------------------------------------------------------------
 * fun_sign: Returns -1, 0, or 1 based on the the sign of its argument.
 */

FUNCTION(fun_sign)
{
	NVAL num;

	num = aton(fargs[0]);
	if (num < 0) {
	    safe_known_str("-1", 2, buff, bufc);
	} else {
	    safe_bool(buff, bufc, (num > 0));
	}
}

/* ------------------------------------------------------------------------
 * Bitwise functions.
 */

FUNCTION(fun_shl)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) << atoi(fargs[1]));
}

FUNCTION(fun_shr)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) >> atoi(fargs[1]));
}

FUNCTION(fun_band)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) & atoi(fargs[1]));
}

FUNCTION(fun_bor)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) | atoi(fargs[1]));
}

FUNCTION(fun_bnand)
{
    safe_ltos(buff, bufc, atoi(fargs[0]) & ~(atoi(fargs[1])));
}

/* ---------------------------------------------------------------------------
 * fun_max, fun_min: Return maximum (minimum) value.
 */

FUNCTION(fun_max)
{
    int i;
    NVAL max, val;

    if (nfargs < 1) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	max = aton(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    val = aton(fargs[i]);
	    max = (max < val) ? val : max;
	}
	fval(buff, bufc, max);
    }
}

FUNCTION(fun_min)
{
    int i;
    NVAL min, val;

    if (nfargs < 1) {
	safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
    } else {
	min = aton(fargs[0]);
	for (i = 1; i < nfargs; i++) {
	    val = aton(fargs[i]);
	    min = (min > val) ? val : min;
	}
	fval(buff, bufc, min);
    }
}

FUNCTION(fun_inc)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) + 1);
}

FUNCTION(fun_dec)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) - 1);
}
