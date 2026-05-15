/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PciIo.h>

#include "pal_uefi.h"
#include "pal_status.h"

#define DMA_COHERENT 0x1
#define DMA_NOT_COHERENT   0x2

#define PAL_MAX_DMA_MAPS 8

static PAL_DMA_MAP gPalDmaMaps[PAL_MAX_DMA_MAPS];
static EFI_PCI_IO_PROTOCOL *gPalDmaPciIo;

static PAL_DMA_MAP *
pal_dma_alloc_slot(void)
{
  UINTN i;

  for (i = 0; i < PAL_MAX_DMA_MAPS; i++) {
    if (gPalDmaMaps[i].CpuAddr == NULL)
      return &gPalDmaMaps[i];
  }
  return NULL;
}

static PAL_DMA_MAP *
pal_dma_find_slot(void *cpu_addr)
{
  UINTN i;

  for (i = 0; i < PAL_MAX_DMA_MAPS; i++) {
    if (gPalDmaMaps[i].CpuAddr == cpu_addr)
      return &gPalDmaMaps[i];
  }
  return NULL;
}

/**
  @brief   Populate DMA_INFO_TABLE with the information of DMA Controllers
           in the system.

  @param   dma_info_table  Pointer to the DMA_INFO_TABLE data structure

  @return  None
**/
void
pal_dma_create_info_table(DMA_INFO_TABLE *dma_info_table)
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles = NULL;
  UINTN       HandleCount = 0;
  EFI_PCI_IO_PROTOCOL *PciIo = NULL;
  UINT64      Attr = 0;
  UINTN       i;
  dma_info_table->num_dma_ctrls = 0;
  gPalDmaPciIo = NULL;

  /* Discover one PCI I/O device to use for DMA buffer allocation/mapping. */
  Status = gBS->LocateHandleBuffer(ByProtocol,
                                   &gEfiPciIoProtocolGuid,
                                   NULL,
                                   &HandleCount,
                                   &Handles);
  if (!EFI_ERROR(Status) && HandleCount > 0) {
    for (i = 0; i < HandleCount; i++) {
      Status = gBS->HandleProtocol(Handles[i],
                                   &gEfiPciIoProtocolGuid,
                                   (VOID **)&PciIo);
      if (EFI_ERROR(Status)) {
        continue;
      }

      Status = PciIo->Attributes(PciIo,
                                 EfiPciIoAttributeOperationSupported,
                                 0,
                                 &Attr);
      if (EFI_ERROR(Status)) {
        continue;
      }

      if (Attr & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) {
        gPalDmaPciIo = PciIo;
        break;
      }
    }
  }

  if (Handles != NULL) {
    FreePool(Handles);
  }

  if (gPalDmaPciIo == NULL) {
    return;
  }

  /* Expose a single coherent DMA-capable instance so the test can run. */
  dma_info_table->num_dma_ctrls = 1;
  dma_info_table->info[0].host = NULL;
  dma_info_table->info[0].port = gPalDmaPciIo;
  dma_info_table->info[0].target = NULL;
  dma_info_table->info[0].flags = DMA_COHERENT;
  dma_info_table->info[0].type = DMA_TYPE_OTHER;
}

/**
  @brief   Allocate DMA capable memory and return the mapped DMA address

  @param   buffer     Pointer to store CPU accessible buffer address
  @param   length     Size of the buffer to allocate
  @param   dev        Device handle used for DMA mapping
  @param   flags      Attribute flags for the allocation
  @param   dma_addr   Pointer to store the DMA address

  @return  PAL_STATUS_SUCCESS on success, error code otherwise
**/
uint64_t
pal_dma_mem_alloc(void **buffer, uint32_t length, void *dev, uint32_t flags, UINT64 *dma_addr)
{
  EFI_STATUS           Status;
  UINTN                Pages;
  EFI_PCI_IO_PROTOCOL  *PciIo = (EFI_PCI_IO_PROTOCOL *)dev;
  PAL_DMA_MAP          *Entry;
  EFI_PCI_IO_PROTOCOL_OPERATION Operation;
  UINTN                NumberOfBytes;

  if (PciIo == NULL) {
    return PAL_STATUS_NO_RESOURCE;
  }

  Pages = EFI_SIZE_TO_PAGES(length);
  Entry = pal_dma_alloc_slot();
  if (Entry == NULL) {
    return PAL_STATUS_NO_RESOURCE;
  }

  Entry->UsingPci = TRUE;
  Entry->PciIo = PciIo;
  Entry->Pages = Pages;

  Status = PciIo->AllocateBuffer(PciIo,
                                 AllocateAnyPages,
                                 EfiBootServicesData,
                                 Pages,
                                 &Entry->CpuAddr,
                                 0);
  if (EFI_ERROR(Status)) {
    return PAL_STATUS_NO_RESOURCE;
  }

  ZeroMem(Entry->CpuAddr, length);

  if (flags & DMA_COHERENT)
    Operation = EfiPciIoOperationBusMasterCommonBuffer;
  else
    Operation = EfiPciIoOperationBusMasterRead;

  NumberOfBytes = length;
  Status = PciIo->Map(PciIo,
                      Operation,
                      Entry->CpuAddr,
                      &NumberOfBytes,
                      (EFI_PHYSICAL_ADDRESS *)dma_addr,
                      &Entry->Mapping);
  if (EFI_ERROR(Status)) {
    PciIo->FreeBuffer(PciIo, Pages, Entry->CpuAddr);
    ZeroMem(Entry, sizeof(*Entry));
    return PAL_STATUS_NO_RESOURCE;
  }

  *buffer = Entry->CpuAddr;
  return PAL_STATUS_SUCCESS;
}

/**
  @brief   Free DMA capable memory allocated by pal_dma_mem_alloc

  @param   buffer    CPU accessible buffer address to be freed
  @param   mem_dma   DMA address corresponding to the buffer
  @param   length    Size of the buffer
  @param   port      Device handle used for DMA mapping
  @param   flags     Attribute flags used for the allocation

  @return  None
**/
void
pal_dma_mem_free(void *buffer, UINT64 mem_dma, unsigned int length, void *port, unsigned int flags)
{
  PAL_DMA_MAP *Entry;
  UINTN        Pages;

  (void)mem_dma;
  (void)port;
  (void)flags;

  if (buffer == NULL) {
    return;
  }

  Entry = pal_dma_find_slot(buffer);
  if (Entry == NULL) {
    return;
  }

  Pages = Entry->Pages ? Entry->Pages : EFI_SIZE_TO_PAGES(length);

  if (Entry->UsingPci && Entry->PciIo != NULL) {
    if (Entry->Mapping != NULL) {
      Entry->PciIo->Unmap(Entry->PciIo, Entry->Mapping);
    }
    Entry->PciIo->FreeBuffer(Entry->PciIo, Pages, Entry->CpuAddr);
  } else {
    gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)Entry->CpuAddr, Pages);
  }

  ZeroMem(Entry, sizeof(*Entry));
}
