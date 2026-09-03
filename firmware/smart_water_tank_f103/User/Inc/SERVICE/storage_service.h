#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#define STORAGE_SERVICE_MAX_DATA_SIZE  64U

void StorageService_Init(void);
bool StorageService_Load(void *data, uint16_t length);
bool StorageService_Save(const void *data, uint16_t length);

#endif /* STORAGE_SERVICE_H */
