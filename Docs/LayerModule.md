# Haoyue 引擎 Layer 层级系统文档

## 目录

- [1. 概述](#1-概述)
- [2. Layer —— 抽象基类详解](#2-layer--抽象基类详解)
  - [2.1 接口定义](#21-接口定义)
  - [2.2 生命周期钩子](#22-生命周期钩子)
  - [2.3 Timestep 参数](#23-timestep-参数)
- [3. LayerStack —— 分层容器](#3-layerstack--分层容器)
  - [3.1 双区模型：Layer vs Overlay](#31-双区模型layer-vs-overlay)
  - [3.2 插入与移除机制](#32-插入与移除机制)
  - [3.3 迭代器设计](#33-迭代器设计)
- [4. 设计决策分析](#4-设计决策分析)
  - [4.1 为什么使用抽象基类而非接口/虚继承](#41-为什么使用抽象基类而非接口虚继承)
  - [4.2 为什么 OnEvent 遍历是反向的](#42-为什么-onevent-遍历是反向的)
  - [4.3 为什么 Layer 和 Overlay 使用分区而非双容器](#43-为什么-layer-和-overlay-使用分区而非双容器)
  - [4.4 为什么使用裸指针而非智能指针管理 Layer 生命周期](#44-为什么使用裸指针而非智能指针管理-layer-生命周期)
- [5. Layer 在引擎中的运行流程](#5-layer-在引擎中的运行流程)
  - [5.1 主循环中的 Update 阶段](#51-主循环中的-update-阶段)
  - [5.2 事件传播机制](#52-事件传播机制)
  - [5.3 ImGui 渲染阶段](#53-imgui-渲染阶段)
  - [5.4 完整帧时序图](#54-完整帧时序图)
- [6. 具体实现类](#6-具体实现类)
  - [6.1 ImGuiLayer —— UI 渲染层](#61-imguilayer--ui-渲染层)
  - [6.2 EditorLayer —— 编辑器层](#62-editorlayer--编辑器层)
  - [6.3 RuntimeLayer —— 运行时层](#63-runtimelayer--运行时层)
- [7. 如何添加自定义 Layer](#7-如何添加自定义-layer)
- [附录：类图](#附录类图)

---

## 1. 概述

Layer（层）是 Haoyue 引擎中最核心的架构模式之一。它提供了一种**纵向切分应用逻辑**的机制——每个 Layer 代表引擎的一个独立功能模块（如编辑器 UI、场景渲染、调试面板），通过统一的接口接入引擎主循环。

整个 Layer 系统由三个核心类构成：

| 类 | 文件路径 | 职责 |
|----|---------|------|
| `Layer` | [Core/Layer.h](Haoyue/src/Haoyue/Core/Layer.h) | 抽象基类，定义生命周期接口 |
| `LayerStack` | [Core/LayerStack.h](Haoyue/src/Haoyue/Core/LayerStack.h) | 有序容器，管理 Layer 的插入/移除/遍历 |
| `Application` | [Core/Application.h](Haoyue/src/Haoyue/Core/Application.h) | 引擎主循环，驱动所有 Layer 的 Update/Event/ImGui |

一个 Layer 从创建到销毁经历以下生命周期：

```
构造函数 → OnAttach() → [每帧: OnUpdate(ts) → OnImGuiRender()] → OnDetach() → 析构函数
```

事件（Event）在上述任意阶段都可以发生，由 Application 从 LayerStack 顶部向底部传播。

---

## 2. Layer —— 抽象基类详解

### 2.1 接口定义

```cpp
// Layer.h
class Layer
{
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(Timestep ts) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& event) {}

    inline const std::string& GetName() const { return m_DebugName; }
protected:
    std::string m_DebugName;
};
```

**关键设计特点：**

1. **所有虚函数都有默认空实现**。这意味着子类只需要 override 自己关心的钩子，不需要实现所有方法。这是一个典型的"非侵入式接口"模式。

2. **`m_DebugName` 是 protected 成员**。子类可以读取自己的名字（用于日志输出等场景），但外部只能通过 `GetName()` 访问。

3. **析构函数是虚函数**（`virtual ~Layer()`）。这保证了通过 `Layer*` 指针删除子类对象时，会正确调用子类的析构函数，是 C++ 多态安全的必要条件。

### 2.2 生命周期钩子

#### OnAttach / OnDetach

这两个钩子在 Layer 被加入/移出 LayerStack 时调用，**仅调用一次**。适合做资源的初始化和清理：

- **OnAttach**: 加载场景、创建渲染器、注册回调、分配 GPU 资源
- **OnDetach**: 释放资源、保存状态、注销回调

```cpp
// 典型使用：RuntimeLayer.cpp
void RuntimeLayer::OnAttach()
{
    OpenScene("Resources/scenes/levels/Physics2D-Game.hsc"); // 加载场景
    m_SceneRenderer = Ref<SceneRenderer>::Create(m_RuntimeScene, spec); // 创建渲染器
    OnScenePlay(); // 启动场景运行时
}
```

#### OnUpdate(Timestep ts)

**每帧调用**，是 Layer 的"心跳"。参数 `Timestep ts` 携带了上一帧到当前帧的时间间隔（单位为秒），用于实现帧率无关的逻辑更新。

#### OnImGuiRender()

**每帧调用**，专门用于 ImGui 的渲染。与 `OnUpdate` 分离的原因是 ImGui 的渲染必须在特定的 Begin/End 帧之间进行（见第 5.3 节）。

#### OnEvent(Event& event)

当窗口、输入等事件发生时调用。值得注意的是，这个函数接收的是**非 const 引用**，因为 Layer 可能需要修改 `event.Handled = true` 来阻止事件继续传播。

### 2.3 Timestep 参数

```cpp
// Timestep.h
class Timestep
{
public:
    Timestep() {}
    Timestep(float time);

    inline float GetSeconds() const { return m_Time; }
    inline float GetMilliseconds() const { return m_Time * 1000.0f; }

    operator float() { return m_Time; }  // 隐式转换为 float
private:
    float m_Time = 0.0f;
};
```

`Timestep` 是一个轻量级的**值类型**封装，它提供了以下优势：

- **隐式转换为 `float`**：可以直接将 `Timestep` 当作秒数使用，如 `speed * ts`（得益于 `operator float()`）
- **同时支持秒和毫秒**：`GetSeconds()` 和 `GetMilliseconds()` 两个访问器使代码意图清晰
- **帧率无关的运动**：典型的用法是 `position += velocity * ts`，无论帧率是 30fps 还是 144fps，物体移动速度保持一致

---

## 3. LayerStack —— 分层容器

### 3.1 双区模型：Layer vs Overlay

```cpp
// LayerStack.h
class LayerStack
{
public:
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
    std::vector<Layer*>::iterator end() { return m_Layers.end(); }
private:
    std::vector<Layer*> m_Layers;
    unsigned int m_LayerInsertIndex = 0;
};
```

LayerStack 维护一个**单一的 `std::vector<Layer*>`**，但通过 `m_LayerInsertIndex` 将其划分为两个逻辑区域：

```
m_Layers:
[ Layer 0, Layer 1, ..., Layer N-1  |  Overlay 0, Overlay 1, ..., Overlay M-1 ]
  ↑                                  ↑
  m_Layers.begin()                   m_LayerInsertIndex (指向第一个 Overlay)
```

| 特性 | Layer | Overlay |
|------|-------|---------|
| 插入位置 | 在所有 Layer 之后、所有 Overlay 之前 | 永远在 vector 末尾 |
| 事件接收顺序 | 后进先处理（从顶部向底部传播） | 同 Layer，但因为插入在末尾，所以最先收到事件 |
| 典型用途 | 业务逻辑层（编辑器层、运行时场景层） | UI 覆盖层（ImGui 调试面板） |
| 移除时的行为 | `m_LayerInsertIndex--`（维护分区边界） | 仅 erase，`m_LayerInsertIndex` 不变 |

**设计意图**：Overlay 是"浮在 Layer 之上"的 UI 层。在事件传播中，Overlay 先于 Layer 处理事件——因为 Overlay（如 ImGui 面板）通常需要"吞掉"鼠标/键盘事件，防止它们穿透到底层的场景视图中。

### 3.2 插入与移除机制

```cpp
void LayerStack::PushLayer(Layer* layer)
{
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;  // 维护分区边界向右移动
}

void LayerStack::PushOverlay(Layer* overlay)
{
    m_Layers.emplace_back(overlay);  // 直接追加到末尾，m_LayerInsertIndex 不变
}

void LayerStack::PopLayer(Layer* layer)
{
    auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
    if (it != m_Layers.end())
    {
        m_Layers.erase(it);
        m_LayerInsertIndex--;  // 维护分区边界向左移动
    }
}

void LayerStack::PopOverlay(Layer* overlay)
{
    auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
    if (it != m_Layers.end())
        m_Layers.erase(it);  // m_LayerInsertIndex 不变
}
```

关键细节：
- **`PushLayer` 使用 `emplace` 而非 `push_back`**，因为 Layer 必须插入在 Overlay 区域之前
- **`PopLayer` 会递减 `m_LayerInsertIndex`**，因为一个 Layer 被移除后，Layer 和 Overlay 的分界线向左移动了一位
- **`PopOverlay` 不改变 `m_LayerInsertIndex`**，因为 Overlay 都在分界线右边，移除一个不影响边界位置

### 3.3 迭代器设计

LayerStack 暴露了标准的 `begin()` / `end()` 迭代器，这使得它可以被 C++ 范围 for 循环直接遍历：

```cpp
// Application.cpp —— 主循环中的 Update
for (Layer* layer : m_LayerStack)
    layer->OnUpdate(m_TimeStep);
```

这种设计让 LayerStack 可以像标准容器一样使用，并且与 STL 算法兼容。

---

## 4. 设计决策分析

### 4.1 为什么使用抽象基类而非接口/虚继承

Haoyue 的 Layer 采用了**带有默认实现的抽象基类**，而非纯虚接口（所有方法 = 0）。

**对比分析：**

| 方案 | 优点 | 缺点 |
|------|------|------|
| 纯虚接口 | 强制子类实现所有方法，编译器检查 | 即使不关心某些钩子也要写空实现，代码冗余 |
| **默认实现基类（当前方案）** | 子类只覆写关心的方法，简洁灵活 | 不会强制覆写，可能遗漏关键方法 |

对于引擎的 Layer 系统，"简洁灵活"比"强制完整"更重要。一个典型的 Layer 可能只需要 `OnUpdate` 和 `OnImGuiRender`，其他钩子保持为空即可。如果使用纯虚接口，每个新 Layer 都需要写出五个空函数体，这是无意义的样板代码。

**此外，这也是著名的 NVI（Non-Virtual Interface）模式的一种近似。** 虽然当前实现没有使用 NVI（因为钩子直接是 public virtual），但它保留了将来重构为 NVI 的空间——可以在 Layer 基类中添加非虚的 public 方法，内部调用 private virtual 实现。

### 4.2 为什么 OnEvent 遍历是反向的

```cpp
// Application.cpp
void Application::OnEvent(Event& event)
{
    // ... 先处理 WindowResize 和 WindowClose ...

    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
    {
        (*--it)->OnEvent(event);  // 从后往前遍历！
        if (event.Handled)
            break;  // 事件被消费，停止传播
    }
}
```

这个**反向遍历**（从 LayerStack 末尾向开头）的设计理由：

```
LayerStack 排列顺序（从前往后）:
[Layer 0 (底层)] → [Layer 1 (中间)] → [Overlay 0 (顶层)]

事件传播方向（从后往前，即从上往下）:
Overlay 0（先处理）→ Layer 1 → Layer 0（后处理）
```

- **Overlay（UI 面板）在末尾**，应该优先处理事件——用户在 UI 面板上的点击不应穿透到场景
- **`event.Handled` 是短路机制**——一旦某个 Layer 将事件标记为已处理，传播立即停止
- 这遵循了 GUI 编程中的通用模式：**"最上层的元素最先消费事件"**

> 类比：Windows 的消息处理、浏览器 DOM 的事件捕获、Unity 的 UI 事件系统都采用了同样的"从上到下"传播策略。

### 4.3 为什么 Layer 和 Overlay 使用分区而非双容器

一个直观的设计可能是使用两个独立的 vector：

```cpp
// 直观但未被采用的方案
std::vector<Layer*> m_Layers;
std::vector<Layer*> m_Overlays;
```

**当前单容器 + 索引分区方案的优势：**

1. **遍历简单**：`for (Layer* layer : m_LayerStack)` 一次遍历所有层，不需要先遍历 `m_Layers` 再遍历 `m_Overlays`
2. **顺序确定**：所有层的相对顺序是确定的，由单个容器保证。双容器方案需要额外约定谁先谁后
3. **内存局部性**：所有 Layer 指针存储在连续内存中，遍历时的缓存命中率更高（尽管指针本身指向堆上的对象）

**单容器方案的代价**：
- `PopLayer` 需要 `std::find`（O(n) 线性查找），但这在 Layer 数量极少（通常 < 10 个）时不是问题
- 分区逻辑（`m_LayerInsertIndex` 的维护）增加了概念复杂度

### 4.4 为什么使用裸指针而非智能指针管理 Layer 生命周期

观察 `LayerStack` 的设计：

```cpp
std::vector<Layer*> m_Layers;  // 裸指针，非 std::vector<std::unique_ptr<Layer>>
```

Layer 的生命周期由 `Application` 显式管理：

```cpp
// Application 析构函数
Application::~Application()
{
    for (Layer* layer : m_LayerStack)
    {
        layer->OnDetach();
        delete layer;  // 显式 delete
    }
}
```

**使用裸指针的理由：**

1. **生命周期明确且集中**：Layer 的创建和销毁都发生在 Application 中，不存在所有权共享或转移的场景
2. **`OnDetach()` 必须在 delete 之前调用**：使用裸指针时，这个顺序是显式的、可控的。如果使用 `unique_ptr`，`OnDetach()` 的调用时机取决于 `unique_ptr` 析构的时机，而这不是总能被直观看到的
3. **与 ImGui 生态兼容**：ImGui 的很多 API 使用裸指针，使用裸指针减少了不必要的转换
4. **性能**：裸指针的传递和存储没有任何额外开销

> **注意**：这是一个在"明确所有权"的场景下刻意选择裸指针的设计决策，并不意味着引擎在所有场景下都回避智能指针。引擎中的其他模块（如 `Scene`、`Framebuffer`）广泛使用了 `Ref<T>`（即 `std::shared_ptr`）。

---

## 5. Layer 在引擎中的运行流程

### 5.1 主循环中的 Update 阶段

引擎的主循环在 `Application::Run()` 中：

```cpp
void Application::Run()
{
    OnInit();
    while (m_Running)
    {
        m_Window->ProcessEvents();               // 1. 处理窗口事件

        if (!m_Minimized)
        {
            Renderer::BeginFrame();               // 2. 开始渲染帧
            for (Layer* layer : m_LayerStack)     // 3. 顺序更新所有 Layer
                layer->OnUpdate(m_TimeStep);

            // 4. 提交 ImGui 渲染命令到渲染线程
            if (m_Specification.EnableImGui)
            {
                Renderer::Submit([app]() { app->RenderImGui(); });
                Renderer::Submit([=]() { m_ImGuiLayer->End(); });
            }
            Renderer::EndFrame();                  // 5. 结束渲染帧

            m_Window->GetSwapChain().BeginFrame();
            Renderer::WaitAndRender();             // 6. 等待 GPU 完成并呈现
            m_Window->SwapBuffers();
        }

        // 7. 计算帧时间
        float time = GetTime();
        m_TimeStep = time - m_LastFrameTime;
        m_LastFrameTime = time;
    }
    OnShutdown();
}
```

**Update 阶段的关键特征：**

- **顺序执行**：Layer 按照 LayerStack 中的顺序（Layer 在前，Overlay 在后）依次调用 `OnUpdate`。没有多线程并行——这是一个确定性的、可预测的执行顺序。
- **所有 Layer 都会收到 Update**：与事件传播不同，Update 不会被"短路"——每个 Layer 都需要更新自己的状态。
- **最小化时跳过**：当窗口被最小化（`m_Minimized == true`）时，整个 Update + Render 阶段被跳过，节省 CPU/GPU 资源。

### 5.2 事件传播机制

事件处理分为两个阶段：

```
阶段 1: Application 自身处理
  ├── WindowResizeEvent → OnWindowResize()
  └── WindowCloseEvent  → OnWindowClose()
       （这两个是 Application 级事件，不传给 Layer）

阶段 2: LayerStack 从后往前传播
  for (it = end; it != begin; )
      (*--it)->OnEvent(event)
      if (event.Handled) break;
```

**分层处理的典型场景**：

```
用户按下空格键
  → KeyPressedEvent 被创建
  → Application 检查：不是 Resize/Close 事件，进入阶段 2
  → Overlay 0 (ImGui 面板)：检查是否有 ImGui 输入框在焦点 → 如果是，event.Handled = true，事件停止
  → (如果 ImGui 未处理) Layer 0 (编辑器层)：检查快捷键绑定 → 触发相应操作
```

### 5.3 ImGui 渲染阶段

ImGui 的渲染有其特殊性——它必须在 `ImGui::Begin()` 和 `ImGui::End()` 调用对之间进行：

```cpp
// Application.cpp
void Application::RenderImGui()
{
    m_ImGuiLayer->Begin();              // ImGui::NewFrame() + 平台初始化

    // 音频调试面板
    ImGui::Begin("Audio Stats");
    // ... ImGui 控件 ...
    ImGui::End();

    for (Layer* layer : m_LayerStack)   // 所有 Layer 的 ImGui 渲染
        layer->OnImGuiRender();

    m_Profiler->Clear();
}

// 在主循环中，渲染器线程上执行
Renderer::Submit([app]() { app->RenderImGui(); });
Renderer::Submit([=]() { m_ImGuiLayer->End(); });  // ImGui::Render() + 平台清理
```

`OnImGuiRender()` 和 `OnUpdate()` 分离的设计原因：

1. **时机不同**：`OnUpdate` 在主线程执行，`OnImGuiRender` 被提交到渲染线程执行
2. **上下文不同**：`OnImGuiRender` 必须在 ImGui 的 Begin/End 帧之间调用
3. **关注点分离**：游戏逻辑（Update）和 UI 绘制（ImGuiRender）是两个独立的关注点

### 5.4 完整帧时序图

```
一帧的开始
│
├─ ProcessEvents()          ← GLFW 轮询窗口事件
│   └─ OnEvent() 触发       ← Application 分发事件到各 Layer
│
├─ BeginFrame()             ← 渲染器准备新帧
│
├─ for Layer in LayerStack:  ← 顺序遍历（正序）
│   └─ Layer::OnUpdate(ts)   ← 每个 Layer 更新业务逻辑
│         ├─ 物理模拟
│         ├─ 场景更新
│         ├─ C# 脚本执行
│         └─ ...
│
├─ Renderer::Submit → RenderImGui()  ← 提交到渲染线程
│   ├─ ImGuiLayer::Begin()
│   ├─ for Layer in LayerStack: OnImGuiRender()  ← 所有 Layer 绘制 UI
│   └─ ImGuiLayer::End()
│
├─ EndFrame()               ← 渲染器结束命令记录
│
├─ WaitAndRender()          ← 等待 GPU 执行完所有命令
├─ SwapBuffers()            ← 交换前后缓冲区，呈现画面
│
└─ 计算 Timestep            ← 为下一帧准备时间参数，一帧结束
```

---

## 6. 具体实现类

Haoyue 引擎中目前有三个 Layer 子类：

### 6.1 ImGuiLayer —— UI 渲染层

```cpp
// ImGuiLayer.h
class ImGuiLayer : public Layer
{
public:
    virtual void Begin() = 0;
    virtual void End() = 0;

    void SetDarkThemeColors();
    static ImGuiLayer* Create();
};
```

| 属性 | 说明 |
|------|------|
| 类型 | **Overlay**（通过 `Application::PushOverlay` 加入） |
| 职责 | 封装 ImGui 的初始化和帧管理（Begin/End），提供主题设置 |
| 为什么是抽象类 | `Begin()` / `End()` 是纯虚函数，具体实现在 `VulkanImGuiLayer` 中（平台相关代码），而 `ImGuiLayer` 作为公共头文件暴露给上层 |

**它是如何被加入的：**

```cpp
// Application 构造函数
if (m_Specification.EnableImGui)
{
    m_ImGuiLayer = ImGuiLayer::Create();
    PushOverlay(m_ImGuiLayer);  // 作为 Overlay 加入，保证最先收到事件
}
```

### 6.2 EditorLayer —— 编辑器层

```cpp
// EditorLayer.h
class EditorLayer : public Layer
{
    // 覆写所有五个钩子
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event& e) override;
};
```

| 属性 | 说明 |
|------|------|
| 类型 | **Layer**（通过 `Application::PushLayer` 加入） |
| 职责 | 编辑器主逻辑：场景层级面板、内容浏览器、Gizmo 操作、资产编辑、视口渲染等 |
| 文件路径 | [Haoyue-Editor/src/EditorLayer.h](Haoyue-Editor/src/EditorLayer.h) |

EditorLayer 是引擎中最复杂、代码量最大的 Layer，实现了完整的编辑器功能。

### 6.3 RuntimeLayer —— 运行时层

```cpp
// RuntimeLayer.h
class RuntimeLayer : public Layer
{
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;
    // 注意：没有覆写 OnImGuiRender()
};
```

| 属性 | 说明 |
|------|------|
| 类型 | **Layer** |
| 职责 | 运行打包后的游戏：加载场景、驱动场景更新、处理输入事件 |
| 文件路径 | [Haoyue-Runtime/Haoyue-Runtime/src/RuntimeLayer.h](Haoyue-Runtime/Haoyue-Runtime/src/RuntimeLayer.h) |

RuntimeLayer 是打包后游戏的主 Layer，它**不需要 ImGui 渲染**（打包游戏中没有编辑器 UI），因此不覆写 `OnImGuiRender()`，这正体现了基类默认空实现的设计优势。

---

## 7. 如何添加自定义 Layer

创建一个新的 Layer 只需三步：

**步骤 1：继承 Layer 并覆写关心的钩子**

```cpp
class MyGameLayer : public Layer
{
public:
    MyGameLayer() : Layer("MyGameLayer") {}

    void OnAttach() override
    {
        HY_CORE_INFO("MyGameLayer attached!");
        // 初始化资源...
    }

    void OnUpdate(Timestep ts) override
    {
        // 每帧逻辑...
        m_PlayerPosition += m_Velocity * ts;  // ts 隐式转换为 float
    }

    void OnEvent(Event& event) override
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
            return OnKeyPressed(e);
        });
    }

private:
    bool OnKeyPressed(KeyPressedEvent& e)
    {
        if (e.GetKeyCode() == KeyCode::Space)
        {
            // 处理空格键...
            return true;  // 事件已处理
        }
        return false;  // 让事件继续传播
    }

    glm::vec3 m_PlayerPosition;
    float m_Velocity = 5.0f;
};
```

**步骤 2：在 Application 初始化时 Push 到 LayerStack**

```cpp
// 在 CreateApplication() 中
auto* gameLayer = new MyGameLayer();
Application::Get().PushLayer(gameLayer);  // 自动调用 gameLayer->OnAttach()
```

**步骤 3：（可选）在不需要时移除**

```cpp
// Application::Get() 的析构函数会自动清理，但也可以手动移除
// 注意：手动 Pop 不会调用 OnDetach + delete，需要自行处理
```

---

## 附录：类图

```
┌─────────────────────────────────────────────────────┐
│                   Application                        │
│  ┌──────────────────────────────────────────────┐   │
│  │              LayerStack                       │   │
│  │  m_Layers: vector<Layer*>                    │   │
│  │  m_LayerInsertIndex: uint                    │   │
│  │  ┌──────────────────┬──────────────────┐     │   │
│  │  │   Layer 区域      │  Overlay 区域     │     │   │
│  │  │   [0..N-1]        │  [N..N+M-1]       │     │   │
│  │  └──────────────────┴──────────────────┘     │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
│  Run(): while(m_Running) {                           │
│    ProcessEvents() → OnEvent()                       │
│    for layer in LayerStack: layer->OnUpdate(ts)      │
│    RenderImGui(): for layer: layer->OnImGuiRender()  │
│    SwapBuffers()                                     │
│  }                                                   │
└──────┬───────────────┬───────────────┬───────────────┘
       │               │               │
       ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│  EditorLayer │ │ RuntimeLayer │ │  ImGuiLayer  │
│   (Layer)    │ │   (Layer)    │ │  (Overlay)   │
├──────────────┤ ├──────────────┤ ├──────────────┤
│ OnAttach()   │ │ OnAttach()   │ │ Begin()      │
│ OnUpdate()   │ │ OnUpdate()   │ │ End()        │
│ OnEvent()    │ │ OnEvent()    │ │              │
│ OnImGuiRend. │ │              │ │              │
│ OnDetach()   │ │ OnDetach()   │ │              │
└──────────────┘ └──────────────┘ └──────┬───────┘
                                         │
                                         ▼
                                ┌────────────────┐
                                │ VulkanImGuiLayer│
                                │  (具体实现)      │
                                └────────────────┘


继承关系:
  Layer (抽象基类, Core/Layer.h)
  ├── EditorLayer (编辑器, Haoyue-Editor/)
  ├── RuntimeLayer (运行时, Haoyue-Runtime/)
  └── ImGuiLayer (抽象, Haoyue/ImGui/)
       └── VulkanImGuiLayer (具体, Haoyue/Vulkan/)

数据流:
  Application::Run()
    │
    ├── Layer::OnUpdate(Timestep)     ← 顺序遍历 LayerStack（正序）
    │     EditorLayer → RuntimeLayer  (Layer 区域)
    │     ImGuiLayer                  (Overlay 区域)
    │
    ├── Application::OnEvent(Event&)  ← 反向遍历 LayerStack（从后往前）
    │     ImGuiLayer → RuntimeLayer → EditorLayer
    │     (Overlay 先处理，可短路)
    │
    └── Layer::OnImGuiRender()        ← 仅在 ImGui Begin/End 之间调用
          EditorLayer (绘制编辑器 UI)
          (RuntimeLayer 不覆写此方法)
```
