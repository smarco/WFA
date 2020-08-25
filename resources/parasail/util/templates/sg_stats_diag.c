/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2015 Battelle Memorial Institute.
 */
#include "config.h"

#include <stdlib.h>

%(HEADER)s

#include "parasail.h"
#include "parasail/memory.h"
#include "parasail/internal_%(ISA)s.h"

#define SG_STATS
#define SG_SUFFIX %(SUFFIX)s
#include "sg_helper.h"

#define NEG_INF %(NEG_INF)s
%(FIXES)s

#ifdef PARASAIL_TABLE
static inline void arr_store_si%(BITS)s(
        int *array,
        %(VTYPE)s vWH,
        %(INDEX)s i,
        %(INDEX)s s1Len,
        %(INDEX)s j,
        %(INDEX)s s2Len)
{
%(PRINTER)s
}
#endif

#ifdef PARASAIL_ROWCOL
static inline void arr_store_rowcol(
        int *row,
        int *col,
        %(VTYPE)s vWH,
        %(INDEX)s i,
        %(INDEX)s s1Len,
        %(INDEX)s j,
        %(INDEX)s s2Len)
{
%(PRINTER_ROWCOL)s
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME %(NAME_TABLE)s
#else
#ifdef PARASAIL_ROWCOL
#define FNAME %(NAME_ROWCOL)s
#else
#define FNAME %(NAME)s
#endif
#endif

parasail_result_t* FNAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix,
        int s1_beg, int s1_end, int s2_beg, int s2_end)
{
    const %(INDEX)s N = %(LANES)s; /* number of values in vector */
    const %(INDEX)s PAD = N-1;
    const %(INDEX)s PAD2 = PAD*2;
    const %(INDEX)s s1Len_PAD = s1Len+PAD;
    const %(INDEX)s s2Len_PAD = s2Len+PAD;
    %(INT)s * const restrict s1      = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s1Len+PAD);
    %(INT)s * const restrict s2B     = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _H_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _HM_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _HS_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _HL_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _F_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _FM_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _FS_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict _FL_pr = parasail_memalign_%(INT)s(%(ALIGNMENT)s, s2Len+PAD2);
    %(INT)s * const restrict s2 = s2B+PAD; /* will allow later for negative indices */
    %(INT)s * const restrict H_pr = _H_pr+PAD;
    %(INT)s * const restrict HM_pr = _HM_pr+PAD;
    %(INT)s * const restrict HS_pr = _HS_pr+PAD;
    %(INT)s * const restrict HL_pr = _HL_pr+PAD;
    %(INT)s * const restrict F_pr = _F_pr+PAD;
    %(INT)s * const restrict FM_pr = _FM_pr+PAD;
    %(INT)s * const restrict FS_pr = _FS_pr+PAD;
    %(INT)s * const restrict FL_pr = _FL_pr+PAD;
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(s1Len, s2Len);
#else
#ifdef PARASAIL_ROWCOL
    parasail_result_t *result = parasail_result_new_rowcol3(s1Len, s2Len);
#else
    parasail_result_t *result = parasail_result_new_stats();
#endif
#endif
    %(INDEX)s i = 0;
    %(INDEX)s j = 0;
    %(INDEX)s end_query = s1Len-1;
    %(INDEX)s end_ref = s2Len-1;
    const %(INT)s NEG_LIMIT = (-open < matrix->min ?
        INT%(WIDTH)s_MIN + open : INT%(WIDTH)s_MIN - matrix->min) + 1;
    const %(INT)s POS_LIMIT = INT%(WIDTH)s_MAX - matrix->max - 1;
    %(INT)s score = NEG_LIMIT;
    %(INT)s matches = NEG_LIMIT;
    %(INT)s similar = NEG_LIMIT;
    %(INT)s length = NEG_LIMIT;
    %(VTYPE)s vNegLimit = %(VSET1)s(NEG_LIMIT);
    %(VTYPE)s vPosLimit = %(VSET1)s(POS_LIMIT);
    %(VTYPE)s vSaturationCheckMin = vPosLimit;
    %(VTYPE)s vSaturationCheckMax = vNegLimit;
    %(VTYPE)s vNegInf = %(VSET1)s(NEG_LIMIT);
    %(VTYPE)s vOpen = %(VSET1)s(open);
    %(VTYPE)s vGap  = %(VSET1)s(gap);
    %(VTYPE)s vZero = %(VSET1)s(0);
    %(VTYPE)s vOne = %(VSET1)s(1);
    %(VTYPE)s vN = %(VSET1)s(N);
    %(VTYPE)s vGapN = s1_beg ? %(VSET1)s(0) : %(VSET1)s(gap*N);
    %(VTYPE)s vNegOne = %(VSET1)s(-1);
    %(VTYPE)s vI = %(VSET)s(%(DIAG_I)s);
    %(VTYPE)s vJreset = %(VSET)s(%(DIAG_J)s);
    %(VTYPE)s vMaxHRow = vNegInf;
    %(VTYPE)s vMaxMRow = vNegInf;
    %(VTYPE)s vMaxSRow = vNegInf;
    %(VTYPE)s vMaxLRow = vNegInf;
    %(VTYPE)s vMaxHCol = vNegInf;
    %(VTYPE)s vMaxMCol = vNegInf;
    %(VTYPE)s vMaxSCol = vNegInf;
    %(VTYPE)s vMaxLCol = vNegInf;
    %(VTYPE)s vLastValH = vNegInf;
    %(VTYPE)s vLastValM = vNegInf;
    %(VTYPE)s vLastValS = vNegInf;
    %(VTYPE)s vLastValL = vNegInf;
    %(VTYPE)s vEndI = vNegInf;
    %(VTYPE)s vEndJ = vNegInf;
    %(VTYPE)s vILimit = %(VSET1)s(s1Len);
    %(VTYPE)s vILimit1 = %(VSUB)s(vILimit, vOne);
    %(VTYPE)s vJLimit = %(VSET1)s(s2Len);
    %(VTYPE)s vJLimit1 = %(VSUB)s(vJLimit, vOne);
    %(VTYPE)s vIBoundary = s1_beg ? %(VSET1)s(0) : %(VSET)s(
            %(DIAG_IBoundary)s);

    /* convert _s1 from char to int in range 0-23 */
    for (i=0; i<s1Len; ++i) {
        s1[i] = matrix->mapper[(unsigned char)_s1[i]];
    }
    /* pad back of s1 with dummy values */
    for (i=s1Len; i<s1Len_PAD; ++i) {
        s1[i] = 0; /* point to first matrix row because we don't care */
    }

    /* convert _s2 from char to int in range 0-23 */
    for (j=0; j<s2Len; ++j) {
        s2[j] = matrix->mapper[(unsigned char)_s2[j]];
    }
    /* pad front of s2 with dummy values */
    for (j=-PAD; j<0; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }
    /* pad back of s2 with dummy values */
    for (j=s2Len; j<s2Len_PAD; ++j) {
        s2[j] = 0; /* point to first matrix row because we don't care */
    }

    /* set initial values for stored row */
    if (s2_beg) {
        for (j=0; j<s2Len; ++j) {
            H_pr[j] = 0;
            HM_pr[j] = 0;
            HS_pr[j] = 0;
            HL_pr[j] = 0;
            F_pr[j] = NEG_INF;
            FM_pr[j] = 0;
            FS_pr[j] = 0;
            FL_pr[j] = 0;
        }
    }
    else {
        for (j=0; j<s2Len; ++j) {
            H_pr[j] = -open - j*gap;
            HM_pr[j] = 0;
            HS_pr[j] = 0;
            HL_pr[j] = 0;
            F_pr[j] = NEG_INF;
            FM_pr[j] = 0;
            FS_pr[j] = 0;
            FL_pr[j] = 0;
        }
    }
    /* pad front of stored row values */
    for (j=-PAD; j<0; ++j) {
        H_pr[j] = 0;
        HM_pr[j] = 0;
        HS_pr[j] = 0;
        HL_pr[j] = 0;
        F_pr[j] = 0;
        FM_pr[j] = 0;
        FS_pr[j] = 0;
        FL_pr[j] = 0;
    }
    /* pad back of stored row values */
    for (j=s2Len; j<s2Len+PAD; ++j) {
        H_pr[j] = 0;
        HM_pr[j] = 0;
        HS_pr[j] = 0;
        HL_pr[j] = 0;
        F_pr[j] = 0;
        FM_pr[j] = 0;
        FS_pr[j] = 0;
        FL_pr[j] = 0;
    }
    H_pr[-1] = 0; /* upper left corner */

    /* iterate over query sequence */
    for (i=0; i<s1Len; i+=N) {
        %(VTYPE)s case1 = vZero;
        %(VTYPE)s case2 = vZero;
        %(VTYPE)s vNH = vNegInf;
        %(VTYPE)s vNM = vZero;
        %(VTYPE)s vNS = vZero;
        %(VTYPE)s vNL = vZero;
        %(VTYPE)s vWH = vNegInf;
        %(VTYPE)s vWM = vZero;
        %(VTYPE)s vWS = vZero;
        %(VTYPE)s vWL = vZero;
        %(VTYPE)s vE = vNegInf;
        %(VTYPE)s vE_opn = vNegInf;
        %(VTYPE)s vE_ext = vNegInf;
        %(VTYPE)s vEM = vZero;
        %(VTYPE)s vES = vZero;
        %(VTYPE)s vEL = vZero;
        %(VTYPE)s vF = vNegInf;
        %(VTYPE)s vF_opn = vNegInf;
        %(VTYPE)s vF_ext = vNegInf;
        %(VTYPE)s vFM = vZero;
        %(VTYPE)s vFS = vZero;
        %(VTYPE)s vFL = vZero;
        %(VTYPE)s vJ = vJreset;
        %(VTYPE)s vs1 = %(VSET)s(
                %(DIAG_VS1)s);
        %(VTYPE)s vs2 = vNegInf;
        %(DIAG_MATROW_DECL)s
        %(VTYPE)s vIltLimit = %(VCMPLT)s(vI, vILimit);
        %(VTYPE)s vIeqLimit1 = %(VCMPEQ)s(vI, vILimit1);
        vNH = %(VRSHIFT)s(vNH, %(BYTES)s);
        vNH = %(VINSERT)s(vNH, H_pr[-1], %(LAST_POS)s);
        vWH = %(VRSHIFT)s(vWH, %(BYTES)s);
        vWH = %(VINSERT)s(vWH, s1_beg ? 0 : (-open - i*gap), %(LAST_POS)s);
        H_pr[-1] = -open - (i+N)*gap;
        /* iterate over database sequence */
        for (j=0; j<s2Len+PAD; ++j) {
            %(VTYPE)s vMat;
            %(VTYPE)s vNWH = vNH;
            %(VTYPE)s vNWM = vNM;
            %(VTYPE)s vNWS = vNS;
            %(VTYPE)s vNWL = vNL;
            vNH = %(VRSHIFT)s(vWH, %(BYTES)s);
            vNH = %(VINSERT)s(vNH, H_pr[j], %(LAST_POS)s);
            vNM = %(VRSHIFT)s(vWM, %(BYTES)s);
            vNM = %(VINSERT)s(vNM, HM_pr[j], %(LAST_POS)s);
            vNS = %(VRSHIFT)s(vWS, %(BYTES)s);
            vNS = %(VINSERT)s(vNS, HS_pr[j], %(LAST_POS)s);
            vNL = %(VRSHIFT)s(vWL, %(BYTES)s);
            vNL = %(VINSERT)s(vNL, HL_pr[j], %(LAST_POS)s);
            vF = %(VRSHIFT)s(vF, %(BYTES)s);
            vF = %(VINSERT)s(vF, F_pr[j], %(LAST_POS)s);
            vFM = %(VRSHIFT)s(vFM, %(BYTES)s);
            vFM = %(VINSERT)s(vFM, FM_pr[j], %(LAST_POS)s);
            vFS = %(VRSHIFT)s(vFS, %(BYTES)s);
            vFS = %(VINSERT)s(vFS, FS_pr[j], %(LAST_POS)s);
            vFL = %(VRSHIFT)s(vFL, %(BYTES)s);
            vFL = %(VINSERT)s(vFL, FL_pr[j], %(LAST_POS)s);
            vF_opn = %(VSUB)s(vNH, vOpen);
            vF_ext = %(VSUB)s(vF, vGap);
            vF = %(VMAX)s(vF_opn, vF_ext);
            case1 = %(VCMPGT)s(vF_opn, vF_ext);
            vFM = %(VBLEND)s(vFM, vNM, case1);
            vFS = %(VBLEND)s(vFS, vNS, case1);
            vFL = %(VBLEND)s(vFL, vNL, case1);
            vFL = %(VADD)s(vFL, vOne);
            vE_opn = %(VSUB)s(vWH, vOpen);
            vE_ext = %(VSUB)s(vE, vGap);
            vE = %(VMAX)s(vE_opn, vE_ext);
            case1 = %(VCMPGT)s(vE_opn, vE_ext);
            vEM = %(VBLEND)s(vEM, vWM, case1);
            vES = %(VBLEND)s(vES, vWS, case1);
            vEL = %(VBLEND)s(vEL, vWL, case1);
            vEL = %(VADD)s(vEL, vOne);
            vs2 = %(VRSHIFT)s(vs2, %(BYTES)s);
            vs2 = %(VINSERT)s(vs2, s2[j], %(LAST_POS)s);
            vMat = %(VSET)s(
                    %(DIAG_MATROW_USE)s
                    );
            vNWH = %(VADD)s(vNWH, vMat);
            vWH = %(VMAX)s(vNWH, vE);
            vWH = %(VMAX)s(vWH, vF);
            case1 = %(VCMPEQ)s(vWH, vNWH);
            case2 = %(VCMPEQ)s(vWH, vF);
            vWM = %(VBLEND)s(
                    %(VBLEND)s(vEM, vFM, case2),
                    %(VADD)s(vNWM,
                        %(VAND)s(
                            %(VCMPEQ)s(vs1,vs2),
                            vOne)),
                    case1);
            vWS = %(VBLEND)s(
                    %(VBLEND)s(vES, vFS, case2),
                    %(VADD)s(vNWS,
                        %(VAND)s(
                            %(VCMPGT)s(vMat,vZero),
                            vOne)),
                    case1);
            vWL = %(VBLEND)s(
                    %(VBLEND)s(vEL, vFL, case2),
                    %(VADD)s(vNWL, vOne), case1);
            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                %(VTYPE)s cond = %(VCMPEQ)s(vJ,vNegOne);
                vWH = %(VBLEND)s(vWH, vIBoundary, cond);
                vWM = %(VANDNOT)s(cond, vWM);
                vWS = %(VANDNOT)s(cond, vWS);
                vWL = %(VANDNOT)s(cond, vWL);
                vE = %(VBLEND)s(vE, vNegInf, cond);
                vEM = %(VANDNOT)s(cond, vEM);
                vES = %(VANDNOT)s(cond, vES);
                vEL = %(VANDNOT)s(cond, vEL);
            }
            vSaturationCheckMin = %(VMIN)s(vSaturationCheckMin, vWH);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vWH);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vWM);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vWS);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vWL);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vWL);
            vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vJ);
#ifdef PARASAIL_TABLE
            arr_store_si%(BITS)s(result->stats->tables->score_table, vWH, i, s1Len, j, s2Len);
            arr_store_si%(BITS)s(result->stats->tables->matches_table, vWM, i, s1Len, j, s2Len);
            arr_store_si%(BITS)s(result->stats->tables->similar_table, vWS, i, s1Len, j, s2Len);
            arr_store_si%(BITS)s(result->stats->tables->length_table, vWL, i, s1Len, j, s2Len);
#endif
#ifdef PARASAIL_ROWCOL
            arr_store_rowcol(result->stats->rowcols->score_row,   result->stats->rowcols->score_col, vWH, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->matches_row, result->stats->rowcols->matches_col, vWM, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->similar_row, result->stats->rowcols->similar_col, vWS, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->length_row,  result->stats->rowcols->length_col, vWL, i, s1Len, j, s2Len);
#endif
            H_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vWH,0);
            HM_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vWM,0);
            HS_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vWS,0);
            HL_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vWL,0);
            F_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vF,0);
            FM_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vFM,0);
            FS_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vFS,0);
            FL_pr[j-%(LAST_POS)s] = (%(INT)s)%(VEXTRACT)s(vFL,0);
            /* as minor diagonal vector passes across the i or j limit
             * boundary, extract the last value of the column or row */
            {
                %(VTYPE)s vJeqLimit1 = %(VCMPEQ)s(vJ, vJLimit1);
                %(VTYPE)s vJgtNegOne = %(VCMPGT)s(vJ, vNegOne);
                %(VTYPE)s vJltLimit = %(VCMPLT)s(vJ, vJLimit);
                %(VTYPE)s cond_j = %(VAND)s(vIltLimit, vJeqLimit1);
                %(VTYPE)s cond_i = %(VAND)s(vIeqLimit1,
                        %(VAND)s(vJgtNegOne, vJltLimit));
                %(VTYPE)s cond_max_row = %(VCMPGT)s(vWH, vMaxHRow);
                %(VTYPE)s cond_max_col = %(VCMPGT)s(vWH, vMaxHCol);
                %(VTYPE)s cond_last_val = %(VAND)s(vIeqLimit1, vJeqLimit1);
                %(VTYPE)s cond_all_row = %(VAND)s(cond_max_row, cond_i);
                %(VTYPE)s cond_all_col = %(VAND)s(cond_max_col, cond_j);
                vMaxHRow = %(VBLEND)s(vMaxHRow, vWH, cond_all_row);
                vMaxMRow = %(VBLEND)s(vMaxMRow, vWM, cond_all_row);
                vMaxSRow = %(VBLEND)s(vMaxSRow, vWS, cond_all_row);
                vMaxLRow = %(VBLEND)s(vMaxLRow, vWL, cond_all_row);
                vMaxHCol = %(VBLEND)s(vMaxHCol, vWH, cond_all_col);
                vMaxMCol = %(VBLEND)s(vMaxMCol, vWM, cond_all_col);
                vMaxSCol = %(VBLEND)s(vMaxSCol, vWS, cond_all_col);
                vMaxLCol = %(VBLEND)s(vMaxLCol, vWL, cond_all_col);
                vLastValH = %(VBLEND)s(vLastValH, vWH, cond_last_val);
                vLastValM = %(VBLEND)s(vLastValM, vWM, cond_last_val);
                vLastValS = %(VBLEND)s(vLastValS, vWS, cond_last_val);
                vLastValL = %(VBLEND)s(vLastValL, vWL, cond_last_val);
                vEndI = %(VBLEND)s(vEndI, vI, cond_all_col);
                vEndJ = %(VBLEND)s(vEndJ, vJ, cond_all_row);
            }
            vJ = %(VADD)s(vJ, vOne);
        }
        vI = %(VADD)s(vI, vN);
        vIBoundary = %(VSUB)s(vIBoundary, vGapN);
        vSaturationCheckMax = %(VMAX)s(vSaturationCheckMax, vI);
    }

    /* alignment ending position */
    {
        %(INT)s max_rowh = NEG_INF;
        %(INT)s max_rowm = NEG_INF;
        %(INT)s max_rows = NEG_INF;
        %(INT)s max_rowl = NEG_INF;
        %(INT)s max_colh = NEG_INF;
        %(INT)s max_colm = NEG_INF;
        %(INT)s max_cols = NEG_INF;
        %(INT)s max_coll = NEG_INF;
        %(INT)s last_valh = NEG_INF;
        %(INT)s last_valm = NEG_INF;
        %(INT)s last_vals = NEG_INF;
        %(INT)s last_vall = NEG_INF;
        %(INT)s *rh = (%(INT)s*)&vMaxHRow;
        %(INT)s *rm = (%(INT)s*)&vMaxMRow;
        %(INT)s *rs = (%(INT)s*)&vMaxSRow;
        %(INT)s *rl = (%(INT)s*)&vMaxLRow;
        %(INT)s *ch = (%(INT)s*)&vMaxHCol;
        %(INT)s *cm = (%(INT)s*)&vMaxMCol;
        %(INT)s *cs = (%(INT)s*)&vMaxSCol;
        %(INT)s *cl = (%(INT)s*)&vMaxLCol;
        %(INT)s *lh = (%(INT)s*)&vLastValH;
        %(INT)s *lm = (%(INT)s*)&vLastValM;
        %(INT)s *ls = (%(INT)s*)&vLastValS;
        %(INT)s *ll = (%(INT)s*)&vLastValL;
        %(INT)s *i = (%(INT)s*)&vEndI;
        %(INT)s *j = (%(INT)s*)&vEndJ;
        %(INDEX)s k;
        for (k=0; k<N; ++k, ++rh, ++rm, ++rs, ++rl, ++ch, ++cm, ++cs, ++cl, ++lh, ++lm, ++ls, ++ll, ++i, ++j) {
            if (*ch > max_colh || (*ch == max_colh && *i < end_query)) {
                max_colh = *ch;
                end_query = *i;
                max_colm = *cm;
                max_cols = *cs;
                max_coll = *cl;
            }
            if (*rh > max_rowh) {
                max_rowh = *rh;
                end_ref = *j;
                max_rowm = *rm;
                max_rows = *rs;
                max_rowl = *rl;
            }
            if (*lh > last_valh) {
                last_valh = *lh;
                last_valm = *lm;
                last_vals = *ls;
                last_vall = *ll;
            }
        }
        if (s1_end && s2_end) {
            if (max_colh > max_rowh || (max_colh == max_rowh && end_ref == s2Len-1)) {
                score = max_colh;
                end_ref = s2Len-1;
                matches = max_colm;
                similar = max_cols;
                length = max_coll;
            }
            else {
                score = max_rowh;
                end_query = s1Len-1;
                matches = max_rowm;
                similar = max_rows;
                length = max_rowl;
            }
        }
        else if (s1_end) {
            score = max_colh;
            end_ref = s2Len-1;
            matches = max_colm;
            similar = max_cols;
            length = max_coll;
        }
        else if (s2_end) {
            score = max_rowh;
            end_query = s1Len-1;
            matches = max_rowm;
            similar = max_rows;
            length = max_rowl;
        }
        else {
            score = last_valh;
            end_query = s1Len-1;
            end_ref = s2Len-1;
            matches = last_valm;
            similar = last_vals;
            length = last_vall;
        }
    }

    if (%(VMOVEMASK)s(%(VOR)s(
            %(VCMPLT)s(vSaturationCheckMin, vNegLimit),
            %(VCMPGT)s(vSaturationCheckMax, vPosLimit)))) {
        result->flag |= PARASAIL_FLAG_SATURATED;
        score = 0;
        matches = 0;
        similar = 0;
        length = 0;
        end_query = 0;
        end_ref = 0;
    }

    result->score = score;
    result->end_query = end_query;
    result->end_ref = end_ref;
    result->stats->matches = matches;
    result->stats->similar = similar;
    result->stats->length = length;
    result->flag |= PARASAIL_FLAG_SG | PARASAIL_FLAG_DIAG
        | PARASAIL_FLAG_STATS
        | PARASAIL_FLAG_BITS_%(WIDTH)s | PARASAIL_FLAG_LANES_%(LANES)s;
    result->flag |= s1_beg ? PARASAIL_FLAG_SG_S1_BEG : 0;
    result->flag |= s1_end ? PARASAIL_FLAG_SG_S1_END : 0;
    result->flag |= s2_beg ? PARASAIL_FLAG_SG_S2_BEG : 0;
    result->flag |= s2_end ? PARASAIL_FLAG_SG_S2_END : 0;
#ifdef PARASAIL_TABLE
    result->flag |= PARASAIL_FLAG_TABLE;
#endif
#ifdef PARASAIL_ROWCOL
    result->flag |= PARASAIL_FLAG_ROWCOL;
#endif

    parasail_free(_FL_pr);
    parasail_free(_FS_pr);
    parasail_free(_FM_pr);
    parasail_free(_F_pr);
    parasail_free(_HL_pr);
    parasail_free(_HS_pr);
    parasail_free(_HM_pr);
    parasail_free(_H_pr);
    parasail_free(s2B);
    parasail_free(s1);

    return result;
}

SG_IMPL_ALL
