### About
Cardinalis is a Vulkan renderer I work on during my spare time, written in C++, this is my platform to implement rendering techniques I saw or read about, prototype and try all kinds of optimizations, the main goal always being to have fun learning and experimenting.

### Features
Deferred Shading with Light Volumes <br/>
Normal Mapping <br/>
Cascaded Shadow Mapping <br/>
Image Based Lighting : Diffuse + Specular (Split Sum Approximation) <br/>
Physically Based Direct Lighting (Microfacet model, Cook-Torrance) <br/>
Volumetric Fog (Sunlight)

### Gallery
#### Bistro scene 
* Deferred shading with light volumes
* Physically based direct lighting, image based lighting
* Cascaded shadow mapping, volumetric fog (sunlight, point lights)

<img src="screenshots/bistro_fog.png" width="1024">
<img src="screenshots/bistro_volumetric_point_directional.png" width="1024">

#### Volumetric Sunlight in Sponza
![Sponza Volumetric Fog](screenshots/sponza_fog.png "Sponza Volumetric Fog") 
![Fog buffer](screenshots/sponza_fog_only.png "Fog buffer")


### References
[Normal Mapping Without Precomputed Tangents](http://www.thetenthplanet.de/archives/1180) <br/>
Volumetric Light Effects in Killzone: Shadow Fall, GPU Pro 5 <br/>
[Real Shading in Unreal Engine 4](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf) <br/>
[Cascaded Shadow Maps, DirectX Technical Articles](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps) <br/>
[The Rendering Technology of KILLZONE 2](https://www.gdcvault.com/play/1330/The-Rendering-Technology-of-KILLZONE) <br/>
[Compact Normal Storage for small G-Buffers](https://aras-p.info/texts/CompactNormalStorage.htm) <br/>
