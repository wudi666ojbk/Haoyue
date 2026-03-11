Scene: Scene Name
Environment:
  AssetHandle: 6323043749067612265
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 12392346523579991874
    Parent: 5561134054991576534
    Children:
      []
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, -5.25212717, -4.11461163]
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
  - Entity: 5561134054991576534
    Parent: 0
    Children:
      - Handle: 12392346523579991874
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 6.31091785, 4.11461163]
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
      AssetID: 17520533498056549991
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
    MeshColliderComponent:
      IsConvex: true
      IsTrigger: false
      OverrideMesh: false
      Material: 1609627856876029936
  - Entity: 5471747392929574277
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.00999999978, 0.00999999978, 0.00999999978]
    MeshComponent:
      AssetID: 13423979752956760632
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
      IsConvex: false
      IsTrigger: false
      OverrideMesh: false
      Material: 14007657995324562006
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
      EnvironmentMap: 6323043749067612265
      Intensity: 1
      Angle: 0
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
PhysicsLayers:
  []