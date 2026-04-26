#version 460 core

out vec4 fragColor;


uniform mat4 u_InvProjection;
uniform mat4 u_InvView;

uniform bool u_LimbDarken;
uniform float u_SunScale;

uniform float u_MoonIntensity;

uniform vec3 u_SunDir;
uniform vec3 u_MoonDir;


uniform vec3 u_Observer;

in vec2 v_Pos;

const float PI = 3.14159265359;
const float Hr = 8000;
const float Hm = 1200;
const float Ho = 8000;

const vec3 rayleigh = vec3(5.8, 13.5, 33.1) * 1e-6;
const vec3 mie = vec3(21, 21, 21) * 1e-6;
const vec3 ozone = vec3(3.426, 8.298, 0.356) * 0.06 * 1e-5;
const float radiusEarth = 6360e3;
const float radiusAtmo = 6420e3;
const float ZenithH = radiusAtmo - radiusEarth;
float originH = length(u_Observer) - radiusEarth;
const int STEPS = 16;

const float earthShineIntensity = 1.291e2;



float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius);

float far = 100.0f;
float near = 0.1f;

vec3 get_world_view_ray(vec2 ndc)
{
// far plane(1)
    vec4 clip = vec4(ndc, 1.0, 1.0);

    vec4 viewPos = u_InvProjection * clip;
    viewPos /= viewPos.w;

    vec3 viewDir = normalize(viewPos.xyz);
    vec3 worldDir = (u_InvView * vec4(viewDir, 0.0)).xyz;

    return worldDir;
}



float rayleigh_phase(vec3 view_dir, vec3 lightDir) {
     float mu = dot(view_dir, lightDir);
    return (3.0 / (16.0 * PI)) * (1.f + mu * mu);
}

float mie_phase(vec3 view_dir, vec3 lightDir) {
    float mu = dot(view_dir, lightDir);
    float g = 0.76;
    float denom = 1.0 + g * g - 2.0 * g * mu;
    return (1.0 - g * g) / (4.0 * PI * pow(denom, 1.5));
}

vec3 sigma_s_rayleigh(vec3 position) {
     float h = length(position) - radiusEarth;
    return rayleigh * exp(-h / Hr);
}

vec3 sigma_s_mie(vec3 position) {
     float h = length(position) - radiusEarth;
    return mie * exp(-h / Hm);
}

vec3 sigma_a_ozone(vec3 position) {
     float h = length(position) - radiusEarth;
    return ozone * exp(-h / Ho);
}

vec3 sigma_t(vec3 position) {
    return sigma_s_rayleigh(position) + 
           1.11 * sigma_s_mie(position) +
           sigma_a_ozone(position);
}





vec3 integrate_sigma_t(vec3 from, vec3 to) {
     vec3 ds = (to - from) / float(STEPS);
    vec3 accumulation = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; ++i) {
         vec3 s = from + (i + 0.5) * ds;
        accumulation += sigma_t(s);
    }

    return accumulation * length(ds);
}

vec3 transmittance(vec3 from, vec3 to) {
     vec3 integral = integrate_sigma_t(from, to);
    return exp(-integral);
}




 float sunRad = radians(0.53 / 2.0) * u_SunScale;
 float sun_solid_angle = 2 * PI * (1 - cos(sunRad));

  float moonRad = radians(0.488 / 2.0) * u_SunScale;
 float moon_solid_angle = 2 * PI * (1 - cos(moonRad));

//vec3 TrZenith = transmittance(u_Observer, vec3(0, radiusEarth + ZenithH, 0));
 vec3 TrZenith = exp(-(rayleigh * Hr * (exp(-originH / Hr) - exp(-ZenithH / Hr)) +
                            mie * Hm * (exp(-originH / Hm) - exp(-ZenithH / Hm)) + 
                            ozone * Ho * (exp(-originH / Ho) - exp(-ZenithH / Ho))));



vec3 sunIlluminanceGround = vec3(2.0 * PI);
vec3 L_outerspace = (sunIlluminanceGround / sun_solid_angle) / TrZenith;

vec3 j_sun(vec3 position, vec3 view_dir, vec3 sunDir) {
     float distance_out_atmosphere =
        intersectRaySphereFromInside(position, sunDir, radiusAtmo);
     vec3 out_atmosphere = position + sunDir * distance_out_atmosphere;
    
     vec3 trToSun = transmittance(position, out_atmosphere);
     vec3 rayleigh_diffusion = sigma_s_rayleigh(position) * rayleigh_phase(view_dir, sunDir);
     vec3 mie_diffusion = sigma_s_mie(position) * mie_phase(view_dir, sunDir);
   
    return L_outerspace * trToSun * sun_solid_angle * (rayleigh_diffusion + mie_diffusion);
}

const float moonIrradianceInCdM2 = 8.970e5;
 vec3 j_moon(vec3 pos, vec3 view_dir, vec3 moonDir, vec3 sunDir) {
    
    const float C = 0.072;
    const float d = 3.844e8;
    const float rm = 1.737e6;

    float mPhase = PI - acos(clamp(dot(sunDir, moonDir), -1.0, 1.0));


 vec3 Esm = L_outerspace * sun_solid_angle;

 float phaseTerm = (1 - sin(mPhase/2.0) * tan (mPhase/2.0) * log(1.0/tan(mPhase/4)));


    float distToAtmos = intersectRaySphereFromInside(pos, moonDir, radiusAtmo);
    vec3 toMoonOutAtmos = pos + u_MoonDir * distToAtmos;


    vec3 rayleigh_diff = sigma_s_rayleigh(pos) * rayleigh_phase(view_dir, u_MoonDir);
    vec3 mie_diff = sigma_s_mie(pos) * mie_phase(view_dir, u_MoonDir);

    vec3 trToMoon = transmittance(pos, toMoonOutAtmos);
    
    // TODO: Add earthshine contribution
    vec3 moonIllumGround = vec3((2 * C * (rm * rm)/(3 * d * d) * Esm  * phaseTerm));
    vec3 L_moon = vec3((moonIllumGround / moon_solid_angle ) / TrZenith) ;
   return L_moon * trToMoon * moon_solid_angle * u_MoonIntensity * (rayleigh_diff + mie_diff);

}

vec3 compute_luminance(vec3 out_atmosphere, vec3 sunDir, vec3 moonDir) {
     vec3 ds = (out_atmosphere - u_Observer) / STEPS;
     vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < STEPS; ++i) {
         vec3 s = u_Observer + (i + 0.5) * ds;
         vec3 tr = transmittance(u_Observer, s);
        acc += tr * j_sun(s, direction, sunDir);
      acc += tr * j_moon(s, direction, moonDir, sunDir);
    }

    return acc * length(ds);
}

vec3 direct_light_from_sun(vec3 direction, vec3 out_atmosphere, vec3 sunDir) {
    float cos_theta = dot(direction, sunDir);

  
    float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(sunRad * 0.95, sunRad * 1.05, angle);
     
     if (!u_LimbDarken) {
        return disk * L_outerspace * transmittance(u_Observer, out_atmosphere);
     }

     // Limb darkening 
    float r = angle / sunRad;
    float mu = sqrt(max(0.0, 1.0 - r * r));    
    
    vec3 u = vec3(1.0);                           
    vec3 a = vec3(0.397, 0.503, 0.652);          
    
    vec3 limb_factor = 1.0 - u * (1.0 - pow(vec3(mu), a));
    return disk  * limb_factor * L_outerspace * transmittance(u_Observer, out_atmosphere); 
}



float intersectRaySphereFromOutside(vec3 rayOrigin, vec3 rayDir, float radius)
{
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0) return -1.0;
    return -b - sqrt(discriminant);
}

vec3 compute_brdf(float angToSun, float angToEarth, float moonPhase) {

    const float g = 0.6;

    float B;
    if (moonPhase >= PI/2.0) {
        B = 1.0;
    }
    else {
    float tanPhi = max(tan(moonPhase), 0.001);
        B = 2.0 - (tanPhi / (2.0 * g)) * (1.0 - exp(-g / tanPhi)) * (3.0 - exp(-g / tanPhi));
    }

    const float t = 0.1;
    const float S = (sin(abs(moonPhase)) + (PI - abs(moonPhase)) * cos(abs(moonPhase))) / PI + t * (1 - 0.5 * cos(abs(moonPhase))) * (1 - 0.5 * cos(abs(moonPhase)));

    float cos_i = max(cos(angToSun), 1e-6);
float cos_e = max(cos(angToEarth), 1e-6);
    return vec3(2/(3*PI) * B * S * (1/(1 + cos_e/cos_i)));
}

vec3 direct_light_from_moon(vec3 direction, vec3 out_atmosphere, vec3 sunDir, vec3 brdf) {
    float cos_theta = dot(direction, sunDir);

  
    float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(moonRad * 0.95, moonRad * 1.05, angle);
     
        return disk * vec3(0.07, 0.065, 0.06) * brdf * L_outerspace * transmittance(u_Observer, out_atmosphere) * 50.0;

}

void main(){
	     vec3 view_direction = get_world_view_ray(v_Pos);

     vec3 up = vec3(0.0, 1.0, 0.0);
     if (abs(dot(up, -u_MoonDir)) > 0.9999) {
     up = vec3(0.0, 0.0, 1.0);
     }

     vec3 tangent = cross(-u_MoonDir, up);
     vec3 biTangent = cross(-u_MoonDir, tangent);


     vec3 moonOffset = view_direction - u_MoonDir * dot(view_direction, u_MoonDir);

     vec3 moonNorm = normalize(tangent * moonOffset.x + biTangent * moonOffset.y + (-u_MoonDir) * sqrt(1 - moonRad * moonRad));

     float angMoonSun = acos(dot(u_SunDir, moonNorm));
     float angMoonEarth = acos(dot(-u_MoonDir, moonNorm));



     float earthPhase = acos(dot(u_SunDir, u_MoonDir));
     float moonPhase = PI - earthPhase;


     vec3 brdf = compute_brdf(angMoonSun, angMoonEarth, moonPhase);


     float earthShineLuminance = earthShineIntensity * 0.5 * (1 - sin(earthPhase/2) * tan(earthPhase/2) * log((1/tan(earthPhase/4))));

     float distOutToSun = intersectRaySphereFromInside(u_Observer, u_SunDir, radiusAtmo);
     float distOutToMoon = intersectRaySphereFromInside(u_Observer, u_MoonDir, radiusAtmo);

     vec3 sunPoint = u_Observer + u_SunDir * distOutToSun;
     vec3 moonPoint = u_Observer + u_MoonDir * distOutToMoon;

     float angleMoonToSun = atan((sunPoint.y - moonPoint.y) / (sunPoint.x - moonPoint.x));
     float distance_out =
        intersectRaySphereFromInside(u_Observer, view_direction, radiusAtmo);
     vec3 view_out = u_Observer + view_direction * distance_out;

    vec3 luminance = compute_luminance(view_out, u_SunDir, u_MoonDir);
    luminance += direct_light_from_sun(view_direction, view_out, u_SunDir);
   luminance += direct_light_from_moon(view_direction, view_out, u_MoonDir, brdf);
   luminance = luminance / (1.0f + luminance);
  // luminance = vec3(1.0f) - exp(-luminance * 40.0f);

//  float d = dot(u_Observer, view_direction);
//  if (d < 0.0) {
//    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
//    return;
//  }
//  if (d >= 0.0) {
//    fragColor = vec4(0.0, 0.0, 1.0, 1.0);
//    return;
//  }
    float earthHit = intersectRaySphereFromOutside(u_Observer, view_direction, radiusEarth);
if (earthHit > 0.0)
{
//    fragColor = vec4(0.02, 0.25, 0.45, 1.0);
    fragColor = vec4(0.02, 0.16, 0.05, 1.0);
    return;
}
fragColor = vec4(luminance, 1.0);
}

float intersectRaySphereFromInside(const vec3 rayOrigin,
                                   const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    float t = -b + sqrt(discriminant);
    return t;
}