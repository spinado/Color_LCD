/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#ifndef _USART1_H_
#define _USART1_H_

#include "stdio.h"

void usart1_init(void);
const uint8_t* usart1_get_rx_buffer(void);
void usart1_start_dma_transfer(uint8_t ui8_len);

#endif
