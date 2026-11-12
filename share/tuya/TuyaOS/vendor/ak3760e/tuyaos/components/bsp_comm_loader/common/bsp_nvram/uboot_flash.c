#include <common.h>
#include <nvram/bcmnvram.h>
#include <spi_flash.h>

#include <jffs2/jffs2.h>
#include <uboot_made/uboot_made.h>
/* partition handling routines */
extern int mtdparts_init(void);
extern int find_dev_and_part(const char *id, struct mtd_device **dev,
        u8 *part_num, struct part_info **part);


static unsigned int current_write_counter = 0;
static u_int32_t current_offs = 0;

extern unsigned short verify_zbuf_chksum(char *zbuf_with_header);
#if FIRMWARE_STORE_MEDIUM == 1
extern int spinor_flash_read(char *buf, unsigned long offset, unsigned long *len);
extern int spinor_flash_write(char *buf, unsigned long offset, unsigned long len);
#elif FIRMWARE_STORE_MEDIUM == 2
extern int spinand_flash_read(char *buf, unsigned long offset, unsigned long *len);
extern int spinand_flash_write(char *buf, unsigned long offset, unsigned long len);
extern int spinand_flash_isbad(int dev, unsigned long off);
#endif
extern int get_mtd_partition_size(char *partname, int partnamelen);

int ra_mtd_write_nm(int offs, loff_t to, u_int32_t len, char *buf)
{
#if FIRMWARE_STORE_MEDIUM == 1
	return spinor_flash_write(buf, offs+to, (unsigned long)len);
#elif FIRMWARE_STORE_MEDIUM == 2
	return spinand_flash_write(buf, offs+to, (unsigned long)len);
#endif
}

int ra_mtd_read_nm(int offs, loff_t from, u_int32_t len, char *buf)
{
#if FIRMWARE_STORE_MEDIUM == 1
	return spinor_flash_read(buf, offs+from, (unsigned long *)&len);
#elif FIRMWARE_STORE_MEDIUM == 2
	return spinand_flash_read(buf, offs+from, (unsigned long *)&len);
#endif
}

int _nvram_check(struct nvram_header *header)
{
	if (header->magic == 0xFFFFFFFF || header->magic == 0)
		return 1; // NVRAM MTD is clear

	if (header->magic != NVRAM_MAGIC)
		return -1; // NVRAM is garbage

	if (header->len < sizeof(struct nvram_header))
		return -2; // NVRAM size underflow

	if (header->len > NVRAM_SPACE)
		return -3; // NVRAM size overflow

	if (verify_zbuf_chksum((char*)header))
		return -4; // NVRAM content is corrupted

	return 0;
}

int nvram_flash_read_zbuf(char *zbuf, u_int32_t len)
{
    int ii;
    int ret;
    struct nvram_header *header = (struct nvram_header*)zbuf;
    int nvram_sz=0;

    nvram_sz = NVRAM_FLASH_PARTITION_SIZE;

    current_write_counter = 0;

	for (ii = 0; ii < (nvram_sz/NVRAM_SPACE); ii++) {
#if FIRMWARE_STORE_MEDIUM == 2
		// TODO: check bad blocks
		if(spinand_flash_isbad(0, (unsigned long)(ii*NVRAM_SPACE+NVRAM_UBOOT_FLASH_OFFSET) ))
		{
            printf("%d block=0x%lx is badblk\n", __LINE__, (unsigned long)(ii*NVRAM_SPACE+NVRAM_UBOOT_FLASH_OFFSET));
            continue;
		}
#endif

		ret = ra_mtd_read_nm(NVRAM_UBOOT_FLASH_OFFSET, ii*NVRAM_SPACE, len, zbuf);
		if (ret)
			continue;

		ret = _nvram_check(header);
		if (ret)
			continue;

		if (current_write_counter <= header->write_counter) {
			current_write_counter = header->write_counter;
			current_offs = ii*NVRAM_SPACE;
		}
	}

	ret = ra_mtd_read_nm(NVRAM_UBOOT_FLASH_OFFSET, current_offs, len, zbuf);
	if (ret < 0) {
		printf("%s: read flash failed, ret: %d\n", __func__, ret);
		return -1;
	}

	printf("%s: offs: 0x%x, len: %d\n", __func__, current_offs, len);

	return 0;
}

int nvram_flash_write_zbuf(char *zbuf, u_int32_t len)
{
    int ret;
     struct nvram_header *header = (struct nvram_header*)zbuf;
    int retry=0;
    int nvram_sz=0;

    current_write_counter++;
    header->write_counter = current_write_counter;

    nvram_sz = NVRAM_FLASH_PARTITION_SIZE;

    retry = nvram_sz/NVRAM_SPACE;
    printf("%s %d nvram_sz=0x%x\n", __func__, __LINE__, nvram_sz);
    while(retry)
    {
        retry--;
        current_offs += NVRAM_SPACE;
        if (current_offs + NVRAM_SPACE > nvram_sz)
            current_offs = 0;

#if FIRMWARE_STORE_MEDIUM == 1
        break;
#elif FIRMWARE_STORE_MEDIUM == 2
        // TODO: check bad blocks
        if(spinand_flash_isbad(0, (unsigned long)(current_offs+NVRAM_UBOOT_FLASH_OFFSET)))
        {
            printf("%d block=0x%lx is badblk\n", __LINE__, (unsigned long)(current_offs+NVRAM_UBOOT_FLASH_OFFSET));
            continue;
        }
        else
        {
            break;
        }
#endif
    }

#if FIRMWARE_STORE_MEDIUM == 2
    if(!retry)
    {
        printf("%s: only one good block!!!!, retry: %d\n", __func__, retry);
    }
#endif

	ret = ra_mtd_write_nm(NVRAM_UBOOT_FLASH_OFFSET, current_offs, len, zbuf);
	if (ret < 0) {
		printf("%s: write flash failed, ret: %d\n", __func__, ret);
		return -1;
	}

	printf("%s: offs: 0x%x, len: %d, header->len %d\n", __func__, current_offs, len, header->len);

	return 0;
}
