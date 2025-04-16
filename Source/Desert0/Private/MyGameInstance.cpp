#include "MyGameInstance.h"

void UMyGameInstance::SetHardMode(bool bHard)
{
    bIsHardMode = bHard;
    UE_LOG(LogTemp, Warning, TEXT("change difficulty: %s"), bHard ? TEXT("HARD") : TEXT("EASY"));
}
