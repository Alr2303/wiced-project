#
# $ Copyright Broadcom Corporation $
#

NAME = CYFM4_Peripheral_Libraries

GLOBAL_INCLUDES :=  .                                       \
                    common                                  \
                    drivers                                 \
                    drivers/adc                             \
                    drivers/bt                              \
                    drivers/can                             \
                    drivers/clk                             \
                    drivers/cr                              \
                    drivers/crc                             \
                    drivers/csv                             \
                    drivers/dac                             \
                    drivers/dma                             \
                    drivers/dstc                            \
                    drivers/dt                              \
                    drivers/exint                           \
                    drivers/extif                           \
                    drivers/flash                           \
                    drivers/gpio                            \
                    drivers/hbif                            \
                    drivers/hsspi                           \
                    drivers/i2cs                            \
                    drivers/i2s                             \
                    drivers/icc                             \
                    drivers/lcd                             \
                    drivers/lpm                             \
                    drivers/lvd                             \
                    drivers/mfs                             \
                    drivers/mft                             \
                    drivers/pcrc                            \
                    drivers/ppg                             \
                    drivers/qprc                            \
                    drivers/rc                              \
                    drivers/reset                           \
                    drivers/rtc                             \
                    drivers/sdif                            \
                    drivers/uid                             \
                    drivers/vbat                            \
                    drivers/wc                              \
                    drivers/wdg                             \
                    utilities                               \
                    ../../../$(HOST_ARCH)/CMSIS             \
                    devices/fm4/$(HOST_MCU_PART_NUMBER)/common  
                    
$(NAME)_SOURCES := \
                    drivers/adc/adc.c                       \
                    drivers/bt/bt.c                         \
                    drivers/can/can_pre.c                   \
                    drivers/can/can.c                       \
                    drivers/can/canfd.c                     \
                    drivers/clk/clk.c                       \
                    drivers/cr/cr.c                         \
                    drivers/crc/crc.c                       \
                    drivers/csv/csv.c                       \
                    drivers/dac/dac.c                       \
                    drivers/dma/dma.c                       \
                    drivers/dstc/dstc.c                     \
                    drivers/dt/dt.c                         \
                    drivers/exint/exint.c                   \
                    drivers/extif/extif.c                   \
                    drivers/flash/dualflash.c               \
                    drivers/flash/mainflash.c               \
                    drivers/flash/workflash.c               \
                    drivers/hbif/hbif.c                     \
                    drivers/hsspi/hsspi.c                   \
                    drivers/i2cs/i2cs.c                     \
                    drivers/i2s/i2s.c                       \
                    drivers/icc/icc.c                       \
                    drivers/lcd/lcd.c                       \
                    drivers/lpm/lpm.c                       \
                    drivers/lvd/lvd.c                       \
                    drivers/mfs/mfs.c                       \
                    drivers/mft/mft_adcmp.c                 \
                    drivers/mft/mft_frt.c                   \
                    drivers/mft/mft_icu.c                   \
                    drivers/mft/mft_ocu.c                   \
                    drivers/mft/mft_wfg.c                   \
                    drivers/pcrc/pcrc.c                     \
                    drivers/ppg/ppg.c                       \
                    drivers/qprc/qprc.c                     \
                    drivers/rc/rc.c                         \
                    drivers/reset/reset.c                   \
                    drivers/rtc/rtc.c                       \
                    drivers/sdif/sdif.c                     \
                    drivers/uid/uid.c                       \
                    drivers/vbat/vbat.c                     \
                    drivers/wc/wc.c                         \
                    drivers/wdg/hwwdg.c                     \
                    drivers/wdg/swwdg.c                     \
                    drivers/interrupts_fm0p_type_1-a.c      \
                    drivers/interrupts_fm0p_type_1-b.c      \
                    drivers/interrupts_fm0p_type_2-a.c      \
                    drivers/interrupts_fm0p_type_2-b.c      \
                    drivers/interrupts_fm0p_type_3.c        \
                    drivers/interrupts_fm3_type_a.c         \
                    drivers/interrupts_fm3_type_b.c         \
                    drivers/interrupts_fm3_type_c.c         \
                    drivers/interrupts_fm4_type_a.c         \
                    drivers/interrupts_fm4_type_b.c         \
                    drivers/interrupts_fm4_type_c.c         \
                    drivers/pdl.c                           \
                    utilities/eeprom/i2c_int_at24cxx.c      \
                    utilities/eeprom/i2c_polling_at24cxx.c  \
                    utilities/hyper_flash/s26kl512s.c       \
                    utilities/i2s_codec/wm8731.c            \
                    utilities/nand_flash/s34ml01g.c         \
                    utilities/printf_scanf/uart_io.c        \
                    utilities/qspi_flash/flashS25FL164K.c   \
                    utilities/sd_card/sd_card.c             \
                    utilities/sd_card/sd_cmd.c              \
                    utilities/sdram/is42s16800.c            \
                    utilities/seg_lcd/cl010/cl010.c         \
                    utilities/seg_lcd/tsdh1188/tsdh1188.c   \
                    devices/fm4/$(HOST_MCU_PART_NUMBER)/common/system_$(HOST_MCU_VARIANT).c

ifeq ($(TOOLCHAIN_NAME),GCC)
GLOBAL_CFLAGS += -fms-extensions -fno-strict-aliasing -Wno-missing-braces # Required for compiling unnamed structure and union fields
endif