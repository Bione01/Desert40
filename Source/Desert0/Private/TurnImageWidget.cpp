#include "TurnImageWidget.h"
#include "Components/Image.h"

void UTurnImageWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UTurnImageWidget::SetTurnImage(bool bIsPlayerTurn)
{
    if (Image_YourTurn && Image_EnemyTurn)
    {
        Image_YourTurn->SetVisibility(bIsPlayerTurn ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        Image_EnemyTurn->SetVisibility(bIsPlayerTurn ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
    }
}

void UTurnImageWidget::PlayTurnAnimation(bool bIsPlayerTurn)
{
    if (bIsPlayerTurn)
    {
        if (!bIsPlayerCurrentlyVisible)
        {
            // Nemico era visibile → chiudi nemico → mostra player
            if (Anim_SlideOutEnemy) PlayAnimation(Anim_SlideOutEnemy);
            if (Anim_SlideInPlayer) PlayAnimation(Anim_SlideInPlayer);
            bIsPlayerCurrentlyVisible = true;
        }
    }
    else
    {
        if (bIsPlayerCurrentlyVisible)
        {
            // Player era visibile → chiudi player → mostra nemico
            if (Anim_SlideOutPlayer) PlayAnimation(Anim_SlideOutPlayer);
            if (Anim_SlideInEnemy) PlayAnimation(Anim_SlideInEnemy);
            bIsPlayerCurrentlyVisible = false;
        }
    }
}
