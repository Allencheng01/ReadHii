/** @file
    Copyright (c) 2020, Allen Cheng(AllenCheng01@gmail.com). All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include "GetHii.h"

EFI_BOOT_SERVICES         *pBS         = NULL;
EFI_RUNTIME_SERVICES      *pRS         = NULL;
EFI_HII_DATABASE_PROTOCOL *HiiDatabase = NULL;
EFI_HII_STRING_PROTOCOL   *MyHiiString  = NULL;
static EFI_GUID           HiiString_Guid = EFI_HII_STRING_PROTOCOL_GUID;

#if 0
#define MyDebugPrint(...) Print(__VA_ARGS__)
#else
#define MyDebugPrint(...)
#endif

static void StrCpyAtoW(CHAR16 *dest, CHAR8 *src){
  UINTN i;

  for(i = 0; src[i] != 0x00; i++){
    dest[i] = (UINT16)src[i];
  }
  dest[i] = 0x00;
}

static void StrCopyW(CHAR16 *dsc, CHAR16 *src){
  UINTN i;

  for(i = 0; src[i]; i++) dsc[i] = src[i];
  dsc[i] = 0;
}

UINTN StrLenA(CHAR8 *Str){
  UINTN Len = 0;
  for(Len = 0; Str[Len]!=0x00; Len++);
  Len += 1;
  return Len;
}

// static void CopyGuid(EFI_GUID *guid_des, EFI_GUID *guid_src){
//   UINT8 *des = (UINT8 *)guid_des;
//   UINT8 *src = (UINT8 *)guid_src;
//   UINTN Index = 0;

//   for(Index = 0; Index < sizeof(EFI_GUID); Index++)
//     des[Index] = src[Index];
// }

struct VarStore_Struct
{
  CHAR16 Name[128];
  EFI_GUID Guid;
  UINT16 VarStoreId;
};

static struct VarStore_Struct VarStore[32];

static void VarStoreInit(){
  UINTN Len;
  UINTN i;

  Len = sizeof(VarStore)/sizeof(VarStore[0]);
  for(i = 0; i < Len; i++){
    pBS->SetMem(&VarStore[i].Name, sizeof(VarStore[i].Name)/sizeof(VarStore[i].Name[0]), 0);
    VarStore[i].VarStoreId = 0;
    VarStore[i].Guid.Data1 = 0;
    VarStore[i].Guid.Data2 = 0;
    VarStore[i].Guid.Data3 = 0;
    VarStore[i].Guid.Data4[0] = 0;
    VarStore[i].Guid.Data4[1] = 0;
    VarStore[i].Guid.Data4[2] = 0;
    VarStore[i].Guid.Data4[3] = 0;
  }
}

static void VarStoreWrite(CHAR16 *Name, EFI_GUID Guid, UINT16 VarStoreId){
  UINTN Index;

  for(Index = 0; ; Index++){
    if(VarStore[Index].VarStoreId == 0)
      break;
  }
  VarStore[Index].VarStoreId = VarStoreId;
  CopyGuid(&VarStore[Index].Guid, &Guid);
  StrCopyW(VarStore[Index].Name, Name);
  MyDebugPrint(L"[%d] Name: %s Guid: %g VarStoreId: 0x%x\n", Index, VarStore[Index].Name, VarStore[Index].Guid, VarStore[Index].VarStoreId);
}

static void VarStoreGetNameGuidById(CHAR16 *VarName, EFI_GUID *VarGuid, UINT16 VarStoreId){
  UINTN Index;

  for(Index = 0; VarStore[Index].VarStoreId != 0; Index++){
    if(VarStoreId == VarStore[Index].VarStoreId){
      StrCopyW(VarName, VarStore[Index].Name);
      *VarGuid = VarStore[Index].Guid;
      break;
    }
  }
}

EFI_STATUS GetHiiHandleBuffer(UINTN *gHandleBufferLength, EFI_HII_HANDLE **gHiiHandleBuffer){
  UINTN           HandleBufferLength = 0;
  EFI_HII_HANDLE* HiiHandleBuffer = NULL;
  EFI_STATUS Status;

  pBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid,NULL,&HiiDatabase);
  if(HiiDatabase != NULL){
    Status = HiiDatabase->ListPackageLists(
                              HiiDatabase,
                              EFI_HII_PACKAGE_TYPE_ALL,
                              NULL,
                              &HandleBufferLength,
                              NULL
                              );
    if(Status != EFI_BUFFER_TOO_SMALL){
      MyDebugPrint(L"ListPackageLists Get BufferSize fail\n");
    }

    pBS->AllocatePool(EfiBootServicesData, HandleBufferLength, (void **)&HiiHandleBuffer);
    pBS->SetMem(HiiHandleBuffer, HandleBufferLength, 0x00);

    Status = HiiDatabase->ListPackageLists(
                              HiiDatabase,
                              EFI_HII_PACKAGE_TYPE_ALL,
                              NULL,
                              &HandleBufferLength,
                              HiiHandleBuffer
                              );
    if(Status != EFI_SUCCESS){
      MyDebugPrint(L"ListPackageLists fail\n");
    }
  }
  else{
    MyDebugPrint(L"Could not locate EFI_HII_DATABASE_PROTOCOL\n");
    return EFI_UNSUPPORTED;
  }
  *gHandleBufferLength = HandleBufferLength;
  *gHiiHandleBuffer = HiiHandleBuffer;

  return EFI_SUCCESS;
}

static CHAR16 *GetHiiString(EFI_HII_HANDLE pHandle, EFI_STRING_ID StringId){
  EFI_STATUS  Status;
  CHAR16      *StrBuff;
  UINTN       StrLen;

  StrBuff            = NULL;
  StrLen             = 0;
  Status             = EFI_SUCCESS;

  Status = MyHiiString->GetString(MyHiiString, "en-US", pHandle, StringId, NULL, &StrLen, NULL);
  if(Status == EFI_BUFFER_TOO_SMALL){
    pBS->AllocatePool(EfiBootServicesData, StrLen, &StrBuff);
    Status = MyHiiString->GetString(MyHiiString, "en-US", pHandle, StringId, StrBuff, &StrLen, NULL);
  }
  else{
    Status = MyHiiString->GetString(MyHiiString, "eng", pHandle, StringId, NULL, &StrLen, NULL);
    if(Status != EFI_BUFFER_TOO_SMALL){
      MyDebugPrint(L"Could not get string length with en-US/eng\n");
    }
    else{
      pBS->AllocatePool(EfiBootServicesData, StrLen, &StrBuff);
      Status = MyHiiString->GetString(MyHiiString, "eng", pHandle, StringId, StrBuff, &StrLen, NULL);
    }
  }

  return StrBuff;
}

EFI_STATUS ParsingValue(EFI_HII_HANDLE pHandle, EFI_IFR_OP_HEADER *OpCodeData){
  EFI_IFR_ONE_OF    *IfrOneOf;
  EFI_IFR_CHECKBOX  *IfrCheckBox;
  EFI_IFR_NUMERIC   *IfrNumeric;
  UINT16            NameId;
  UINT16            VarStoreId;
  UINT16            VarOffset;
  UINT8             Flags;
  CHAR16            *Name;
  UINT8             *data;
  UINTN             DataLen;
  EFI_STATUS        Status;
  static CHAR16     VarName[128];
  EFI_GUID          VarGuid;
  UINT32            Attribute;
  UINT64            Value;

  Status        = EFI_SUCCESS;
  IfrOneOf      = NULL;
  IfrCheckBox   = NULL;
  IfrNumeric    = NULL;
  NameId        = 0;
  VarStoreId    = 0;
  VarOffset     = 0;
  Flags         = 0;
  Name          = NULL;
  data          = NULL;
  DataLen       = 0;
  Attribute     = 0;
  Value         = 0;

  switch (OpCodeData->OpCode)
  {
    case EFI_IFR_NUMERIC_OP:
      IfrNumeric = (EFI_IFR_NUMERIC *)OpCodeData;
      NameId = IfrNumeric->Question.Header.Prompt;
      VarStoreId = IfrNumeric->Question.VarStoreId;
      VarOffset = IfrNumeric->Question.VarStoreInfo.VarOffset;
      Flags = (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE);
      break;
    case EFI_IFR_ONE_OF_OP:
      IfrOneOf = (EFI_IFR_ONE_OF *)OpCodeData;
      NameId = IfrOneOf->Question.Header.Prompt;
      VarStoreId = IfrOneOf->Question.VarStoreId;
      VarOffset = IfrOneOf->Question.VarStoreInfo.VarOffset;
      Flags = (IfrOneOf->Flags & EFI_IFR_NUMERIC_SIZE);
      break;
    case EFI_IFR_CHECKBOX_OP:
      IfrCheckBox = (EFI_IFR_CHECKBOX *)OpCodeData;
      NameId = IfrCheckBox->Question.Header.Prompt;
      VarStoreId = IfrCheckBox->Question.VarStoreId;
      VarOffset = IfrCheckBox->Question.VarStoreInfo.VarOffset;
      Flags = EFI_IFR_NUMERIC_SIZE_1;
      break;
  }

  if(NameId != 0){
    Name = GetHiiString(pHandle, NameId);
    MyDebugPrint(L"Name : %s VarStoreId = 0x%x ", Name, VarStoreId);
    VarStoreGetNameGuidById(VarName, &VarGuid, VarStoreId);
    MyDebugPrint(L"VarName = %s VarGuid = %g\n", VarName, VarGuid);

    Status = pRS->GetVariable(VarName, &VarGuid, &Attribute, &DataLen, data);
    if(Status != EFI_BUFFER_TOO_SMALL){
      MyDebugPrint(L"Could not load var: %s\n", VarName);
      MyDebugPrint(L"Status = 0x%x\n", Status);
      return Status;
    }
    pBS->AllocatePool(EfiBootServicesData, DataLen, &data);
    Status = pRS->GetVariable(VarName, &VarGuid, &Attribute, &DataLen, data);
    if(Status != EFI_SUCCESS){
      MyDebugPrint(L"fail to get variable, name : %s\n",VarName);
      return EFI_NOT_FOUND;
    }

    switch (Flags)
    {
    case EFI_IFR_NUMERIC_SIZE_1:
      Value  = data[VarOffset];
      break;
    case EFI_IFR_NUMERIC_SIZE_2:
      Value  = data[VarOffset];
      Value |= data[VarOffset + 1] << 8;
      break;
    case EFI_IFR_NUMERIC_SIZE_4:
      Value  = data[VarOffset];
      Value |= data[VarOffset + 1] << 8;
      Value |= data[VarOffset + 2] << 16;
      Value |= data[VarOffset + 3] << 24;
      break;
    case EFI_IFR_NUMERIC_SIZE_8:
      break;
    }
    Print(L"%s:[0x%lx]\n", Name, Value);
  }

  pBS->FreePool(Name);
  return EFI_SUCCESS;
}

EFI_STATUS ParsingVarStore(EFI_HII_HANDLE pHandle, EFI_IFR_OP_HEADER *OpCodeData){
  EFI_IFR_VARSTORE *IfrValueStore;
  EFI_IFR_VARSTORE_EFI *IfrValueStoreEfi;
  CHAR16 *Name;

  IfrValueStore = NULL;
  IfrValueStoreEfi = NULL;
  Name = NULL;

  switch (OpCodeData->OpCode)
  {
  case EFI_IFR_VARSTORE_OP:
    IfrValueStore = (EFI_IFR_VARSTORE *)OpCodeData;
    MyDebugPrint(L"IfrValueStore->VarStoreId = 0x%x\n", IfrValueStore->VarStoreId);
    pBS->AllocatePool(EfiBootServicesData, StrLenA(IfrValueStore->Name)*2, &Name);
    StrCpyAtoW(Name, IfrValueStore->Name);
    VarStoreWrite(Name, IfrValueStore->Guid, IfrValueStore->VarStoreId);
    pBS->FreePool(Name);
    break;
  case EFI_IFR_VARSTORE_EFI_OP:
    IfrValueStoreEfi = (EFI_IFR_VARSTORE_EFI *)OpCodeData;
    MyDebugPrint(L"IfrValueStoreEfi->VarStoreId = 0x%x\n", IfrValueStoreEfi->VarStoreId);
    pBS->AllocatePool(EfiBootServicesData, StrLenA(IfrValueStore->Name)*2, &Name);
    StrCpyAtoW(Name, IfrValueStoreEfi->Name);
    VarStoreWrite(Name, IfrValueStoreEfi->Guid, IfrValueStoreEfi->VarStoreId);
    pBS->FreePool(Name);
    break;
  }
  return EFI_SUCCESS;
}

EFI_STATUS ParsingOpCodeFun(EFI_HII_HANDLE pHandle, EFI_HII_PACKAGE_LIST_HEADER *PackageList){
  UINTN Offset;
  UINTN PackageList_Len;
  UINT8 *Package;
  EFI_HII_PACKAGE_HEADER *PackageHeader;

  Offset = 0;
  PackageList_Len = 0;
  Package = NULL;
  PackageHeader = NULL;

  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageList_Len = PackageList->PackageLength;
  while (Offset < PackageList_Len)
  {
    Package = ((UINT8 *)PackageList) + Offset;
    PackageHeader = (EFI_HII_PACKAGE_HEADER *)Package;

    if(PackageHeader->Type == EFI_HII_PACKAGE_FORMS){
      UINTN Offset2;

      Offset2 = sizeof(EFI_HII_PACKAGE_HEADER);
      while(Offset2 < PackageHeader->Length){
        EFI_IFR_OP_HEADER *OpCodeData;

        OpCodeData = (EFI_IFR_OP_HEADER *)(Package + Offset2);
        switch (OpCodeData->OpCode)
        {
        case EFI_IFR_FORM_SET_OP:
          VarStoreInit();
          break;
        case EFI_IFR_VARSTORE_OP:
        case EFI_IFR_VARSTORE_EFI_OP:
          ParsingVarStore(pHandle, OpCodeData);
          break;
        case EFI_IFR_NUMERIC_OP:
        case EFI_IFR_ONE_OF_OP:
        case EFI_IFR_CHECKBOX_OP:
          ParsingValue(pHandle, OpCodeData);
          break;
        }

        Offset2 += OpCodeData->Length;
      }

    }

    Offset += PackageHeader->Length;
  }

  return EFI_SUCCESS;
}

EFI_STATUS HandleEachHiiPackageList(EFI_HII_HANDLE pHandle){
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  UINTN BufferSize;
  EFI_STATUS Status;

  PackageList = NULL;
  BufferSize = 0;
  Status = EFI_SUCCESS;

  Status = HiiDatabase->ExportPackageLists(HiiDatabase, pHandle, &BufferSize, NULL);
  if(Status != EFI_BUFFER_TOO_SMALL){
    MyDebugPrint(L"ExportPackageLists get size fail\n");
    return Status;
  }

  pBS->AllocatePool(EfiBootServicesData, BufferSize, &PackageList);

  Status = HiiDatabase->ExportPackageLists(HiiDatabase, pHandle, &BufferSize, PackageList);
  if(Status != EFI_SUCCESS){
    MyDebugPrint(L"ExportPackageLists fail\n");
    return Status;
  }

  ParsingOpCodeFun(pHandle, PackageList);

  pBS->FreePool(PackageList);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetHiiEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN           HandleBufferLength;
  EFI_HII_HANDLE* HiiHandleBuffer;
  UINTN           Index;
  EFI_STATUS      Status;

  HandleBufferLength = 0;
  HiiHandleBuffer = NULL;
  Index = 0;
  pBS = SystemTable->BootServices;
  pRS = SystemTable->RuntimeServices;

  pBS->LocateProtocol(&HiiString_Guid, NULL, &MyHiiString);
  if(MyHiiString == NULL){
    MyDebugPrint(L"Could not locate EFI_HII_STRING_PROTOCOL\n");
    return EFI_LOAD_ERROR;
  }

  Status = GetHiiHandleBuffer(&HandleBufferLength, &HiiHandleBuffer);
  if(Status != EFI_SUCCESS){
    MyDebugPrint(L"GetHiiHandleBuffer fail\n");
    return EFI_UNSUPPORTED;
  }
  MyDebugPrint(L"HandleBufferLength/sizeof(EFI_HII_HANDLE) = 0x%x\n", HandleBufferLength/sizeof(EFI_HII_HANDLE));

  for(Index = 0; Index < (HandleBufferLength/sizeof(EFI_HII_HANDLE)); Index ++){
    EFI_HII_HANDLE              HiiHandle;
    EFI_HANDLE                  ModuleHandle;
    EFI_HII_CONFIG_ACCESS_PROTOCOL *ModuleConfigAccessProtocol;

    HiiHandle = HiiHandleBuffer[Index];
    Status = HiiDatabase->GetPackageListHandle(HiiDatabase, HiiHandle, &ModuleHandle);
    Status = pBS->HandleProtocol(ModuleHandle, &gEfiHiiConfigRoutingProtocolGuid, &ModuleConfigAccessProtocol);
    HandleEachHiiPackageList(HiiHandle);
  }

  pBS->FreePool(HiiHandleBuffer);
  return EFI_SUCCESS;
}
