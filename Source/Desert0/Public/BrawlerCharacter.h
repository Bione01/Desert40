#pragma once

#include "CoreMinimal.h"
#include "GameCharacter.h"
#include "BrawlerCharacter.generated.h"

class ACell_Actor;

UCLASS()
class DESERT0_API ABrawlerCharacter : public AGameCharacter
{
    GENERATED_BODY()

public:
    ABrawlerCharacter();

    virtual void MoveToCell(ACell_Actor* DestinationCell) override;
};

