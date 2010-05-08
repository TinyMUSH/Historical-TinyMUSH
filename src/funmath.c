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
 * Constant math funcs: PI, E
 */

FUNCTION(fun_pi)
{
	safe_known_str("3.141592654", 11, buff, bufc);
}

FUNCTION(fun_e)
{
	safe_known_str("2.718281828", 11, buff, bufc);
}

/* ---------------------------------------------------------------------------
 * Math operations on one number: SIGN, ABS, FLOOR, CEIL, ROUND, TRUNC,
 *    INC, DEC, SQRT, EXP, LN, [A][SIN,COS,TAN][D]
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

FUNCTION(fun_inc)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) + 1);
}

FUNCTION(fun_dec)
{
	safe_ltos(buff, bufc, atoi(fargs[0]) - 1);
}

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

FUNCTION(fun_exp)
{
	fval(buff, bufc, exp((double)aton(fargs[0])));
}

FUNCTION(fun_ln)
{
	NVAL val;

	val = aton(fargs[0]);
	if (val > 0)
		fval(buff, bufc, log((double)val));
	else
		safe_str("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
}

FUNCTION(handle_trig)
{
	NVAL val;
	int oper, flag;
	static double (* const trig_funcs[8])(double) = {
		sin,  cos,  tan, NULL,	/* XXX no cotangent function */
		asin, acos, atan, NULL
	};

	flag = Func_Flags(fargs);
	oper = flag & TRIG_OPER;

	val = aton(fargs[0]);
	if ((flag & TRIG_ARC) && !(flag & TRIG_TAN) &&
	    ((val < -1) || (val > 1))) {
		safe_tprintf_str(buff, bufc, "#-1 %s ARGUMENT OUT OF RANGE",
				 ((FUN *)fargs[-1])->name);
		return;
	}
	if ((flag & TRIG_DEG) && !(flag & TRIG_ARC))
		val = val * (M_PI / 180);

	val = (trig_funcs[oper])((double)val);

	if ((flag & TRIG_DEG) && (flag & TRIG_ARC))
		val = (val * 180) / M_PI;

	fval(buff, bufc, val);
}

/* ---------------------------------------------------------------------------
 * Base conversion: BASECONV
 */

static char from_base_64[256] = {
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
     52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
     -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
     15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
     -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
     41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static char to_base_64[] =
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static char from_base_36[256] = {
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
     -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
     25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
     -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
     25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static char to_base_36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

FUNCTION(fun_baseconv)
{
     long n, m;
     int from, to, isneg;
     char *frombase, *tobase, *p, *nbp;
     char nbuf[LBUF_SIZE];

     /* Use the base 36 conversion table by default */

     frombase = from_base_36;
     tobase = to_base_36;

     /* Figure out our bases */

     if (!is_integer(fargs[1]) || !is_integer(fargs[2])) {
	 safe_known_str("#-1 INVALID BASE", 16, buff, bufc);
	 return;
     }

     from = atoi(fargs[1]);
     to = atoi(fargs[2]);

     if ((from < 2) || (from > 64) || (to < 2) || (to > 64)) {
	 safe_known_str("#-1 BASE OUT OF RANGE", 21, buff, bufc);
	 return;
     }

     if (from > 36)
	 frombase = from_base_64;
     if (to > 36)
	 tobase = to_base_64;

     /* Parse the number to convert */

     p = Eat_Spaces(fargs[0]);
     n = 0;
     if (p) {
	 /* If we have a leading hyphen, and we're in base 63/64, always
	  *  treat it as a minus sign. PennMUSH consistency.
	  */
	 if ((from < 63) && (to < 63) && (*p == '-')) {
	     isneg = 1;
	     p++;
	 } else {
	     isneg = 0;
	 }
	 while (*p) {
	     n *= from;
	     if (frombase[(unsigned char) *p] >= 0) {
		 n += frombase[(unsigned char) *p];
		 p++;
	     } else {
		 safe_known_str("#-1 MALFORMED NUMBER", 20, buff, bufc);
		 return;
	     }
	 }
	 if (isneg) {
	     safe_chr('-', buff, bufc);
	 }
     }

     /* Handle the case of 0 and less than base case. */

     if (n < to) {
	 safe_chr(tobase[(unsigned char) n], buff, bufc);
	 return;
     }

     /* Build up the number backwards, then reverse it. */

     nbp = nbuf;
     while (n > 0) {
	 m = n % to;
	 n = n / to;
	 safe_chr(tobase[(unsigned char) m], nbuf, &nbp);
     }
     nbp--;
     while (nbp >= nbuf) {
	 safe_chr(*nbp, buff, bufc);
	 nbp--;
     }
}

/* ---------------------------------------------------------------------------
 * Math comparison funcs: GT, GTE, LT, LTE, EQ, NEQ, NCOMP
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

/* ---------------------------------------------------------------------------
 * Two-argument math functions: SUB, DIV, FLOORDIV, FDIV, MODULO, REMAINDER,
 *    POWER, LOG
 */

FUNCTION(fun_sub)
{
	fval(buff, bufc, aton(fargs[0]) - aton(fargs[1]));
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

FUNCTION(fun_power)
{
	NVAL val1, val2;

	val1 = aton(fargs[0]);
	val2 = aton(fargs[1]);
	if (val1 < 0) {
		safe_str("#-1 POWER OF NEGATIVE", buff, bufc);
	} else {
		fval(buff, bufc, pow((double)val1, (double)val2));
	}
}

FUNCTION(fun_log)
{
	NVAL val, base;

	VaChk_Range(1, 2);

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
	    fval(buff, bufc, log((double)val) / log((double)base));
}

/* ------------------------------------------------------------------------
 * Bitwise two-argument integer math functions: SHL, SHR, BAND, BOR, BNAND
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
 * Multi-argument math functions: ADD, MUL, MAX, MIN
 */

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

/* ---------------------------------------------------------------------------
 * Integer point distance functions: DIST2D, DIST3D
 */

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

/* ---------------------------------------------------------------------------
 * Math "accumulator" operations on a list: LADD, LMAX, LMIN
 */

FUNCTION(fun_ladd)
{
    NVAL sum;
    char *cp, *curr;
    Delim isep;

    if (nfargs == 0) {
	safe_chr('0', buff, bufc);
	return;
    }

    VaChk_Only_In(2);

    sum = 0;
    cp = trim_space_sep(fargs[0], &isep);
    while (cp) {
	curr = split_token(&cp, &isep);
	sum += aton(curr);
    }
    fval(buff, bufc, sum);
}

FUNCTION(fun_lmax)
{
    NVAL max, val;
    char *cp, *curr;
    Delim isep;

    VaChk_Only_In(2);

    cp = trim_space_sep(fargs[0], &isep);
    if (cp) {
	curr = split_token(&cp, &isep);
	max = aton(curr);
	while (cp) {
	    curr = split_token(&cp, &isep);
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

    VaChk_Only_In(2);

    cp = trim_space_sep(fargs[0], &isep);
    if (cp) {
	curr = split_token(&cp, &isep);
	min = aton(curr);
	while (cp) {
	    curr = split_token(&cp, &isep);
	    val = aton(curr);
	    if (min > val)
		min = val;
	}
	fval(buff, bufc, min);
    }
}

/* ---------------------------------------------------------------------------
 * Operations on a single vector: VMAG, VUNIT
 * (VDIM is implemented by fun_words)
 */

FUNCTION(handle_vector)
{
    char **v1;
    int n, i, oper;
    NVAL tmp, res = 0;
    Delim isep, osep;

    oper = Func_Mask(VEC_OPER);

    if (oper == VEC_UNIT) {
	VaChk_Only_In_Out(3);
    } else {
	VaChk_Only_In(2);
    }

    /* split the list up, or return if the list is empty */
    if (!fargs[0] || !*fargs[0]) {
	return;
    }
    n = list2arr(&v1, LBUF_SIZE, fargs[0], &isep);

    /* calculate the magnitude */
    for (i = 0; i < n; i++) {
	tmp = aton(v1[i]);
	res += tmp * tmp;
    }

    /* if we're just calculating the magnitude, return it */
    if (oper == VEC_MAG) {
	if (res > 0) {
	    fval(buff, bufc, sqrt(res));
	} else {
	    safe_chr('0', buff, bufc);
	}
	XFREE(v1, "handle_vector.v1");
	return;
    }

    if (res <= 0) {
	safe_str("#-1 CAN'T MAKE UNIT VECTOR FROM ZERO-LENGTH VECTOR",
		 buff, bufc);
	XFREE(v1, "handle_vector.v1");
	return;
    }
    res = sqrt(res);
    fval(buff, bufc, aton(v1[0]) / res);
    for (i = 1; i < n; i++) {
	print_sep(&osep, buff, bufc);
	fval(buff, bufc, aton(v1[i]) / res);
    }
    XFREE(v1, "handle_vector.v1");
}

/* ---------------------------------------------------------------------------
 * Operations on a pair of vectors: VADD, VSUB, VMUL, VDOT, VOR, VAND, VXOR
 */

FUNCTION(handle_vectors)
{
    Delim isep, osep;
    int oper;
    char **v1, **v2;
    NVAL scalar;
    int n, m, i, x, y;

    oper = Func_Mask(VEC_OPER);

    if (oper != VEC_DOT) {
	VaChk_Only_In_Out(4);
    } else {
	/* dot product returns a scalar, so no output delim */
	VaChk_Only_In(3);
    }

    /*
     * split the list up, or return if the list is empty 
     */
    if (!fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1]) {
	return;
    }
    n = list2arr(&v1, LBUF_SIZE, fargs[0], &isep);
    m = list2arr(&v2, LBUF_SIZE, fargs[1], &isep);

    /* It's okay to have vmul() be passed a scalar first or second arg,
     * but everything else has to be same-dimensional.
     */
    if ((n != m) &&
	!((oper == VEC_MUL) && ((n == 1) || (m == 1)))) {
	safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufc);
	XFREE(v1, "handle_vectors.v1");
	XFREE(v2, "handle_vectors.v2");
	return;
    }

    switch (oper) {
	case VEC_ADD:
	    fval(buff, bufc, aton(v1[0]) + aton(v2[0]));
	    for (i = 1; i < n; i++) {
		print_sep(&osep, buff, bufc);
		fval(buff, bufc, aton(v1[i]) + aton(v2[i]));
	    }
	    break;
	case VEC_SUB:
	    fval(buff, bufc, aton(v1[0]) - aton(v2[0]));
	    for (i = 1; i < n; i++) {
		print_sep(&osep, buff, bufc);
		fval(buff, bufc, aton(v1[i]) - aton(v2[i]));
	    }
	    break;
	case VEC_OR:
	     safe_bool(buff, bufc, xlate(v1[0]) || xlate(v2[0]));
	     for (i = 1; i < n; i++) {
		 print_sep(&osep, buff, bufc);
		 safe_bool(buff, bufc, xlate(v1[i]) || xlate(v2[i]));
	     }
	     break;
	case VEC_AND:
	     safe_bool(buff, bufc, xlate(v1[0]) && xlate(v2[0]));
	     for (i = 1; i < n; i++) {
		 print_sep(&osep, buff, bufc);
		 safe_bool(buff, bufc, xlate(v1[i]) && xlate(v2[i]));
	     }
	     break;
	case VEC_XOR:
	     x = xlate(v1[0]);
	     y = xlate(v2[0]);
	     safe_bool(buff, bufc, (x && !y) || (!x && y));
	     for (i = 1; i < n; i++) {
		 print_sep(&osep, buff, bufc);
		 x = xlate(v1[i]);
		 y = xlate(v2[i]);
		 safe_bool(buff, bufc, (x && !y) || (!x && y));
	     }
	     break;
	case VEC_MUL:
	    /* if n or m is 1, this is scalar multiplication.
	     * otherwise, multiply elementwise.
	     */
	    if (n == 1) {
		scalar = aton(v1[0]);
		fval(buff, bufc, aton(v2[0]) * scalar);
		for (i = 1; i < m; i++) {
		    print_sep(&osep, buff, bufc);
		    fval(buff, bufc, aton(v2[i]) * scalar);
		}
	    } else if (m == 1) {
		scalar = aton(v2[0]);
		fval(buff, bufc, aton(v1[0]) * scalar);
		for (i = 1; i < n; i++) {
		    print_sep(&osep, buff, bufc);
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
		    print_sep(&osep, buff, bufc);
		    fval(buff, bufc, aton(v1[i]) * aton(v2[i]));
		}
	    }
	    break;
	case VEC_DOT:
	    /* dot product: (a,b,c) . (d,e,f) = ad + be + cf
	     * 
	     * no cross product implementation yet: it would be
	     * (a,b,c) x (d,e,f) = (bf - ce, cd - af, ae - bd)
	     */
	    scalar = 0;
	    for (i = 0; i < n; i++) {
		scalar += aton(v1[i]) * aton(v2[i]);
	    }
	    fval(buff, bufc, scalar);
	    break;
	default:
	    /* If we reached this, we're in trouble. */
	    safe_str("#-1 UNIMPLEMENTED", buff, bufc);
	    break;
    }
    XFREE(v1, "handle_vectors.v1");
    XFREE(v2, "handle_vectors.v2");
}

/* ---------------------------------------------------------------------------
 * Simple boolean funcs: NOT, NOTBOOL, T
 */

FUNCTION(fun_not)
{
	safe_bool(buff, bufc, !atoi(fargs[0]));
}

FUNCTION(fun_notbool)
{
	safe_bool(buff, bufc, !xlate(fargs[0]));
}

FUNCTION(fun_t)
{
	safe_bool(buff, bufc, xlate(fargs[0]));
}

/* ---------------------------------------------------------------------------
 * Multi-argument boolean funcs: various combinations of
 *    [L,C][AND,OR,XOR][BOOL]
 */

FUNCTION(handle_logic)
{
	Delim isep;
	int flag, oper, i, val;
	char *str, *tbuf, *bp;
	int (*cvtfun)(char *);

	flag = Func_Flags(fargs);
	cvtfun = (flag & LOGIC_BOOL) ? xlate : (int (*)(char *))atoi;
	oper = (flag & LOGIC_OPER);

	/* most logic operations on an empty string should be false */
	val = 0;

	if (flag & LOGIC_LIST) {

	        if (nfargs == 0) {
		     safe_chr('0', buff, bufc);
		     return;
		}

		/* the arguments come in a pre-evaluated list */
		VaChk_Only_In(2);

		bp = trim_space_sep(fargs[0], &isep);
		while (bp) {
	  		tbuf = split_token(&bp, &isep);
			val = ((oper == LOGIC_XOR) && val) ? !cvtfun(tbuf) : cvtfun(tbuf);
			if (((oper == LOGIC_AND) && !val) ||
			    ((oper == LOGIC_OR) && val))
				break;
		}

	} else if (nfargs < 2) {
		/* separate arguments, but not enough of them */
		safe_known_str("#-1 TOO FEW ARGUMENTS", 21, buff, bufc);
		return;

	} else if (flag & FN_NO_EVAL) {
		/* separate, unevaluated arguments */
		tbuf = alloc_lbuf("handle_logic");
		for (i = 0; i < nfargs; i++) { 
			str = fargs[i];
			bp = tbuf;
			exec(tbuf, &bp, player, caller, cause,
			     EV_EVAL | EV_STRIP | EV_FCHECK, &str, cargs, ncargs);
			val = ((oper == LOGIC_XOR) && val) ? !cvtfun(tbuf) : cvtfun(tbuf);
			if (((oper == LOGIC_AND) && !val) ||
			    ((oper == LOGIC_OR) && val))
				break;
		}
		free_lbuf(tbuf);

	} else {
		/* separate, pre-evaluated arguments */
		for (i = 0; i < nfargs; i++) { 
			val = ((oper == LOGIC_XOR) && val) ? !cvtfun(fargs[i]) : cvtfun(fargs[i]);
			if (((oper == LOGIC_AND) && !val) ||
			    ((oper == LOGIC_OR) && val))
				break;
		}
	}

	safe_bool(buff, bufc, val);
}

/* ---------------------------------------------------------------------------
 * ltrue() and lfalse(): Get boolean values for an entire list. 
 */

FUNCTION(handle_listbool)
{
     Delim isep, osep;
     int flag, n;
     char *tbuf, *bp, *bb_p;

     flag = Func_Flags(fargs);
     VaChk_Only_In_Out(3);

     if (!fargs[0] || !*fargs[0])
	 return;

     bb_p = *bufc;
     bp = trim_space_sep(fargs[0], &isep);
     while (bp) {
	 tbuf = split_token(&bp, &isep);
	 if (*bufc != bb_p) {
	     print_sep(&osep, buff, bufc);
	 }
	 if (flag & IFELSE_BOOL) {
	     n = xlate(tbuf);
	 } else {
	     n = !((atoi(tbuf) == 0) && is_number(tbuf));
	 }
	 if (flag & IFELSE_FALSE)
	     n = !n;
	 safe_bool(buff, bufc, n);
     }
}
