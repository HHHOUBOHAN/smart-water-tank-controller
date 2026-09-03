#include "BSP/flash.h"

#include "stm32f1xx_hal.h"

#include <string.h>

#define FLASH_CONFIG_PAGE_SIZE       1024U
#define FLASH_CONFIG_PAGE_A_ADDRESS  0x0800F800UL
#define FLASH_CONFIG_PAGE_B_ADDRESS  0x0800FC00UL

static bool Flash_IsPageValid(Flash_ConfigPage_t page)
{
    return ((uint32_t)page < (uint32_t)FLASH_CONFIG_PAGE_COUNT);
}

static uint32_t Flash_GetPageAddress(Flash_ConfigPage_t page)
{
    return (page == FLASH_CONFIG_PAGE_A) ?
           FLASH_CONFIG_PAGE_A_ADDRESS : FLASH_CONFIG_PAGE_B_ADDRESS;
}

static bool Flash_IsRangeValid(uint32_t offset, uint32_t length)
{
    return (length > 0U) && (offset < FLASH_CONFIG_PAGE_SIZE) &&
           (length <= (FLASH_CONFIG_PAGE_SIZE - offset));
}

uint32_t Flash_GetPageSize(void)
{
    return FLASH_CONFIG_PAGE_SIZE;
}

Flash_Status_t Flash_ErasePage(Flash_ConfigPage_t page)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0xFFFFFFFFUL;
    HAL_StatusTypeDef status;

    if (!Flash_IsPageValid(page))
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET)
    {
        return FLASH_STATUS_BUSY;
    }
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return FLASH_STATUS_UNLOCK_ERROR;
    }

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = Flash_GetPageAddress(page);
    erase.NbPages = 1U;
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    if (HAL_FLASH_Lock() != HAL_OK)
    {
        return FLASH_STATUS_LOCK_ERROR;
    }
    return (status == HAL_OK) ? FLASH_STATUS_OK : FLASH_STATUS_ERASE_ERROR;
}

Flash_Status_t Flash_Read(Flash_ConfigPage_t page,
                          uint32_t offset,
                          void *destination,
                          uint32_t length)
{
    if (!Flash_IsPageValid(page) || (destination == 0))
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }
    if (!Flash_IsRangeValid(offset, length))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }
    (void)memcpy(destination,
                 (const void *)(Flash_GetPageAddress(page) + offset),
                 length);
    return FLASH_STATUS_OK;
}

Flash_Status_t Flash_IsPageErased(Flash_ConfigPage_t page, bool *is_erased)
{
    uint32_t offset;
    const uint16_t *data;

    if (!Flash_IsPageValid(page) || (is_erased == 0))
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }
    data = (const uint16_t *)Flash_GetPageAddress(page);
    *is_erased = true;
    for (offset = 0U; offset < (FLASH_CONFIG_PAGE_SIZE / 2U); offset++)
    {
        if (data[offset] != 0xFFFFU)
        {
            *is_erased = false;
            break;
        }
    }
    return FLASH_STATUS_OK;
}

Flash_Status_t Flash_Write(Flash_ConfigPage_t page,
                           uint32_t offset,
                           const void *source,
                           uint32_t length)
{
    const uint8_t *bytes = (const uint8_t *)source;
    uint32_t address;
    uint32_t index;
    bool erased;

    if (!Flash_IsPageValid(page) || (source == 0))
    {
        return FLASH_STATUS_INVALID_ARGUMENT;
    }
    if (!Flash_IsRangeValid(offset, length))
    {
        return FLASH_STATUS_OUT_OF_RANGE;
    }
    if (((offset | length) & 1U) != 0U)
    {
        return FLASH_STATUS_ALIGNMENT_ERROR;
    }
    if ((Flash_IsPageErased(page, &erased) != FLASH_STATUS_OK) || !erased)
    {
        return FLASH_STATUS_NOT_ERASED;
    }
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return FLASH_STATUS_UNLOCK_ERROR;
    }

    address = Flash_GetPageAddress(page) + offset;
    for (index = 0U; index < length; index += 2U)
    {
        const uint16_t halfword = (uint16_t)bytes[index] |
                                  ((uint16_t)bytes[index + 1U] << 8U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              address + index,
                              halfword) != HAL_OK)
        {
            (void)HAL_FLASH_Lock();
            return FLASH_STATUS_PROGRAM_ERROR;
        }
    }
    if (HAL_FLASH_Lock() != HAL_OK)
    {
        return FLASH_STATUS_LOCK_ERROR;
    }
    if (memcmp((const void *)address, source, length) != 0)
    {
        return FLASH_STATUS_VERIFY_ERROR;
    }
    return FLASH_STATUS_OK;
}
