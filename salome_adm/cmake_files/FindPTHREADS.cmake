
SET(PTHREADS_ROOT $ENV{PTHREADS_ROOT})
IF(PTHREADS_ROOT)
  FIND_PATH(PTHREADS_INCLUDE_DIR pthread.h ${PTHREADS_ROOT}/include)
  IF(PTHREADS_INCLUDE_DIR)
    SET(PTHREADS_INCLUDES -I${PTHREADS_INCLUDE_DIR})
  ENDIF(PTHREADS_INCLUDE_DIR)
  FIND_LIBRARY(PTHREADS_LIBRARY pthreadVSE2d ${PTHREADS_ROOT}/lib)
ENDIF(PTHREADS_ROOT)