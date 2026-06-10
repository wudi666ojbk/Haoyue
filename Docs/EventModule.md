# Haoyue 引擎事件系统文档

## 目录

- [1. 概述](#1-概述)
- [2. 事件类层次体系](#2-事件类层次体系)
  - [2.1 Event 基类](#21-event-基类)
  - [2.2 EventType 与 EventCategory](#22-eventtype-与-eventcategory)
  - [2.3 事件宏系统](#23-事件宏系统)
  - [2.4 完整事件类型树](#24-完整事件类型树)
- [3. EventDispatcher —— 类型安全的分发器](#3-eventdispatcher--类型安全的分发器)
  - [3.1 模板设计详解](#31-模板设计详解)
  - [3.2 使用范式](#32-使用范式)
  - [3.3 与 std::bind 的配合](#33-与-stdbind-的配合)
- [4. 事件的生命周期：从 GLFW 到 Layer](#4-事件的生命周期从-glfw-到-layer)
  - [4.1 GLFW 回调 → Haoyue 事件创建](#41-glfw-回调--haoyue-事件创建)
  - [4.2 Application 层分发](#42-application-层分发)
  - [4.3 Layer 反向传播](#43-layer-反向传播)
  - [4.4 短路机制](#44-短路机制)
- [5. 设计决策分析](#5-设计决策分析)
  - [5.1 为什么是阻塞式事件而非事件队列](#51-为什么是阻塞式事件而非事件队列)
  - [5.2 为什么使用宏而非模板元编程](#52-为什么使用宏而非模板元编程)
  - [5.3 为什么 Event::Handled 是 public 成员](#53-为什么-eventhandled-是-public-成员)
  - [5.4 为什么 Dispatch 使用 C 风格强转而非 dynamic_cast](#54-为什么-dispatch-使用-c-风格强转而非-dynamic_cast)
- [6. 事件子系统详解](#6-事件子系统详解)
  - [6.1 窗口事件](#61-窗口事件)
  - [6.2 键盘事件](#62-键盘事件)
  - [6.3 鼠标事件](#63-鼠标事件)
  - [6.4 应用生命周期事件](#64-应用生命周期事件)
- [7. 事件与 Input 轮询的互补关系](#7-事件与-input-轮询的互补关系)
- [附录 A：事件类型速查表](#附录-a事件类型速查表)
- [附录 B：事件传播流程图](#附录-b事件传播流程图)

---

## 1. 概述

Haoyue 的事件系统负责将**平台原生输入**（键盘、鼠标、窗口）转化为引擎内部统一的**类型安全事件对象**，并在 Layer 层级体系中**有序传播**。

整个事件系统由三层架构组成：

```
┌──────────────────────────────────────────────────┐
│  Layer 3: 事件消费者                              │
│  EditorLayer, RuntimeLayer, ImGuiLayer,           │
│  EditorCamera...                                  │
│  使用 EventDispatcher 分发具体事件到处理函数        │
├──────────────────────────────────────────────────┤
│  Layer 2: 事件路由                                │
│  Application::OnEvent()                           │
│  → Application 自身处理 → LayerStack 反向传播      │
├──────────────────────────────────────────────────┤
│  Layer 1: 事件生产者                              │
│  WindowsWindow (GLFW 回调)                        │
│  → 将 GLFW 原生事件转化为 Haoyue Event 子类        │
└──────────────────────────────────────────────────┘
```

**核心文件：**

| 文件 | 职责 |
|------|------|
| [Core/Events/Event.h](Haoyue/src/Haoyue/Core/Events/Event.h) | `Event` 基类、`EventDispatcher`、事件枚举、宏定义 |
| [Core/Events/ApplicationEvent.h](Haoyue/src/Haoyue/Core/Events/ApplicationEvent.h) | 窗口与应用生命周期事件 |
| [Core/Events/KeyEvent.h](Haoyue/src/Haoyue/Core/Events/KeyEvent.h) | 键盘事件（按下/释放/输入） |
| [Core/Events/MouseEvent.h](Haoyue/src/Haoyue/Core/Events/MouseEvent.h) | 鼠标事件（移动/滚轮/按钮） |
| [Core/KeyCodes.h](Haoyue/src/Haoyue/Core/KeyCodes.h) | 键码枚举（104 个按键 + 3 个鼠标按钮） |
| [Core/Input.h](Haoyue/src/Haoyue/Core/Input.h) | 输入轮询 API（静态方法） |
| [Core/Base.h](Haoyue/src/Haoyue/Core/Base.h) | `HY_BIND_EVENT_FN` 宏 |
| [Windows/WindowsWindow.cpp](Haoyue/src/Haoyue/Windows/WindowsWindow.cpp) | GLFW 回调 → Haoyue 事件的桥接层 |

---

## 2. 事件类层次体系

### 2.1 Event 基类

```cpp
// Event.h
class Event
{
public:
    bool Handled = false;                             // 短路标志（唯一的非虚数据成员）

    virtual EventType GetEventType() const = 0;       // 运行时类型标识
    virtual const char* GetName() const = 0;          // 类型名（调试用）
    virtual int GetCategoryFlags() const = 0;         // 分类掩码
    virtual std::string ToString() const { return GetName(); }  // 可读表示

    inline bool IsInCategory(EventCategory category)
    {
        return GetCategoryFlags() & category;          // 位掩码检查
    }
};
```

**设计要点：**

- `Event` 是一个**多态基类**，不是模板——所有事件类型共享同一个基类指针 `Event*`，这是它能在一个 `std::function<void(Event&)>` 回调中统一传递的前提
- `Handled` 是唯一的非虚数据成员，且标记为 `public`——事件消费者可以直接 `event.Handled = true` 来阻止传播（详见 5.3 节）
- 三个纯虚函数 (`GetEventType`, `GetName`, `GetCategoryFlags`) 共同构成了一个轻量级的 **RTTI 替代方案**，不依赖编译器的 `typeid`（RTTI 通常在游戏引擎中被禁用以节省二进制体积）

### 2.2 EventType 与 EventCategory

```cpp
enum class EventType
{
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication    = BIT(0),   // 1 << 0 = 1
    EventCategoryInput          = BIT(1),   // 1 << 1 = 2
    EventCategoryKeyboard       = BIT(2),   // 1 << 2 = 4
    EventCategoryMouse          = BIT(3),   // 1 << 3 = 8
    EventCategoryMouseButton    = BIT(4)    // 1 << 4 = 16
};
```

**两个枚举服务于不同目的：**

| 枚举 | 用途 | 使用场景 |
|------|------|---------|
| `EventType` | **精确类型匹配**，决定 Dispatch 到哪个处理函数 | `EventDispatcher::Dispatch<T>()` 内部用 `T::GetStaticType()` 比较 |
| `EventCategory` | **分类过滤**，允许按组处理事件 | `event.IsInCategory(EventCategoryInput)` 可以匹配所有键盘+鼠标事件 |

`EventCategory` 使用**位掩码**（每个值是 2 的幂），因此一个事件可以同时属于多个类别。例如 `EventCategoryKeyboard | EventCategoryInput` 表示"既是键盘事件也是输入事件"。这是典型的组合模式——`IsInCategory` 只需一次按位与操作即可判断。

`BIT(x)` 宏展开为 `(1 << x)`，定义在 [Core/Base.h](Haoyue/src/Haoyue/Core/Base.h) 中。

### 2.3 事件宏系统

每个事件子类必须声明两个宏：

```cpp
#define EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() { return EventType::##type; } \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
    virtual int GetCategoryFlags() const override { return category; }
```

**宏展开示例**：对于 `KeyPressedEvent`：

```cpp
// 源码中写：
EVENT_CLASS_TYPE(KeyPressed)
EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

// 展开后等效于：
static EventType GetStaticType() { return EventType::KeyPressed; }
virtual EventType GetEventType() const override { return GetStaticType(); }
virtual const char* GetName() const override { return "KeyPressed"; }
virtual int GetCategoryFlags() const override { return EventCategoryKeyboard | EventCategoryInput; }
```

**设计意图**：

- `GetStaticType()` 是一个**静态方法**，不需要实例就能获取类型，这是 `EventDispatcher` 模板分发的关键——它调用 `T::GetStaticType()` 而非 `event.GetEventType()`
- `GetEventType()` 是实例方法，直接委托给静态版本，保证了一致性
- 宏消除了**大量样板代码**——14 个事件类，每个都需要覆写 3 个虚函数和 1 个静态方法，宏将 4 行缩减为 1-2 行

### 2.4 完整事件类型树

```
Event (抽象基类)
├── WindowResizeEvent       [Application]
├── WindowCloseEvent        [Application]
├── AppTickEvent            [Application]
├── AppUpdateEvent          [Application]
├── AppRenderEvent          [Application]
├── KeyEvent (抽象中间类)    [Keyboard | Input]
│   ├── KeyPressedEvent     [Keyboard | Input]
│   ├── KeyReleasedEvent    [Keyboard | Input]
│   └── KeyTypedEvent       [Keyboard | Input]
├── MouseButtonEvent (抽象)  [Mouse | Input]
│   ├── MouseButtonPressedEvent  [Mouse | Input]
│   └── MouseButtonReleasedEvent [Mouse | Input]
├── MouseMovedEvent         [Mouse | Input]
└── MouseScrolledEvent      [Mouse | Input]
```

**层次深度为 2 层**：`Event` → 中间抽象类（如 `KeyEvent`）→ 具体事件类。中间抽象类的构造函数是 `protected`，防止直接实例化：

```cpp
class KeyEvent : public Event
{
protected:
    KeyEvent(KeyCode keycode) : m_KeyCode(keycode) {}  // 只能被子类调用
    KeyCode m_KeyCode;
public:
    inline KeyCode GetKeyCode() const { return m_KeyCode; }
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)  // 子类继承此分类
};
```

这样 `KeyPressedEvent` 和 `KeyReleasedEvent` 共享 `GetKeyCode()` 接口，符合 DRY 原则。

---

## 3. EventDispatcher —— 类型安全的分发器

### 3.1 模板设计详解

```cpp
class EventDispatcher
{
    template<typename T>
    using EventFn = std::function<bool(T&)>;
public:
    EventDispatcher(Event& event)
        : m_Event(event)
    {
    }

    template<typename T>
    bool Dispatch(EventFn<T> func)
    {
        if (m_Event.GetEventType() == T::GetStaticType())  // 运行时类型检查
        {
            m_Event.Handled = func(*(T*)&m_Event);          // C 风格强制转换 + 调用
            return true;                                     // 类型匹配成功
        }
        return false;                                        // 类型不匹配
    }
private:
    Event& m_Event;
};
```

**这是整个事件系统的核心分发机制**，其工作原理分为三步：

**第 1 步：类型匹配**

```cpp
if (m_Event.GetEventType() == T::GetStaticType())
```

- `m_Event.GetEventType()` 是运行时虚函数调用，返回实际的 `EventType`
- `T::GetStaticType()` 是编译期静态方法调用（模板参数 `T` 在编译期已确定）
- 两个 `EventType` 枚举值比较，O(1) 开销

**第 2 步：向下转型**

```cpp
func(*(T*)&m_Event)
```

- 把 `Event&` 强制转换为 `T&`（例如 `KeyPressedEvent&`）
- 这是安全的，因为第 1 步已经确认了运行时类型匹配
- 使用 C 风格强转而非 `dynamic_cast`，因为引擎禁用了 RTTI（详见 5.4 节）

**第 3 步：记录处理状态**

```cpp
m_Event.Handled = func(...);
```

- 处理函数的返回值（`bool`）被写入 `event.Handled`
- 返回 `true` 表示"事件已被消费，停止传播"
- `Dispatch` 本身返回 `bool` 表示"类型是否匹配"，而非事件是否被处理

### 3.2 使用范式

典型的 `OnEvent` 实现遵循固定模式：

```cpp
void EditorLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);

    // 链式 Dispatch 调用——每个 dispatch 尝试匹配一种类型
    dispatcher.Dispatch<KeyPressedEvent>(HY_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HY_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));

    // 链式调用的妙处：如果第一个 Dispatch 不匹配（事件不是 KeyPressedEvent），
    // 它返回 false 但不影响后续 Dispatch，直到找到匹配的类型
}
```

**为什么链式 Dispatch 是合理的**：每个 `Dispatch<T>()` 只在 `e` 确实是 `T` 类型时才执行回调。如果类型不匹配，`Dispatch` 是一个无副作用的空操作。这保证了**只有一个 Dispatch 会实际执行**（因为一个事件只有一种类型），但代码读起来像是声明了"如果事件是 X 类型，就交给这个函数处理"。

**事件处理函数的签名规范**：

```cpp
bool OnKeyPressedEvent(KeyPressedEvent& e)  // 必须返回 bool
{
    if (e.GetKeyCode() == KeyCode::Escape)
    {
        // 处理 Escape 键...
        return true;   // 事件已消费，不再传播
    }
    return false;      // 未处理，让事件继续传播
}
```

### 3.3 与 std::bind 的配合

`HY_BIND_EVENT_FN` 宏定义在 [Core/Base.h](Haoyue/src/Haoyue/Core/Base.h)：

```cpp
#define HY_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)
```

展开后示例：

```cpp
// HY_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent)
// 展开为：
std::bind(&EditorLayer::OnKeyPressedEvent, this, std::placeholders::_1)
```

**为什么需要 `std::bind`？** 因为 `EventDispatcher::Dispatch<T>` 期望一个 `std::function<bool(T&)>`（只有一个参数），但成员函数隐式携带 `this` 指针。`std::bind` 将 `this` 绑定为第一个参数，将事件的引用留给第二个参数（`_1` 占位符），使成员函数适配成期望的签名。

> **注意**：在现代 C++ 中，`std::bind` 通常可以用 lambda 替代（`[this](KeyPressedEvent& e) { return OnKeyPressedEvent(e); }`），但 `std::bind` 的写法更紧凑，且在该项目中是统一的惯用法。

---

## 4. 事件的生命周期：从 GLFW 到 Layer

### 4.1 GLFW 回调 → Haoyue 事件创建

整个事件流的起点在 [WindowsWindow.cpp](Haoyue/src/Haoyue/Windows/WindowsWindow.cpp) 的 `Init()` 方法中。每个 GLFW 回调都是一个**匿名 lambda**，将 GLFW 的原生数据转化为 Haoyue 事件对象：

```cpp
// 示例：键盘回调
glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto& data = *((WindowData*)glfwGetWindowUserPointer(window));  // 取出 WindowData

    switch (action)
    {
    case GLFW_PRESS:
    {
        KeyPressedEvent event((KeyCode)key, 0);   // GLFW key → Haoyue KeyCode, repeat=0
        data.EventCallback(event);                 // 调用 Application::OnEvent
        break;
    }
    case GLFW_RELEASE:
    {
        KeyReleasedEvent event((KeyCode)key);
        data.EventCallback(event);
        break;
    }
    case GLFW_REPEAT:
    {
        KeyPressedEvent event((KeyCode)key, 1);   // repeat=1 表示自动重复
        data.EventCallback(event);
        break;
    }
    }
});
```

**GLFW `action` 与 Haoyue 事件的映射关系：**

| GLFW action | Haoyue 事件 | 说明 |
|-------------|------------|------|
| `GLFW_PRESS` | `KeyPressedEvent(key, 0)` | `repeatCount=0` 表示首次按下 |
| `GLFW_REPEAT` | `KeyPressedEvent(key, 1)` | `repeatCount=1` 表示系统自动重复（按住不放） |
| `GLFW_RELEASE` | `KeyReleasedEvent(key)` | 按键释放 |
| `GLFW_PRESS` (mouse) | `MouseButtonPressedEvent(button)` | 鼠标按钮按下 |
| `GLFW_RELEASE` (mouse) | `MouseButtonReleasedEvent(button)` | 鼠标按钮释放 |

**WindowData 的回调链路**：

```
GLFW 原生事件
  → glfwSetXxxCallback lambda（GLFW window* 参数）
  → WindowData::EventCallback（std::function<void(Event&)>）
  → Application::OnEvent（由 Application 构造函数设置）
```

`EventCallback` 在 `Application` 构造时注册：

```cpp
// Application.cpp
m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
// 等价于：
m_Window->SetEventCallback(
    std::bind(&Application::OnEvent, this, std::placeholders::_1)
);
```

### 4.2 Application 层分发

```cpp
// Application.cpp
void Application::OnEvent(Event& event)
{
    // ── 阶段 1：Application 自身处理 ──
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
    dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
    // 这两个事件是 Application 级别的，不需要传给 Layer

    // ── 阶段 2：LayerStack 反向传播 ──
    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
    {
        (*--it)->OnEvent(event);  // 从末尾向前遍历
        if (event.Handled)
            break;                // 短路：事件被某层消费，停止传播
    }
}
```

**两个阶段的分工：**

- **阶段 1**：Application 拦截 `WindowResizeEvent` 和 `WindowCloseEvent`。这两个事件关乎整个应用的存亡（调整帧缓冲大小、关闭应用），不应该交给 Layer 处理
- **阶段 2**：将事件按**从上到下**（从 Overlay 到 Layer）的顺序传播给 LayerStack 中的每一层

### 4.3 Layer 反向传播

在 Layer 内部，事件再次通过 `EventDispatcher` 分发到具体的处理函数：

```
Application::OnEvent(event)
  │
  ├── Application 自身: OnWindowResize / OnWindowClose
  │
  └── 反向遍历 LayerStack:
        │
        ├── Overlay 0 (ImGuiLayer)::OnEvent(event)
        │     ├── EventDispatcher → KeyPressedEvent? → ImGui_ImplGlfw_KeyCallback
        │     ├── EventDispatcher → MouseButtonPressedEvent? → ImGui_ImplGlfw_MouseButtonCallback
        │     └── ... (ImGui 内部处理，消费掉在 UI 上的点击/按键)
        │     └── 如果 Handled=true → 短路，不传给后续 Layer
        │
        ├── Layer 0 (EditorLayer)::OnEvent(event)
        │     ├── 先传给 m_Scene->OnEvent(event)  // 场景内的实体也可能响应事件
        │     ├── EventDispatcher → KeyPressedEvent? → OnKeyPressedEvent
        │     └── EventDispatcher → MouseButtonPressedEvent? → OnMouseButtonPressed
        │     └── 如果 Handled=true → 短路
        │
        └── (如果没有任何 Layer 消费事件，事件自然消亡)
```

### 4.4 短路机制

短路机制由两个要素配合完成：

1. **`Event::Handled`（public bool）**：事件消费者设置 `event.Handled = true`
2. **`Application::OnEvent` 中的遍历逻辑**：检测到 `event.Handled` 后立即 `break`

**短路的典型场景**：

```
用户在 ImGui 面板上点击鼠标
  ──MouseButtonPressedEvent──→ ImGuiLayer::OnEvent
                                  ├── ImGui 判断点击位置在某个窗口内
                                  ├── ImGui 内部处理点击（选中按钮、聚焦输入框等）
                                  └── event.Handled = true  ← 消费事件
                                                              │
                          ┌─────────────────────────────────┘
                          │ break! 事件不再传递
                          ▼
              EditorLayer 收不到这个点击事件
              → 视口内的 Gizmo 不会被意外选中
              → 场景中的实体不会收到错误的点击
```

如果没有短路机制，点击 ImGui 面板的同时也会操作到背后的 3D 场景，这是不可接受的交互 Bug。

---

## 5. 设计决策分析

### 5.1 为什么是阻塞式事件而非事件队列

源码注释中已经坦诚了这个设计选择：

```cpp
// Events in Haoyue are currently blocking, meaning when an event occurs it
// immediately gets dispatched and must be dealt with right then an there.
// For the future, a better strategy might be to buffer events in an event
// bus and process them during the "event" part of the update stage.
```

| 方案 | 描述 | 优点 | 缺点 |
|------|------|------|------|
| **阻塞式（当前）** | GLFW 回调中立即创建 Event → 立即在调用栈中传播 → 立即被处理 | 实现简单、延迟极低、无内存分配 | 事件处理函数中不能做耗时操作（阻塞整个事件循环）；处理顺序由调用栈决定 |
| **事件队列** | GLFW 回调将 Event push 到队列 → 主循环在固定阶段批量取出处理 | 处理时机可控、可以优先级排序、可以合并冗余事件（如连续 MouseMoved） | 额外内存分配、需要管理队列生命周期、增加一帧延迟 |

**Haoyue 选择阻塞式的原因**：

1. **引擎规模**：当前引擎 Layer 数量少（通常 < 5 个），事件处理逻辑轻量（切换状态、更新相机），阻塞式不会造成可感知的延迟
2. **即时性需求**：ImGui 需要同步处理输入（在 `NewFrame()` 之前必须完成所有输入的注入），阻塞式天然满足
3. **简单性**：不需要额外的内存管理、线程安全、事件合并逻辑——这些复杂性在当前阶段不值得引入

注释提到"未来可能缓冲事件"——当引擎需要支持**事件重放（录屏/回放系统）**、**网络同步输入**、或**宏命令系统**时，事件队列将是必要的。

### 5.2 为什么使用宏而非模板元编程

`EVENT_CLASS_TYPE` 和 `EVENT_CLASS_CATEGORY` 是宏，而非模板。原因：

1. **宏可以生成字符串字面量**：`virtual const char* GetName() const override { return #type; }` 中的 `#type`（字符串化操作符）是预处理器独有的能力，模板无法做到
2. **宏可以生成 `static` 方法**：`GetStaticType()` 必须是静态成员函数，因为 `EventDispatcher` 需要在不创建实例的情况下获取类型。模板无法在类体内生成静态方法定义
3. **代码量极小**：每个事件类只需 1-2 行宏调用，认知负担低

**为什么不使用 `typeid` / RTTI？**

- 大多数游戏引擎会通过编译器标志（如 MSVC 的 `/GR-`）禁用 RTTI，因为 `typeid` 会为每个多态类生成 `type_info` 数据，增加二进制体积
- 自定义的 `EventType` 枚举比 `typeid` 比较更快（整数比较 vs 字符串比较）

### 5.3 为什么 Event::Handled 是 public 成员

```cpp
class Event
{
public:
    bool Handled = false;  // 直接 public 访问
```

传统面向对象会将其封装为 `private` + `SetHandled()` / `IsHandled()` 方法。这里选择 public 的原因：

1. **简洁的消费语义**：`event.Handled = true` 比 `event.SetHandled(true)` 更直观、代码量更少
2. **C 风格的结构体思维**：在游戏引擎开发中，事件被视作"数据的载体"而非"行为的封装"，更接近 POD（Plain Old Data）的哲学
3. **性能**：直接内存访问，无函数调用开销（虽然微不足道，但对每帧数十个事件来说积少成多）
4. **GLFW/ImGui 惯例**：GLFW 和 ImGui 的事件结构体也使用公开字段，Haoyue 遵循了游戏行业的惯例

### 5.4 为什么 Dispatch 使用 C 风格强转而非 dynamic_cast

```cpp
m_Event.Handled = func(*(T*)&m_Event);  // C-style cast
```

**原因**：引擎禁用了 RTTI（运行时类型信息），而 `dynamic_cast` 依赖 RTTI。在禁用 RTTI 的情况下，`dynamic_cast` 会导致编译错误或未定义行为。

**安全性保证**：这个强转是安全的，因为调用前已经通过 `m_Event.GetEventType() == T::GetStaticType()` 确保了运行时类型匹配。两个独立的"类型标识系统"（枚举值比较 + C 风格强制转换）相互验证，构成了一个无需 RTTI 的手动类型安全机制。

---

## 6. 事件子系统详解

### 6.1 窗口事件

```cpp
// ApplicationEvent.h

class WindowResizeEvent : public Event
{
    // 数据: m_Width, m_Height (unsigned int)
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
};

class WindowCloseEvent : public Event
{
    // 无额外数据——关闭信号本身就是全部信息
};
```

**Application 对窗口事件的处理**（[Application.cpp](Haoyue/src/Haoyue/Core/Application.cpp)）：

```cpp
bool Application::OnWindowResize(WindowResizeEvent& e)
{
    int width = e.GetWidth(), height = e.GetHeight();
    if (width == 0 || height == 0)
    {
        m_Minimized = true;      // 窗口最小化 → 暂停渲染
        return false;
    }
    m_Minimized = false;

    m_Window->GetSwapChain().OnResize(width, height);  // 重建交换链
    // 通知所有帧缓冲也调整尺寸
    auto& fbs = FramebufferPool::GetGlobal()->GetAll();
    for (auto& fb : fbs)
    {
        if (!fb->GetSpecification().NoResize)
            fb->Resize(width, height);
    }
    return false;  // 不阻止事件继续传播（Layer 可能也需要知道尺寸变化）
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    m_Running = false;
    g_ApplicationRunning = false;
    return true;   // 事件已处理
}
```

### 6.2 键盘事件

```cpp
// KeyEvent.h

class KeyEvent : public Event          // 抽象中间类
{
protected:
    KeyEvent(KeyCode keycode);
    KeyCode m_KeyCode;
public:
    inline KeyCode GetKeyCode() const;
};

class KeyPressedEvent : public KeyEvent
{
    // 额外数据: m_RepeatCount (int)
    int GetRepeatCount() const;
    // 0 = 首次按下, 1+ = 系统自动重复
};

class KeyReleasedEvent : public KeyEvent
{
    // 无额外数据
};

class KeyTypedEvent : public KeyEvent
{
    // 无额外数据，但 KeyCode 存的是 Unicode 码点（通过 glfwSetCharCallback）
};
```

**`KeyPressed` vs `KeyTyped` 的区别**：
- `KeyPressed` 是物理按键事件（"键盘上的 A 键被按下了"），由 `glfwSetKeyCallback` 产生
- `KeyTyped` 是字符输入事件（"用户输入了字符 'a'"），由 `glfwSetCharCallback` 产生，考虑了 Shift、输入法等修饰

大多数游戏逻辑应使用 `KeyPressed`/`KeyReleased`（关心物理按键），文本输入框应使用 `KeyTyped`（关心字符）。

### 6.3 鼠标事件

```cpp
// MouseEvent.h

class MouseMovedEvent : public Event
{
    float GetX() const;       // 鼠标在窗口中的 X 坐标
    float GetY() const;       // 鼠标在窗口中的 Y 坐标
};

class MouseScrolledEvent : public Event
{
    float GetXOffset() const; // 水平滚轮偏移
    float GetYOffset() const; // 垂直滚轮偏移（大多数鼠标只有这个）
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
    int GetMouseButton() const;  // 0=左键, 1=右键, 2=中键
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
    int GetMouseButton() const;
};
```

### 6.4 应用生命周期事件

```cpp
class AppTickEvent : public Event {};      // 每帧开始
class AppUpdateEvent : public Event {};    // 更新阶段
class AppRenderEvent : public Event {};    // 渲染阶段
```

这三个事件**已在系统中定义但当前未激活使用**。它们的设计意图是让 Layer 在特定的帧阶段（Tick/Update/Render）执行逻辑——但目前引擎通过直接调用 `Layer::OnUpdate()` 而非通过事件机制来实现。这可能是未来"事件队列"重构的一部分。

---

## 7. 事件与 Input 轮询的互补关系

Haoyue 提供了**两种获取输入的方式**：事件驱动和轮询。

| 方式 | API | 适用场景 |
|------|-----|---------|
| **事件驱动** | `OnEvent(Event& e)` + `EventDispatcher` | 离散事件：按键按下/释放、鼠标点击、窗口大小变化 |
| **轮询** | `Input::IsKeyPressed(KeyCode)`、`Input::GetMousePosition()` | 连续状态：角色移动（"W 键是否正在被按住"）、鼠标位置 |

```cpp
// Input.h —— 轮询 API
class Input
{
public:
    static bool IsKeyPressed(KeyCode keycode);        // 每帧检查按键状态
    static bool IsMouseButtonPressed(MouseButton button);
    static float GetMouseX();
    static float GetMouseY();
    static std::pair<float, float> GetMousePosition();
    static void SetCursorMode(CursorMode mode);       // 光标模式（Normal/Hidden/Locked）
    static CursorMode GetCursorMode();
};
```

**为什么需要两种方式并存？**

- **事件适合"触发式"逻辑**：空格键跳跃——你只需要知道"按下的那一瞬间"，而不是每帧检查
- **轮询适合"持续式"逻辑**：WASD 移动——每一帧你需要知道"W 键现在是否被按着"，而不是等 KeyPressed 事件来触发一次移动

**实际使用案例**（[EditorCamera.cpp](Haoyue/src/Haoyue/Editor/EditorCamera.cpp)）：

```cpp
void EditorCamera::OnUpdate(Timestep ts)
{
    // 轮询方式：每帧检查鼠标按钮是否正在被按住（连续操作）
    if (Input::IsKeyPressed(KeyCode::LeftAlt))
    {
        const glm::vec2& mouse = Input::GetMousePosition();
        glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
        // ... 根据按住的按钮来平移/旋转/缩放 ...
        if (Input::IsMouseButtonPressed(MouseButton::Middle))
            MousePan(delta);
        else if (Input::IsMouseButtonPressed(MouseButton::Left))
            MouseRotate(delta);
    }
}

void EditorCamera::OnEvent(Event& e)
{
    // 事件方式：鼠标滚轮的离散事件
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(HY_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
}

bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
{
    float delta = e.GetYOffset() * 0.1f;
    MouseZoom(delta);
    return false;  // 不阻止事件继续传播（其他 Layer 也可以用滚轮）
}
```

---

## 附录 A：事件类型速查表

| 事件类 | EventType | Category | 数据字段 | GLFW 来源 |
|--------|-----------|----------|---------|----------|
| `WindowResizeEvent` | `WindowResize` | `Application` | `width, height` | `glfwSetWindowSizeCallback` |
| `WindowCloseEvent` | `WindowClose` | `Application` | 无 | `glfwSetWindowCloseCallback` |
| `AppTickEvent` | `AppTick` | `Application` | 无 | （未使用） |
| `AppUpdateEvent` | `AppUpdate` | `Application` | 无 | （未使用） |
| `AppRenderEvent` | `AppRender` | `Application` | 无 | （未使用） |
| `KeyPressedEvent` | `KeyPressed` | `Keyboard \| Input` | `keyCode, repeatCount` | `glfwSetKeyCallback` |
| `KeyReleasedEvent` | `KeyReleased` | `Keyboard \| Input` | `keyCode` | `glfwSetKeyCallback` |
| `KeyTypedEvent` | `KeyTyped` | `Keyboard \| Input` | `keyCode` (Unicode) | `glfwSetCharCallback` |
| `MouseButtonPressedEvent` | `MouseButtonPressed` | `Mouse \| Input` | `button` | `glfwSetMouseButtonCallback` |
| `MouseButtonReleasedEvent` | `MouseButtonReleased` | `Mouse \| Input` | `button` | `glfwSetMouseButtonCallback` |
| `MouseMovedEvent` | `MouseMoved` | `Mouse \| Input` | `mouseX, mouseY` | `glfwSetCursorPosCallback` |
| `MouseScrolledEvent` | `MouseScrolled` | `Mouse \| Input` | `xOffset, yOffset` | `glfwSetScrollCallback` |

## 附录 B：事件传播流程图

```
  ┌─────────────────────────────────────────────────────────────────┐
  │                      GLFW 原生事件                                │
  │  (OS 消息 → glfwPollEvents → glfwSetXxxCallback lambdas)        │
  └────────────┬────────────────────────────────────────────────────┘
               │
               │ lambda 中创建 Haoyue Event 子类，调用 EventCallback
               ▼
  ┌─────────────────────────────────────────────────────────────────┐
  │              Application::OnEvent(event)                         │
  │                                                                  │
  │  ┌──────────────────────────────────────┐                       │
  │  │ 阶段 1: EventDispatcher              │                       │
  │  │   Dispatch<WindowResizeEvent>  ──────→ OnWindowResize()      │
  │  │   Dispatch<WindowCloseEvent>   ──────→ OnWindowClose()       │
  │  │   (Application 级别事件，不传给 Layer)                        │
  │  └──────────────────────────────────────┘                       │
  │                                                                  │
  │  ┌──────────────────────────────────────┐                       │
  │  │ 阶段 2: LayerStack 反向遍历          │                       │
  │  │                                      │                       │
  │  │   for (it = end; it != begin;)       │                       │
  │  │     (*--it)->OnEvent(event)          │                       │
  │  │     if (event.Handled) break         │                       │
  │  │                                      │                       │
  │  │   遍历顺序:                           │                       │
  │  │   [末尾] Overlay 0 (ImGuiLayer)      │ ← 最先处理           │
  │  │          Overlay 1                   │                       │
  │  │          ...                          │                       │
  │  │          Layer 0   (EditorLayer)     │ ← 最后处理           │
  │  │   [开头] Layer 1                     │                       │
  │  └──────────────────────────────────────┘                       │
  └─────────────────────────────────────────────────────────────────┘
               │
               │ 在每个 Layer::OnEvent 内部:
               ▼
  ┌─────────────────────────────────────────────────────────────────┐
  │                Layer::OnEvent(event)                             │
  │                                                                  │
  │   EventDispatcher dispatcher(event);                            │
  │   dispatcher.Dispatch<KeyPressedEvent>(handler1);               │
  │   dispatcher.Dispatch<MouseButtonPressedEvent>(handler2);       │
  │   dispatcher.Dispatch<MouseScrolledEvent>(handler3);            │
  │   ...                                                            │
  │                                                                  │
  │   ┌─────────────────────────────────────────┐                   │
  │   │ Dispatch<T> 内部:                        │                   │
  │   │   1. event.GetEventType() == T::GetStaticType() ?           │
  │   │   2. 是 → event.Handled = handler((T&)event)                │
  │   │   3. 否 → return false (无副作用)        │                   │
  │   └─────────────────────────────────────────┘                   │
  └─────────────────────────────────────────────────────────────────┘
               │
               │ 如果 event.Handled == true:
               │   Application::OnEvent 中的 break 生效
               │   事件在此 Layer 被消费，不再向下传播
               ▼
         [事件消亡]
```
