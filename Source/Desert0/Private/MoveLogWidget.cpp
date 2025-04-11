#include "MoveLogWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UMoveLogWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMoveLogWidget::AddMoveEntry(const FString& MoveText)
{
    if (!MoveLogScrollBox) return;

    UTextBlock* NewText = NewObject<UTextBlock>(MoveLogScrollBox);
    if (!NewText) return;

    NewText->SetText(FText::FromString(MoveText));

    // === FONT ===
    FSlateFontInfo FontInfo;
    FontInfo.FontObject = LoadObject<UObject>(nullptr, TEXT("/Script/Engine.Font'/Game/Text_Mat/UI_Turns/Cinzel-Bold_Font.Cinzel-Bold_Font'")); // grassetto se disponibile
    FontInfo.Size = 22; // leggermente più grande
    NewText->SetFont(FontInfo);

    // === COLORE ===
    if (MoveText.StartsWith("HP:"))
    {
        // verde scuro più intenso
        NewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.10f, 0.15f, 0.05f, 1.0f)));
    }
    else if (MoveText.StartsWith("AI:"))
    {
        // grigio più scuro
        NewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.0f)));
    }
    else
    {
        NewText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    // === Add e scrolla ===
    MoveLogScrollBox->AddChild(NewText);
    MoveLogScrollBox->ScrollToEnd();
}
