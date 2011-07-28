/*
Generic Sizable Byte Array
Written by LanceVorgin
*/

#pragma once

class CByteArray {
public:
    CByteArray(){
        m_pbBuffer = NULL;
        m_uiSize = 0;
    }

    ~CByteArray(){
        if(m_pbBuffer)
            delete[] m_pbBuffer;
    }

    void Clear(){
        if(m_pbBuffer)
            delete[] m_pbBuffer;

        m_pbBuffer = NULL;

        m_uiSize = 0;
    }

    void CopyTo(BYTE* pbDest){
        if(m_pbBuffer)
            memcpy(pbDest, m_pbBuffer, m_uiSize);
    }

    void CopyFrom(BYTE* pbSrc, unsigned int uiSize){
        Clear();

        Add(pbSrc, uiSize);
    }

    BYTE* Copy(){
        if(!m_pbBuffer)
            return NULL;

        BYTE* pbBuffer = new BYTE[m_uiSize];

        CopyTo(pbBuffer);

        return pbBuffer;        
    }

    void Grow(unsigned int uiSize){
        BYTE* pbNewBuffer = new BYTE[m_uiSize + uiSize];

        if(m_pbBuffer){
            CopyTo(pbNewBuffer);

            delete[] m_pbBuffer;
        }

        m_pbBuffer = pbNewBuffer;

        m_uiSize += uiSize;
    }

    int SetEntry(int iIndex, void* pValue, unsigned int uiSize){
        if((iIndex < 0) || ((iIndex + uiSize) > m_uiSize))
            return -1;

        memcpy(&m_pbBuffer[iIndex], pValue, uiSize);

        return iIndex;
    }

    int Add(void* pValue, unsigned int uiSize){
        Grow(uiSize);

        return SetEntry(m_uiSize - uiSize, pValue, uiSize);
    }

    BYTE& operator[](int iIndex) const {
        return m_pbBuffer[iIndex];
    }

    BYTE* GetBuffer(){
        return m_pbBuffer;
    }

    unsigned int Size(){
        return m_uiSize;
    }

    int Peek(){
        return (int)m_uiSize;
    }

    int AddBYTE(BYTE bVal){         return Add(&bVal, 1);       }
    int AddWORD(WORD wVal){         return Add(&wVal, 2);       }
    int AddDWORD(DWORD dwVal){      return Add(&dwVal, 4);      }

    void operator+=(BYTE bVal){     AddBYTE(bVal);              }
    void operator+=(WORD wVal){     AddWORD(wVal);              }
    void operator+=(DWORD dwVal){   AddDWORD(dwVal);            }

    int operator+(BYTE bVal){       return AddBYTE(bVal);       }
    int operator+(WORD wVal){       return AddWORD(wVal);       }
    int operator+(DWORD dwVal){     return AddDWORD(dwVal);     }

    BYTE& BYTEAt(int iIndex){       return (BYTE&)m_pbBuffer[iIndex];   }
    WORD& WORDAt(int iIndex){       return (WORD&)m_pbBuffer[iIndex];   }
    DWORD& DWORDAt(int iIndex){     return (DWORD&)m_pbBuffer[iIndex];  }

private:
    BYTE* m_pbBuffer;

    unsigned int m_uiSize;
};
