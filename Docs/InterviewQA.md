# Haoyue 引擎 —— 面试题深度分析

本文档针对游戏引擎面试中常见的四个核心技术问题，基于 Haoyue 引擎的实际实现进行深度分析。

---

## 问题一：引擎如何调用 C# 脚本系统？脚本系统如何访问场景中的对象？

### 一、引擎如何调用 C# 脚本

Haoyue 引擎通过**嵌入 Mono 运行时**来调用 C# 脚本，整个架构分为三个层次：

#### 1.1 Mono 运行时嵌入

```
引擎启动
  └── ScriptEngine::Init()
       ├── mono_set_assemblies_path("mono/lib")     ← 设置 Mono 库搜索路径
       ├── mono_jit_init("Haoyue")                  ← 创建根域（加载 mscorlib）
       └── mono_domain_create_appdomain(...)         ← 创建应用域（加载用户代码）
```

采用**双域架构**：根域持有系统程序集（mscorlib），应用域持有用户程序集。应用域可以随时卸载并重建——这是热重载的技术基础。

#### 1.2 双程序集架构

```
Haoyue-ScriptCore.dll（引擎 C# API 层）
  ├── 定义 Entity 基类、生命周期方法（OnCreate/OnUpdate/OnDestroy）
  ├── 定义 Input、Physics、TransformComponent 等引擎 API 类
  └── 声明所有 Internal Call 的外部方法

用户程序集（如 ExampleApp.dll）
  ├── 引用 Haoyue-ScriptCore
  ├── 继承 Entity 实现游戏脚本
  └── 调用 ScriptCore 中的 API
```

分离的好处：API 不随用户代码重载，用户代码重载只影响 AppAssembly，热重载范围可控。

#### 1.3 Internal Call 双向绑定机制

这是 C++ 调用 C# 和 C# 调用 C++ 的核心通道：

**C++ 侧注册**（ScriptEngineRegistry.cpp）：
```cpp
mono_add_internal_call("Haoyue.Input::IsKeyPressed_Native",
    Haoyue::Script::Haoyue_Input_IsKeyPressed);
```

**C# 侧声明**（ScriptCore.dll）：
```csharp
[MethodImpl(MethodImplOptions.InternalCall)]
public static extern bool IsKeyPressed_Native(KeyCode key);
```

引擎注册了 60+ 个 Internal Call 函数，覆盖 Input、Physics、Transform、Rendering 等所有子系统。类型映射规则：

| C# 类型 | C++ 类型 | 说明 |
|---------|----------|------|
| `bool` | `bool` | 直接传递 |
| `float` | `float` | 直接传递 |
| `ulong` | `uint64_t` | Entity ID 使用 |
| `ref Vector3` | `glm::vec3*` | 指针传递，避免跨边界内存布局问题 |
| `MonoArray*` | `MonoArray*` | 托管数组 |

#### 1.4 脚本生命周期

```
1. InitScriptEntity()      → 通过反射获取 MonoClass，缓存所有生命周期方法指针
2. InstantiateEntityClass() → mono_object_new 创建 C# 实例 → 传入 entityID
3. OnCreate()              → 首次调用 C# OnCreate()
4. OnUpdate(float ts)      → 每帧遍历 ScriptComponent，调用 C# OnUpdate()
5. OnDestroy()             → 实体销毁时调用 C# OnDestroy()
```

### 二、脚本如何访问场景中的对象

C# 脚本通过 **entityID（UUID）** 作为句柄来访问场景中的对象，而不是直接持有 C++ 对象指针。

#### 2.1 核心设计：entityID 作为句柄

```cpp
// 所有 Wrapper 函数都通过 entityID 来定位实体
void Haoyue_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
{
    Entity entity = LookupEntity(entityID);  // 从 EntityInstanceMap 查找
    *outTranslation = entity.GetComponent<TransformComponent>().Translation;
}
```

#### 2.2 三层索引结构 EntityInstanceMap

```cpp
// SceneID → EntityID → EntityInstanceData
using EntityInstanceMap = std::unordered_map<UUID,
    std::unordered_map<UUID, EntityInstanceData>>;
```

`EntityInstanceData` 包含：
- `EntityInstance`：持有 GCHandle（指向 C# MonoObject）和 EntityScriptClass 指针
- `ModuleFieldMap`：模块名 → 字段名 → PublicField 的映射

#### 2.3 运行时访问流程

C# 脚本访问场景对象的完整调用链：

```
C#: transform.Translation
  → GetTranslation_Native(EntityID, ref outTranslation)  [Internal Call]
    → Haoyue_TransformComponent_GetTranslation(entityID, &out)
      → EntityInstanceMap[sceneID][entityID] → 查找 Entity
        → entity.GetComponent<TransformComponent>().Translation
          → memcpy(out, component.Translation)
```

**为什么选择 entityID 而非直接传递 MonoObject 指针？**

1. **GC 安全**：Mono 的 GC 可能在任意时刻移动对象，直接存储指针会在 GC 后悬空
2. **ABI 稳定**：`uint64_t` 通过寄存器传递，跨边界开销最小
3. **无上下文依赖**：entityID 是自包含的，不需要额外传递场景上下文
4. **持久化兼容**：entityID 即 UUID，可序列化，场景保存/加载时保持一致

#### 2.4 ECS 集成：ScriptComponent

```cpp
struct ScriptComponent
{
    std::string ModuleName;  // "Namespace.ClassName"，如 "ExampleApp.PlayerController"
};
```

通过 ECS 的观察者模式自动同步：
```cpp
m_Registry.on_construct<ScriptComponent>().connect<&OnScriptComponentConstruct>();
m_Registry.on_destroy<ScriptComponent>().connect<&OnScriptComponentDestroy>();
```

当编辑器中添加/移除 ScriptComponent 时，ScriptEngine 自动收到通知，无需轮询。

---

## 问题二：引擎架构中 5 大模块是如何组织到一起的？

### 5 大模块概览

| 模块 | 底层技术 | 核心职责 |
|------|---------|---------|
| **渲染模块** | Vulkan | 3D/2D 渲染管线、PBR/风格化材质、级联阴影 |
| **ECS 模块** | EnTT | 实体-组件-系统架构，场景管理，序列化 |
| **脚本模块** | Mono (C#) | C# 脚本嵌入、Internal Call 绑定、热重载 |
| **物理模块** | PhysX + Box2D | 3D/2D 碰撞检测、刚体模拟 |
| **音频模块** | miniaudio | 3D 空间音频、双线程架构、声源管理 |

### 组织方式

#### 2.1 Scene —— 中央调度中心

Scene 是五大模块的**聚合点**，持有 `entt::registry` 作为所有实体和组件的数据容器：

```cpp
class Scene : public Asset
{
    entt::registry m_Registry;   // ECS 核心 —— 所有实体和组件存储于此
    EntityMap m_EntityIDMap;     // UUID → Entity 辅助索引
    // 场景级渲染属性（环境光照、天空盒）
    // 场景级物理世界引用
    // ...
};
```

每个模块通过**组件（Component）**挂载到实体上。组件是纯数据结构（POD），不含业务逻辑——逻辑在各模块的 System 中实现。

#### 2.2 初始化顺序

```cpp
Application::Init()
  ├── Renderer::Init()            // 1. 渲染（创建 Vulkan 设备、窗口、SwapChain）
  ├── ScriptEngine::Init()        // 2. 脚本（启动 Mono 运行时，加载程序集）
  ├── Physics::Init()             // 3. 物理（初始化 PhysX + Box2D）
  ├── Audio::MiniAudioEngine::Init()  // 4. 音频（启动音频线程）
  └── AssetManager::Init()        // 5. 资源管理（加载资源注册表）
```

初始化顺序有依赖关系：渲染必须最先（创建窗口），资源管理最后（需等待文件系统和渲染就绪）。

#### 2.3 每帧系统编排

Scene::OnUpdate() 按固定顺序驱动各系统——这个顺序经过精心设计：

```
Scene::OnUpdate(Timestep ts)
  │
  ├── 1. 2D 物理系统 (Box2D)
  │     └── box2DWorld->Step() → 将物理位置写回 TransformComponent
  │
  ├── 2. 脚本系统 (C# via Mono)
  │     └── 遍历 ScriptComponent → ScriptEngine::OnUpdateEntity() → C# OnUpdate()
  │
  ├── 3. Transform 后处理
  │     └── 计算 World Matrix + Up/Right/Forward 方向向量
  │
  ├── 4. 音频系统
  │     └── 收集声源位置 → SubmitSourceUpdateData() → 音频线程消费
  │
  └── 5. 3D 物理系统 (PhysX)
        └── Physics::Simulate(ts)
```

**顺序设计的核心逻辑**：

- **物理先于脚本**：脚本可能读取物理计算结果（如碰撞后的位置）
- **脚本在 Transform 之前**：脚本可能在 `OnUpdate()` 中修改 Transform
- **音频在末尾**：音频需要读取最终的 Transform 位置
- **渲染独立于 OnUpdate**：在 `OnRenderRuntime()` 中单独执行，通过 EnTT group 高效遍历

#### 2.4 模块间的数据交换

模块之间**不直接耦合**，而是通过 ECS 组件作为数据交换的中介：

```
物理引擎 ──写入──→ TransformComponent ←──读取── 渲染系统
                      ↑
              脚本系统 (通过 Internal Call 读写)
                      ↑
              音频系统 (读取位置做空间化)
```

具体数据流：

- **物理 → 渲染**：物理仿真后更新 `TransformComponent.Translation/Rotation`，渲染时读取
- **脚本 → 其他**：C# 通过 Internal Call 操作 C++ 组件数据
- **主线程 → 音频线程**：通过 `SubmitSourceUpdateData()` + swap 零拷贝交换
- **C# → C++**：通过 Internal Call（60+ 函数覆盖所有子系统）

#### 2.5 架构图

```
                         ┌─────────────────┐
                         │  Application    │
                         │  (主循环 + 窗口) │
                         └────────┬────────┘
                                  │
                    ┌─────────────┼─────────────┐
                    │             │             │
              ┌─────▼─────┐ ┌────▼────┐ ┌─────▼─────┐
              │   Scene   │ │  Editor │ │  Project  │
              │ (ECS容器)  │ │  界面   │ │   管理    │
              └─────┬─────┘ └─────────┘ └───────────┘
                    │
    ┌───────┬───────┼───────┬───────┬───────┐
    │       │       │       │       │       │
┌───▼──┐ ┌─▼──┐ ┌──▼──┐ ┌──▼──┐ ┌──▼──┐ ┌──▼──┐
│Render│ │ECS │ │Script│ │Phys │ │Audio│ │Asset│
│      │ │    │ │      │ │     │ │     │ │Mgr  │
│Vulkan│ │EnTT│ │Mono  │ │PhysX│ │mini │ │UUID │
│      │ │    │ │      │ │+Box2│ │audio│ │based│
└──┬───┘ └─┬──┘ └──┬───┘ └──┬──┘ └──┬──┘ └──┬──┘
   │       │       │       │       │       │
   └───────┴───────┴───────┴───────┴───────┘
                  │
         通过 ECS 组件交换数据
    (TransformComponent, MeshComponent, etc.)
```

---

## 问题三：C# 热更新如何实现？脚本换 C++ 可以实现吗？

### 一、C# 热更新的实现

Haoyue 利用 Mono 的 **AppDomain 卸载/重建** 机制实现 C# 脚本热更新。

#### 1.1 核心流程

```cpp
void ScriptEngine::ReloadAssembly(const std::string& path)
{
    // 1. 保存旧域中的字段值（用户编辑的数据不能丢）
    SaveOldFieldValues();

    // 2. 卸载旧域，创建新域
    mono_domain_unload(s_MonoDomain);              // 旧域完全卸载，所有 C# 对象被 GC
    s_MonoDomain = mono_domain_create_appdomain(...); // 创建全新应用域

    // 3. 在新域中重新加载程序集
    LoadAssembly("Haoyue-ScriptCore.dll");  // 引擎 API 层
    LoadAssembly("ExampleApp.dll");         // 用户代码层

    // 4. 重新注册所有 Internal Call（这是全局的，不会随 Domain 卸载而清除）
    ScriptEngineRegistry::RegisterAll();

    // 5. 对当前场景的所有实体重新调用 InitScriptEntity()
    //    此过程中保留旧字段值（通过 m_StoredValueBuffer）
    ReinitAllScriptEntities();
}
```

#### 1.2 关键技术点

**Domain 隔离**：
- Mono 的 AppDomain 支持完整的程序集卸载和重载
- 卸载 Domain 后，该域中所有 C# 对象被 GC 回收，static 变量自然清除
- 根域（持有 mscorlib）不受影响，避免重载系统程序集

**字段值保留**：
```cpp
// 重载前保存旧字段值
std::unordered_map<std::string, PublicField> oldFields;
for (auto& [fieldName, field] : fieldMap)
    oldFields.emplace(fieldName, std::move(field));

// 重载后恢复
if (oldFields.find(name) != oldFields.end())
    fieldMap.emplace(name, std::move(oldFields.at(name)));
```

编辑器修改的字段值存储在 `m_StoredValueBuffer`（C++ 侧缓冲区），重载后自动恢复。

**即时生效原理**：
- 热重载触发后的**下一帧**，Scene::OnUpdateRuntime() 遍历 ScriptComponent 时，自动调用新程序集中的 OnUpdate 方法
- 编辑器的 ImGui 界面也是每帧重新渲染，无需手动刷新

#### 1.3 方案对比

| 方案 | 实现复杂度 | 迭代速度 | 内存隔离 | Haoyue 选择 |
|------|-----------|---------|---------|-------------|
| **AppDomain 热卸载** | 中等 | 秒级 | 部分 | ✅ 采用 |
| 独立进程 | 极高 | 慢（进程启动） | 完全 | ❌ 太重 |
| 反射重新加载 | 低 | 快 | 差（内存泄漏） | ❌ 不可靠 |

### 二、C++ 能否实现热更新？如何实现？

**答案：可以，但实现难度远高于 C#。**

#### 2.1 方案一：动态链接库热替换（主流方案）

```cpp
// 将游戏逻辑编译为 .dll/.so
// 运行时动态加载和替换

// 加载
HMODULE handle = LoadLibrary("GameLogic.dll");
auto CreateGameObject = (IGameObject*(*)(void))GetProcAddress(handle, "CreateGameObject");

// 热更新：卸载旧 DLL，加载新 DLL
FreeLibrary(oldHandle);
HMODULE newHandle = LoadLibrary("GameLogic_new.dll");
// 重新获取函数指针...
```

**优点**：
- 游戏逻辑独立编译，引擎无需重新编译
- 修改后只需编译一个小 DLL（秒级）

**核心挑战**：

1. **虚函数表兼容性（最难解决）**：
   - C++ 的 vtable 布局没有标准化 ABI
   - 新旧 DLL 中同一个类的 vtable 布局可能不同
   - **解决**：使用 C 风格接口或纯虚接口（COM 模式），避免跨 DLL 边界的 C++ 对象

2. **内存布局一致性**：
   - 新旧 DLL 中的 struct/class 如果有任何字段增减，内存布局改变
   - **解决**：使用接口继承 + 工厂模式，数据访问通过 getter/setter

3. **静态/全局变量状态迁移**：
   - 卸载 DLL 后 static 变量丢失
   - **解决**：将状态数据存储在引擎侧（类似 m_StoredValueBuffer 的设计）

4. **内存分配器一致性**：
   - 旧 DLL 分配的内存不能在新 DLL 中释放（堆不匹配）
   - **解决**：统一使用引擎提供的分配器接口

#### 2.2 方案二：C++ 脚本化（业界参考）

许多商业引擎实际上使用脚本语言（Lua、C#、Python）而不用原生 C++ 做热更新：

| 引擎 | 热更新方案 | 理由 |
|------|-----------|------|
| Unreal Engine | Blueprint + C++ Live Coding | C++ Live Coding 仅限函数体修改，不能改头文件 |
| Unity | C# (IL2CPP 不支持 JIT) | 编辑器用 Mono JIT，发布版无热更 |
| Godot | GDScript / C# | 脚本语言天然支持 |
| 自研引擎 | Lua / C# | 动态语言最适合热更 |

#### 2.3 总结建议

C++ 热更新在技术上是可行的（通过 DLL 热替换 + C 接口层），但工程复杂度远高于 C#/Lua 等托管语言。核心矛盾在于：**C++ 是静态编译语言，缺乏运行时的类型信息和 GC，使得"安全地替换运行时代码"极为困难。**

在实际面试中，可以从以下几个角度展开：
1. 承认 C++ 热更新比 C# 困难，但可行
2. 说明核心方案：DLL 热替换 + COM 式接口
3. 指出主要挑战：vtable 兼容性、内存布局、状态迁移
4. 解释为什么大多数引擎选择脚本语言做热更新层

---

## 问题四：确定性资源句柄系统是什么？解决了什么问题？

### 一、什么是确定性资源句柄

Haoyue 引擎中，`AssetHandle` 就是 `UUID`（64 位随机生成的全局唯一标识符）：

```cpp
using AssetHandle = UUID;  // 64-bit globally unique identifier

class Asset : public RefCounted
{
public:
    AssetHandle Handle;     // 资源的唯一标识
    uint16_t Flags;         // 状态标记（Missing/Invalid）
    // ...
};
```

**"确定性"的含义**：资源的句柄在导入引擎时**一次确定**，此后**永不改变**。无论资源文件被移动、重命名，还是引擎重启，同一个资源的 Handle 始终保持不变。

### 二、系统架构

```
AssetManager
  ├── s_AssetRegistry: map<文件路径, AssetMetadata>   ← 持久化到 AssetRegistry.hzr
  └── s_LoadedAssets:  map<AssetHandle, Ref<Asset>>   ← 内存中的已加载资源

AssetMetadata
  ├── Handle: UUID         ← 确定性句柄，导入时分配
  ├── FilePath: string     ← 当前文件路径（可变）
  ├── Type: AssetType      ← 资源类型（Scene/Mesh/Texture/Audio...）
  └── IsDataLoaded: bool   ← 是否已加载到内存
```

**注册表持久化**（AssetRegistry.hzr，YAML 格式）：
```yaml
Resources:
  - Handle: 12345678901234567890
    FilePath: Resources/models/Player.fbx
    Type: MeshAsset
  - Handle: 98765432109876543210
    FilePath: Resources/textures/diffuse.png
    Type: Texture
```

### 三、解决了什么问题？

#### 问题 1：序列化引用断裂

**场景**：场景文件（.hsc）中引用了一个网格资源 `Player.fbx`。

**没有句柄系统时**：
```yaml
# 场景文件直接存储文件路径
MeshComponent:
  Mesh: Resources/models/Player.fbx   # 如果文件被移动，引用断裂！
```

**有了句柄系统后**：
```yaml
# 场景文件存储句柄
MeshComponent:
  MeshHandle: 12345678901234567890   # 无论文件怎么移动，句柄不变
```

引擎通过 `AssetManager::GetAsset<Mesh>(handle)` 加载，内部从 Registry 查找当前文件路径。

#### 问题 2：文件重命名/移动后丢失引用

当用户在编辑器中重命名或移动资源文件时：
```cpp
bool AssetManager::MoveAsset(AssetHandle assetHandle, const std::string& destinationPath)
{
    AssetMetadata assetInfo = GetMetadata(assetHandle);
    FileSystem::MoveFile(assetInfo.FilePath, destinationPath);

    // 更新 Registry 中的路径映射，但 Handle 不变
    s_AssetRegistry.erase(assetInfo.FilePath);
    assetInfo.FilePath = newPath;
    s_AssetRegistry[assetInfo.FilePath] = assetInfo;  // Handle 保持一致

    WriteRegistryToFile();  // 持久化更新
}
```

所有引用该资源的场景和组件**无需任何修改**，因为 Handle 没有变。

#### 问题 3：跨会话的一致性

**场景**：保存场景 → 关闭引擎 → 重启引擎 → 加载场景。

没有句柄系统时，如果使用内存指针作为引用，重启后指针全部失效。使用文件路径则面临路径变更问题。

UUID 作为句柄天然解决此问题：
- UUID 在资源导入时随机生成并持久化到 Registry
- 引擎重启后，从 Registry 文件恢复 UUID → 文件路径映射
- 场景反序列化时，通过 UUID 查找对应资源

#### 问题 4：资源生命周期管理

```cpp
template<typename T>
static Ref<T> GetAsset(AssetHandle assetHandle)
{
    auto& metadata = GetMetadata(assetHandle);

    Ref<Asset> asset = nullptr;
    if (!metadata.IsDataLoaded)
    {
        // 懒加载：首次访问时才加载到内存
        metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
        s_LoadedAssets[assetHandle] = asset;
    }
    else
    {
        // 命中缓存：直接返回已加载的资源
        asset = s_LoadedAssets[assetHandle];
    }
    return asset.As<T>();
}
```

资源使用 `Ref<T>`（引用计数智能指针），当没有任何引用时自动释放。Handle 作为索引键，实现了：
- **懒加载**：只在需要时加载
- **缓存复用**：同一资源多处以 Handle 引用，内存中只有一份
- **自动释放**：引用计数归零后释放 GPU/CPU 内存

#### 问题 5：跨模块引用的一致性

C# 脚本通过 API 操作渲染资源时：
```cpp
// C++ 侧：Ref<T> 作为不透明句柄传递给 C#
void* Haoyue_Texture2D_Constructor(uint32_t width, uint32_t height)
{
    auto result = Texture2D::Create(ImageFormat::RGBA, width, height);
    return new Ref<Texture2D>(result);  // 在堆上分配 Ref，指针传给 C#
}
```

```csharp
// C# 侧：通过 IntPtr 持有
public class Texture2D
{
    internal IntPtr m_Instance;  // 指向 C++ Ref<Texture2D>
    // ...
}
```

资源句柄系统确保了 C++ 和 C# 之间对同一个资源的引用是安全且一致的。

### 四、设计要点总结

| 设计决策 | 说明 |
|----------|------|
| **Handle = UUID** | 64 位，随机生成，全局唯一，碰撞概率可忽略 |
| **Registry 持久化** | YAML 格式存储，引擎重启时恢复映射 |
| **Handle 不可变** | 导入时分配，永不改变，文件移动只更新路径映射 |
| **文件路径为索引键** | Registry 以路径为 key，因为 Handle → 路径是 1:1 关系 |
| **懒加载 + 引用计数** | 按需加载，无引用时自动释放 |

**一句话总结**：确定性资源句柄系统用一个永不改变的 UUID 作为资源的"身份证号"，将资源的**逻辑身份**与**物理位置（文件路径）**解耦。文件可以移动、重命名，但只要 Handle 不变，所有引用自动保持有效。这是现代游戏引擎资源管理的标准实践。

---

## 附录：面试回答要点速查

### 问题一速查：引擎如何调用脚本 / 脚本如何访问对象
- **嵌入方式**：Mono 运行时嵌入 C++ 进程
- **互调机制**：Internal Call（`mono_add_internal_call`），60+ 函数
- **访问对象**：通过 entityID（UUID）→ EntityInstanceMap（三层 unordered_map）→ ECS Registry 查找组件
- **双程序集**：ScriptCore（引擎 API）+ 用户程序集（游戏逻辑）

### 问题二速查：5 大模块如何组织
- **中心**：Scene（持有 entt::registry）
- **连接方式**：ECS 组件是模块间数据交换的中介
- **初始化顺序**：渲染 → 脚本 → 物理 → 音频 → 资源管理
- **运行顺序**：2D物理 → 脚本 → Transform → 音频 → 3D物理 → 渲染

### 问题三速查：C# 热更新 / C++ 热更新
- **C# 方案**：Mono AppDomain 卸载 → 重建 → 重新加载程序集
- **字段保留**：m_StoredValueBuffer 缓冲区机制
- **C++ 可行吗**：可以，通过 DLL 热替换 + C 接口层
- **C++ 主要困难**：vtable 兼容性、内存布局、状态迁移、堆一致性

### 问题四速查：确定性资源句柄
- **本质**：AssetHandle = UUID (64-bit)
- **确定性**：导入时一次分配，永不改变
- **解决问题**：序列化引用断裂、文件移动后引用失效、跨会话一致性、懒加载与缓存
- **Registry**：YAML 文件持久化 Handle ↔ FilePath 映射
