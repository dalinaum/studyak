cmd_usr/initramfs_data.o := /home/leonardo/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-eabi-gcc -Wp,-MD,usr/.initramfs_data.o.d  -nostdinc -isystem /home/leonardo/CodeSourcery/Sourcery_G++_Lite/bin/../lib/gcc/arm-none-eabi/4.4.1/include -Dlinux -Iinclude  -I/home/leonardo/mykernel/kernel/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-s5pv210/include -Iarch/arm/plat-s5p/include -Iarch/arm/plat-samsung/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables  -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -gdwarf-2       -c -o usr/initramfs_data.o usr/initramfs_data.S

deps_usr/initramfs_data.o := \
  usr/initramfs_data.S \
  /home/leonardo/mykernel/kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

usr/initramfs_data.o: $(deps_usr/initramfs_data.o)

$(deps_usr/initramfs_data.o):
