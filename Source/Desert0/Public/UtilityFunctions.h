#pragma once

#include "CoreMinimal.h"

static inline FString ConvertToChessNotation(int32 Row, int32 Col)
{
    TCHAR Letter = 'A' + Col;
    int32 Number = Row + 1;
    return FString::Printf(TEXT("%c%d"), Letter, Number);
}
