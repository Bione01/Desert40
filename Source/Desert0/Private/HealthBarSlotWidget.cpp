#include "HealthBarSlotWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "GameCharacter.h"

UHealthBarSlotWidget::UHealthBarSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UHealthBarSlotWidget::Init(AGameCharacter* InCharacter)
{
    CharacterRef = InCharacter;

    if (!CharacterRef || !IconImage) return;

    if (CharacterRef->IsBrawler())
    {
        if (CharacterRef->bIsAIControlled && AIBrawlerIcon)
            IconImage->SetBrushFromTexture(AIBrawlerIcon);
        else if (!CharacterRef->bIsAIControlled && PlayerBrawlerIcon)
            IconImage->SetBrushFromTexture(PlayerBrawlerIcon);
    }
    else if (CharacterRef->IsSniper())
    {
        if (CharacterRef->bIsAIControlled && AISniperIcon)
            IconImage->SetBrushFromTexture(AISniperIcon);
        else if (!CharacterRef->bIsAIControlled && PlayerSniperIcon)
            IconImage->SetBrushFromTexture(PlayerSniperIcon);
    }
}

void UHealthBarSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateHealthBar();
}

void UHealthBarSlotWidget::UpdateHealthBar()
{
    if (!CharacterRef || !HealthBar) return;

    float MaxHP = (float)CharacterRef->MaxHealth;
    if (MaxHP <= 0.f) return;

    float Percent = FMath::Clamp((float)CharacterRef->Health / MaxHP, 0.f, 1.f);
    HealthBar->SetPercent(Percent);

    // Aggiorna testo
    if (HealthText)
    {
        int32 DisplayedHealth = FMath::Max(0, CharacterRef->Health);
        FString Text = FString::Printf(TEXT("%d / %d"), DisplayedHealth, CharacterRef->MaxHealth);
        HealthText->SetText(FText::FromString(Text));
    }

    if (CharacterRef->Health <= 0 && DeadIcon && IconImage)
    {
        IconImage->SetBrushFromTexture(DeadIcon);
    }
}



