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
## Atmosphere:
1. Participating media:\
    At some point in a volume, the light received by an observer goes through Extinction ( $&sigma;_t$ ) events ( **Out-Scattering ($&sigma;_s$) & Absorption($&sigma;_a$)**), changing how it is perceived, this perceived light at the point is further "changed" by a special in-scattering event, the case where light is scattered back into the view of the observer, the probability of this is described by the **Phase Function**.

2. Transmittance:\
    Describes of light to be transmitted through the atmosphere, the higher the distance, the less light will travel through the medium.

    _Tr_($x_{a}$, $x_{b}) = exp(- \int_{x = x_{a}}^{x_{b}} \ $&sigma;<sub>t</sub>(x)_dt_)
    
3. Phase function:\
    Function of in-scattering, the phase function describes the probability of light being scattered into the viewing direction.
4. Extinction events in atmosphere:\
   - Rayleigh Scattering: Main scattering event responsible for the blue or yellow-orangish colours depending on the direction of the solar rays. It describes the scattering of light by particles smaller than it's wavelength. The rayleigh scattering distribution is given as e<sup>{-h / 8000 _km_}</sup> and at sea level it's coefficients are (5.8e<sup>-6</sup>, 1.35e<sup>-5</sup>, 3.31e<sup>-5</sup>).\ The phase function of rayleigh scattering is given as:
   $p(\theta)=\frac{3}{16\pi}(1 + \cos^2(\theta))$.
   - Mie scaterring:\ Describes scattering by particles roughly the same size, such as dust. The mie distribution is given as e<sup>{-h / 1200 _km_}</sup> and at sea level it's scattering coefficient ($&sigma;_s$) is $\ge$ 2e<sup>-6</sup>. Mie has little absorption, overall giving; $&sigma;_t$ = 1.11 * $&sigma;_s$(x). \ The phase function for mie scattering is given as:
   $p(\theta) = \frac{1 – g^2}{4\pi \left(1 + g^2 – 2g \cos\theta \right)^{1.5}}$
   
   - Ozone absorption: Ozone is a gas in the atmosphere, absorbing a lot of green and red wavelengths. The distribution is given as e<sup>{-h / 8000 _km_}</sup> and at sea level it's absorption coefficients are (3.426, 8.298, 0.356) * 0.06 * 10<sup>-5</sup>⁢𝑚.
  
## Sun & Moon Positions:
1. Coordinate systems: 
    All positions are modelled in their respective spherical coordinate systems (Latitude & Longitude for earth position, Azimuth & Altitude for celestial bodies). For the direction for fragment shader, conversion to cartesian coordinates are performed.
    ```
                                        dir.x = cos(altitude) * sin(azimuth)
		                                dir.y = sin(altitude)
			                            dir.z = cos(altitude) * cos(azimuth)
    ```
2. Time Of Day: To keep a consistent time of day regardless of location, there is no consideration for standard meridian time, a constant solar time for the sun and the moon is modelled by a consistent julian day update based on how long the simulation has been running.

3. Sun Position:
    Described based on hour angle, solar declination and observer's latitude position.
   
	declination = 0.006918 - 0.399912 * cos(g) + 0.070257 * sin(g) - 0.006758 * cos(2 * g) + 0.000907 * sin(2 * g) - 0.002697 * cos(3 * g) + 0.001480 * sin(3 * g);
   	altitude = sin(lat) * sin(declination) + cos(lat) * cos(declination) * cos(hourAngle);
    altitude = sin(altitude);
    azimuth  = atan2((sin(hourAngle)), (sin(lat) * cos(hourAngle) - tan(dec) * cos(lat)));


 
   
# Implementation:
1. Atmosphere: From the viewing direction, we gain the intersection point at the edge of the atmosphere.
                  <img width="432" height="432" alt="Earth_Atmos" src="https://github.com/user-attachments/assets/a83b7c3c-7a26-42cb-b213-0c4d1210205e" />\
   The transmittance to that point is obtained, evaluating the previously described extinction events that happen in the atmosphere.  

```
vec3 compute_luminance(vec3 out_atmosphere, vec3 sunDir, vec3 moonDir) {

    // Get step size
    vec3 ds = (out_atmosphere - u_Observer) / STEPS;
    vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < STEPS; ++i) {
        vec3 s = u_Observer + (i + 0.5) * ds;
        vec3 tr = computeTransmittance(u_Observer, s);
        // Accumulate lighting from moon and sun
        acc += tr * (j_sun(s, direction, sunDir) + (j_moon(s, direction, moonDir, sunDir)) + (j_sun(s, direction, normalize(u_Observer))/3.0));
    }

    return acc * length(ds);
}
```
Due to the sun and moon being the only light sources, an extra reduced sunlight term from zenith as ambient light (though this adds more expense and is not realistic or physically based).

# References:
Meeus, Jean: Astronomical Algorithms
Willmann-Bell; Hardcover (1st ed. 1991), ISBN: 0943396352

https://indico.cern.ch/event/1458227/contributions/6524456/attachments/3068549/5564316/ACP2025_Mavyline.pdf

https://graphics.stanford.edu/~henrik/papers/nightsky/nightsky.pdf

https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf

https://cpp-rendering.io/sky-and-atmosphere-rendering/


