

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <jffs2/load_kernel.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <uboot_made/uboot_made.h>

#include "flash_adapter.h"


#if FIRMWARE_STORE_MEDIUM == 1
#include <spi_flash.h>

static struct spi_flash *adapter_flash;

#elif FIRMWARE_STORE_MEDIUM == 2
#include <linux/mtd/mtd.h>
#include <nand.h>
#endif



#if FIRMWARE_STORE_MEDIUM == 1

int spinor_flash_read(char *buf, unsigned long offset, unsigned long *len)
{
        int ret;

        if (adapter_flash)
                spi_flash_free(adapter_flash);

        adapter_flash = spi_flash_probe(0, 0, 0, 0);

        /* Consistency checking */
        if (offset + *len > adapter_flash->size) {
                printf("ERROR: attempting past flash size (%#x)\n",
                                adapter_flash->size);
                return -1;
        }

        ret = spi_flash_read(adapter_flash, offset, *len, buf);
        if (ret) {
                printf("%s.%d, read flash failed, %d\n", __func__, __LINE__, ret);
                return ret;
        }

        return ret;
}

int spinor_flash_write(char *buf, unsigned long offset, unsigned long len)
{
        int ret;
        unsigned long write_len, write_step;
        unsigned int remainder;

        if (adapter_flash)
                spi_flash_free(adapter_flash);

        adapter_flash = spi_flash_probe(0, 0, 0, 0);

        /* Consistency checking */
        if (offset + len > adapter_flash->size) {
                printf("ERROR: attempting past flash size (%#x)\n",
                                adapter_flash->size);
                return -1;
        }

        write_len   = len;
        write_step  = adapter_flash->sector_size;
        if (write_len < write_step)
                write_len = write_step;
        else {
                remainder = write_len % write_step;
                if (remainder != 0)
                        write_len = write_len + write_step - remainder;
        }

	/** unprotect all **/
#if NOR_FLASH_PROTECT_SUPPORT == 1
        spi_flash_protect(adapter_flash, 0, 0, false);
#endif

        ret = spi_flash_erase(adapter_flash, offset, write_len);
        if (ret) {
                printf("%s.%d, erase flash failed, %d\n", __func__, __LINE__, ret);
                return ret;
        }

#if NOR_FLASH_PROTECT_SUPPORT == 1
        spi_flash_protect(adapter_flash, 0, 0, false);
#endif

        ret = spi_flash_write(adapter_flash, offset, len, buf);
        if (ret) {
                printf("%s.%d, write flash failed, %d\n", __func__, __LINE__, ret);
                return ret;
        }

    return ret;
}

#elif FIRMWARE_STORE_MEDIUM == 2
int spinand_flash_read(char *buf, unsigned long offset, unsigned long *len)
{
    struct mtd_info *mtd;
    int dev = nand_curr_device;
    int bufsize = 0x8000000;
    int ret;

    mtd = &nand_info[dev];
    if (!mtd) {
        puts("\nno devices available\n");
        return -1;
    }

    /* Consistency checking */
    if (offset + *len > mtd->size) {
        printf("ERROR: attempting past flash size (%#x)\n", mtd->size);
        return -1;
    }

    ret = nand_read_skip_bad(mtd, offset, len, NULL, bufsize, (u_char *)buf);
    if (ret) {
        printf("%s.%d, read flash failed, %d\n", __func__, __LINE__, ret);
        return -1;
    }

    return 0;
}

int spinand_flash_write(char *buf, unsigned long offset, unsigned long len)
{
    struct mtd_info *mtd;
    int dev = nand_curr_device;
    nand_erase_options_t opts;
    int bufsize = 0x8000000;
    int ret;

    mtd = &nand_info[dev];
    if (!mtd) {
        puts("\nno devices available\n");
        return -1;
    }

    /* Consistency checking */
    if (offset + len > mtd->size) {
        printf("ERROR: attempting past flash size (%#x)\n", mtd->size);
        return -1;
    }

    memset(&opts, 0, sizeof(opts));
    opts.offset = offset;
    opts.length = len;
    opts.jffs2  = 0;
    opts.quiet  = 1;
    opts.spread = 1; // skip bad blocks
    ret = nand_erase_opts(mtd, &opts);
    if (ret) {
        printf("%s.%d, erase flash failed, %d\n", __func__, __LINE__, ret);
        return -1;
    }

    ret = nand_write_skip_bad(mtd, offset, &len, NULL, bufsize, (u_char *)buf,0);
    if (ret) {
        printf("%s.%d, write flash failed, %d\n", __func__, __LINE__, ret);
        return -1;
    }

    return 0;
}

int spinand_flash_isbad(int dev, unsigned long off)
{
    nand_info_t *nandinfo;

    nandinfo = &nand_info[dev];
    if (nand_block_isbad(nandinfo, off))
        return 1;

    return 0;
}
#endif

