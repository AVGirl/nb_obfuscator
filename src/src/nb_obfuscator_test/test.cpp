#include "pch.h"
#include <iostream>
#include <string>
#include "nb_obfuscator/nb_obfuscator.h"
#include <windows.h>

#ifdef _DEBUG
TEST(TestCaseName, TestName) {
#if defined(_M_IX86)
  EXPECT_TRUE(GenConfusion(L"E:\\workspace\\nb_obfuscator\\bin\\Win32\\Release\\nb_obfuscator_test.exe"));
#elif (defined(_M_X64) || defined(__IA64__))
  EXPECT_TRUE(GenConfusion(L"E:\\workspace\\nb_obfuscator\\bin\\x64\\Release\\nb_obfuscator_test.exe"));
#endif
}

#endif // _DEBUG

DWORD Align(DWORD dwNum, DWORD dwAlign)
{
  if (dwNum % dwAlign == 0)
  {
    return dwNum;
  }
  else
  {
    return (dwNum / dwAlign + 1) * dwAlign;
  }
}

int add_sec(const wchar_t* pe_file)
{
  char szFilePath[MAX_PATH];//Ҫ�������ļ�����·��
  OPENFILENAME ofn;//����ṹ�����ô򿪶Ի���ѡ��Ҫ�������ļ����䱣��·��

  HANDLE hFile;// �ļ����
  HANDLE hMapping;// ӳ���ļ����
  LPVOID ImageBase;// ӳ���ַ

  PIMAGE_DOS_HEADER  pDH = NULL;//ָ��IMAGE_DOS�ṹ��ָ��
  PIMAGE_NT_HEADERS  pNtH = NULL;//ָ��IMAGE_NT�ṹ��ָ��
  PIMAGE_FILE_HEADER pFH = NULL;;//ָ��IMAGE_FILE�ṹ��ָ��
  PIMAGE_OPTIONAL_HEADER pOH = NULL;//ָ��IMAGE_OPTIONALE�ṹ��ָ��
  PIMAGE_SECTION_HEADER pSH1 = NULL;//ָ��IMAGE_SECTION_TABLE�ṹ��ָ��first
  PIMAGE_SECTION_HEADER pSH2 = NULL;//ָ��IMAGE_SECTION_TABLE�ṹ��ָ��two
  PIMAGE_SECTION_HEADER pSH3 = NULL;//ָ��IMAGE_SECTION_TABLE�ṹ��ָ��three

  //��Ҫ�ĳ�ʼ��
  //ѡ��Ҫ�������ļ��󣬾���3���򿪲�ӳ��ѡ����ļ��������ڴ���
  //1.�����ļ��ں˶�������������hFile�����ļ�������洢����λ��ͨ�������ϵͳ
  hFile = CreateFile(pe_file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (!hFile)
  {
    MessageBoxA(NULL, "���ļ�����", NULL, MB_OK);
    return 0;
  }

  //2.�����ļ�ӳ���ں˶��󣨷��������ڴ棩�����������hFileMapping
  hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
  if (!hMapping)
  {
    CloseHandle(hFile);
    return FALSE;
  }

  //3.���ļ�����ӳ�䵽���̵ĵ�ַ�ռ䣬���ص�ӳ���ַ������ImageBase��
  ImageBase = MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
  if (!ImageBase)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return FALSE;
  }

  //IMAGE_DOS Header�ṹָ��
  pDH = (PIMAGE_DOS_HEADER)ImageBase;
  //IMAGE_NT Header�ṹָ��
  pNtH = (PIMAGE_NT_HEADERS)((DWORD)pDH + pDH->e_lfanew);
  //IMAGE_File Header�ṹָ��
  pFH = &pNtH->FileHeader;
  //IMAGE_Optional Header�ṹָ��
  pOH = &pNtH->OptionalHeader;

  //IMAGE_SECTION_TABLE�ṹ��ָ��3�з���
  pSH1 = IMAGE_FIRST_SECTION(pNtH);// IMAGE_FIRST_SECTION��
  pSH2 = (PIMAGE_SECTION_HEADER)((DWORD)pNtH + sizeof(IMAGE_NT_HEADERS));
  pSH3 = (PIMAGE_SECTION_HEADER)((DWORD)pDH + pOH->SizeOfHeaders);

  // ����ļ��Ƿ���һ����Ч��PE�ļ�
  // IMAGE_DOS_SIGNATURE ��ֵΪ4D5A
  // IMAGE_NT_SIGNATURE ��ֵΪPE00
  if (pDH->e_magic != IMAGE_DOS_SIGNATURE || pNtH->Signature != IMAGE_NT_SIGNATURE)
  {
    printf("Not valid PE file...");
    return -1;
  }

  // ����PSectionָ��ָ��ԭ�����еĵ�һ��Section��������һ���µ�Section�ṹ��secToAdd
  PIMAGE_SECTION_HEADER pSection = NULL;
  IMAGE_SECTION_HEADER secToAdd = { 0 };
  pSection = (PIMAGE_SECTION_HEADER)((BYTE*)pOH + pFH->SizeOfOptionalHeader);


  DWORD dwSectionNum = pFH->NumberOfSections;
  DWORD dwSectionAlign = pOH->SectionAlignment;
  DWORD dwFileAlign = pOH->FileAlignment;
  DWORD dwOEP = pOH->AddressOfEntryPoint;    // ����ִ����ڵ�ַ
  dwOEP = (DWORD)(pOH->ImageBase + dwOEP);   // ӳ����ʼ��ַ+ִ����ڵ�ַ

  // ��PSectionָ��ԭ���������һ��Section���������һ��Section�����������µ�Section
  pSection = pSection + dwSectionNum - 1;

  // ��������ӵ�section������
  strcpy((char *)secToAdd.Name, ".For");
  // ��������ӵ�section������ֵ�������һ��sectionȡֵ��ͬ
  secToAdd.Characteristics = pSection->Characteristics;

  // ��section��С����
  DWORD vsize = 0x234;
  secToAdd.Misc.VirtualSize = vsize;
  // ����֮ǰ�����Align�������ã��õ�����������section�ߴ��С����������ʵ�ʴ�СΪ0x234��������СΪ0x400
  secToAdd.SizeOfRawData = Align(secToAdd.Misc.VirtualSize, dwFileAlign);

  // ����Align�������õ������ڴ���봦���ĳߴ�
  // ��Section��RVA����ԭ�������һ��Section��RVA���ϸý����ڴ��е�ӳ��ߴ�
  secToAdd.VirtualAddress = pSection->VirtualAddress +
    Align(pSection->Misc.VirtualSize, dwSectionAlign);

  // ��Section��FA��ַ�������һ��Section��FA���ϸýڵ��ļ�����ߴ�
  secToAdd.PointerToRawData = pSection->PointerToRawData + pSection->SizeOfRawData;

  // pSectionָ��ԭ���������һ���ڱ����һ����д���µĽڱ�ṹ
  pSection++;
  //pSection->Characteristics = 0xE00000E0;
  secToAdd.Characteristics = 0xE00000E0;
  memcpy(pSection, &secToAdd, sizeof(IMAGE_SECTION_HEADER));

  // �������ӵ�section����Ϣ
  char cName[9];
  char cBuff[9];
  printf("\n�ڱ���ӳɹ����½ڱ����ϢΪ��\n");
  printf("\nName = %s", secToAdd.Name);
  //memset(cName, 0, sizeof(cName));
  //memcpy(cName, secToAdd.Name, 4);
  //puts(cName);

  printf("\nVirtualSize = %08lX", secToAdd.Misc.VirtualSize);
  //wsprintf(cBuff, "%08lX", secToAdd.Misc.VirtualSize);
  //puts(cBuff);

  printf("\nVirtualAddress = %08lX", secToAdd.VirtualAddress);
  /*wsprintf(cBuff, "%08lX", secToAdd.VirtualAddress);
  puts(cBuff);*/

  printf("\nSizeOfRawData = %08lX", secToAdd.SizeOfRawData);
  //wsprintf(cBuff, "%08lX", secToAdd.SizeOfRawData);
  //puts(cBuff);

  printf("\nPointerToRawData = %08lX", secToAdd.PointerToRawData);
  //wsprintf(cBuff, "%08lX", secToAdd.PointerToRawData);
  //puts(cBuff);
  printf("\n");

  // ����PE�ļ��нڱ������
  WORD dwSizeAdd = 0x1;
  pNtH->FileHeader.NumberOfSections += dwSizeAdd;
  //pFH->NumberOfSections += 1;

 // �޸ĳ����ӳ���С
  pOH->SizeOfImage = pOH->SizeOfImage + Align(secToAdd.Misc.VirtualSize, dwSectionAlign);

  // �޸��ļ���С
  BYTE bNum = '\x0';
  DWORD dwWritten = 0;
  ::SetFilePointer(hFile, 0, 0, FILE_END);
  ::WriteFile(hFile, &bNum, secToAdd.SizeOfRawData, &dwWritten, NULL);
  ::UnmapViewOfFile(ImageBase);
  ::CloseHandle(hMapping);
  ::CloseHandle(hFile);

  return 0;
}

TEST(TestCaseName1, TestName1) {
  std::wstring file;
  std::wcout << L"please input pe path:" << std::endl;
  std::wcin >> file;
  EXPECT_TRUE(GenConfusion(file.c_str()));
}