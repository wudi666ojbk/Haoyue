Scene: Scene Name
Environment:
  AssetHandle: 7777554117957742455
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 16844037665270894117
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
      EnvironmentMap: 7777554117957742455
      Intensity: 1
      Angle: 0
  - Entity: 8711824436246927141
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 8.93375397, 0]
      Rotation: [0, 1.57079637, 0]
      Scale: [0.999998212, 1, 0.999998212]
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
  - Entity: 4655832978979727422
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Empty Entity
    TransformComponent:
      Position: [-49.4744797, 30.9813614, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 5, 5]
    MeshComponent:
      AssetID: 2465601043907353901
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
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [1, 1, 1]
      IsTrigger: false
      Material: 1088619180590167763
  - Entity: 15213035846546605980
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.100000001, 0.100000001, 0.100000001]
    MeshComponent:
      AssetID: 5084203466852979931
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
      Material: 245906476557249746
PhysicsLayers:
  []