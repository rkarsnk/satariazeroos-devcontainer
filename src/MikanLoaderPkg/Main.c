#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

struct MemoryMap
{
    UINTN buffer_size;
    VOID *buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

/*
 * ポインタ引数mapを渡して，メモリマップを取得する．
 */
EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
    if (map->buffer == NULL)
    {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;

    // gBS->GetMemoryMap()は，UEFIブートサービスが提供するAPI．
    return gBS->GetMemoryMap(&map->map_size,
                             (EFI_MEMORY_DESCRIPTOR *)map->buffer,
                             &map->map_key,
                             &map->descriptor_size,
                             &map->descriptor_version);
}

/*
 * メモリタイプの値からメモリタイプの名称を取得する．
 */
const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return L"EfiReservedMemoryType";
    case EfiLoaderCode:
        return L"EfiLoaderCode";
    case EfiLoaderData:
        return L"EfiLoaderData";
    case EfiBootServicesCode:
        return L"EfiBootServicesCode";
    case EfiBootServicesData:
        return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return L"EfiConventionalMemory";
    case EfiUnusableMemory:
        return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return L"EfiPalCode";
    case EfiPersistentMemory:
        return L"EfiPersistentMemory";
    case EfiMaxMemoryType:
        return L"EfiMaxMemoryType";
    default:
        return L"InvalidMemoryType";
    }
}

/*
 * メモリマップをfileで指定したファイルに書き出す
 */
EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header =
        "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
          map->buffer,
          map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
         iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
         iter += map->descriptor_size, i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
        len = AsciiSPrint(buf,
                          sizeof(buf),
                          "%u, %x, %-ls, %08lx, %lx, %lx\n",
                          i,
                          desc->Type,
                          GetMemoryTypeUnicode(desc->Type),
                          desc->PhysicalStart,
                          desc->NumberOfPages,
                          desc->Attribute & 0xffffflu);
        file->Write(file, &len, buf);
    }

    return EFI_SUCCESS;
}

/* ------------------------------------------------------------------------
   1. image_handleからDeviceHandleを取得する．
   2. DeviceHandleからEfiSimpleFileSystemProtocol fsを取得する．
   3. fsからroot volumeを取得する．
   ------------------------------------------------------------------------ */
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    gBS->OpenProtocol(image_handle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&loaded_image,
                      image_handle,
                      NULL,
                      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    gBS->OpenProtocol(loaded_image->DeviceHandle,
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **)&fs,
                      image_handle,
                      NULL,
                      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    fs->OpenVolume(fs, root);

    return EFI_SUCCESS;
}
/**
 * Graphic Output Protocol を取得する．
 */
EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop)
{
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;
    gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles);

    gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    switch (fmt)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
        return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
        return L"PixelBitMask";
    case PixelBltOnly:
        return L"PixelBltOnly";
    case PixelFormatMax:
        return L"PixelFormatMax";
    default:
        return L"InvalidPixelFormat";
    }
}

/*
 * Main関数
 */
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{

    Print(L"--------------------------------------------------------------------------\n");
    Print(L"  #####                                      #######                      \n");
    Print(L" #     #   ##   #####   ##   #####  #   ##        #  ###### #####   ####  \n");
    Print(L" #        #  #    #    #  #  #    # #  #  #      #   #      #    # #    # \n");
    Print(L"  #####  #    #   #   #    # #    # # #    #    #    #####  #    # #    # \n");
    Print(L"       # ######   #   ###### #####  # ######   #     #      #####  #    # \n");
    Print(L" #     # #    #   #   #    # #   #  # #    #  #      #      #   #  #    # \n");
    Print(L"  #####  #    #   #   #    # #    # # #    # ####### ###### #    #  ####  \n");
    Print(L"--------------------------------------------------------------------------\n");

    // MemoryMap構造体の変数 memmapにUEFIブートサービスから
    // 取得したメモリマップを格納する．
    CHAR8 memmap_buf[4096 * 4];
    struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
    GetMemoryMap(&memmap);

    // ファイルを書き出すボリュームを開く．
    EFI_FILE_PROTOCOL *root_dir;
    OpenRootDir(image_handle, &root_dir);

    // memmapファイルをOpenする
    EFI_FILE_PROTOCOL *memmap_file;
    root_dir->Open(root_dir,
                   &memmap_file,
                   L"\\memmap.txt",
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE |
                       EFI_FILE_MODE_CREATE,
                   0);

    // memmapファイルに取得したメモリマップを書き出す．
    SaveMemoryMap(&memmap, memmap_file);
    memmap_file->Close(memmap_file);

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    OpenGOP(image_handle, &gop);
    Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine);
    Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize);
  
    UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
    int color = 200;
    for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i) {
      frame_buffer[i] = color;
    }

    EFI_FILE_PROTOCOL *kernel_file;
    // kernel.elfファイルをOpenする
    root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0);

    // EFI_FILE_INFO型を格納できるメモリ領域のサイズを指定
    // sizeof(CHAR16) * 12 バイトだけ大きめに指定
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size,
        file_info_buffer);

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    // リンカオプションで指定した --image-baseの値
    EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
    // カーネルを配置するメモリ領域を確保
    UINTN kernel_page_length = (kernel_file_size + 0xfff) / 0x1000;
    gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        kernel_page_length,
        &kernel_base_addr);

    // カーネルを確保したメモリ領域に読み込む
    kernel_file->Read(
        kernel_file,
        &kernel_file_size,
        (VOID *)kernel_base_addr);
    Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

    // UEFI BootServiceを停止する
    EFI_STATUS status;
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status))
    {
        status = GetMemoryMap(&memmap);
        if (EFI_ERROR(status))
        {
            Print(L"failed to get memory map: %r\n", status);
            while (1)
                ;
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if (EFI_ERROR(status))
        {
            Print(L"Could not exit boot service: %r\n", status);
            while (1)
                ;
        }
    }

    // kernelを起動する
    UINT64 entry_addr = *(UINT64 *)(kernel_base_addr + 24);

    typedef void EntryPointType(void);
    EntryPointType *entry_point = (EntryPointType *)entry_addr;
    entry_point();

    Print(L"All done\n");

    while (1)
        ;
    return EFI_SUCCESS;
}
