#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pic_remap(void);
void pic_send_eoi(unsigned char irq);

void pic_init_timer(uint32_t frequency);
void pic_unmask_irqs();
void pic_unmask_irq(unsigned char irq);

#ifdef __cplusplus
}
#endif

