#include "Components/SingularisPreviewComponent.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

USingularisPreviewComponent::USingularisPreviewComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = false;
}

void USingularisPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) 清理幽灵预览演员，避免世界内残留
	DestroyPreviewActor();

	Super::EndPlay(EndPlayReason);
}

const FTransform& USingularisPreviewComponent::GetPreviewTransform() const
{
	// 1) 通过预览演员实时获取
	if (const AActor* Actor = PreviewActor.Get())
		return Actor->GetActorTransform();

	// 2) 无有效预览演员时回退到缓存变换
	return PreviewTransform;
}

void USingularisPreviewComponent::SetPreviewActorClass(const TSubclassOf<AActor> NewActorClass)
{
	// 1) 幂等：演员类未变化时直接返回
	if (PreviewActorClass == NewActorClass) return;
	PreviewActorClass = NewActorClass;

	// 2) 非预览状态仅缓存，等待下次启动时按新类生成
	if (!IsPreviewing()) return;

	// 3) 预览进行中按新类重建幽灵实例，保持当前变换与合法性
	DestroyPreviewActor();
	SpawnPreviewActor();
}

void USingularisPreviewComponent::StartPreview()
{
	// 1) 幂等：已在预览状态时直接返回
	if (bIsPreviewing) return;
	bIsPreviewing = true;

	// 2) 生成幽灵预览演员
	SpawnPreviewActor();

	// 3) 广播预览启动事件
	OnPreviewStartedEvent.Broadcast();
}

void USingularisPreviewComponent::StopPreview()
{
	// 1) 幂等：非预览状态时直接返回
	if (!bIsPreviewing) return;
	bIsPreviewing = false;

	// 2) 销毁幽灵预览演员
	DestroyPreviewActor();

	// 3) 广播预览停止事件
	OnPreviewStoppedEvent.Broadcast();
}

void USingularisPreviewComponent::SetPreviewTransform(const FTransform& Transform)
{
	// 1) 缓存目标变换，供重建预览实例时恢复
	PreviewTransform = Transform;

	// 2) 零信任：无预览演员时忽略
	if (!PreviewActor.IsValid()) return;

	// 3) 应用目标变换
	PreviewActor->SetActorTransform(Transform);
}

void USingularisPreviewComponent::SetPreviewValidity(const bool bNewValidity)
{
	// 1) 幂等：合法性状态未变化时直接返回
	if (bIsValid == bNewValidity) return;
	bIsValid = bNewValidity;

	// 2) 刷新反馈材质
	ApplyValidityMaterial();

	// 3) 广播合法性变化事件
	OnPreviewValidityChangedEvent.Broadcast(bIsValid);
}

void USingularisPreviewComponent::SpawnPreviewActor()
{
	// 1) 零信任：预览演员类未配置或世界无效时直接返回
	if (!PreviewActorClass) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	// 2) 幽灵化生成：忽略碰撞，避免生成时被地形阻挡而失败
	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = World->SpawnActor<AActor>(PreviewActorClass, PreviewTransform, SpawnParameters);
	if (!IsValid(SpawnedActor)) return;
	PreviewActor = SpawnedActor;

	// 3) 幽灵化：关闭实时表现，仅保留纯视觉
	SpawnedActor->SetActorTickEnabled(false);

	PreviewPrimitives.Empty();

	TArray<UPrimitiveComponent*> Primitives;
	SpawnedActor->GetComponents(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!IsValid(Primitive)) continue;

		Primitive->SetSimulatePhysics(false);
		Primitive->SetCastShadow(false);
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Primitive->SetGenerateOverlapEvents(false);
		PreviewPrimitives.Add(Primitive);
	}

	// 4) 应用当前合法性材质
	ApplyValidityMaterial();
}

void USingularisPreviewComponent::DestroyPreviewActor()
{
	// 1) 清空图元缓存
	PreviewPrimitives.Empty();

	// 2) 摘除引用后销毁预览演员
	AActor* Actor = PreviewActor.Get();
	PreviewActor = nullptr;
	if (IsValid(Actor)) Actor->Destroy();
}

void USingularisPreviewComponent::ApplyValidityMaterial() const
{
	// 1) 零信任：预览演员无效时直接返回
	if (!PreviewActor.IsValid()) return;

	// 2) 按合法性选择材质，未配置时保持当前材质
	UMaterialInterface* Material = bIsValid ? ValidMaterial.Get() : InvalidMaterial.Get();
	if (!IsValid(Material)) return;

	// 3) 替换全部缓存图元的所有材质槽位
	for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : PreviewPrimitives)
	{
		if (!Primitive.IsValid()) continue;

		const int32 MaterialCount = Primitive->GetNumMaterials();
		for (auto Index = 0; Index < MaterialCount; ++Index)
			Primitive->SetMaterial(Index, Material);
	}
}
