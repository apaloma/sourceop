/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BYTEBUF_H
#define BYTEBUF_H

#define NETWORK_ORDERED 1

#define ntohll(x) (((unsigned long long)(ntohl((unsigned int)(x & 0xFFFFFFFF))) << 32) \
    | (unsigned int)ntohl(((unsigned int)(x >> 32))))
#define htonll(x) (((unsigned long long)(htonl((unsigned int)(x & 0xFFFFFFFF))) << 32) \
    | (unsigned int)htonl(((unsigned int)(x >> 32))))

// ByteBuf class that keeps in mind that some architectures require word
// aligned access

class ByteBufRead
{
public:
    ByteBufRead(void *pData, int bytes);

    int ReadByte();
    unsigned short PeekUShort();
    unsigned short ReadUShort();
    short ReadShort();
    int ReadLong();
    unsigned int ReadULong();
    unsigned long long ReadULongLong();
    float ReadFloat();
    double ReadDouble();
    bool ReadString(char *pStr, int maxLen);
    bool ReadBytes(void *pOut, int nBytes);

    bool IsOverflowed() { return m_bOverflow; }

private:
    unsigned char *m_pData;
    int m_iMaxBytes;
    int m_iPos;

    bool m_bOverflow;
};

class ByteBufWrite
{
public:
    ByteBufWrite(void *pData, int maxLen);

    void WriteByte(int val);
    void WriteUShort(unsigned short val);
    void WriteShort(short val);
    void WriteLong(int val);
    void WriteULong(unsigned int val);
    void WriteULongLong(unsigned long long val);
    void WriteFloat(float val);
    void WriteDouble(double val);
    bool WriteString(const char *pStr);
    bool WriteBytes(const void *pBuf, int nBytes);

    bool IsOverflowed() { return m_bOverflow; }
    int GetNumBytesWritten() { return m_iPos; }
    unsigned char *GetData() { return (unsigned char *)m_pData; }

private:
    unsigned char *m_pData;
    int m_iMaxBytes;
    int m_iPos;

    bool m_bOverflow;
};

#endif
