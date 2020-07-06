/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2015 Battelle Memorial Institute.
 */
#include "config.h"

#include <stdlib.h>



#include "parasail.h"
#include "parasail/memory.h"
#include "parasail/internal_altivec.h"

#define SG_STATS
#define SG_SUFFIX _diag_altivec_128_32
#include "sg_helper.h"

#define NEG_INF (INT32_MIN/(int32_t)(2))


#ifdef PARASAIL_TABLE
static inline void arr_store_si128(
        int *array,
        vec128i vWH,
        int32_t i,
        int32_t s1Len,
        int32_t j,
        int32_t s2Len)
{
    if (0 <= i+0 && i+0 < s1Len && 0 <= j-0 && j-0 < s2Len) {
        array[1LL*(i+0)*s2Len + (j-0)] = (int32_t)_mm_extract_epi32(vWH, 3);
    }
    if (0 <= i+1 && i+1 < s1Len && 0 <= j-1 && j-1 < s2Len) {
        array[1LL*(i+1)*s2Len + (j-1)] = (int32_t)_mm_extract_epi32(vWH, 2);
    }
    if (0 <= i+2 && i+2 < s1Len && 0 <= j-2 && j-2 < s2Len) {
        array[1LL*(i+2)*s2Len + (j-2)] = (int32_t)_mm_extract_epi32(vWH, 1);
    }
    if (0 <= i+3 && i+3 < s1Len && 0 <= j-3 && j-3 < s2Len) {
        array[1LL*(i+3)*s2Len + (j-3)] = (int32_t)_mm_extract_epi32(vWH, 0);
    }
}
#endif

#ifdef PARASAIL_ROWCOL
static inline void arr_store_rowcol(
        int *row,
        int *col,
        vec128i vWH,
        int32_t i,
        int32_t s1Len,
        int32_t j,
        int32_t s2Len)
{
    if (i+0 == s1Len-1 && 0 <= j-0 && j-0 < s2Len) {
        row[j-0] = (int32_t)_mm_extract_epi32(vWH, 3);
    }
    if (j-0 == s2Len-1 && 0 <= i+0 && i+0 < s1Len) {
        col[(i+0)] = (int32_t)_mm_extract_epi32(vWH, 3);
    }
    if (i+1 == s1Len-1 && 0 <= j-1 && j-1 < s2Len) {
        row[j-1] = (int32_t)_mm_extract_epi32(vWH, 2);
    }
    if (j-1 == s2Len-1 && 0 <= i+1 && i+1 < s1Len) {
        col[(i+1)] = (int32_t)_mm_extract_epi32(vWH, 2);
    }
    if (i+2 == s1Len-1 && 0 <= j-2 && j-2 < s2Len) {
        row[j-2] = (int32_t)_mm_extract_epi32(vWH, 1);
    }
    if (j-2 == s2Len-1 && 0 <= i+2 && i+2 < s1Len) {
        col[(i+2)] = (int32_t)_mm_extract_epi32(vWH, 1);
    }
    if (i+3 == s1Len-1 && 0 <= j-3 && j-3 < s2Len) {
        row[j-3] = (int32_t)_mm_extract_epi32(vWH, 0);
    }
    if (j-3 == s2Len-1 && 0 <= i+3 && i+3 < s1Len) {
        col[(i+3)] = (int32_t)_mm_extract_epi32(vWH, 0);
    }
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME parasail_sg_flags_stats_table_diag_altivec_128_32
#else
#ifdef PARASAIL_ROWCOL
#define FNAME parasail_sg_flags_stats_rowcol_diag_altivec_128_32
#else
#define FNAME parasail_sg_flags_stats_diag_altivec_128_32
#endif
#endif

parasail_result_t* FNAME(
        const char * const restrict _s1, const int s1Len,
        const char * const restrict _s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix,
        int s1_beg, int s1_end, int s2_beg, int s2_end)
{
    const int32_t N = 4; /* number of values in vector */
    const int32_t PAD = N-1;
    const int32_t PAD2 = PAD*2;
    const int32_t s1Len_PAD = s1Len+PAD;
    const int32_t s2Len_PAD = s2Len+PAD;
    int32_t * const restrict s1      = parasail_memalign_int32_t(16, s1Len+PAD);
    int32_t * const restrict s2B     = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _H_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _HM_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _HS_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _HL_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _F_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _FM_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _FS_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict _FL_pr = parasail_memalign_int32_t(16, s2Len+PAD2);
    int32_t * const restrict s2 = s2B+PAD; /* will allow later for negative indices */
    int32_t * const restrict H_pr = _H_pr+PAD;
    int32_t * const restrict HM_pr = _HM_pr+PAD;
    int32_t * const restrict HS_pr = _HS_pr+PAD;
    int32_t * const restrict HL_pr = _HL_pr+PAD;
    int32_t * const restrict F_pr = _F_pr+PAD;
    int32_t * const restrict FM_pr = _FM_pr+PAD;
    int32_t * const restrict FS_pr = _FS_pr+PAD;
    int32_t * const restrict FL_pr = _FL_pr+PAD;
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(s1Len, s2Len);
#else
#ifdef PARASAIL_ROWCOL
    parasail_result_t *result = parasail_result_new_rowcol3(s1Len, s2Len);
#else
    parasail_result_t *result = parasail_result_new_stats();
#endif
#endif
    int32_t i = 0;
    int32_t j = 0;
    int32_t end_query = s1Len-1;
    int32_t end_ref = s2Len-1;
    const int32_t NEG_LIMIT = (-open < matrix->min ?
        INT32_MIN + open : INT32_MIN - matrix->min) + 1;
    const int32_t POS_LIMIT = INT32_MAX - matrix->max - 1;
    int32_t score = NEG_LIMIT;
    int32_t matches = NEG_LIMIT;
    int32_t similar = NEG_LIMIT;
    int32_t length = NEG_LIMIT;
    vec128i vNegLimit = _mm_set1_epi32(NEG_LIMIT);
    vec128i vPosLimit = _mm_set1_epi32(POS_LIMIT);
    vec128i vSaturationCheckMin = vPosLimit;
    vec128i vSaturationCheckMax = vNegLimit;
    vec128i vNegInf = _mm_set1_epi32(NEG_LIMIT);
    vec128i vOpen = _mm_set1_epi32(open);
    vec128i vGap  = _mm_set1_epi32(gap);
    vec128i vZero = _mm_set1_epi32(0);
    vec128i vOne = _mm_set1_epi32(1);
    vec128i vN = _mm_set1_epi32(N);
    vec128i vGapN = s1_beg ? _mm_set1_epi32(0) : _mm_set1_epi32(gap*N);
    vec128i vNegOne = _mm_set1_epi32(-1);
    vec128i vI = _mm_set_epi32(0,1,2,3);
    vec128i vJreset = _mm_set_epi32(0,-1,-2,-3);
    vec128i vMaxHRow = vNegInf;
    vec128i vMaxMRow = vNegInf;
    vec128i vMaxSRow = vNegInf;
    vec128i vMaxLRow = vNegInf;
    vec128i vMaxHCol = vNegInf;
    vec128i vMaxMCol = vNegInf;
    vec128i vMaxSCol = vNegInf;
    vec128i vMaxLCol = vNegInf;
    vec128i vLastValH = vNegInf;
    vec128i vLastValM = vNegInf;
    vec128i vLastValS = vNegInf;
    vec128i vLastValL = vNegInf;
    vec128i vEndI = vNegInf;
    vec128i vEndJ = vNegInf;
    vec128i vILimit = _mm_set1_epi32(s1Len);
    vec128i vILimit1 = _mm_sub_epi32(vILimit, vOne);
    vec128i vJLimit = _mm_set1_epi32(s2Len);
    vec128i vJLimit1 = _mm_sub_epi32(vJLimit, vOne);
    vec128i vIBoundary = s1_beg ? _mm_set1_epi32(0) : _mm_set_epi32(
            -open-0*gap,
            -open-1*gap,
            -open-2*gap,
            -open-3*gap);

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
        vec128i case1 = vZero;
        vec128i case2 = vZero;
        vec128i vNH = vNegInf;
        vec128i vNM = vZero;
        vec128i vNS = vZero;
        vec128i vNL = vZero;
        vec128i vWH = vNegInf;
        vec128i vWM = vZero;
        vec128i vWS = vZero;
        vec128i vWL = vZero;
        vec128i vE = vNegInf;
        vec128i vE_opn = vNegInf;
        vec128i vE_ext = vNegInf;
        vec128i vEM = vZero;
        vec128i vES = vZero;
        vec128i vEL = vZero;
        vec128i vF = vNegInf;
        vec128i vF_opn = vNegInf;
        vec128i vF_ext = vNegInf;
        vec128i vFM = vZero;
        vec128i vFS = vZero;
        vec128i vFL = vZero;
        vec128i vJ = vJreset;
        vec128i vs1 = _mm_set_epi32(
                s1[i+0],
                s1[i+1],
                s1[i+2],
                s1[i+3]);
        vec128i vs2 = vNegInf;
        const int * const restrict matrow0 = &matrix->matrix[matrix->size*s1[i+0]];
        const int * const restrict matrow1 = &matrix->matrix[matrix->size*s1[i+1]];
        const int * const restrict matrow2 = &matrix->matrix[matrix->size*s1[i+2]];
        const int * const restrict matrow3 = &matrix->matrix[matrix->size*s1[i+3]];
        vec128i vIltLimit = _mm_cmplt_epi32(vI, vILimit);
        vec128i vIeqLimit1 = _mm_cmpeq_epi32(vI, vILimit1);
        vNH = _mm_srli_si128(vNH, 4);
        vNH = _mm_insert_epi32(vNH, H_pr[-1], 3);
        vWH = _mm_srli_si128(vWH, 4);
        vWH = _mm_insert_epi32(vWH, s1_beg ? 0 : (-open - i*gap), 3);
        H_pr[-1] = -open - (i+N)*gap;
        /* iterate over database sequence */
        for (j=0; j<s2Len+PAD; ++j) {
            vec128i vMat;
            vec128i vNWH = vNH;
            vec128i vNWM = vNM;
            vec128i vNWS = vNS;
            vec128i vNWL = vNL;
            vNH = _mm_srli_si128(vWH, 4);
            vNH = _mm_insert_epi32(vNH, H_pr[j], 3);
            vNM = _mm_srli_si128(vWM, 4);
            vNM = _mm_insert_epi32(vNM, HM_pr[j], 3);
            vNS = _mm_srli_si128(vWS, 4);
            vNS = _mm_insert_epi32(vNS, HS_pr[j], 3);
            vNL = _mm_srli_si128(vWL, 4);
            vNL = _mm_insert_epi32(vNL, HL_pr[j], 3);
            vF = _mm_srli_si128(vF, 4);
            vF = _mm_insert_epi32(vF, F_pr[j], 3);
            vFM = _mm_srli_si128(vFM, 4);
            vFM = _mm_insert_epi32(vFM, FM_pr[j], 3);
            vFS = _mm_srli_si128(vFS, 4);
            vFS = _mm_insert_epi32(vFS, FS_pr[j], 3);
            vFL = _mm_srli_si128(vFL, 4);
            vFL = _mm_insert_epi32(vFL, FL_pr[j], 3);
            vF_opn = _mm_sub_epi32(vNH, vOpen);
            vF_ext = _mm_sub_epi32(vF, vGap);
            vF = _mm_max_epi32(vF_opn, vF_ext);
            case1 = _mm_cmpgt_epi32(vF_opn, vF_ext);
            vFM = _mm_blendv_epi8(vFM, vNM, case1);
            vFS = _mm_blendv_epi8(vFS, vNS, case1);
            vFL = _mm_blendv_epi8(vFL, vNL, case1);
            vFL = _mm_add_epi32(vFL, vOne);
            vE_opn = _mm_sub_epi32(vWH, vOpen);
            vE_ext = _mm_sub_epi32(vE, vGap);
            vE = _mm_max_epi32(vE_opn, vE_ext);
            case1 = _mm_cmpgt_epi32(vE_opn, vE_ext);
            vEM = _mm_blendv_epi8(vEM, vWM, case1);
            vES = _mm_blendv_epi8(vES, vWS, case1);
            vEL = _mm_blendv_epi8(vEL, vWL, case1);
            vEL = _mm_add_epi32(vEL, vOne);
            vs2 = _mm_srli_si128(vs2, 4);
            vs2 = _mm_insert_epi32(vs2, s2[j], 3);
            vMat = _mm_set_epi32(
                    matrow0[s2[j-0]],
                    matrow1[s2[j-1]],
                    matrow2[s2[j-2]],
                    matrow3[s2[j-3]]
                    );
            vNWH = _mm_add_epi32(vNWH, vMat);
            vWH = _mm_max_epi32(vNWH, vE);
            vWH = _mm_max_epi32(vWH, vF);
            case1 = _mm_cmpeq_epi32(vWH, vNWH);
            case2 = _mm_cmpeq_epi32(vWH, vF);
            vWM = _mm_blendv_epi8(
                    _mm_blendv_epi8(vEM, vFM, case2),
                    _mm_add_epi32(vNWM,
                        _mm_and_si128(
                            _mm_cmpeq_epi32(vs1,vs2),
                            vOne)),
                    case1);
            vWS = _mm_blendv_epi8(
                    _mm_blendv_epi8(vES, vFS, case2),
                    _mm_add_epi32(vNWS,
                        _mm_and_si128(
                            _mm_cmpgt_epi32(vMat,vZero),
                            vOne)),
                    case1);
            vWL = _mm_blendv_epi8(
                    _mm_blendv_epi8(vEL, vFL, case2),
                    _mm_add_epi32(vNWL, vOne), case1);
            /* as minor diagonal vector passes across the j=-1 boundary,
             * assign the appropriate boundary conditions */
            {
                vec128i cond = _mm_cmpeq_epi32(vJ,vNegOne);
                vWH = _mm_blendv_epi8(vWH, vIBoundary, cond);
                vWM = _mm_andnot_si128(cond, vWM);
                vWS = _mm_andnot_si128(cond, vWS);
                vWL = _mm_andnot_si128(cond, vWL);
                vE = _mm_blendv_epi8(vE, vNegInf, cond);
                vEM = _mm_andnot_si128(cond, vEM);
                vES = _mm_andnot_si128(cond, vES);
                vEL = _mm_andnot_si128(cond, vEL);
            }
            vSaturationCheckMin = _mm_min_epi32(vSaturationCheckMin, vWH);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vWH);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vWM);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vWS);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vWL);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vWL);
            vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vJ);
#ifdef PARASAIL_TABLE
            arr_store_si128(result->stats->tables->score_table, vWH, i, s1Len, j, s2Len);
            arr_store_si128(result->stats->tables->matches_table, vWM, i, s1Len, j, s2Len);
            arr_store_si128(result->stats->tables->similar_table, vWS, i, s1Len, j, s2Len);
            arr_store_si128(result->stats->tables->length_table, vWL, i, s1Len, j, s2Len);
#endif
#ifdef PARASAIL_ROWCOL
            arr_store_rowcol(result->stats->rowcols->score_row,   result->stats->rowcols->score_col, vWH, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->matches_row, result->stats->rowcols->matches_col, vWM, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->similar_row, result->stats->rowcols->similar_col, vWS, i, s1Len, j, s2Len);
            arr_store_rowcol(result->stats->rowcols->length_row,  result->stats->rowcols->length_col, vWL, i, s1Len, j, s2Len);
#endif
            H_pr[j-3] = (int32_t)_mm_extract_epi32(vWH,0);
            HM_pr[j-3] = (int32_t)_mm_extract_epi32(vWM,0);
            HS_pr[j-3] = (int32_t)_mm_extract_epi32(vWS,0);
            HL_pr[j-3] = (int32_t)_mm_extract_epi32(vWL,0);
            F_pr[j-3] = (int32_t)_mm_extract_epi32(vF,0);
            FM_pr[j-3] = (int32_t)_mm_extract_epi32(vFM,0);
            FS_pr[j-3] = (int32_t)_mm_extract_epi32(vFS,0);
            FL_pr[j-3] = (int32_t)_mm_extract_epi32(vFL,0);
            /* as minor diagonal vector passes across the i or j limit
             * boundary, extract the last value of the column or row */
            {
                vec128i vJeqLimit1 = _mm_cmpeq_epi32(vJ, vJLimit1);
                vec128i vJgtNegOne = _mm_cmpgt_epi32(vJ, vNegOne);
                vec128i vJltLimit = _mm_cmplt_epi32(vJ, vJLimit);
                vec128i cond_j = _mm_and_si128(vIltLimit, vJeqLimit1);
                vec128i cond_i = _mm_and_si128(vIeqLimit1,
                        _mm_and_si128(vJgtNegOne, vJltLimit));
                vec128i cond_max_row = _mm_cmpgt_epi32(vWH, vMaxHRow);
                vec128i cond_max_col = _mm_cmpgt_epi32(vWH, vMaxHCol);
                vec128i cond_last_val = _mm_and_si128(vIeqLimit1, vJeqLimit1);
                vec128i cond_all_row = _mm_and_si128(cond_max_row, cond_i);
                vec128i cond_all_col = _mm_and_si128(cond_max_col, cond_j);
                vMaxHRow = _mm_blendv_epi8(vMaxHRow, vWH, cond_all_row);
                vMaxMRow = _mm_blendv_epi8(vMaxMRow, vWM, cond_all_row);
                vMaxSRow = _mm_blendv_epi8(vMaxSRow, vWS, cond_all_row);
                vMaxLRow = _mm_blendv_epi8(vMaxLRow, vWL, cond_all_row);
                vMaxHCol = _mm_blendv_epi8(vMaxHCol, vWH, cond_all_col);
                vMaxMCol = _mm_blendv_epi8(vMaxMCol, vWM, cond_all_col);
                vMaxSCol = _mm_blendv_epi8(vMaxSCol, vWS, cond_all_col);
                vMaxLCol = _mm_blendv_epi8(vMaxLCol, vWL, cond_all_col);
                vLastValH = _mm_blendv_epi8(vLastValH, vWH, cond_last_val);
                vLastValM = _mm_blendv_epi8(vLastValM, vWM, cond_last_val);
                vLastValS = _mm_blendv_epi8(vLastValS, vWS, cond_last_val);
                vLastValL = _mm_blendv_epi8(vLastValL, vWL, cond_last_val);
                vEndI = _mm_blendv_epi8(vEndI, vI, cond_all_col);
                vEndJ = _mm_blendv_epi8(vEndJ, vJ, cond_all_row);
            }
            vJ = _mm_add_epi32(vJ, vOne);
        }
        vI = _mm_add_epi32(vI, vN);
        vIBoundary = _mm_sub_epi32(vIBoundary, vGapN);
        vSaturationCheckMax = _mm_max_epi32(vSaturationCheckMax, vI);
    }

    /* alignment ending position */
    {
        int32_t max_rowh = NEG_INF;
        int32_t max_rowm = NEG_INF;
        int32_t max_rows = NEG_INF;
        int32_t max_rowl = NEG_INF;
        int32_t max_colh = NEG_INF;
        int32_t max_colm = NEG_INF;
        int32_t max_cols = NEG_INF;
        int32_t max_coll = NEG_INF;
        int32_t last_valh = NEG_INF;
        int32_t last_valm = NEG_INF;
        int32_t last_vals = NEG_INF;
        int32_t last_vall = NEG_INF;
        int32_t *rh = (int32_t*)&vMaxHRow;
        int32_t *rm = (int32_t*)&vMaxMRow;
        int32_t *rs = (int32_t*)&vMaxSRow;
        int32_t *rl = (int32_t*)&vMaxLRow;
        int32_t *ch = (int32_t*)&vMaxHCol;
        int32_t *cm = (int32_t*)&vMaxMCol;
        int32_t *cs = (int32_t*)&vMaxSCol;
        int32_t *cl = (int32_t*)&vMaxLCol;
        int32_t *lh = (int32_t*)&vLastValH;
        int32_t *lm = (int32_t*)&vLastValM;
        int32_t *ls = (int32_t*)&vLastValS;
        int32_t *ll = (int32_t*)&vLastValL;
        int32_t *i = (int32_t*)&vEndI;
        int32_t *j = (int32_t*)&vEndJ;
        int32_t k;
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

    if (_mm_movemask_epi8(_mm_or_si128(
            _mm_cmplt_epi32(vSaturationCheckMin, vNegLimit),
            _mm_cmpgt_epi32(vSaturationCheckMax, vPosLimit)))) {
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
        | PARASAIL_FLAG_BITS_32 | PARASAIL_FLAG_LANES_4;
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

