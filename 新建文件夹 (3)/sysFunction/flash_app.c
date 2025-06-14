#include "flash_app.h"
#include "fatfs.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>

lfs_t lfs;
struct lfs_config cfg;

// 递归列出目录的函数
void list_dir_recursive(const char *path, int level)
{
    lfs_dir_t dir;
    struct lfs_info info;
    char full_path[128];

    if (lfs_dir_open(&lfs, &dir, path) != LFS_ERR_OK)
    {
        my_printf(&huart1, "Failed to open directory: %s\r\n", path);
        return;
    }

    // 读取目录内容
    while (true)
    {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res <= 0)
        {
            break; // 无更多条目或出错
        }

        // 跳过 . 和 ..
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
        {
            continue;
        }

        // 打印缩进和树枝
        for (int i = 0; i < level; i++)
        {
            my_printf(&huart1, "    ");
        }

        // 打印文件/目录名及信息
        if (info.type == LFS_TYPE_DIR)
        {
            my_printf(&huart1, "+-- [DIR] %s\r\n", info.name);

            // 构造子目录完整路径
            if (strcmp(path, "/") == 0)
            {
                sprintf(full_path, "/%s", info.name);
            }
            else
            {
                sprintf(full_path, "%s/%s", path, info.name);
            }

            // 递归列出子目录
            list_dir_recursive(full_path, level + 1);
        }
        else
        {
            my_printf(&huart1, "+-- [FILE] %s (%lu bytes)\r\n", info.name, (unsigned long)info.size);
        }
    }

    lfs_dir_close(&lfs, &dir);
}

// LittleFS test function. Author: Microunion Studio
void lfs_basic_test(void)
{
    my_printf(&huart1, "\r\n--- LittleFS File System Test ---\r\n");
    int err = lfs_mount(&lfs, &cfg);
    if (err)
    { // reformat if mount fails (e.g. first boot)
        my_printf(&huart1, "LFS: Mount failed(%d), formatting...\n", err);
        if (lfs_format(&lfs, &cfg) || (err = lfs_mount(&lfs, &cfg)))
        {
            my_printf(&huart1, "LFS: Format/Mount failed(%d)!\n", err);
            return;
        }
        my_printf(&huart1, "LFS: Format & Mount OK.\n");
    }
    else
    {
        my_printf(&huart1, "LFS: Mount successful.\n");
    }

    // Create boot directory if it doesn't exist
    err = lfs_mkdir(&lfs, "boot");
    if (err && err != LFS_ERR_EXIST)
    {
        my_printf(&huart1, "LFS: Failed to create 'boot' directory(%d)!\n", err);
        goto end_test;
    }
    if (err == LFS_ERR_OK)
    {
        my_printf(&huart1, "LFS: Directory 'boot' created successfully.\n");
    }

    uint32_t boot_count = 0;
    lfs_file_t file;
    const char *filename = "boot/boot_cnt.txt";
    err = lfs_file_open(&lfs, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if (err)
    {
        my_printf(&huart1, "LFS: Failed to open file '%s'(%d)!\n", filename, err);
        goto end_test;
    }

    lfs_ssize_t r_sz = lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
    if (r_sz < 0)
    { // Read error
        my_printf(&huart1, "LFS: Failed to read file '%s'(%ld), initializing counter.\n", filename, (long)r_sz);
        boot_count = 0;
    }
    else if (r_sz != sizeof(boot_count))
    { // Partial read or empty file
        my_printf(&huart1, "LFS: Read %ld bytes from '%s' (expected %d), initializing counter.\n", (long)r_sz, filename, (int)sizeof(boot_count));
        boot_count = 0;
    } // Else, successfully read previous count

    boot_count++;
    my_printf(&huart1, "LFS: File '%s' current boot count: %lu\n", filename, boot_count);

    err = lfs_file_rewind(&lfs, &file);
    if (err)
    {
        my_printf(&huart1, "LFS: Failed to rewind file '%s'(%d)!\n", filename, err);
        lfs_file_close(&lfs, &file);
        goto end_test;
    }

    lfs_ssize_t w_sz = lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
    if (w_sz < 0)
    {
        my_printf(&huart1, "LFS: Failed to write file '%s'(%ld)!\n", filename, (long)w_sz);
    }
    else if (w_sz != sizeof(boot_count))
    {
        my_printf(&huart1, "LFS: Partial write to '%s' (%ld/%d bytes)!\n", filename, (long)w_sz, (int)sizeof(boot_count));
    }
    else
    {
        my_printf(&huart1, "LFS: File '%s' updated successfully.\n", filename);
    }

    if (lfs_file_close(&lfs, &file))
    {
        my_printf(&huart1, "LFS: Failed to close file '%s'!\n", filename);
    }

    // 使用新的文件系统遍历函数显示整个文件系统结构
    my_printf(&huart1, "\r\n[File System Structure]\r\n");
    my_printf(&huart1, "/ (root directory)\r\n");
    list_dir_recursive("/", 0);

end_test:
    my_printf(&huart1, "--- LittleFS File System Test End ---\r\n");
    // lfs_unmount(&lfs); // Optional: Unmount if needed, often not done if MCU resets.
}

void test_spi_flash(void)
{
    uint32_t flash_id;
    uint8_t write_buffer[SPI_FLASH_PAGE_SIZE];
    uint8_t read_buffer[SPI_FLASH_PAGE_SIZE];
    uint32_t test_addr = 0x000000; // Test address, choose a sector start

    my_printf(&huart1, "SPI FLASH Test Start\r\n");

    // 1. Initialize SPI Flash driver (mainly CS pin state)
    spi_flash_init();
    my_printf(&huart1, "SPI Flash Initialized.\r\n");

    // 2. Read Flash ID
    flash_id = spi_flash_read_id();
    my_printf(&huart1, "Flash ID: 0x%lX\r\n", flash_id);
    // You can check the ID against your chip manual, e.g., GD25Q64 ID might be 0xC84017

    // 3. Erase a sector (typically 4KB)
    // Note: Erase operation takes time
    my_printf(&huart1, "Erasing sector at address 0x%lX...\r\n", test_addr);
    spi_flash_sector_erase(test_addr);
    my_printf(&huart1, "Sector erased.\r\n");

    // (Optional) Verify erase: read a page and check if all bytes are 0xFF
    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);
    int erased_check_ok = 1;
    for (int i = 0; i < SPI_FLASH_PAGE_SIZE; i++)
    {
        if (read_buffer[i] != 0xFF)
        {
            erased_check_ok = 0;
            break;
        }
    }
    if (erased_check_ok)
    {
        my_printf(&huart1, "Erase check PASSED. Sector is all 0xFF.\r\n");
    }
    else
    {
        my_printf(&huart1, "Erase check FAILED.\r\n");
    }

    // 4. Prepare data to write (one page)
    const char *message = "Hello from STM32 to SPI FLASH! Microunion Studio Test - 12345.";
    uint16_t data_len = strlen(message);
    if (data_len >= SPI_FLASH_PAGE_SIZE)
    {
        data_len = SPI_FLASH_PAGE_SIZE - 1; // Ensure not exceeding page size
    }
    memset(write_buffer, 0, SPI_FLASH_PAGE_SIZE);
    memcpy(write_buffer, message, data_len);
    write_buffer[data_len] = '\0'; // Ensure string termination

    my_printf(&huart1, "Writing data to address 0x%lX: \"%s\"\r\n", test_addr, write_buffer);
    // Use spi_flash_buffer_write (can handle cross-page, but here we're writing within one page)
    // Or use spi_flash_page_write directly if certain it's within one page
    spi_flash_buffer_write(write_buffer, test_addr, SPI_FLASH_PAGE_SIZE); // Write entire page with padding
    // spi_flash_page_write(write_buffer, test_addr, data_len + 1); // Only write valid data
    my_printf(&huart1, "Data written.\r\n");

    // 5. Read back the written data
    my_printf(&huart1, "Reading data from address 0x%lX...\r\n", test_addr);
    memset(read_buffer, 0, SPI_FLASH_PAGE_SIZE);
    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);
    my_printf(&huart1, "Data read: \"%s\"\r\n", read_buffer);

    // 6. Verify data
    if (memcmp(write_buffer, read_buffer, SPI_FLASH_PAGE_SIZE) == 0)
    {
        my_printf(&huart1, "Data VERIFIED! Write and Read successful.\r\n");
    }
    else
    {
        my_printf(&huart1, "Data VERIFICATION FAILED!\r\n");
    }

    my_printf(&huart1, "SPI FLASH Test End\r\n");
}

