Scene: Scene Name
Environment:
  AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\env\birchwood_4k.hdr
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 9444046521930815822
    TagComponent:
      Tag: Capsule
    TransformComponent:
      Position: [0, 0.92374295, 2.98023224e-08]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\meshes\Capsule.fbx
    MeshColliderComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\meshes\Capsule.fbx
      IsConvex: true
      IsTrigger: false
  - Entity: 12392346523579991874
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 1.0587908, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera: some camera data...
      Primary: true
  - Entity: 5561134054991576534
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [-0.787863374, 0.826377511, 0.470583737]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    ScriptComponent:
      ModuleName: FPSExample.FPSPlayer
      StoredFields:
        - Name: WalkingSpeed
          Type: 1
          Data: 3
        - Name: RunSpeed
          Type: 1
          Data: 5
        - Name: JumpForce
          Type: 1
          Data: 1
        - Name: CameraForwardOffset
          Type: 1
          Data: 0.200000003
        - Name: CameraYOffset
          Type: 1
          Data: 0.850000024
        - Name: MouseSensitivity
          Type: 1
          Data: 2
    MeshComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\meshes\Capsule.fbx
    RigidBodyComponent:
      BodyType: 1
      Mass: 1
      LinearDrag: 0
      AngularDrag: 0.0500000007
      DisableGravity: false
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: true
        LockRotationY: true
        LockRotationZ: true
    PhysicsMaterialComponent:
      StaticFriction: 0.100000001
      DynamicFriction: 1
      Bounciness: 0
    CapsuleColliderComponent:
      Radius: 0.5
      Height: 1
      IsTrigger: false
  - Entity: 4800910954900174469
    TagComponent:
      Tag: Mesh
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\meshes\CubeScene.fbx
    RigidBodyComponent:
      BodyType: 0
      Mass: 1
      LinearDrag: 0
      AngularDrag: 0.0500000007
      DisableGravity: false
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    MeshColliderComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\meshes\CubeScene.fbx
      IsConvex: true
      IsTrigger: false
  - Entity: 2143608321399101581
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentAssetPath: D:\Programming\C++\HazelPhysics\HaoYue-Editor\Resources\env\birchwood_4k.hdr
      Intensity: 1
      Angle: 0
  - Entity: 13882838760121718506
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [2.35444903, 0, -0.441568285]
      Scale: [1, 1, 1]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
PhysicsLayers:
  []