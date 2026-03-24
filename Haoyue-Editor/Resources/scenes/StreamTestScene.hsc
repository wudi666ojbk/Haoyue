Scene: Scene Name
Environment:
  AssetHandle: 5100181990263002040
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 15134723723926110614
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: 天空光
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentMap: 5100181990263002040
      Intensity: 1
      Angle: 0
  - Entity: 3586463467297568945
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 13.8065729, 46]
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
    RigidBody2DComponent:
      BodyType: 1
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
      Density: 1
      Friction: 1
  - Entity: 10482468196881171404
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: gun
    TransformComponent:
      Position: [0, 91.4110565, 0]
      Rotation: [0, 0, 0]
      Scale: [0.359999985, 0.360000014, 0.360000014]
    MeshComponent:
      AssetID: 10170827037760768299
    RigidBody2DComponent:
      BodyType: 1
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
      Density: 1
      Friction: 1
  - Entity: 14445970739577976395
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: platform2
    TransformComponent:
      Position: [-10.8582497, 1.71247244, 2.49667664e-08]
      Rotation: [0, 0, 0]
      Scale: [36, 4.00000238, 4]
    MeshComponent:
      AssetID: 2465601043907353901
    RigidBody2DComponent:
      BodyType: 0
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [18, 2]
      Density: 1
      Friction: 1
  - Entity: 15439361105258763253
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: platform
    TransformComponent:
      Position: [13.6415138, 2.8245554, -3.03643901e-08]
      Rotation: [0, 0, 0]
      Scale: [36, 3.99999952, 4]
    MeshComponent:
      AssetID: 2465601043907353901
    RigidBody2DComponent:
      BodyType: 0
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [18, 1]
      Density: 1
      Friction: 1
  - Entity: 16879531909728311346
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Cube
    TransformComponent:
      Position: [0, 14.43297, -7.4505806e-09]
      Rotation: [0, 0, 0]
      Scale: [4, 4, 1]
    MeshComponent:
      AssetID: 2465601043907353901
    RigidBody2DComponent:
      BodyType: 1
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
      Density: 1
      Friction: 1
PhysicsLayers:
  []