这是一个对 UE5 中 [SmartObjects](https://dev.epicgames.com/documentation/unreal-engine/smart-objects-in-unreal-engine?application_version=5.5) 功能模块的 **用法演示** 及 **功能拓展**。通过该项目你能获知：

- SmartObject的基本交互流程（概念+实例）

- 如何实现 自由度更高的交互细节设置、交互行为表现

- 一些实践经验总结



-----------------------------------------------------------

- [SmartObjects交互流程简介](#smartobjects交互流程简介)
- [项目结构](#项目结构)
- [项目用法示例](#项目用法示例)
- [功能拓展](#功能拓展)
  - [拓展前因](#拓展前因)
  - [拓展内容](#拓展内容)

-----------------------------------------------------------



## SmartObjects交互流程简介

SO交互流程，核心由这几部分构成：

- SmartObjectSubsystem：智能对象子系统。囊括交互流程中的核心接口

- SmartObject/SO：即智能对象。其通过挂载 `USmartObjectComponent` 后即可向 SmartObjectSubsystem注册成为 智能对象
- 流程载体：通过 BehaviorTree或StateTree 以桥接交互流程
- 交互表现：通过SmartObjects模块内的 Definition、GameplayBehavior等 以进行具体交互表现（SO筛选、交互行为等）

> 各部分的细节介绍，也可移步 [AI-SmartObjects - SouthBegonia](https://github.com/SouthBegonia/UnrealWorld/blob/main/Doc_UE_GamePlay/Doc_UE_GamePlay_AI.md#smart-objects)

![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/20260530154545970.png)



![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/CrossLine_01.png)



## 项目结构

项目主要由2部分构成：

- MySmartObjectUtility插件：对 `SmartObjects`模块 进行的功能拓展（纯工具代码、无具体业务示例）
- Content项目业务：使用了 `MySmartObjectUtility`插件 进行的 SO交互用法演示（含业务示例场景、资源、配置等）

```
Demo_UE_SmartObject
├── Plugins
│   └── MySmartObjectUtility 插件根目录
│       ├── Utility_BT		行为树相关拓展目录
│       │   ├── BTTask_FindSmartObject.h
│       │   ├── BTTask_GetClaimedSmartObjectSlotTransform.h
│       │   └── BTTask_MoveAndUseSmartObject.h
│       │
│       ├── Utility_ST		状态树相关拓展目录
│       │   ├── STTask_FindSmartObject.h
│       │   ├── STTask_GetSOClaimedSlotTransform.h
│       │   └── STTask_MoveAndUseSmartObject.h
│       │
│       ├── Utility			工具库函数拓展目录
│       │   └── SmartObjectBlueprintFunctionLibraryEx.h
│       │
│       ├── Interface		接口拓展目录
│       │   ├── SmartObjectInteracteeInterface.h
│       │   └── SmartObjectInteractorInterface.h
│       │
│       └── Actors			交互流程设计对象拓展目录
│           ├── SmartObjectActorBase.h
│           ├── NPCAIController.h
│           └── NPCCharacter.h
│
└── Content			用法演示目录
    └── SO_Map		演示场景目录
    └── SO_Actos	SmartObjects对象相关目录
    └── SO_NPC		NPC对象相关目录
    └── SO_Configuration	SmartObjects模块涉及的配置目录
        └── SO_Definition	SmartObjectDefinition目录
        └── BT				行为树业务目录
        └── ST				状态树业务目录
```



![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/CrossLine_01.png)



## 项目用法示例

例如项目内 通过 BehaviorTree及StateTree 实现的 **相同功能的交互示例**：首先尝试SamrtObject交互（查找、移动、交互行为），交互成功、失败后进行随机位置的移动，后从头重复

![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/20260701161817748.gif)

↓↓↓ BT_SO_NPC：

![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/20260701160130740.png)

↓↓↓ ST_SO_NPC：

![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/20260701160220756.png)

其中的智能对象示例 BP_SO_Lean，其配置了Definition为 DA_SOD_Lean（其存在2个Slot，功能均为播放Montage）：

![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/20260701160957912.png)



![](https://southbegonia.oss-cn-chengdu.aliyuncs.com/Pic/CrossLine_01.png)



## 功能拓展

### 拓展前因

截止 UE 5.5版本，官方提供的SO相关的工具方法有：

- BehaviorTree
  - `UBTTask_FindAndUseGameplayBehaviorSmartObject : public UBTTaskNode`：寻找、移动至并使用SmartObject的 行为树任务节点
- AITask
  - `UAITask_UseGameplayBehaviorSmartObject : public UAITask`：
    - `static UAITask_UseGameplayBehaviorSmartObject* UseSmartObjectWithGameplayBehavior()`：根据SO的ClaimHandle 直接使用SmartObject的 AITask
    - `static UAITask_UseGameplayBehaviorSmartObject* MoveToAndUseSmartObjectWithGameplayBehavior()`：根据SO的ClaimHandle 移动至并使用SmartObject的 AITask

但上述工具方法存在一些问题：

- **移动逻辑固定** 且 **移动逻辑中的 GoalLocation固定为 SlotLocation**（源码位于 `UAITask_UseGameplayBehaviorSmartObject::Activate()`）。而实际应用场景中 Slot是可能包含 一个或多个Entrance的，且可能有需求 **需要移动到Entrance再对Slot进行交互的**（例如 驾驶汽车需要先移动至车门（Entrance），而后在交互的同时更新位置到驾驶座（Slot））
- 除了节点自身 **外界很难获取SO相关信息**（ClaimHandle、Slot）：例如 GameplayBehavior触发事件 `UGameplayBehavior::Trigger(AActor& Avatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner/* = nullptr*/)` 仅有这3个参数信息，我甚至无法得知Slot朝向以让Avatar进行旋转匹配

### 拓展内容

因此 **本项目将围绕上述问题进行扩展**：

|               扩展方向                |                           针对问题                           |                           具体内容                           |                           代码目录                           |
| :-----------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
| 细化拆分交互流程，以提供 更高的自由度 |  FindAndUseGameplayBehaviorSmartObject 这类节点功能过于融合  | 拆分为了 FindSmartObject、MoveAndUseSmartObject 类的节点 到BT/ST；同时将常用工具方法 综合到 `SmartObjectBlueprintFunctionLibraryEX` | [Utility](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Utility)、[Utility_BT](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Utility_BT)、[Utility_ST](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Utility_ST) |
|      交互双方 能获取更多交互信息      |       希望交互双方 能更方便的知晓 交互起始点、交互对象       | 实现了 `InteractorInterface`/`InteracteeInterface`，并接入到BT/ST的节点内，以让交互双方得知 InteractionBegin/InteractionEnd 事件 | [Interface](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Interface) |
|            外部获取SO信息             | 一次交互中 仅存在一份ClaimHandle，通过此数据我们就可获取绝大部分信息 | 在上述 InteractorInterface 接口内实现 Set/GetSOClaimHandle 方法，并让业务自行选择存取位置（例如BT存黑板键中，ST可存在Character中），则其他业务就可通过此接口获取数据（比如 GameplayBehavior内） | [Interface](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Interface) |
|              业务封装-SO              | 官方提供的SO基类（`AGenericSmartObject`）默认Hidden的，不好结合StaticMesh组装为SO（当然这不是必须的，只要SO最终能与StaticMesh坐标适配即可） | 封装一个SO基类（`ASmartObjectActorBase`），并接入上文的 `InteracteeInterface`，但未预设添加StaticMesh，可按需调整 | [Actors](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Actors) |
|          业务封装-Character           | 严格来讲一个Character是无需额外Comp就可进行SO交互的，但缺失某些Comp 可能会让交互表现单一 | 从`ACharacter`派生出`ANPCCharacter `并预设功能组件（`USmartObjectUserComponent`用以标识交互者；`UMotionWarpingComponent`用以处理交互位置；`UAbilitySystemComponent`用以处理动画的网络同步）；同时也接入上文的 `InteractorInterface`；以及设定了AI单位常用的配置参数（RVO/Movement/Rotation） | [Actors](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Actors) |
|         业务封装-AIController         |      避免各NPC需要差异化添加BT/ST组件 以走AILogic的问题      | 从`AAIController`派生出`ANPCAIController `并实现 BT/ST流程自选化 | [Actors](https://github.com/SouthBegonia/Demo_UE_SmartObject/tree/main/Plugins/MySmartObjectUtility/Source/MySmartObjectUtility/Public/Actors) |
