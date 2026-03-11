Scene: Scene Name
Environment:
  AssetHandle: 11624192501257848883
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 11454923107030377596
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
      EnvironmentMap: 11624192501257848883
      Intensity: 1
      Angle: 0
  - Entity: 12984092959234517976
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Plane
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [8.92000008, 8.92000008, 8.92000008]
    MeshComponent:
      AssetID: 13380697606938407232
  - Entity: 8714298519950371179
    Parent: 0
    Children:
      []
    TagComponent:
      Tag: Cube
    TransformComponent:
      Position: [0, 1.4825381, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetID: 14364674889920913861
  - Entity: 15547206608783364829
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
PhysicsLayers:
  []