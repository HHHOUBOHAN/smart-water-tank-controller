#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdbool.h>
#include <stdint.h>

//配Flash配置页编号。
typedef enum
{
    FLASH_CONFIG_PAGE_A = 0,
    FLASH_CONFIG_PAGE_B,

    FLASH_CONFIG_PAGE_COUNT
} Flash_ConfigPage_t;

//Flash底层操作状态。
typedef enum 
{
    // 操作成功。
    FLASH_STATUS_OK = 0,

    /*
     * 参数错误。
     *
     * 例如：
     * - 页编号错误
     * - 数据指针为空
     * - 长度为0
     * - is_erased指针为空
     */
    FLASH_STATUS_INVALID_ARGUMENT,

    /*
     * 访问范围超出单个配置页。
     *
     * 例如：
     * offset超过页面大小，
     * 或offset + length超过页面边界。
     */
    FLASH_STATUS_OUT_OF_RANGE,

    /*
     * 写入地址或写入长度没有按照16位对齐。
     *
     * STM32F103主Flash按照16位半字编程，
     * 所以写入offset和length必须是2的倍数。
     */
    FLASH_STATUS_ALIGNMENT_ERROR,

    /*
     * Flash控制器当前正在执行其他操作。
     */
    FLASH_STATUS_BUSY,

    /*
     * 准备写入的目标区域不是擦除状态。
     *
     * Flash擦除后的数据应该全部为0xFF。
     * Flash_Write()不会自动擦除页面。
     */
    FLASH_STATUS_NOT_ERASED,

    /*
     * Flash解锁失败。
     */
    FLASH_STATUS_UNLOCK_ERROR,

    /*
     * Flash页面擦除失败。
     */
    FLASH_STATUS_ERASE_ERROR,

    /*
     * Flash半字编程失败。
     */
    FLASH_STATUS_PROGRAM_ERROR,

    /*
     * 写入完成后，读取数据与原始数据不一致。
     */
    FLASH_STATUS_VERIFY_ERROR,

    /*
     * 操作完成后重新锁定Flash失败。
     */
    FLASH_STATUS_LOCK_ERROR
} Flash_Status_t;

//获取单个配置页大小
//  * 当前STM32F103C8T6的配置页大小应为：
//  *
//  * 1024字节，也就是0x400字节。
uint32_t Flash_GetPageSize(void);

//擦除指定配置页
Flash_Status_t Flash_ErasePage(Flash_ConfigPage_t page);

//指定配置页读取数据
Flash_Status_t Flash_Read(
    Flash_ConfigPage_t page,
    uint32_t offset,
    void *destination,
    uint32_t length
);



//向指定配置页写入数据。
Flash_Status_t Flash_Write(
    Flash_ConfigPage_t page,
    uint32_t offset,
    const void *source,
    uint32_t length
);

//检查指定配置页是否全部处于擦除状态。
Flash_Status_t Flash_IsPageErased(
    Flash_ConfigPage_t page,
    bool *is_erased
);



#endif
