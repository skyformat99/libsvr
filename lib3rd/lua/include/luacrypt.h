#ifndef __LUACRYPT__H__
#define __LUACRYPT__H__

typedef enum SegmentType
{
    eNormal = 0,
    eCrypt,
}SegmentType;

#define MAX_CRYPT_SIZE (1024)
#define MAX_PATH 260
#define ERRNO_RET(f, e, r) if(e) {fclose(f); return(r);}
#define TRUE  1
#define FALSE 0
typedef int BOOL;

typedef struct SEGMENT
{
    SegmentType eType;
    char* pChunk;
    char* pCrypt;
    char* pBase64;
    long iCrypt;
    long iChunk;
    long iBase64;
    struct SEGMENT* pNext;
    struct SEGMENT* pLast;
}SEGMENT;

typedef struct LUAFILE
{
    long iSectionNum;
    long iLuaSize;
    SEGMENT SegmentHead;
    FILE* pFile;
}LUAFILE;

BOOL OpenCrypt(LUAFILE* pLF, const char *filename);
void CloseCrypt(LUAFILE* pLF);
BOOL ReadCrypt(LUAFILE* pLF, BOOL bEnCrypt);
void DoEnCrypt(LUAFILE* pLF);
void DoDeCrypt(LUAFILE* pLF);

#endif