/*
 * @file:      mx6tabiris_spl_mmc.c
 * @author:    Binh Nguyen - tabiris1612@gmail.com
 * @version:   0.1
 * @date:      02-22-2020
 * @copyright: Copyright (c) 2020
 */

#include <common.h>
#include <spl.h>
#include <malloc.h>
#include <wdt.h>
#include <bootcount.h>
#include <asm/arch/sys_proto.h>

#define SPL_MMC_BOOT 1
#define SPL_NAND_BOOT 0

u32 spl_boot_device(void)
{
    unsigned int bmode = readl(&src_base->sbmr2);

    /* Check if force booting from usb */
    if (((bmode >> 24) & 0x03) == 0x01)
        return BOOT_DEVICE_BOARD;

#ifdef SPL_MMC_BOOT
    return BOOT_DEVICE_MMC1;

#elif SPL_NAND_BOOT
    return BOOT_DEVICE_NAND;

#endif

    return BOOT_DEVICE_NONE;
}

static int spl_boot_from_device(struct spl_image_info *spl_image,
                                u32 spl_boot_list[],
                                int num_devices)
{
    struct spl_boot_device boot_dev;
    int ret;

    for (int count = 0; count < num_devices; count++)
    {
        boot_dev.boot_device = spl_boot_list[count];
        boot_dev.boot_device_name = "NULL";

        switch (boot_dev.boot_device)
        {
        case BOOT_DEVICE_BOARD:
            /* TODO */
            break;

        case BOOT_DEVICE_MMC1:
            ret = spl_mmc_load_image(spl_image, &boot_dev);

            if (!ret)
            {
                spl_image->boot_device = spl_boot_list[count];
                return 0;
            }
            break;

        case BOOT_DEVICE_NAND:
            /* TODO */
            break;
        }
    }

    return -ENODEV;
}

void board_init_r(gd_t *dummy1, ulong dummy2)
{
    int ret;
    struct spl_image_info spl_image;

    /* Boot device priorities */
    u32 spl_boot_list[] = {
        BOOT_DEVICE_MMC1,
        BOOT_DEVICE_NAND,
        BOOT_DEVICE_NOR,
    };

    puts("\ni.MX6Q Tabiris Secondary Program Loader\n");

    spl_set_bd();

    /* Alloc memory for SPL usage */
    mem_malloc_init(CONFIG_SYS_SPL_MALLOC_START,
                    CONFIG_SYS_SPL_MALLOC_SIZE);
    gd->flags |= GD_FLG_FULL_MALLOC_INIT;

    /* Setup SPL init process: device tree & driver model */
    if (!(gd->flags & GD_FLG_SPL_INIT))
    {
        ret = spl_init();
        if (ret)
        {
            printf("SPL: Init failed\n");
            hang();	
        }
    }

    /* Init i.MX6 DRAM bank size called from arch/arm/mach-imx/spl.c */
    if (IS_ENABLED(CONFIG_SPL_OS_BOOT))
        dram_init_banksize();

    bootcount_inc();

    /* Setup memory and params for spl_image  */
    memset(&spl_image, '\0', sizeof(spl_image));
    spl_image.arg = (void *)CONFIG_SYS_SPL_ARGS_ADDR;
    spl_image.boot_device = BOOT_DEVICE_NONE;

    /* Check for booting device */
    spl_boot_list[0] = spl_boot_device();

    /* Booting process */
    ret = spl_boot_from_device(&spl_image, spl_boot_list,
                               ARRAY_SIZE(spl_boot_list));
    if (ret)
    {
        puts("SPL: Failed to boot from all devices\n");
        hang();
    }

    puts("SPL: Jumping to U-boot\n");

    spl_board_prepare_for_boot();
	jump_to_image_no_args(&spl_image);
}
