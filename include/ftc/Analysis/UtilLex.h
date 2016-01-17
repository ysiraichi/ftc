#ifndef UTILLEX_H
#define UTILLEX_H

#define STDLEN 64

#include <string.h>

void copyToBuffer(char **Buffer, char **BufferPtr, 
    int *MaxSize, char *YYText) {
  int YYSize = strlen(YYText);
  int Len = (int)(*BufferPtr - *Buffer);
  if (*MaxSize < Len + YYSize) {
    *MaxSize += (YYSize - (*MaxSize - Len)) + STDLEN;
    *Buffer   = (char*) realloc(*Buffer, *MaxSize);
  }

  *BufferPtr = *Buffer + Len;
  if (YYSize > 1) strcpy(*BufferPtr, YYText);
  else *(BufferPtr[0]) = *YYText;

  *BufferPtr += YYSize;
}

void copyToBufferChar(char **Buffer, char **BufferPtr, 
    int *MaxSize, char Char) {
  char ASCIIChar[2];
  ASCIIChar[0] = Char;
  ASCIIChar[1] = '\0';
  copyToBuffer(Buffer, BufferPtr, 
      MaxSize, ASCIIChar);
}

#endif
