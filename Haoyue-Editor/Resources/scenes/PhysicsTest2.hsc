Scene: Scene Name
Environment:
  AssetPath: assets\env\pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 15213035846546605980
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.100000001, 0.100000001, 0.100000001]
    MeshComponent:
      AssetPath: assets\meshes\sponza\sponza.obj
    RigidBodyComponent:
      BodyType: 0
      Mass: 1
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    PhysicsMaterialComponent:
      StaticFriction: 1
      DynamicFriction: 1
      Bounciness: 0
    MeshColliderComponent:
      AssetPath: assets\meshes\sponza\sponza.obj
      IsTrigger: false
  - Entity: 4655832978979727422
    TagComponent:
      Tag: Empty Entity
    TransformComponent:
      Position: [-49.4744797, 30.9813614, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 5, 5]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBodyComponent:
      BodyType: 1
      Mass: 1
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    PhysicsMaterialComponent:
      StaticFriction: 1
      DynamicFriction: 1
      Bounciness: 1
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [1, 1, 1]
      IsTrigger: false
  - Entity: 8711824436246927141
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [16.2292404, 288.519653, 0.464014918]
      Rotation: [0, 0, 0]
      Scale: [0.999880314, 0.999906898, 0.999875307]
    CameraComponent:
      Camera: some camera data...
      Primary: true
PhysicsLayers:
  []