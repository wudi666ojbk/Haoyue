Scene: Scene Name
Environment:
  AssetHandle: 4314599293696497732
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 13882838760121718506
    Parent: 0
    Children:
      []
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
  - Entity: 2143608321399101581
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
      EnvironmentMap: 4314599293696497732
      Intensity: 3.78999996
      Angle: 0
  - Entity: 4800910954900174469
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Mesh
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [3.42677975, 3.42677975, 3.42677975]
    MeshComponent:
      AssetID: 16379748338207419655
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
      IsConvex: true
      IsTrigger: false
      OverrideMesh: false
      Material: 0
  - Entity: 5561134054991576534
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [-0.787863374, 1.2080369, 0.470583737]
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
        - Name: MouseSensitivity
          Type: 1
          Data: 2
    MeshComponent:
      AssetID: 9275131759233489157
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
    CapsuleColliderComponent:
      Radius: 0.5
      Height: 1
      IsTrigger: false
      Material: 7702160544966247814
  - Entity: 12392346523579991874
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 2.05968499, 4.65492296]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera:
        ProjectionType: 0
        PerspectiveFOV: 45
        PerspectiveNear: 0.100000001
        PerspectiveFar: 1000
        OrthographicSize: 10
        OrthographicNear: -1
        OrthographicFar: 1
      Primary: true
PhysicsLayers:
  []