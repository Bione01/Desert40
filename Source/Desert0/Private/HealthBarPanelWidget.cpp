#include "HealthBarPanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "HealthBarSlotWidget.h"
#include "GameCharacter.h"

void UHealthBarPanelWidget::AddHealthBarForCharacter(AGameCharacter* Character)
{
    if (!HealthBarSlotWidgetClass || !Character) return;

    UHealthBarSlotWidget* NewSlot = CreateWidget<UHealthBarSlotWidget>(GetWorld(), HealthBarSlotWidgetClass);
    if (!NewSlot) return;

    NewSlot->Init(Character);

    UVerticalBox* TargetContainer = Character->bIsAIControlled ? TopContainer : BottomContainer;
    if (!TargetContainer) return;

    UVerticalBoxSlot* NewBoxSlot = TargetContainer->AddChildToVerticalBox(NewSlot);
    if (NewBoxSlot)
    {
        NewBoxSlot->SetPadding(FMargin(0.f, 10.f));
        NewBoxSlot->SetSize(ESlateSizeRule::Automatic);
    }
}

