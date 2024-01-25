# About
Cardinalis is a Vulkan renderer written in C++. <br/>

# Features
## Renderer
* Deferred Shading <br/>
  * with optimization rendering light volumes for directional and point lights 
* Normal Mapping <br/>
* Cascaded Shadow Mapping <br/>
* Image Based Lighting <br/>
  * Diffuse, Specular (Split Sum Approximation) prefiltering 
* PBR direct lighting <br/>
  * Metallic/Roughness material model
  * Cook-Torrance BRDF
* Volumetric Lighting
  * Dithered Ray-marched sunlight scattering
  * Point lights
### Post-FX
* Gaussian blur
  * Compute shader based two-pass gaussian blur using separable property of gaussian filters
## Application
* Gltf file loading
  * Supports KHR_lights_punctual

# Gallery
![Sponza Volumetric Fog](screenshots/sponza_fog.png "Sponza Volumetric Fog") 
<img src="screenshots/bistro_fog.png" width="1024">
<img src="screenshots/bistro_volumetric_point_directional.png" width="1024">
![Fog buffer](screenshots/sponza_fog_only.png "Fog buffer")

# References
[Normal Mapping Without Precomputed Tangents](http://www.thetenthplanet.de/archives/1180) <br/>
Volumetric Light Effects in Killzone: Shadow Fall, GPU Pro 5 <br/>
[Real Shading in Unreal Engine 4](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf) <br/>
[Cascaded Shadow Maps, DirectX Technical Articles](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps) <br/>
[The Rendering Technology of KILLZONE 2](https://www.gdcvault.com/play/1330/The-Rendering-Technology-of-KILLZONE) <br/>
[Compact Normal Storage for small G-Buffers](https://aras-p.info/texts/CompactNormalStorage.htm) <br/>
