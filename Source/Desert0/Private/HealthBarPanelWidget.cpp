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

    UVerticalBoxSlot* Slot = TargetContainer->AddChildToVerticalBox(NewSlot);
    if (Slot)
    {
        Slot->SetPadding(FMargin(0.f, 10.f));
        Slot->SetSize(ESlateSizeRule::Automatic);
    }
}

