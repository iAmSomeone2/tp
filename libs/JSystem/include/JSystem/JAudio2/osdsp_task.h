#ifndef OSDSP_TASK_H
#define OSDSP_TASK_H

#include <dsp.h>

extern DSPTaskInfo* DSP_prior_task;

void DsyncFrame2(u32 param_0, uintptr_t param_1, uintptr_t param_2);

#endif /* OSDSP_TASK_H */
