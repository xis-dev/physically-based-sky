# Real-time Physically Based Sky
Real-time physically based atmosphere and celestial rendering with accurate ephemeris positions for the Sun and Moon.

<img width="1591" height="897" alt="image" src="https://github.com/user-attachments/assets/0a01652c-b6f7-4c13-8032-6ca865d03ce0" />


## Dependencies:
GLAD

GLFW

GLM 

IMGUI

All dependencies are handled locally or are automatically installed via CMake.

## Limitations:

- Single Scattering
- Atmospheric light only considers sun and moon.
- Cloud lighting only considers sun.

## Overview:

### Atmosphere

Light travelling through the atmosphere undergoes **extinction** events (out-scattering ($\sigma_s$) and absorption ($\sigma_a$)), described as the extinction coefficient $\sigma_t = \sigma_s + \sigma_a$.
The fraction of light surviving a path through the medium is given by the **transmittance**:

$$Tr(x_a, x_b) = \exp\left(-\int_{x_a}^{x_b} \sigma_t(x)dt\right)$$

There is the more special case of in-scattering, which is the probability of light being scattered back into the viewing direction and it is described by the **phase function** $p(\theta)$.

Three different extinction events are modelled:

- **Rayleigh scattering**: This scattering is caused by particles smaller than the 
wavelength of the light. Responsible for the blue sky and orange/red sunsets. 
Density fall off is given as $e^{-h/8000}$. Its phase function is given as:

$$p(\theta) = \frac{3}{16\pi}(1 + \cos^2\theta)$$

- **Mie scattering**: This scattering is caused by larger particles such as aerosols and 
dust. Density fall off is given as $e^{-h/1200}$, Mie contains little absorption thus its extinction $\sigma_t = 1.11\sigma_s$. Its phase function is given as:

$$p(\theta) = \frac{1 - g^2}{4\pi(1 + g^2 - 2g\cos\theta)^{1.5}}$$

- **Ozone absorption**: Ozone is a gas in the atmosphere, absorbing a lot of green and red wavelengths, contributing to the deeper blue of the upper sky. Follows the same 
density falloff as **Rayleigh**.

<img width="1594" height="893" alt="image" src="https://github.com/user-attachments/assets/412ef9f0-9bdc-4c75-9071-8d248dfce660" />




---
### Sun & Moon Positions

- **Coordinate System**: Both celestial bodies are positioned in spherical/horizontal coordinates (altitude and azimuth) derived from the observer's latitude, longitude, and the current Julian Day.
Directions are converted to Cartesian for the fragment shader.

- **Time Of Day**: Due to the nature of the project, the time of the day is solely computed in solar time for the sun and Julian Day + time elapsed for the moon.

- **Sun**: Its altitude and azimuth are gotten from the hour angle and solar declination using **Spencer**'s formula.

- **Moon**: Follows a more complex method due to orbital perturbations such as eccentricity and node regression. Its position derived from **Meeus**' algorithm, computing the moon's ecliptic longitude and latitude, converting to equatorial coordinates (right ascension and declination), and then to local horizontal coordinates.

---

### Clouds

Volumetric clouds are rendered via raymarching through a shell in the atmosphere between 4-6 km height. The density at each sample point is evaluated using FBM noise with a height gradient. Lighting accounts for atmospheric transmittance from the sample point to the sun and cloud shadowing by another light transmittance march towards the sun. Clouds follow a more stylized approach.

<img width="1597" height="898" alt="image" src="https://github.com/user-attachments/assets/dfd7ad87-56ea-47a8-b2d5-2c4a1f599137" />

---

### Sun

Rendered as a disk of its luminance, limb darkening achieved through **Neckel**'s algorithm. Due to the limitations of the renderer, its ground-illuminance at zenith is given as $2\pi$.
<img width="1593" height="901" alt="image" src="https://github.com/user-attachments/assets/65ab7c53-e34b-46ff-b9ca-00a9a7a14573" />


### Moon

The moon uses the **Hapke–Lommel–Seeliger** reflectance model for its appearance and lack of limb darkening. Illuminance from the moon is derived from the **Lommel–Seeliger** law, accounting for the moon's phase angle, albedo, and distance.

<img width="1592" height="900" alt="image" src="https://github.com/user-attachments/assets/bf9b37ef-932e-4754-a9fe-e60eb4d5c1c2" />

---

## Future Considerations:
- Multi-scattering.
- Use of LUTs.
- Star rendering.
- Better noise implementation & sampling.
- Increased physically based light sources; Airglow, Stars, Ground Reflection, Earthshine, Cosmic Light, etc.

## References:
Meeus, J. (1991). Astronomical algorithms. Willmann-Bell. 

Motari, M., Okeng’o, G., Ndegwa1, R., & Ali, R. L. (n.d.). An improved solar declination angle formula. https://indico.cern.ch/event/1458227/contributions/6524456/attachments/3068549/5564316/ACP2025_Mavyline.pdf 

Jensen, H. W., Durand, F., Stark, M. M., Premoze, S., Dorsey, J., & Shirley, P. (n.d.). A physically-based Nightsky model. http://graphics.stanford.edu/~henrik/papers/nightsky/ 

Hillaire, S. (n.d.). Physically based sky, atmosphere and cloud rendering ... https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf 

Morrier, A. (2025, May 11). Sky and atmosphere rendering: A physical approach. CPP Rendering - Antoine MORRIER. https://cpp-rendering.io/sky-and-atmosphere-rendering/ 

Hill, S. (n.d.). BakingLab/BakingLab/ACES.hlsl at master · TheRealMJP/bakinglab. GitHub. https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl  

Neckel, H. On the wavelength dependency of solar limb darkening (λλ303 to 1099 nm). Sol Phys 167, 9–23 (1996). https://doi.org/10.1007/BF00146325


