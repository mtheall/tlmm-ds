#pragma once
#ifndef __TLMM_H__
#define __TLMM_H__

#ifdef TLMM_HAS_IO
#include <stdio.h>
#endif
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tlmmProgram tlmmProgram;

// Return values
typedef enum {
  TLMM_SUCCESS = 0,
  TLMM_FAILURE,
  TLMM_PARSE_ERROR,
} tlmmReturn;

// interface functions
tlmmProgram* tlmmInitProgram      (void);
tlmmReturn   tlmmTerminateProgram (tlmmProgram **prog);
#ifdef TLMM_COMPILE
tlmmReturn   tlmmParseProgram     (tlmmProgram *prog, const char *program);
#endif
tlmmReturn   tlmmLoadProgramBinary(tlmmProgram *prog, const void *data, size_t size);
float        tlmmGetValue         (tlmmProgram *prog, float ref);
const char*  tlmmGetEquation      (tlmmProgram *prog);
#ifdef TLMM_HAS_IO
tlmmReturn   tlmmLoadProgram      (tlmmProgram *prog, const char *filename);
tlmmReturn   tlmmSaveProgram      (tlmmProgram *prog, const char *filename);
#endif

#ifdef __cplusplus
}

#ifndef TLMM_LEAN
namespace tlmm {
  class Program {
  private:
    tlmmProgram *prog;

  public:
    Program();
    ~Program();

#ifdef TLMM_HAS_IO
    tlmmReturn Load (const char *filename);
    tlmmReturn Save (const char *filename);
#endif

#ifdef TLMM_COMPILE
    tlmmReturn Parse(const char *program);
#endif

    float GetValue(float ref);
    const char* GetEquation();
  };
}
#endif /* TLMM_LEAN */
#endif /* __cplusplus */
#endif /* __TLMM_H__ */
