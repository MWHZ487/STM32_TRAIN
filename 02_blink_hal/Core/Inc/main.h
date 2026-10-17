/**
 ******************************************************************************
 * @file    main.h
 * @brief   HAL 版工程的公共头文件
 ******************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 引入 HAL 库总头文件。它内部会根据 stm32f1xx_hal_conf.h 里
 * 的开关，自动包含你启用那些模块的头文件。 */
#include "stm32f1xx_hal.h"

/* 出错时的统一处理函数（定义在 main.c） */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
