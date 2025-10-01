#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/TextActor.h"

IMPLEMENT_CLASS(ATextActor, AActor)

ATextActor::ATextActor()
{
	TextComponent = CreateDefaultSubobject<UTextComponent>("UTextComponent");
	SetRootComponent(TextComponent);
}