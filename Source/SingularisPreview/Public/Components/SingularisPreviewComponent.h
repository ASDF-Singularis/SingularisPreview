#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisPreviewComponent.generated.h"

class AActor;
class UMaterialInterface;
class UPrimitiveComponent;

#pragma region 委托签名

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPreviewStartedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPreviewStoppedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPreviewValidityChangedSignature, bool, bIsValid);

#pragma endregion

/**
 * 引力奇点预览组件。
 *
 * 为即将落地生成的目标演员提供"幽灵"预览表现:按预览演员类动态生成一个
 * 关闭碰撞与物理的预览实例,由外部驱动其变换与合法性(合法/非法)反馈。
 * 纯客户端视觉层,不参与网络复制,亦不承载实际生成逻辑。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点预览组件")
)
class SINGULARISPREVIEW_API USingularisPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/**
	 * 合法状态材质。
	 *
	 * 应用于预览演员的全部图元槽位,通常为半透明绿色材质。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisPreview|引力奇点预览组件|参数",
		meta = (DisplayName = "合法材质")
	)
	TObjectPtr<UMaterialInterface> ValidMaterial = nullptr;

	/**
	 * 非法状态材质。
	 *
	 * 应用于预览演员的全部图元槽位,通常为半透明红色材质。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisPreview|引力奇点预览组件|参数",
		meta = (DisplayName = "非法材质")
	)
	TObjectPtr<UMaterialInterface> InvalidMaterial = nullptr;

#pragma endregion

#pragma region 事件分发器

	/** 预览启动时广播 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisPreview|引力奇点预览组件|事件分发器",
		meta = (DisplayName = "预览启动时触发")
	)
	FOnPreviewStartedSignature OnPreviewStarted{};

	/** 预览停止时广播 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisPreview|引力奇点预览组件|事件分发器",
		meta = (DisplayName = "预览停止时触发")
	)
	FOnPreviewStoppedSignature OnPreviewStopped{};

	/** 合法性状态变化时广播 */
	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisPreview|引力奇点预览组件|事件分发器",
		meta = (DisplayName = "合法性变化时触发")
	)
	FOnPreviewValidityChangedSignature OnPreviewValidityChanged{};

#pragma endregion

private:
#pragma region Internal Variable

	/** 当前幽灵预览演员实例,其有效性即预览状态 */
	TWeakObjectPtr<AActor> PreviewActor = nullptr;

	/** 预览演员上缓存的可替换材质的图元集合 */
	TArray<TWeakObjectPtr<UPrimitiveComponent>> PreviewPrimitives{};

	/** 当前预览演员类,由 SetPreviewActorClass 动态驱动 */
	TSubclassOf<AActor> PreviewActorClass = nullptr;

	/** 当前预览变换,重建预览实例时用于恢复 */
	FTransform PreviewTransform{};

	/** 当前合法性状态,由外部 SetPreviewValidity 驱动 */
	bool bIsValid = true;

#pragma endregion

public:
#pragma region Constructors

	USingularisPreviewComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

#pragma region State

	/**
	 * 当前是否处于预览状态。
	 *
	 * 由预览演员实例的有效性直接推导,无冗余状态。
	 *
	 * @return 存在有效的预览演员时返回 true。
	 */
	UFUNCTION(
		BlueprintPure,
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|State",
		meta = (DisplayName = "IsPreviewing")
	)
	bool IsPreviewing() const { return PreviewActor.IsValid(); }

	/**
	 * 获取当前预览演员实例。
	 *
	 * @return 预览演员,未在预览时返回 nullptr。
	 */
	UFUNCTION(
		BlueprintPure,
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|State",
		meta = (DisplayName = "GetPreviewActor")
	)
	AActor* GetPreviewActor() const { return PreviewActor.Get(); }

#pragma endregion

#pragma region API

	/**
	 * 动态设置预览演员类。幂等,类未变化时直接返回。
	 *
	 * 预览进行中调用将按新类重建幽灵实例,并保持当前变换与合法性状态。
	 *
	 * @param NewActorClass 新的预览演员类。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|API",
		meta = (DisplayName = "SetPreviewActorClass")
	)
	void SetPreviewActorClass(TSubclassOf<AActor> NewActorClass);

	/**
	 * 启动预览。幂等,已在预览状态时直接返回。
	 *
	 * 按当前预览演员类生成幽灵实例并应用当前合法性材质。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|API",
		meta = (DisplayName = "StartPreview")
	)
	void StartPreview();

	/**
	 * 停止预览。幂等,非预览状态时直接返回。
	 *
	 * 销毁幽灵实例并广播停止事件。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|API",
		meta = (DisplayName = "StopPreview")
	)
	void StopPreview();

	/**
	 * 更新预览变换(如射线命中点)。
	 *
	 * @param Transform 预览目标变换。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|API",
		meta = (DisplayName = "SetPreviewTransform")
	)
	void SetPreviewTransform(const FTransform& Transform);

	/**
	 * 更新预览合法性并刷新反馈材质。幂等,状态未变化时直接返回。
	 *
	 * @param bNewValidity 是否合法,合法使用 ValidMaterial,非法使用 InvalidMaterial。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisPreview|引力奇点预览组件|API",
		meta = (DisplayName = "SetPreviewValidity")
	)
	void SetPreviewValidity(bool bNewValidity);

#pragma endregion

private:
#pragma region Internal Function

	/**
	 * 生成幽灵预览演员并缓存可替换材质的图元。
	 * 生成后关闭碰撞、物理、阴影与实时表现,仅保留纯视觉。
	 */
	void SpawnPreviewActor();

	/**
	 * 销毁幽灵预览演员并清空缓存。
	 */
	void DestroyPreviewActor();

	/**
	 * 按当前合法性状态刷新预览演员全部图元的材质。
	 * 未配置对应材质时跳过替换,保持演员默认材质。
	 */
	void ApplyValidityMaterial() const;

#pragma endregion
};
