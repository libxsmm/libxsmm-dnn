/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/libxsmm/libxsmm-dnn/                *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Alexander Heinecke (Intel Corp.)
******************************************************************************/
#include <libxsmm_dnn.h>
#include <dnn_common.h>

#if defined(_OPENMP)
# include <omp.h>
#endif


int main(int argc, char* argv[])
{
  float *naive_input, *naive_output, *naive_filter, *naive_delinput, *naive_deloutput, *naive_delfilter, *naive_bias, *naive_delbias;
  libxsmm_bfloat16 *naive_input_bf16, *naive_filter_bf16, *naive_output_bf16, *naive_delinput_bf16, *naive_delfilter_bf16, *naive_deloutput_bf16, *naive_bias_bf16, *naive_delbias_bf16;
  libxsmm_bfloat8 *naive_input_bf8, *naive_filter_bf8, *naive_output_bf8, *naive_delinput_bf8, *naive_delfilter_bf8, *naive_deloutput_bf8, *naive_bias_bf8, *naive_delbias_bf8;

  float *naive_libxsmm_output, *naive_libxsmm_delinput, *naive_libxsmm_delfilter;
  libxsmm_bfloat16 *naive_libxsmm_output_bf16, *naive_libxsmm_delinput_bf16, *naive_libxsmm_delfilter_bf16, *naive_libxsmm_delbias_bf16;
  libxsmm_bfloat8 *naive_libxsmm_output_bf8, *naive_libxsmm_delinput_bf8, *naive_libxsmm_delfilter_bf8, *naive_libxsmm_delbias_bf8;

  float *input_libxsmm, *output_libxsmm, *filter_libxsmm, *delinput_libxsmm, *deloutput_libxsmm, *delfilter_libxsmm, *bias_libxsmm, *delbias_libxsmm;
  libxsmm_bfloat16 *input_libxsmm_bf16, *output_libxsmm_bf16, *filter_libxsmm_bf16, *delinput_libxsmm_bf16, *deloutput_libxsmm_bf16, *delfilter_libxsmm_bf16, *bias_libxsmm_bf16, *delbias_libxsmm_bf16;
  libxsmm_bfloat8 *input_libxsmm_bf8, *output_libxsmm_bf8, *filter_libxsmm_bf8, *delinput_libxsmm_bf8, *deloutput_libxsmm_bf8, *delfilter_libxsmm_bf8, *bias_libxsmm_bf8, *delbias_libxsmm_bf8;
  unsigned char *relumask_libxsmm;

  libxsmm_datatype in_dt, out_dt, comp_dt;

  libxsmm_dnn_fc_eltw_fuse my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_NONE;
  libxsmm_dnn_fc_layout my_layout = LIBXSMM_DNN_FC_LAYOUT_PACKED;
  libxsmm_dnn_fc_fwd_config libxsmm_dnn_fc_fwd;
  libxsmm_dnn_fc_bwd_config libxsmm_dnn_fc_bwd;

  naive_fullyconnected_t naive_param;
  void* scratch = 0;;

  /* some parameters we can overwrite via cli,
     default is some inner layer of overfeat */
  int iters = 10;         /* repetitions of benchmark */
  int nImg = 256;          /* mini-batch size, "N" */
  int nIFm = 1024;          /* number of input feature maps, "C" */
  int nOFm = 1024;          /* number of output feature maps, "K" */
  int fuse_type = 0;      /* 0: nothing fused, 1: elementwise fused, 2: relu fused, 3: relu and elementwise fused, 4: relu fused with mask, 5: relu with mask and elementwise fused */
  char type = 'A';        /* 'A': ALL, 'F': FP, 'B': BP, 'U', WU */
  int bn = 32;
  int bk = 32;
  int bc = 32;
  int prec = 4;   /* 4=f32, 2=bf16, 1=bf8 */
  int layout = 0;

  const char *const env_check = getenv("CHECK");
  const double check = LIBXSMM_ABS(NULL == env_check ? 1 : atof(env_check));

#if defined(_OPENMP)
  int nThreads = omp_get_max_threads(); /* number of threads */
#else
  int nThreads = 1; /* number of threads */
#endif

  libxsmm_timer_tickint l_start, l_end;
  double l_total = 0.0;
  double gflop = 0.0;
  int i;

  libxsmm_matdiff_info norms_fwd, norms_bwd, norms_upd, diff;
  libxsmm_matdiff_clear(&norms_fwd);
  libxsmm_matdiff_clear(&norms_bwd);
  libxsmm_matdiff_clear(&norms_upd);
  libxsmm_matdiff_clear(&diff);

  naive_input = NULL;
  naive_output = NULL;
  naive_filter = NULL;
  naive_delinput = NULL;
  naive_deloutput = NULL;
  naive_delfilter = NULL;
  naive_bias = NULL;
  naive_delbias = NULL;
  naive_input_bf16 = NULL;
  naive_filter_bf16 = NULL;
  naive_output_bf16 = NULL;
  naive_delinput_bf16 = NULL;
  naive_delfilter_bf16 = NULL;
  naive_deloutput_bf16 = NULL;
  naive_bias_bf16 = NULL;
  naive_delbias_bf16 = NULL;
  naive_input_bf8 = NULL;
  naive_filter_bf8 = NULL;
  naive_output_bf8 = NULL;
  naive_delinput_bf8 = NULL;
  naive_delfilter_bf8 = NULL;
  naive_deloutput_bf8 = NULL;
  naive_bias_bf8 = NULL;
  naive_delbias_bf8 = NULL;
  naive_libxsmm_output = NULL;
  naive_libxsmm_delinput = NULL;
  naive_libxsmm_delfilter = NULL;
  naive_libxsmm_output_bf16 = NULL;
  naive_libxsmm_delinput_bf16 = NULL;
  naive_libxsmm_delfilter_bf16 = NULL;
  naive_libxsmm_delbias_bf16 = NULL;
  naive_libxsmm_output_bf8 = NULL;
  naive_libxsmm_delinput_bf8 = NULL;
  naive_libxsmm_delfilter_bf8 = NULL;
  naive_libxsmm_delbias_bf8 = NULL;
  input_libxsmm = NULL;
  output_libxsmm = NULL;
  filter_libxsmm = NULL;
  delinput_libxsmm = NULL;
  deloutput_libxsmm = NULL;
  delfilter_libxsmm = NULL;
  bias_libxsmm = NULL;
  delbias_libxsmm = NULL;
  input_libxsmm_bf16 = NULL;
  output_libxsmm_bf16 = NULL;
  filter_libxsmm_bf16 = NULL;
  delinput_libxsmm_bf16 = NULL;
  deloutput_libxsmm_bf16 = NULL;
  delfilter_libxsmm_bf16 = NULL;
  bias_libxsmm_bf16 = NULL;
  delbias_libxsmm_bf16 = NULL;
  input_libxsmm_bf8 = NULL;
  output_libxsmm_bf8 = NULL;
  filter_libxsmm_bf8 = NULL;
  delinput_libxsmm_bf8 = NULL;
  deloutput_libxsmm_bf8 = NULL;
  delfilter_libxsmm_bf8 = NULL;
  bias_libxsmm_bf8 = NULL;
  delbias_libxsmm_bf8 = NULL;
  relumask_libxsmm = NULL;

  if (argc > 1 && !strncmp(argv[1], "-h", 3)) {
    printf("Usage: %s iters nImg nIFm nOFm fuse_type type bn bk bc prec vnnipack\n", argv[0]);
    return 0;
  }
  libxsmm_rng_set_seed(1);

  /* reading new values from cli */
  i = 1;
  if (argc > i) iters      = atoi(argv[i++]);
  if (argc > i) nImg       = atoi(argv[i++]);
  if (argc > i) nIFm       = atoi(argv[i++]);
  if (argc > i) nOFm       = atoi(argv[i++]);
  if (argc > i) fuse_type  = atoi(argv[i++]);
  if (argc > i) type       = *(argv[i++]);
  if (argc > i) bn         = atoi(argv[i++]);
  if (argc > i) bk         = atoi(argv[i++]);
  if (argc > i) bc         = atoi(argv[i++]);
  if (argc > i) prec       = atoi(argv[i++]);
  if (argc > i) layout     = atoi(argv[i++]);

  if (type != 'A' && type != 'F' && type != 'B' && type != 'U' && type != 'M') {
    printf("type needs to be 'A' (All), 'F' (FP only), 'B' (BP only), 'U' (UP only). 'M' (BPUP-fused only)\n");
    return -1;
  }
  if ( (fuse_type < 0) || (fuse_type > 5) ) {
    printf("fuse type needs to be 0 (None), 1 (Bias), 2 (ReLU mask), 3 (Bias+ReLU mask), 4 (ReLU), 5 (Bias+ReLU)\n");
    return -1;
  }
  if ( (prec != 4) && (prec != 2) && (prec != 1) ) {
    printf("prec needs to be 4 (F32), 2 (BF16), 1 (BF8)\n");
    return -1;
  }

  if ( (type != 'F') && (layout == 6 || layout == 14 || layout == 0) ) {
    printf("Illegal layout for non-Forward\n");
    return -1;
  }
  if ( (prec == 4) && ( (layout != 0) && (layout != 1)) ) {
    printf("layout must not specify for FP32\n");
    return -1;
  } else if ( (prec == 1) && (layout != 2) ) {
    printf("illegal layout for FP8\n");
    return -1;
  }
  if ( (layout == 14) && (( fuse_type == 3) || ( fuse_type == 2 )) ) {
    printf("illegal layout & relu with mask for BF16\n");
    return -1;
  }

  printf( "iters : %d, nImg : %d, nIFm: %d, nOFm: %d, fuse_type: %d, type: %d, bn: %d, bk: %d, bc: %d, prec: %d, layout: %d\n", iters, nImg, nIFm, nOFm, fuse_type, type, bn, bk, bc, prec, layout);
  /* set struct for naive convolution */
  naive_param.N = nImg;
  naive_param.C = nIFm;
  naive_param.K = nOFm;
  naive_param.fuse_type = fuse_type;

#if defined(__SSE3__)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
#endif

  if ( prec == 4 ) {
    in_dt = LIBXSMM_DATATYPE_F32;
    out_dt = LIBXSMM_DATATYPE_F32;
    comp_dt = LIBXSMM_DATATYPE_F32;
  } else if ( prec == 2 ) {
    in_dt = LIBXSMM_DATATYPE_BF16;
    out_dt = LIBXSMM_DATATYPE_BF16;
    comp_dt = LIBXSMM_DATATYPE_F32;
  } else {
    in_dt = LIBXSMM_DATATYPE_BF8;
    out_dt = LIBXSMM_DATATYPE_BF8;
    comp_dt = LIBXSMM_DATATYPE_F32;
  }

  /* print some summary */
  printf("##########################################\n");
  printf("#          Setting Up (Common)           #\n");
  printf("##########################################\n");
  printf("PARAMS: N:%d  C:%d  K:%d\n", nImg, nIFm, nOFm);
  printf("PARAMS: ITERS:%d", iters); if (LIBXSMM_FEQ(0, check)) printf("  Threads:%d\n", nThreads); else printf("\n");
  printf("SIZE Input  (MB): %10.2f MiB\n", (double)(nImg*nIFm*LIBXSMM_TYPESIZE(in_dt))/(1024.0*1024.0) );
  printf("SIZE Output (MB): %10.2f MiB\n", (double)(nImg*nOFm*LIBXSMM_TYPESIZE(in_dt))/(1024.0*1024.0) );
  printf("SIZE Input   (1): %10.2f MiB\n", (double)(1*nIFm*   LIBXSMM_TYPESIZE(in_dt))/(1024.0*1024.0) );
  printf("SIZE Output  (1): %10.2f MiB\n", (double)(1*nOFm*   LIBXSMM_TYPESIZE(in_dt))/(1024.0*1024.0) );
  printf("SIZE Filter     : %10.2f MiB\n", (double)(nIFm*nOFm*LIBXSMM_TYPESIZE(in_dt))/(1024.0*1024.0) );

  /* allocate data */
  naive_input                = (float*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(float), 2097152);
  naive_delinput             = (float*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(float), 2097152);
  naive_output               = (float*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(float), 2097152);
  naive_deloutput            = (float*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(float), 2097152);
  naive_filter               = (float*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(float), 2097152);
  naive_delfilter            = (float*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(float), 2097152);
  naive_bias                 = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
  naive_delbias              = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
  if ( prec == 2 ) {
    naive_input_bf16         = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_delinput_bf16      = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_output_bf16        = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_deloutput_bf16     = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_filter_bf16        = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_delfilter_bf16     = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_bias_bf16          = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat16), 2097152);
    naive_delbias_bf16       = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat16), 2097152);
  } else if ( prec == 1){
    naive_input_bf8         = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_delinput_bf8      = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_output_bf8        = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_deloutput_bf8     = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_filter_bf8        = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_delfilter_bf8     = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_bias_bf8          = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat8), 2097152);
    naive_delbias_bf8       = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat8), 2097152);
  }
  naive_libxsmm_delinput     = (float*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(float), 2097152);
  naive_libxsmm_output       = (float*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(float), 2097152);
  naive_libxsmm_delfilter    = (float*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(float), 2097152);
  if ( prec == 2 ) {
    naive_libxsmm_delinput_bf16  = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_libxsmm_output_bf16    = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_libxsmm_delfilter_bf16 = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    naive_libxsmm_delbias_bf16   = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nOFm*     sizeof(libxsmm_bfloat16), 2097152);
  } else if ( prec == 1) {
    naive_libxsmm_delinput_bf8  = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_libxsmm_output_bf8    = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_libxsmm_delfilter_bf8 = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    naive_libxsmm_delbias_bf8   = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nOFm*     sizeof(libxsmm_bfloat8), 2097152);
  }

  if ( prec == 4 ) {
    input_libxsmm              = (float*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(float), 2097152);
    delinput_libxsmm           = (float*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(float), 2097152);
    output_libxsmm             = (float*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(float), 2097152);
    deloutput_libxsmm          = (float*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(float), 2097152);
    filter_libxsmm             = (float*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(float), 2097152);
    delfilter_libxsmm          = (float*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(float), 2097152);
    bias_libxsmm               = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
    delbias_libxsmm            = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
  } else if ( prec == 2) {
    input_libxsmm_bf16         = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat16), 2097152);
    delinput_libxsmm_bf16      = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat16), 2097152);
    output_libxsmm_bf16        = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    deloutput_libxsmm_bf16     = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    filter_libxsmm_bf16        = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    delfilter_libxsmm_bf16     = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat16), 2097152);
    bias_libxsmm_bf16          = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat16), 2097152);
    delbias_libxsmm_bf16       = (libxsmm_bfloat16*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat16), 2097152);
    delbias_libxsmm            = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
  } else if ( prec == 1) {
    input_libxsmm_bf8         = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat8), 2097152);
    delinput_libxsmm_bf8      = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nIFm*sizeof(libxsmm_bfloat8), 2097152);
    output_libxsmm_bf8        = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    deloutput_libxsmm_bf8     = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nImg*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    filter_libxsmm_bf8        = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    delfilter_libxsmm_bf8     = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nIFm*nOFm*sizeof(libxsmm_bfloat8), 2097152);
    bias_libxsmm_bf8          = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat8), 2097152);
    delbias_libxsmm_bf8       = (libxsmm_bfloat8*)libxsmm_aligned_malloc( nOFm     *sizeof(libxsmm_bfloat8), 2097152);
    delbias_libxsmm            = (float*)libxsmm_aligned_malloc( nOFm     *sizeof(float), 2097152);
  }
  relumask_libxsmm           = (unsigned char*)libxsmm_aligned_malloc(((nImg*nOFm)/8)*sizeof(unsigned char), 2097152);

  /* initialize data */
  init_buf( naive_input,     nImg*nIFm, 0, 0 );
  init_buf( naive_delinput,  nImg*nIFm, 0, 0 );
  init_buf( naive_output,    nImg*nOFm, 0, 0 );
  init_buf( naive_deloutput, nImg*nOFm, 0, 0 );
  init_buf( naive_filter,    nIFm*nOFm, 0, 0 );
  init_buf( naive_delfilter, nIFm*nOFm, 0, 0 );
  init_buf( naive_bias,      nOFm,      0, 0 );
  init_buf( naive_delbias,   nOFm,      0, 0 );

  if ( prec == 2 ) {
    libxsmm_rne_convert_fp32_bf16( naive_input,     naive_input_bf16,     nImg*nIFm );
    libxsmm_rne_convert_fp32_bf16( naive_delinput,  naive_delinput_bf16,  nImg*nIFm );
    libxsmm_rne_convert_fp32_bf16( naive_output,    naive_output_bf16,    nImg*nOFm );
    libxsmm_rne_convert_fp32_bf16( naive_deloutput, naive_deloutput_bf16, nImg*nOFm );
    libxsmm_rne_convert_fp32_bf16( naive_filter,    naive_filter_bf16,    nIFm*nOFm );
    libxsmm_rne_convert_fp32_bf16( naive_delfilter, naive_delfilter_bf16, nIFm*nOFm );
    libxsmm_rne_convert_fp32_bf16( naive_bias,      naive_bias_bf16,      nOFm );
    libxsmm_rne_convert_fp32_bf16( naive_delbias,   naive_delbias_bf16,   nOFm );
  } else if ( prec == 1 ) {
    libxsmm_rne_convert_fp32_bf8( naive_input,     naive_input_bf8,     nImg*nIFm );
    libxsmm_rne_convert_fp32_bf8( naive_delinput,  naive_delinput_bf8,  nImg*nIFm );
    libxsmm_rne_convert_fp32_bf8( naive_output,    naive_output_bf8,    nImg*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_deloutput, naive_deloutput_bf8, nImg*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_filter,    naive_filter_bf8,    nIFm*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_delfilter, naive_delfilter_bf8, nIFm*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_bias,      naive_bias_bf8,      nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_delbias,   naive_delbias_bf8,   nOFm );

    /* quantize naive inputs passed to refence implementation */
    libxsmm_convert_bf8_f32( naive_input_bf8,     naive_input,     nImg*nIFm );
    libxsmm_convert_bf8_f32( naive_deloutput_bf8, naive_deloutput, nImg*nOFm );
    libxsmm_convert_bf8_f32( naive_filter_bf8,    naive_filter,    nIFm*nOFm );
    libxsmm_convert_bf8_f32( naive_bias_bf8,      naive_bias,      nOFm );
  }

  if (LIBXSMM_NEQ(0, check)) {
    printf("##########################################\n");
    printf("#         Computing Reference ...        #\n");
    printf("##########################################\n");
    if (type == 'A' || type == 'F') {
      naive_fullyconnected_fused_fp(&naive_param, naive_input, naive_output, naive_filter, naive_bias);
    }
    if (type == 'A' || type == 'B' || type == 'M') {
      naive_fullyconnected_fused_bp(&naive_param, naive_delinput, naive_deloutput, naive_filter, naive_delbias, naive_output);
    }
    if (type == 'A' || type == 'U' || type == 'M') {
      naive_fullyconnected_wu(&naive_param, naive_input, naive_deloutput, naive_delfilter);
    }
    printf("##########################################\n");
    printf("#      Computing Reference ... done      #\n");
    printf("##########################################\n");
  }
  /* quantize outputs computed with reference implementation */
  if ( prec == 1 ) {
    libxsmm_rne_convert_fp32_bf8( naive_delinput,  naive_delinput_bf8,  nImg*nIFm );
    libxsmm_rne_convert_fp32_bf8( naive_output,    naive_output_bf8,    nImg*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_delfilter, naive_delfilter_bf8, nIFm*nOFm );
    libxsmm_rne_convert_fp32_bf8( naive_delbias,   naive_delbias_bf8,   nOFm );

    libxsmm_convert_bf8_f32( naive_delinput_bf8,  naive_delinput,  nImg*nIFm );
    libxsmm_convert_bf8_f32( naive_output_bf8,    naive_output,    nImg*nOFm );
    libxsmm_convert_bf8_f32( naive_delfilter_bf8, naive_delfilter, nIFm*nOFm );
    libxsmm_convert_bf8_f32( naive_delbias_bf8,   naive_delbias,   nOFm );
  }
  printf("\n");
  printf("##########################################\n");
  printf("#      Setting Up  (custom-Storage)      #\n");
  printf("##########################################\n");

  if (nImg % bn != 0) {
    bn = nImg;
  }
  if (nIFm % bc != 0) {
    bc = nIFm;
  }
  if (nOFm % bk != 0) {
    bk = nOFm;
  }

  if ( fuse_type == 0 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_NONE;
  } else if ( fuse_type == 1 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_BIAS;
  } else if ( fuse_type == 2 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_RELU_WITH_MASK;
  } else if ( fuse_type == 3 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_BIAS_RELU_WITH_MASK;
  } else if ( fuse_type == 4 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_RELU;
  } else if ( fuse_type == 5 ) {
    my_fuse = LIBXSMM_DNN_FC_ELTW_FUSE_BIAS_RELU;
  } else {
    printf("Illegal fusion\n");
    return -1;
  }

  if ( layout == 0 ) {
    my_layout = LIBXSMM_DNN_FC_LAYOUT_FLAT;
  } else if ( layout == 1 ) {
    my_layout = LIBXSMM_DNN_FC_LAYOUT_PACKED;
  } else if ( layout == 2 ) {
    my_layout = LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT;
  } else if ( layout == 6 ) {
    my_layout = LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS;
  } else if ( layout == 14 ) {
    my_layout = LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS_OACT_TRANS;
  } else {
    printf("Illegal packing\n");
    return -1;
  }

  /* scratch memory size */
  size_t alloc_size = 0;

  if (type == 'A' || type == 'F') {
    libxsmm_dnn_fc_fwd = setup_libxsmm_dnn_fc_fwd(nImg, nIFm, nOFm, bn, bc, bk, nThreads, my_fuse, my_layout, in_dt, out_dt, comp_dt);

    alloc_size = libxsmm_dnn_fc_fwd.scratch_size;
  }
  if (type == 'A' || type == 'B' || type == 'U' || type == 'M') {
    libxsmm_dnn_fc_bwd = setup_libxsmm_dnn_fc_bwd(nImg, nIFm, nOFm, bn, bc, bk, nThreads, my_fuse, in_dt, out_dt, comp_dt);

    alloc_size = LIBXSMM_MAX( libxsmm_dnn_fc_fwd.scratch_size, libxsmm_dnn_fc_bwd.scratch_size);
  }

  /* we can also use the layout functions and set the data on our
     own external to the library */
  if ( prec == 2 ) {
    if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_FLAT ) {
      copy_buf_uint16( naive_input_bf16,          input_libxsmm_bf16, nImg*nIFm );
      copy_buf_uint16( naive_delinput_bf16,       delinput_libxsmm_bf16, nImg*nIFm );
      copy_buf_uint16( naive_output_bf16,         output_libxsmm_bf16, nImg*nOFm );
      copy_buf_uint16( naive_deloutput_bf16,      deloutput_libxsmm_bf16, nImg*nOFm );
      matrix_copy_KC_to_CK_bf16( naive_filter_bf16,         filter_libxsmm_bf16, nIFm, nOFm );
      matrix_copy_KC_to_CK_bf16( naive_delfilter_bf16,      delfilter_libxsmm_bf16, nIFm, nOFm );
      copy_buf_uint16( naive_bias_bf16,    bias_libxsmm_bf16,    nOFm);
      copy_buf_uint16( naive_delbias_bf16, delbias_libxsmm_bf16, nOFm);
    } else {
      if ( (my_layout == LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS) || (my_layout == LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS_OACT_TRANS) ) {
        matrix_copy_NC_to_NCNC_bf16_vnniT( naive_input_bf16,     input_libxsmm_bf16,     1, nImg, nIFm, bn, bc );
      } else {
        matrix_copy_NC_to_NCNC_bf16( naive_input_bf16,     input_libxsmm_bf16,     1, nImg, nIFm, bn, bc );
      }
      matrix_copy_NC_to_NCNC_bf16( naive_delinput_bf16,  delinput_libxsmm_bf16,  1, nImg, nIFm, bn, bc );
      if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS_OACT_TRANS ) {
        matrix_copy_NC_to_NCNC_bf16_vnniT( naive_output_bf16,    output_libxsmm_bf16,    1, nImg, nOFm, bn, bk );
      } else {
        matrix_copy_NC_to_NCNC_bf16( naive_output_bf16,    output_libxsmm_bf16,    1, nImg, nOFm, bn, bk );
      }
      matrix_copy_NC_to_NCNC_bf16( naive_deloutput_bf16, deloutput_libxsmm_bf16, 1, nImg, nOFm, bn, bk );
      if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_PACKED) {
        matrix_copy_KC_to_KCCK_bf16_flat( naive_filter_bf16,    filter_libxsmm_bf16      , nIFm, nOFm, bc, bk );
        matrix_copy_KC_to_KCCK_bf16_flat( naive_delfilter_bf16, delfilter_libxsmm_bf16   , nIFm, nOFm, bc, bk );
      } else {
        matrix_copy_KC_to_KCCK_bf16( naive_filter_bf16,    filter_libxsmm_bf16      , nIFm, nOFm, bc, bk );
        matrix_copy_KC_to_KCCK_bf16( naive_delfilter_bf16, delfilter_libxsmm_bf16   , nIFm, nOFm, bc, bk );
      }
      matrix_copy_NC_to_NCNC_bf16( naive_bias_bf16,    bias_libxsmm_bf16,    1, 1, nOFm, 1, nOFm );
      matrix_copy_NC_to_NCNC_bf16( naive_delbias_bf16, delbias_libxsmm_bf16, 1, 1, nOFm, 1, nOFm );
    }
  } else if ( prec == 1 ) {
    matrix_copy_NC_to_NCNC_bf8( naive_input_bf8,     input_libxsmm_bf8,     1, nImg, nIFm, bn, bc );
    matrix_copy_NC_to_NCNC_bf8( naive_delinput_bf8,  delinput_libxsmm_bf8,  1, nImg, nIFm, bn, bc );
    matrix_copy_NC_to_NCNC_bf8( naive_output_bf8,    output_libxsmm_bf8,    1, nImg, nOFm, bn, bk );
    matrix_copy_NC_to_NCNC_bf8( naive_deloutput_bf8, deloutput_libxsmm_bf8, 1, nImg, nOFm, bn, bk );
    matrix_copy_KC_to_KCCK_bf8( naive_filter_bf8,    filter_libxsmm_bf8      , nIFm, nOFm, bc, bk );
    matrix_copy_KC_to_KCCK_bf8( naive_delfilter_bf8, delfilter_libxsmm_bf8   , nIFm, nOFm, bc, bk );
    matrix_copy_NC_to_NCNC_bf8( naive_bias_bf8,    bias_libxsmm_bf8,    1, 1, nOFm, 1, nOFm );
    matrix_copy_NC_to_NCNC_bf8( naive_delbias_bf8, delbias_libxsmm_bf8, 1, 1, nOFm, 1, nOFm );
  } else {
    if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_FLAT ) {
      copy_buf( naive_input,          input_libxsmm, nImg*nIFm );
      copy_buf( naive_delinput,       delinput_libxsmm, nImg*nIFm );
      copy_buf( naive_output,         output_libxsmm, nImg*nOFm );
      copy_buf( naive_deloutput,      deloutput_libxsmm, nImg*nOFm );
      matrix_copy_KC_to_CK( naive_filter,         filter_libxsmm, nIFm, nOFm );
      matrix_copy_KC_to_CK( naive_delfilter,      delfilter_libxsmm, nIFm, nOFm );
    } else {
      matrix_copy_NC_to_NCNC( naive_input,          input_libxsmm,     1, nImg, nIFm, bn, bc );
      matrix_copy_NC_to_NCNC( naive_delinput,       delinput_libxsmm,  1, nImg, nIFm, bn, bc );
      matrix_copy_NC_to_NCNC( naive_output,         output_libxsmm,    1, nImg, nOFm, bn, bk );
      matrix_copy_NC_to_NCNC( naive_deloutput,      deloutput_libxsmm, 1, nImg, nOFm, bn, bk );
      matrix_copy_KC_to_KCCK( naive_filter,         filter_libxsmm      , nIFm, nOFm, bc, bk );
      matrix_copy_KC_to_KCCK( naive_delfilter,      delfilter_libxsmm   , nIFm, nOFm, bc, bk );
    }
    copy_buf(naive_bias,    bias_libxsmm,    nOFm);
    copy_buf(naive_delbias, delbias_libxsmm, nOFm);
  }

  /* let's allocate and bind scratch */
  if (alloc_size > 0) {
    scratch = libxsmm_aligned_malloc( alloc_size, 2097152 );
    init_buf( (float*)(scratch), (alloc_size)/4, 0, 0 );
  }

  if ((type == 'A' || type == 'F') && LIBXSMM_NEQ(0, check)) {
    printf("##########################################\n");
    printf("#   Correctness - FWD (custom-Storage)   #\n");
    printf("##########################################\n");

#if defined(_OPENMP)
#   pragma omp parallel
#endif
    {
#if defined(_OPENMP)
      const int tid = omp_get_thread_num();
#else
      const int tid = 0;
#endif
      if ( prec == 2 ) {
        libxsmm_dnn_fc_fwd_exec_bf16( libxsmm_dnn_fc_fwd, filter_libxsmm_bf16, input_libxsmm_bf16, output_libxsmm_bf16,
                             bias_libxsmm_bf16, relumask_libxsmm, 0, tid, scratch );
      } else if ( prec == 1 ) {
        libxsmm_dnn_fc_fwd_exec_bf8( libxsmm_dnn_fc_fwd, filter_libxsmm_bf8, input_libxsmm_bf8, output_libxsmm_bf8,
                             bias_libxsmm_bf8, relumask_libxsmm, 0, tid, scratch );
      } else {
        libxsmm_dnn_fc_fwd_exec_f32( libxsmm_dnn_fc_fwd, filter_libxsmm, input_libxsmm, output_libxsmm,
            bias_libxsmm, relumask_libxsmm, 0, tid, scratch );
      }
    }

    /* copy out data */
    if ( prec == 2 ) {
      if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_VNNIPACK_WT_IACT_TRANS_OACT_TRANS ) {
        matrix_copy_NCNC_vnniT_to_NC_bf16( output_libxsmm_bf16, naive_libxsmm_output_bf16, 1, nImg, nOFm, bn, bk );
      } else if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_FLAT  ) {
        copy_buf_uint16( output_libxsmm_bf16, naive_libxsmm_output_bf16, nImg*nOFm );
      } else {
        matrix_copy_NCNC_to_NC_bf16( output_libxsmm_bf16, naive_libxsmm_output_bf16, 1, nImg, nOFm, bn, bk );
      }
      libxsmm_convert_bf16_f32( naive_libxsmm_output_bf16, naive_libxsmm_output, nImg*nOFm );
    } else if ( prec == 1 ) {
      matrix_copy_NCNC_to_NC_bf8( output_libxsmm_bf8, naive_libxsmm_output_bf8, 1, nImg, nOFm, bn, bk );
      libxsmm_convert_bf8_f32( naive_libxsmm_output_bf8, naive_libxsmm_output, nImg*nOFm );
    } else {
      if ( my_layout == LIBXSMM_DNN_FC_LAYOUT_FLAT ) {
        copy_buf( output_libxsmm, naive_libxsmm_output, nImg*nOFm );
      } else {
        matrix_copy_NCNC_to_NC( output_libxsmm, naive_libxsmm_output, 1, nImg, nOFm, bn, bk );
      }
    }

    /* compare */
    libxsmm_matdiff(&norms_fwd, LIBXSMM_DATATYPE_F32, nImg*nOFm, 1, naive_output, naive_libxsmm_output, 0, 0);
    printf("L1 reference  : %.25g\n", norms_fwd.l1_ref);
    printf("L1 test       : %.25g\n", norms_fwd.l1_tst);
    printf("L2 abs.error  : %.24f\n", norms_fwd.l2_abs);
    printf("L2 rel.error  : %.24f\n", norms_fwd.l2_rel);
    printf("Linf abs.error: %.24f\n", norms_fwd.linf_abs);
    printf("Linf rel.error: %.24f\n", norms_fwd.linf_rel);
    printf("Check-norm    : %.24f\n", libxsmm_matdiff_epsilon(&norms_fwd));
    libxsmm_matdiff_reduce(&diff, &norms_fwd);
  }

  if ( (type == 'A' || type == 'M') && LIBXSMM_NEQ(0, check) ) {
    printf("##########################################\n");
    printf("# Correctness - BWDUPD (custom-Storage)  #\n");
    printf("##########################################\n");

#if defined(_OPENMP)
#   pragma omp parallel
#endif
    {
#if defined(_OPENMP)
      const int tid = omp_get_thread_num();
#else
      const int tid = 0;
#endif
      if ( prec == 2 ) {
        libxsmm_dnn_fc_bwd_exec_bf16( libxsmm_dnn_fc_bwd, filter_libxsmm_bf16, delinput_libxsmm_bf16, deloutput_libxsmm_bf16, delfilter_libxsmm_bf16,
                             input_libxsmm_bf16, delbias_libxsmm_bf16, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
      } else if ( prec == 1 ) {
        libxsmm_dnn_fc_bwd_exec_bf8( libxsmm_dnn_fc_bwd, filter_libxsmm_bf8, delinput_libxsmm_bf8, deloutput_libxsmm_bf8, delfilter_libxsmm_bf8,
                             input_libxsmm_bf8, delbias_libxsmm_bf8, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
      } else {
        libxsmm_dnn_fc_bwd_exec_f32( libxsmm_dnn_fc_bwd, filter_libxsmm, delinput_libxsmm, deloutput_libxsmm, delfilter_libxsmm,
            input_libxsmm, delbias_libxsmm, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
      }
    }

    /* copy out data */
    if ( prec == 2 ) {
      matrix_copy_NCNC_to_NC_bf16( delinput_libxsmm_bf16, naive_libxsmm_delinput_bf16, 1, nImg, nIFm, bn, bc );
      libxsmm_convert_bf16_f32( naive_libxsmm_delinput_bf16, naive_libxsmm_delinput, nImg*nIFm );
      matrix_copy_KCCK_to_KC_bf16( delfilter_libxsmm_bf16, naive_libxsmm_delfilter_bf16, nIFm, nOFm, bc, bk );
      libxsmm_convert_bf16_f32( naive_libxsmm_delfilter_bf16, naive_libxsmm_delfilter, nIFm*nOFm );
    } else if ( prec == 1 ) {
      matrix_copy_NCNC_to_NC_bf8( delinput_libxsmm_bf8, naive_libxsmm_delinput_bf8, 1, nImg, nIFm, bn, bc );
      libxsmm_convert_bf8_f32( naive_libxsmm_delinput_bf8, naive_libxsmm_delinput, nImg*nIFm );
      matrix_copy_KCCK_to_KC_bf8( delfilter_libxsmm_bf8, naive_libxsmm_delfilter_bf8, nIFm, nOFm, bc, bk );
      libxsmm_convert_bf8_f32( naive_libxsmm_delfilter_bf8, naive_libxsmm_delfilter, nIFm*nOFm );
    } else {
      matrix_copy_NCNC_to_NC( delinput_libxsmm, naive_libxsmm_delinput, 1, nImg, nIFm, bn, bc );
      matrix_copy_KCCK_to_KC( delfilter_libxsmm, naive_libxsmm_delfilter, nIFm, nOFm, bc, bk );
    }

    /* compare */
    libxsmm_matdiff(&norms_bwd, LIBXSMM_DATATYPE_F32, nImg*nIFm, 1, naive_delinput, naive_libxsmm_delinput, 0, 0);
    printf("L1 reference  : %.25g\n", norms_bwd.l1_ref);
    printf("L1 test       : %.25g\n", norms_bwd.l1_tst);
    printf("L2 abs.error  : %.24f\n", norms_bwd.l2_abs);
    printf("L2 rel.error  : %.24f\n", norms_bwd.l2_rel);
    printf("Linf abs.error: %.24f\n", norms_bwd.linf_abs);
    printf("Linf rel.error: %.24f\n", norms_bwd.linf_rel);
    printf("Check-norm    : %.24f\n", libxsmm_matdiff_epsilon(&norms_bwd));
    libxsmm_matdiff_reduce(&diff, &norms_bwd);
    if ( (fuse_type == 1) || (fuse_type == 3) ) {
      if ( prec == 2 ) {
        libxsmm_convert_bf16_f32( delbias_libxsmm_bf16, delbias_libxsmm, nOFm );
      } else if ( prec == 1 ) {
        libxsmm_convert_bf8_f32( delbias_libxsmm_bf8, delbias_libxsmm, nOFm );
      }
      libxsmm_matdiff(&norms_bwd, LIBXSMM_DATATYPE_F32, nOFm, 1, naive_delbias, delbias_libxsmm, 0, 0);
      printf("L1 reference  : %.25g\n", norms_bwd.l1_ref);
      printf("L1 test       : %.25g\n", norms_bwd.l1_tst);
      printf("L2 abs.error  : %.24f\n", norms_bwd.l2_abs);
      printf("L2 rel.error  : %.24f\n", norms_bwd.l2_rel);
      printf("Linf abs.error: %.24f\n", norms_bwd.linf_abs);
      printf("Linf rel.error: %.24f\n", norms_bwd.linf_rel);
      printf("Check-norm    : %.24f\n", libxsmm_matdiff_epsilon(&norms_bwd));
      libxsmm_matdiff_reduce(&diff, &norms_bwd);
    }
    libxsmm_matdiff(&norms_upd, LIBXSMM_DATATYPE_F32, nIFm*nOFm, 1, naive_delfilter, naive_libxsmm_delfilter, 0, 0);
    printf("L1 reference  : %.25g\n", norms_upd.l1_ref);
    printf("L1 test       : %.25g\n", norms_upd.l1_tst);
    printf("L2 abs.error  : %.24f\n", norms_upd.l2_abs);
    printf("L2 rel.error  : %.24f\n", norms_upd.l2_rel);
    printf("Linf abs.error: %.24f\n", norms_upd.linf_abs);
    printf("Linf rel.error: %.24f\n", norms_upd.linf_rel);
    printf("Check-norm    : %.24f\n", libxsmm_matdiff_epsilon(&norms_upd));
    libxsmm_matdiff_reduce(&diff, &norms_upd);
  }

  if (type == 'A' || type == 'F') {
    printf("##########################################\n");
    printf("#   Performance - FWD (custom-Storage)   #\n");
    printf("##########################################\n");
    l_start = libxsmm_timer_tick();
#if defined(_OPENMP)
#   pragma omp parallel private(i)
#endif
    {
#if defined(_OPENMP)
      const int tid = omp_get_thread_num();
#else
      const int tid = 0;
#endif
      for (i = 0; i < iters; ++i) {
        if ( prec == 2 ) {
          libxsmm_dnn_fc_fwd_exec_bf16( libxsmm_dnn_fc_fwd, filter_libxsmm_bf16, input_libxsmm_bf16, output_libxsmm_bf16,
                               bias_libxsmm_bf16, relumask_libxsmm, 0, tid, scratch );
        } else if ( prec == 1 ) {
          libxsmm_dnn_fc_fwd_exec_bf8( libxsmm_dnn_fc_fwd, filter_libxsmm_bf8, input_libxsmm_bf8, output_libxsmm_bf8,
                               bias_libxsmm_bf8, relumask_libxsmm, 0, tid, scratch );
        } else {
          libxsmm_dnn_fc_fwd_exec_f32( libxsmm_dnn_fc_fwd, filter_libxsmm, input_libxsmm, output_libxsmm,
              bias_libxsmm, relumask_libxsmm, 0, tid, scratch );
        }
      }
    }
    l_end = libxsmm_timer_tick();
    l_total = libxsmm_timer_duration(l_start, l_end);

    gflop = (2.0*(double)nImg*(double)nIFm*(double)nOFm*(double)iters) / (1000*1000*1000);

    printf("GFLOP  = %.5g\n", gflop/(double)iters);
    printf("fp time = %.5g\n", l_total/iters);
    printf("GFLOPS  = %.5g\n", gflop/l_total);

    printf("PERFDUMP,FP,%s,%i,%i,%i,%i,%.5g,%.5g,%f,%f,%f,%f,%f,%f,%f\n", LIBXSMM_VERSION, nThreads, nImg, nIFm,
        nOFm, l_total/iters, gflop/l_total, norms_fwd.l1_ref, norms_fwd.l1_tst,
        norms_fwd.l2_abs, norms_fwd.l2_rel, norms_fwd.linf_abs, norms_fwd.linf_rel, norms_fwd.normf_rel);
  }

  if (type == 'A' || type == 'M') {
    printf("##########################################\n");
    printf("# Performance - BWDUPD (custom-Storage)  #\n");
    printf("##########################################\n");
    l_start = libxsmm_timer_tick();
#if defined(_OPENMP)
#   pragma omp parallel private(i)
#endif
    {
#if defined(_OPENMP)
      const int tid = omp_get_thread_num();
#else
      const int tid = 0;
#endif
      for (i = 0; i < iters; ++i) {
        if ( prec == 2 ) {
          libxsmm_dnn_fc_bwd_exec_bf16( libxsmm_dnn_fc_bwd, filter_libxsmm_bf16, delinput_libxsmm_bf16, deloutput_libxsmm_bf16, delfilter_libxsmm_bf16,
                               input_libxsmm_bf16, delbias_libxsmm_bf16, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
        } else if ( prec == 1 ) {
          libxsmm_dnn_fc_bwd_exec_bf8( libxsmm_dnn_fc_bwd, filter_libxsmm_bf8, delinput_libxsmm_bf8, deloutput_libxsmm_bf8, delfilter_libxsmm_bf8,
                               input_libxsmm_bf8, delbias_libxsmm_bf8, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
        } else {
          libxsmm_dnn_fc_bwd_exec_f32( libxsmm_dnn_fc_bwd, filter_libxsmm, delinput_libxsmm, deloutput_libxsmm, delfilter_libxsmm,
              input_libxsmm, delbias_libxsmm, relumask_libxsmm, LIBXSMM_DNN_FC_PASS_BWD, 0, tid, scratch );
        }
      }
    }
    l_end = libxsmm_timer_tick();
    l_total = libxsmm_timer_duration(l_start, l_end);

    gflop = (4.0*(double)nImg*(double)nIFm*(double)nOFm*(double)iters) / (1000*1000*1000);

    printf("GFLOP  = %.5g\n", gflop/(double)iters);
    printf("fp time = %.5g\n", l_total/iters);
    printf("GFLOPS  = %.5g\n", gflop/l_total);

    printf("PERFDUMP,UP,%s,%i,%i,%i,%i,%.5g,%.5g,%f,%f,%f,%f,%f,%f,%f\n", LIBXSMM_VERSION, nThreads, nImg, nIFm,
        nOFm, l_total/iters, gflop/l_total, norms_upd.l1_ref, norms_upd.l1_tst,
        norms_upd.l2_abs, norms_upd.l2_rel, norms_upd.linf_abs, norms_upd.linf_rel, norms_upd.normf_rel);
  }

  /* deallocate data */
  if (type == 'A' || type == 'F') {
    destroy_libxsmm_dnn_fc_fwd(&libxsmm_dnn_fc_fwd);
  }
  if (type == 'A' || type == 'B' || type == 'U' || type == 'M') {
    destroy_libxsmm_dnn_fc_bwd(&libxsmm_dnn_fc_bwd);
  }

  if ( scratch != NULL ) {
    libxsmm_free(scratch);
  }
  libxsmm_free(naive_input);
  libxsmm_free(naive_output);
  libxsmm_free(naive_delinput);
  libxsmm_free(naive_deloutput);
  libxsmm_free(naive_filter);
  libxsmm_free(naive_delfilter);
  libxsmm_free(naive_bias);
  libxsmm_free(naive_delbias);
  libxsmm_free(naive_libxsmm_output);
  libxsmm_free(naive_libxsmm_delinput);
  libxsmm_free(naive_libxsmm_delfilter);
  if ( prec == 0 ) {
    libxsmm_free(input_libxsmm);
    libxsmm_free(output_libxsmm);
    libxsmm_free(delinput_libxsmm);
    libxsmm_free(deloutput_libxsmm);
    libxsmm_free(filter_libxsmm);
    libxsmm_free(delfilter_libxsmm);
    libxsmm_free(bias_libxsmm);
  } else if ( prec == 2 ) {
    libxsmm_free(input_libxsmm_bf16);
    libxsmm_free(output_libxsmm_bf16);
    libxsmm_free(delinput_libxsmm_bf16);
    libxsmm_free(deloutput_libxsmm_bf16);
    libxsmm_free(filter_libxsmm_bf16);
    libxsmm_free(delfilter_libxsmm_bf16);
    libxsmm_free(bias_libxsmm_bf16);
    libxsmm_free(naive_libxsmm_output_bf16);
    libxsmm_free(naive_libxsmm_delinput_bf16);
    libxsmm_free(naive_libxsmm_delfilter_bf16);
    libxsmm_free(naive_libxsmm_delbias_bf16);
  } else {
    libxsmm_free(input_libxsmm_bf8);
    libxsmm_free(output_libxsmm_bf8);
    libxsmm_free(delinput_libxsmm_bf8);
    libxsmm_free(deloutput_libxsmm_bf8);
    libxsmm_free(filter_libxsmm_bf8);
    libxsmm_free(delfilter_libxsmm_bf8);
    libxsmm_free(bias_libxsmm_bf8);
    libxsmm_free(naive_libxsmm_output_bf8);
    libxsmm_free(naive_libxsmm_delinput_bf8);
    libxsmm_free(naive_libxsmm_delfilter_bf8);
    libxsmm_free(naive_libxsmm_delbias_bf8);
  }

  if (LIBXSMM_NEQ(0, check)) {
    const double error = check * libxsmm_matdiff_epsilon(&diff);
    const double espilon = (4 == prec ? 4E-3 : (2 == prec ? 4E-2 : 8E-1));
    if (error <= espilon) {
      fprintf(stderr, "\nSUCCESS (error=%.24f)\n\n\n", error);
    }
    else if (LIBXSMM_NOTNAN(diff.v_tst)) {
      fprintf(stderr, "\nFAILED (error=%.24f %g != %g)\n\n\n",
        error, diff.v_ref, diff.v_tst);
      return EXIT_FAILURE;
    }
    else {
      fprintf(stderr, "\nFAILED (error=%.24f)\n\n\n", error);
      return EXIT_FAILURE;
    }
  }
  else printf("\n\n\n");

  return EXIT_SUCCESS;
}
