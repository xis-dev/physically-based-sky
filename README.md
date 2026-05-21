Real-time physically based atmosphere and celestial rendering with accurate ephemeris positions for the Sun and Moon.


# Dependencies:
GLAD
GLFW
GLM 
IMGUI

All dependencies are handled locally or are automatically installed via CMake.

# Limitations:

- Single Scattering
- Atmospheric light only considers sun and moon.
- Cloud lighting only considers sun.

# Overview:
1. Participating media:
    - At some point in a volume, the light received by an observer goes through Extinction events ( Scattering & Absorption), changing how it is perceived, this perceived light at the point is further "changed" by a special in-scattering event, the case where light is scattered back into the view of the observer, the probability of this is gained from the Phase Function.

# Implementation:
1. Atmosphere: From the viewing direction, we gain the intersection point at the edge of the atmosphere, 
                  <img width="432" height="432" alt="Earth_Atmos" src="https://github.com/user-attachments/assets/a83b7c3c-7a26-42cb-b213-0c4d1210205e" />



# References:
Meeus, Jean: Astronomical Algorithms
Willmann-Bell; Hardcover (1st ed. 1991), ISBN: 0943396352

https://indico.cern.ch/event/1458227/contributions/6524456/attachments/3068549/5564316/ACP2025_Mavyline.pdf

https://graphics.stanford.edu/~henrik/papers/nightsky/nightsky.pdf

https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf

https://cpp-rendering.io/sky-and-atmosphere-rendering/


