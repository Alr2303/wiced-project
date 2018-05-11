#
# $ Copyright Broadcom Corporation $
#

NAME = STM32F4xx_Peripheral_Libraries

GLOBAL_INCLUDES :=  . \
                    inc \
                    ../../../$(HOST_ARCH)/CMSIS

$(NAME)_SOURCES := \
                   src/misc.c \
                   src/stm32f4xx_adc.c \
                   src/stm32f4xx_can.c \
                   src/stm32f4xx_crc.c \
                   src/stm32f4xx_dac.c \
                   src/stm32f4xx_dbgmcu.c \
                   src/stm32f4xx_dma.c \
                   src/stm32f4xx_exti.c \
                   src/stm32f4xx_flash.c \
                   src/stm32f4xx_gpio.c \
                   src/stm32f4xx_rng.c \
                   src/stm32f4xx_i2c.c \
                   src/stm32f4xx_iwdg.c \
                   src/stm32f4xx_pwr.c \
                   src/stm32f4xx_rcc.c \
                   src/stm32f4xx_rtc.c \
                   src/stm32f4xx_sdio.c \
                   src/stm32f4xx_spi.c \
                   src/stm32f4xx_syscfg.c \
                   src/stm32f4xx_tim.c \
                   src/stm32f4xx_usart.c \
                   src/stm32f4xx_dfsdm.c \
                   src/stm32f4xx_wwdg.c

# Add FSMC some STM32F4xx MCUs
ifeq ($(filter $(HOST_MCU_VARIANT), STM32F411 STM32F401 STM32F412 STM32F410),)                
$(NAME)_SOURCES += src/stm32f4xx_fsmc.c
endif 
