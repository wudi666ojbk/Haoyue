Scene: Scene Name
Environment:
  AssetHandle: 1453555556797978549
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 4020892924108347660
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Ground
    TransformComponent:
      Position: [-10, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 0.25, 5]
    MeshComponent:
      AssetID: 17589729741413358927
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
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [2, 2, 2]
      IsTrigger: false
      Material: 10121066009159327136
  - Entity: 16519350140026489313
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Ground
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 0.25, 5]
    MeshComponent:
      AssetID: 17589729741413358927
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
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [2, 2, 2]
      IsTrigger: false
      Material: 10121066009159327136
  - Entity: 8139519735174842173
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Capsule
    TransformComponent:
      Position: [-3.39538145, 1.47949338, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetID: 4036347301274105804
  - Entity: 14183082477086148524
    Parent: 0
    Children:
      - Handle: 17841934379095175961
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 1.39999998, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    ScriptComponent:
      ModuleName: FPSExample.FPSPlayer
      StoredFields:
        - Name: WalkingSpeed
          Type: 1
          Data: 5
        - Name: RunSpeed
          Type: 1
          Data: 10
        - Name: JumpForce
          Type: 1
          Data: 2.5
        - Name: MouseSensitivity
          Type: 1
          Data: 1
    MeshComponent:
      AssetID: 4036347301274105804
    RigidBodyComponent:
      BodyType: 1
      Mass: 1
      LinearDrag: 0
      AngularDrag: 0
      DisableGravity: false
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: true
        LockRotationY: false
        LockRotationZ: true
    CapsuleColliderComponent:
      Radius: 0.5
      Height: 1
      IsTrigger: false
      Material: 10121066009159327136
  - Entity: 17660423772096889750
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Ground
    TransformComponent:
      Position: [10, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 0.25, 5]
    MeshComponent:
      AssetID: 17589729741413358927
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
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [2, 2, 2]
      IsTrigger: false
      Material: 10121066009159327136
  - Entity: 15200110747878749015
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentMap: 1453555556797978549
      Intensity: 1
      Angle: 0
  - Entity: 11043057851675603335
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1.39626336, 0.17453292, 0]
      Scale: [1, 1, 1]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 17841934379095175961
    Parent: 14183082477086148524
    Children:
      []
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 0.899999976, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera:
        ProjectionType: 0
        PerspectiveFOV: 70
        PerspectiveNear: 0.00999999978
        PerspectiveFar: 10000
        OrthographicSize: 10
        OrthographicNear: -1
        OrthographicFar: 1
      Primary: true
PhysicsLayers:
  []