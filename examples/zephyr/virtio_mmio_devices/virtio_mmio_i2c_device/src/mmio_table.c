#include <openamp/virtio_mmio.h>

#define __resource Z_GENERIC_SECTION(.mmio_table)

static struct virtio_mmio_dev_table __resource __attribute__((used)) mmio_table = EMPTY_MMIO_TABLE;
