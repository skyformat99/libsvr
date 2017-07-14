#include <openssl/des.h>
#include <string.h>
#include <ctype.h>
#include "luacrypt.h"

#define CUR_VERSION 0

static void Base64Encode(const char *pInput, int iInSize, char* pOutput)
{
    const unsigned char b64encode_table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefgh"
        "ijklmnopqrstuvwxyz0123456789+/";

    unsigned char cTmp[3]={0};
    int iDiv3Num = iInSize / 3;
    int iMod3Num = iInSize % 3;
    int iIndex = 0;
    int i = 0;

    for(i = 0; i < iDiv3Num; i++)
    {
        cTmp[0] = *pInput++;
        cTmp[1] = *pInput++;
        cTmp[2] = *pInput++;

        pOutput[iIndex++] = b64encode_table[cTmp[0] >> 2];
        pOutput[iIndex++] = b64encode_table[((cTmp[0] << 4) | (cTmp[1] >> 4)) & 0x3F];
        pOutput[iIndex++] = b64encode_table[((cTmp[1] << 2) | (cTmp[2] >> 6)) & 0x3F];
        pOutput[iIndex++] = b64encode_table[cTmp[2] & 0x3F];		
    }

    if(iMod3Num == 1)
    {
        cTmp[0] = *pInput++;
        pOutput[iIndex++] = b64encode_table[(cTmp[0] & 0xFC) >> 2];
        pOutput[iIndex++] = b64encode_table[((cTmp[0] & 0x03) << 4)];
        pOutput[iIndex++] = '=';
        pOutput[iIndex++] = '=';
    }
    else if(iMod3Num == 2)
    {
        cTmp[0] = *pInput++;
        cTmp[1] = *pInput++;
        pOutput[iIndex++] = b64encode_table[(cTmp[0] & 0xFC) >> 2];
        pOutput[iIndex++] = b64encode_table[((cTmp[0] & 0x03) << 4) | ((cTmp[1] & 0xF0) >> 4)];
        pOutput[iIndex++] = b64encode_table[((cTmp[1] & 0x0F) << 2)];
        pOutput[iIndex++] =  '=';
    }
}

static void Base64Decode(const char *pInput, int iInSize, char* pOutput)
{
    const unsigned char b64decode_table[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,62, 0, 0, 0,63,
        52,53,54,55,56,57,58,59,60,61, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25, 0, 0, 0, 0, 0,
        0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51
    };

    int nValue;
    int i = 0;
    int iIndex = 0;

    while (i < iInSize)
    {
        nValue = (int)(b64decode_table[*pInput++] << 18);
        nValue += (int)(b64decode_table[*pInput++] << 12);
        pOutput[iIndex++] = (nValue & 0x00FF0000) >> 16;

        if (*pInput != '=')
        {
            nValue += (int)(b64decode_table[*pInput++] << 6);
            pOutput[iIndex++] = (nValue & 0x0000FF00) >> 8;

            if (*pInput != '=')
            {
                nValue += (int)(b64decode_table[*pInput++]);
                pOutput[iIndex++] = nValue & 0x000000FF;
            }
        }
        i += 4;
    }
}

static void NcbcEnCrypt0(const char* pInput, int iInSize, char* pOutput, int* pOutSize)
{
    int i = 0, j = 0, k = 0, t = 0;
    char* pIn = (char*)pInput;
    char* pOut = pOutput;
    char szInput[MAX_CRYPT_SIZE + 1];
    char szOutput[MAX_CRYPT_SIZE + 1];

    DES_key_schedule key_schedule;
    DES_cblock ivec;
    const_DES_cblock key[1];
    const char keystring[] = "?d z?!2€?+<Iû}½ð?EP'??„ªAñ[ü{~NöB??€æ]8?_";
    DES_string_to_key(keystring, key);
    DES_set_key_checked(key, &key_schedule);

    i = iInSize / MAX_CRYPT_SIZE;
    j = iInSize % MAX_CRYPT_SIZE;

    *(int*)(pOut) = 0;  //°æ±¾
    pOut += sizeof(int);
    k += sizeof(int);

    *(int*)(pOut) = j; //½áÎ²³¤¶È
    pOut += sizeof(int);
    k += sizeof(int);

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    for (; i > 0; --i)
    {
        memcpy(szInput, pIn, MAX_CRYPT_SIZE);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            MAX_CRYPT_SIZE, &key_schedule, &ivec, DES_ENCRYPT);
       
        memcpy(pOut, szOutput, MAX_CRYPT_SIZE);

        pIn += MAX_CRYPT_SIZE;
        pOut += MAX_CRYPT_SIZE;
        k += MAX_CRYPT_SIZE;
    }

    if (j)
    { 
        memset(szInput, 0, sizeof(szInput));
        memset(szOutput, 0, sizeof(szOutput));

        t = (j + 7) / 8 * 8;

        memcpy(szInput, pIn, j);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            j, &key_schedule, &ivec, DES_ENCRYPT);

        memcpy(pOut, szOutput, t);
        k += t;
    }

    *pOutSize = k;
}

static void NcbcDeCrypt0(const char* pInput, int iInSize, char* pOutput, int* pOutSize)
{
    int i = 0, j = 0, k = 0, t = 0;
    char* pIn = (char*)pInput;
    char* pOut = pOutput;
    char szInput[MAX_CRYPT_SIZE + 1];
    char szOutput[MAX_CRYPT_SIZE + 1];

    DES_key_schedule key_schedule;
    DES_cblock ivec;
    const_DES_cblock key[1];
    const char keystring[] = "?d z?!2€?+<Iû}½ð?EP'??„ªAñ[ü{~NöB??€æ]8?_";
    DES_string_to_key(keystring, key);
    DES_set_key_checked(key, &key_schedule);

    i = iInSize / MAX_CRYPT_SIZE;
    j = iInSize % MAX_CRYPT_SIZE;

    if (0 != *(int*)(pIn)) //Ð£Ñé°æ±¾
    {
        printf("%s:·Ç·¨µÄ¼ÓÃÜ°æ±¾!\n", __FUNCTION__);
        return;
    }

    pIn += sizeof(int);

    t = *(int*)(pIn);
    pIn += sizeof(int);

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    if (((t + 7) / 8 * 8) >= MAX_CRYPT_SIZE - 8)
    {
        --i;
        j = (t + 7) / 8 * 8;
    }

    for (; i > 0; --i)
    {        
        memcpy(szInput, pIn, MAX_CRYPT_SIZE);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            MAX_CRYPT_SIZE, &key_schedule, &ivec, DES_DECRYPT);
        
        memcpy(pOut, szOutput, MAX_CRYPT_SIZE);

        pIn += MAX_CRYPT_SIZE;
        pOut += MAX_CRYPT_SIZE;
        k += MAX_CRYPT_SIZE;        
    }

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    memcpy(szInput, pIn, j);

    memset((char*)&ivec, 0, sizeof(ivec));
    DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
        j, &key_schedule, &ivec, DES_DECRYPT);

    memcpy(pOut, szOutput, t);
    k += t;

    *pOutSize = k;
}


static void NcbcEnCrypt1(const char* pInput, int iInSize, char* pOutput, int* pOutSize)
{
    int i = 0, j = 0, k = 0, t = 0;
    char* pIn = (char*)pInput;
    char* pOut = pOutput;
    char szInput[MAX_CRYPT_SIZE + 1];
    char szOutput[MAX_CRYPT_SIZE + 1];

    DES_key_schedule key_schedule;
    DES_cblock ivec;
    const_DES_cblock key[1];
    const char keystring[] = "EP'?d z?!2€?+<I?„ªAñ[û}½ð??ü{~NöB€??æ]8?_";
    DES_string_to_key(keystring, key);
    DES_set_key_checked(key, &key_schedule);

    i = iInSize / MAX_CRYPT_SIZE;
    j = iInSize % MAX_CRYPT_SIZE;

    *(int*)(pOut) = 1;  //°æ±¾
    pOut += sizeof(int);
    k += sizeof(int);

    *(int*)(pOut) = j; //½áÎ²³¤¶È
    pOut += sizeof(int);
    k += sizeof(int);

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    for (; i > 0; --i)
    {
        memcpy(szInput, pIn, MAX_CRYPT_SIZE);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            MAX_CRYPT_SIZE, &key_schedule, &ivec, DES_ENCRYPT);

        memcpy(pOut, szOutput, MAX_CRYPT_SIZE);

        pIn += MAX_CRYPT_SIZE;
        pOut += MAX_CRYPT_SIZE;
        k += MAX_CRYPT_SIZE;
    }

    if (j)
    { 
        memset(szInput, 0, sizeof(szInput));
        memset(szOutput, 0, sizeof(szOutput));

        t = (j + 7) / 8 * 8;

        memcpy(szInput, pIn, j);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            j, &key_schedule, &ivec, DES_ENCRYPT);

        memcpy(pOut, szOutput, t);
        k += t;
    }

    *pOutSize = k;
}

static void NcbcDeCrypt1(const char* pInput, int iInSize, char* pOutput, int* pOutSize)
{
    int i = 0, j = 0, k = 0, t = 0;
    char* pIn = (char*)pInput;
    char* pOut = pOutput;
    char szInput[MAX_CRYPT_SIZE + 1];
    char szOutput[MAX_CRYPT_SIZE + 1];

    DES_key_schedule key_schedule;
    DES_cblock ivec;
    const_DES_cblock key[1];
    const char keystring[] = "EP'?d z?!2€?+<I?„ªAñ[û}½ð??ü{~NöB€??æ]8?_";
    DES_string_to_key(keystring, key);
    DES_set_key_checked(key, &key_schedule);

    i = iInSize / MAX_CRYPT_SIZE;
    j = iInSize % MAX_CRYPT_SIZE;

    if (1 != *(int*)(pIn)) //Ð£Ñé°æ±¾
    {
        printf("%s:·Ç·¨µÄ¼ÓÃÜ°æ±¾!\n", __FUNCTION__);
        return;
    }

    pIn += sizeof(int);

    t = *(int*)(pIn);
    pIn += sizeof(int);

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    if (((t + 7) / 8 * 8) >= MAX_CRYPT_SIZE - 8)
    {
        --i;
        j = (t + 7) / 8 * 8;
    }

    for (; i > 0; --i)
    {        
        memcpy(szInput, pIn, MAX_CRYPT_SIZE);

        memset((char*)&ivec, 0, sizeof(ivec));
        DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
            MAX_CRYPT_SIZE, &key_schedule, &ivec, DES_DECRYPT);

        memcpy(pOut, szOutput, MAX_CRYPT_SIZE);

        pIn += MAX_CRYPT_SIZE;
        pOut += MAX_CRYPT_SIZE;
        k += MAX_CRYPT_SIZE;        
    }

    memset(szInput, 0, sizeof(szInput));
    memset(szOutput, 0, sizeof(szOutput));

    memcpy(szInput, pIn, j);

    memset((char*)&ivec, 0, sizeof(ivec));
    DES_ncbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, 
        j, &key_schedule, &ivec, DES_DECRYPT);

    memcpy(pOut, szOutput, t);
    k += t;

    *pOutSize = k;
}

typedef void pfnNcbc(const char*, int, char*, int*);

typedef struct FNCRYPT
{
    pfnNcbc* m_pEnNcbc;
    pfnNcbc* m_pDeNcbc;
}FNCRYPT;

static FNCRYPT g_fnCrypt[] = 
{
    {
        NcbcEnCrypt0,
        NcbcDeCrypt0,
    },

    {
        NcbcEnCrypt1,
        NcbcDeCrypt1,
    },
};

static void Trim(const char* pBuff, size_t iInSize, char** pStart, char** pEnd)
{
    char *poss = (char*)pBuff;
    char *pose = poss + iInSize - 1;

    while ((poss < pose) && isspace(*poss)) ++poss;
    while ((pose > poss) && isspace(*pose)) --pose;

    *pStart = poss;
    *pEnd = pose;
}

BOOL OpenCrypt(LUAFILE* pLF, const char* filename)
{
    if ((NULL == pLF) || (NULL == filename))
        return FALSE;

    pLF->iSectionNum = 0;
    memset(&pLF->SegmentHead, 0, sizeof(SEGMENT));

    pLF->pFile = fopen(filename, "rb");
    if (!pLF->pFile) return FALSE;

    ERRNO_RET(pLF->pFile, fseek(pLF->pFile, 0, SEEK_END), FALSE);
    pLF->iLuaSize = ftell(pLF->pFile);
    ERRNO_RET(pLF->pFile, fseek(pLF->pFile, 0, SEEK_SET), FALSE);

    return TRUE;
}

void CloseCrypt(LUAFILE* pLF)
{
    SEGMENT* pSegment = NULL;
    SEGMENT* pTemp = NULL;

    if (NULL == pLF) return;
    
    if (pLF->pFile)
    {
        fclose(pLF->pFile);
        pLF->pFile = NULL;
    }
    
    pLF->iSectionNum = 0;
    pLF->iLuaSize = 0;

    pSegment = pLF->SegmentHead.pNext;
    pTemp = NULL;
    while (pSegment)
    {
        free(pSegment->pBase64);
        pSegment->pBase64 = NULL;

        free(pSegment->pChunk);
        pSegment->pChunk = NULL;

        free(pSegment->pCrypt);
        pSegment->pCrypt = NULL;

        pTemp = pSegment ->pNext;

        free(pSegment);

        pSegment = pTemp;
    }

    memset(&pLF->SegmentHead, 0, sizeof(SEGMENT));
}

static BOOL NewEnCryptSegment(LUAFILE* pLF, size_t iBuff)
{
    SEGMENT* pSegment = NULL;

    pSegment = (SEGMENT*)malloc(sizeof(SEGMENT));
    if (NULL == pSegment)
        return FALSE;

    memset(pSegment, 0, sizeof(SEGMENT));

    pSegment->iChunk = iBuff;
    pSegment->pChunk = (char*)malloc(iBuff + 1);
    if (!pSegment->pChunk)
        return FALSE;

    memset(pSegment->pChunk, 0, iBuff);

    pSegment->iCrypt = 0;
    pSegment->pCrypt = (char*)malloc(iBuff * 2 + 1);
    if (!pSegment->pCrypt)
        return FALSE;

    memset(pSegment->pCrypt, 0, iBuff * 2 + 1);

    pSegment->iBase64 = 0;
    pSegment->pBase64 = (char*)malloc(iBuff * 2 + 1);
    if (!pSegment->pBase64)
        return FALSE;

    memset(pSegment->pBase64, 0, iBuff * 2 + 1);

    if (NULL == pLF->SegmentHead.pNext)
        pLF->SegmentHead.pNext = pSegment;

    if (pLF->SegmentHead.pLast)
        pLF->SegmentHead.pLast->pNext = pSegment;

    pLF->SegmentHead.pLast = pSegment;

    if (0 == pLF->iSectionNum % 2)
        pSegment->eType = eCrypt;

    return TRUE;
}

static BOOL NewDeCryptSegment(LUAFILE* pLF, size_t iBuff)
{
    SEGMENT* pSegment = NULL;

    pSegment = (SEGMENT*)malloc(sizeof(SEGMENT));
    if (NULL == pSegment)
        return FALSE;

    memset(pSegment, 0, sizeof(SEGMENT));

    pSegment->iChunk = iBuff;
    pSegment->pChunk = (char*)malloc(iBuff + 1);
    if (!pSegment->pChunk)
        return FALSE;

    memset(pSegment->pChunk, 0, iBuff);

    pSegment->iCrypt = 0;
    pSegment->pCrypt = (char*)malloc(iBuff * 2 + 1);
    if (!pSegment->pCrypt)
        return FALSE;

    memset(pSegment->pCrypt, 0, iBuff * 2 + 1);

    pSegment->iBase64 = 0;
    pSegment->pBase64 = (char*)malloc(iBuff * 2 + 1);
    if (!pSegment->pBase64)
        return FALSE;

    memset(pSegment->pBase64, 0, iBuff * 2 + 1);

    if (NULL == pLF->SegmentHead.pNext)
        pLF->SegmentHead.pNext = pSegment;

    if (pLF->SegmentHead.pLast)
        pLF->SegmentHead.pLast->pNext = pSegment;

    pLF->SegmentHead.pLast = pSegment;

    if (0 == pLF->iSectionNum % 2)
        pSegment->eType = eCrypt;

    return TRUE;
}

static BOOL NewSegment(LUAFILE* pLF, size_t iBuff, BOOL bEnCrypt)
{
    if (bEnCrypt)
        return NewEnCryptSegment(pLF, iBuff);
    else
        return NewDeCryptSegment(pLF, iBuff);
}

BOOL ReadCrypt(LUAFILE* pLF, BOOL bEnCrypt)
{
    char* pStart = NULL;
    char* pFind = NULL;

    static const char* CRY_SEGMENT_FLAG = "--###CRYPT_SECTION###";

    char* pCode = (char*)malloc(pLF->iLuaSize + 1);
    if (!pCode) return FALSE;

    memset(pCode, 0, pLF->iLuaSize + 1);
    fread(pCode, pLF->iLuaSize, 1, pLF->pFile);

    pStart = pCode;
    while (pFind = strstr(pStart, CRY_SEGMENT_FLAG))
    {
        pLF->iSectionNum++;

        if (!NewSegment(pLF, pFind - pStart, !bEnCrypt))
        {
            free(pCode);
            return FALSE;
        }

        pFind[0] = 0;
        strcpy(pLF->SegmentHead.pLast->pChunk, pStart);
        pStart = pFind + strlen(CRY_SEGMENT_FLAG);
    }

    if (pStart < pCode + pLF->iLuaSize)
    {
        pLF->iSectionNum++;
        if (!NewSegment(pLF, strlen(pStart), !bEnCrypt))
        {
            free(pCode);
            return FALSE;
        }

        strcpy(pLF->SegmentHead.pLast->pChunk, pStart);
    }

    free(pCode);
    return TRUE;
}

void DoEnCrypt(LUAFILE* pLF)
{
    SEGMENT* pSegment = pLF->SegmentHead.pNext;

    while (pSegment)
    {
        if (eCrypt == pSegment->eType)
        {
            g_fnCrypt[CUR_VERSION].m_pEnNcbc(pSegment->pChunk, pSegment->iChunk, pSegment->pCrypt, (int*)&pSegment->iCrypt);

            pSegment->iBase64 = ((pSegment->iCrypt % 3) > 0 ? (pSegment->iCrypt + 3 - (pSegment->iCrypt % 3)) : pSegment->iCrypt) * 8 / 6;
            Base64Encode(pSegment->pCrypt, pSegment->iCrypt, pSegment->pBase64);
        }

        pSegment = pSegment->pNext;
    }
}

void DoDeCrypt(LUAFILE* pLF)
{
    int iVersion = 0;
    char* pStart = NULL;
    char* pEnd = NULL;
    SEGMENT* pSegment = pLF->SegmentHead.pNext;

    while (pSegment)
    {
        if (eCrypt == pSegment->eType)
        {
            Trim(pSegment->pChunk, pSegment->iChunk, &pStart, &pEnd);
            pSegment->iChunk = pEnd - pStart + 1;

            Base64Decode(pStart, pSegment->iChunk, pSegment->pBase64);

            pSegment->iBase64 = pSegment->iChunk * 6 / 8;

            iVersion = *(int*)(pSegment->pBase64);
            if ((iVersion < 0) || (iVersion >= sizeof(g_fnCrypt) / sizeof(FNCRYPT)))
                return;

            g_fnCrypt[iVersion].m_pDeNcbc(pSegment->pBase64, pSegment->iBase64, pSegment->pCrypt, (int*)&pSegment->iCrypt);
        }

        pSegment = pSegment->pNext;
    }
}