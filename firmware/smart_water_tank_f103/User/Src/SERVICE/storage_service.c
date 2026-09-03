#include "SERVICE/storage_service.h"

#include "BSP/flash.h"

#include <stddef.h>
#include <string.h>

#define STORAGE_MAGIC    0x57415452UL
#define STORAGE_VERSION  6U

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    uint32_t sequence;
    uint8_t data[STORAGE_SERVICE_MAX_DATA_SIZE];
    uint32_t crc32;
} StorageRecord_t;

static bool storage_has_active;
static Flash_ConfigPage_t storage_active_page;
static uint32_t storage_active_sequence;

static uint32_t StorageService_Crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t index;
    uint32_t bit;

    for (index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ?
                  ((crc >> 1U) ^ 0xEDB88320UL) : (crc >> 1U);
        }
    }
    return ~crc;
}

static bool StorageService_RecordValid(const StorageRecord_t *record)
{
    uint32_t expected;

    if ((record->magic != STORAGE_MAGIC) ||
        (record->version != STORAGE_VERSION) ||
        (record->length == 0U) ||
        (record->length > STORAGE_SERVICE_MAX_DATA_SIZE))
    {
        return false;
    }
    expected = StorageService_Crc32((const uint8_t *)record,
                                    (uint32_t)offsetof(StorageRecord_t,
                                                       crc32));
    return (expected == record->crc32);
}

static bool StorageService_ReadRecord(Flash_ConfigPage_t page,
                                      StorageRecord_t *record)
{
    return (Flash_Read(page, 0U, record, sizeof(*record)) ==
            FLASH_STATUS_OK) && StorageService_RecordValid(record);
}

void StorageService_Init(void)
{
    StorageRecord_t record_a;
    StorageRecord_t record_b;
    const bool valid_a = StorageService_ReadRecord(FLASH_CONFIG_PAGE_A,
                                                    &record_a);
    const bool valid_b = StorageService_ReadRecord(FLASH_CONFIG_PAGE_B,
                                                    &record_b);

    storage_has_active = false;
    storage_active_page = FLASH_CONFIG_PAGE_A;
    storage_active_sequence = 0U;

    if (valid_a && (!valid_b || (record_a.sequence >= record_b.sequence)))
    {
        storage_has_active = true;
        storage_active_page = FLASH_CONFIG_PAGE_A;
        storage_active_sequence = record_a.sequence;
    }
    else if (valid_b)
    {
        storage_has_active = true;
        storage_active_page = FLASH_CONFIG_PAGE_B;
        storage_active_sequence = record_b.sequence;
    }
}

bool StorageService_Load(void *data, uint16_t length)
{
    StorageRecord_t record;

    if (!storage_has_active || (data == 0) ||
        (length == 0U) || (length > STORAGE_SERVICE_MAX_DATA_SIZE) ||
        !StorageService_ReadRecord(storage_active_page, &record) ||
        (record.length != length))
    {
        return false;
    }
    (void)memcpy(data, record.data, length);
    return true;
}

bool StorageService_Save(const void *data, uint16_t length)
{
    StorageRecord_t record;
    StorageRecord_t verify;
    Flash_ConfigPage_t target;

    if ((data == 0) || (length == 0U) ||
        (length > STORAGE_SERVICE_MAX_DATA_SIZE))
    {
        return false;
    }

    (void)memset(&record, 0, sizeof(record));
    record.magic = STORAGE_MAGIC;
    record.version = STORAGE_VERSION;
    record.length = length;
    record.sequence = storage_active_sequence + 1U;
    (void)memcpy(record.data, data, length);
    record.crc32 = StorageService_Crc32((const uint8_t *)&record,
                                        (uint32_t)offsetof(StorageRecord_t,
                                                           crc32));

    target = storage_has_active ?
             ((storage_active_page == FLASH_CONFIG_PAGE_A) ?
              FLASH_CONFIG_PAGE_B : FLASH_CONFIG_PAGE_A) :
             FLASH_CONFIG_PAGE_A;

    if ((Flash_ErasePage(target) != FLASH_STATUS_OK) ||
        (Flash_Write(target, 0U, &record, sizeof(record)) != FLASH_STATUS_OK) ||
        !StorageService_ReadRecord(target, &verify) ||
        (verify.sequence != record.sequence) ||
        (verify.length != record.length) ||
        (memcmp(verify.data, record.data, length) != 0))
    {
        return false;
    }

    storage_has_active = true;
    storage_active_page = target;
    storage_active_sequence = record.sequence;
    return true;
}
