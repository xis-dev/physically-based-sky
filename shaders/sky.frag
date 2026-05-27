#version 460 core

out vec4 fragColor;


uniform mat4 u_InvProjection;
uniform mat4 u_InvView;

uniform vec3 u_SunDir;
uniform vec3 u_MoonDir;

uniform float u_Time;

uniform vec3 u_Observer;

uniform bool u_Clouds;

in vec2 v_Pos;

const float PI = 3.14159265359;
// Rayleigh, mie and ozone density falloff factors
const float Hr = 8000;
const float Hm = 1200;
const float Ho = 8000;

// Extinction coefficients
const vec3 rayleigh = vec3(5.8, 13.5, 33.1) * 1e-6;
const vec3 mie = vec3(21, 21, 21) * 1e-6;
const vec3 ozone = vec3(3.426, 8.298, 0.356) * 0.06 * 1e-5;

// Earth consstants
const float radiusEarth = 6360e3;
const float radiusAtmo = 6420e3;
const float ZenithH = radiusAtmo - radiusEarth;
const vec3 ZenithDir = normalize(u_Observer);
float originH = length(u_Observer) - radiusEarth;

// Cloud constant
const float phaseBlend = 0.6;
const float cloudExtinctionCoeff = 0.9;
const float cloudBottom = radiusEarth + 4000.0;
const float cloudTop = radiusEarth + 6000.0;

const int STEPS = 8;



vec3 getWorldViewRay(vec2 ndc)
{
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

vec3 computeTransmittance(vec3 from, vec3 to) {
    vec3 integral = integrate_sigma_t(from, to);
    return exp(-integral);
}

float intersectRaySphereFromInside(const vec3 rayOrigin,
const vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    float t = -b + sqrt(discriminant);
    return t;
}

float intersectRaySphereFromOutside(vec3 rayOrigin, vec3 rayDir, float radius)
{
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0) return -1.0;
    return -b - sqrt(discriminant);
}

// Scale factor so bodies appear larger
const float sizeScale = 5.0f;
float sunRad = radians(0.53 / 2.0) * sizeScale;
float sun_solid_angle = 2 * PI * (1 - cos(sunRad));

float moonRad = radians(0.488 / 2.0) * sizeScale;
float moon_solid_angle = 2 * PI * (1 - cos(moonRad));

vec3 TrZenith = exp(-(rayleigh * Hr * (exp(-originH / Hr) - exp(-ZenithH / Hr)) +
mie * Hm * (exp(-originH / Hm) - exp(-ZenithH / Hm)) +
ozone * Ho * (exp(-originH / Ho) - exp(-ZenithH / Ho))));

vec3 ACESFitted(vec3 color)
{
    mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
    );

    mat3 ACESOutputMat = mat3(
    1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
    );

    color = color * ACESInputMat;

    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;

    color = color * ACESOutputMat;

    color = clamp(color, 0.0, 1.0);

    return color;
}

vec3 sunIlluminanceGround = vec3(2. * PI);
vec3 L_outerspace = (sunIlluminanceGround / sun_solid_angle) / TrZenith;

vec3 j_sun(vec3 position, vec3 view_dir, vec3 sunDir) {
    float distance_out_atmosphere =
    intersectRaySphereFromInside(position, sunDir, radiusAtmo);
    vec3 out_atmosphere = position + sunDir * distance_out_atmosphere;

    vec3 trToSun = computeTransmittance(position, out_atmosphere);

    // Phase functions to sun
    vec3 rayleigh_diffusion = sigma_s_rayleigh(position) * rayleigh_phase(view_dir, sunDir);
    vec3 mie_diffusion = sigma_s_mie(position) * mie_phase(view_dir, sunDir);

    return L_outerspace * trToSun * sun_solid_angle * (rayleigh_diffusion + mie_diffusion);
}

vec3 moonLum;
vec3 j_moon(vec3 pos, vec3 view_dir, vec3 moonDir, vec3 sunDir) {

    const float C = 0.072;
    const float d = 3.844e8;
    const float rm = 1.737e6;

    float mPhase = PI - acos(clamp(dot(sunDir, moonDir), -1.0, 1.0));

    // Irradiance from sun
    vec3 Esm = L_outerspace * sun_solid_angle;

    float phaseTerm = (1 - sin(mPhase/2.0) * tan (mPhase/2.0) * log(1.0/tan(mPhase/4)));


    float distOut = intersectRaySphereFromInside(pos, moonDir, radiusAtmo);
    vec3 toMoonOutAtmos = pos + moonDir * distOut;


    vec3 rayleigh_diff = sigma_s_rayleigh(pos) * rayleigh_phase(view_dir, moonDir);
    vec3 mie_diff = sigma_s_mie(pos) * mie_phase(view_dir, moonDir);

    vec3 trToMoon = computeTransmittance(pos, toMoonOutAtmos);

    // TODO: Add earthshine contribution
    vec3 moonIllumGround = vec3((2 * C * (rm * rm)/(3 * d * d) * Esm  * phaseTerm));
    vec3 L_moon = vec3((moonIllumGround / moon_solid_angle ) / TrZenith) ;
    moonLum = L_moon;
    // Moon intensity boost for clearer nights
    const float moonIntensity = 9999.0f;
    return L_moon * trToMoon * moon_solid_angle * moonIntensity * (rayleigh_diff + mie_diff);

}

vec3 j_zenith(vec3 pos) {
    float distOut = intersectRaySphereFromInside(pos, normalize(u_Observer), radiusAtmo);
    vec3 tr = computeTransmittance(pos, normalize(pos) * distOut);
    vec3 ambientLight = vec3(0.23, 0.23, 0.23);

    vec3 zenithTr = exp(-(rayleigh * Hr * (exp(-(length(pos) - radiusEarth) / Hr) - exp(-ZenithH / Hr)) +
    mie * Hm * (exp(-(length(pos) - radiusEarth) / Hm) - exp(-ZenithH / Hm)) +
    ozone * Ho * (exp(-(length(pos) - radiusEarth) / Ho) - exp(-ZenithH / Ho))));

    return tr * L_outerspace * sun_solid_angle;
}

vec3 compute_luminance(vec3 out_atmosphere, vec3 sunDir, vec3 moonDir) {

    vec3 ds = (out_atmosphere - u_Observer) / STEPS;
    vec3 direction = normalize(ds);
    vec3 acc = vec3(0.0);

    for (int i = 0; i < STEPS; ++i) {
        vec3 s = u_Observer + (i + 0.5) * ds;
        vec3 tr = computeTransmittance(u_Observer, s);
        // Accumulate lighting from moon, sun and sun as if it were at zenith but scaled down to serve as ambient lighting
        acc += tr * (j_sun(s, direction, sunDir) + j_moon(s, direction, moonDir, sunDir) + j_sun(s, direction, ZenithDir) / 7.0 );

    }

    return acc * length(ds) ;
}

vec3 direct_light_from_sun(vec3 direction, vec3 out_atmosphere, vec3 sunDir) {
    float cos_theta = dot(direction, sunDir);


    float angle = acos(cos_theta);
    float disk = 1.0 - smoothstep(sunRad * 0.95, sunRad * 1.05, angle);


    // Limb darkening
    float r = angle / sunRad;
    float mu = sqrt(max(0.0, 1.0 - r * r));

    vec3 u = vec3(1.0);
    vec3 a = vec3(0.397, 0.503, 0.652);

    vec3 limb_factor = 1.0 - u * (1.0 - pow(vec3(mu), a));
    return disk  * limb_factor * L_outerspace * computeTransmittance(u_Observer, out_atmosphere);
}





vec3 computeBrdf(vec3 viewDir) {

    vec3 up = vec3(0.0, 1.0, 0.0);
    if (abs(dot(up, -u_MoonDir)) > 0.9999) {
        up = vec3(0.0, 0.0, 1.0);
    }

    // Form coordinate space from moon to get surface normal
    vec3 tangent = cross(-u_MoonDir, up);
    vec3 biTangent = cross(-u_MoonDir, tangent);

    vec3 moonOffset = viewDir - u_MoonDir * dot(viewDir, u_MoonDir);

    vec3 moonNorm = normalize(tangent * moonOffset.x + biTangent * moonOffset.y + (-u_MoonDir) * sqrt(1 - moonRad * moonRad));

    float angToSun = acos(dot(u_SunDir, moonNorm));
    float angToEarth = acos(dot(-u_MoonDir, moonNorm));


    float earthPhase = acos(dot(u_SunDir, u_MoonDir));
    float moonPhase = PI - earthPhase;

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

    return disk * vec3(0.07, 0.065, 0.06) * brdf * L_outerspace * computeTransmittance(u_Observer, out_atmosphere);

}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash(i), hash(i+vec3(1,0,0)), f.x),
            mix(hash(i+vec3(0,1,0)), hash(i+vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i+vec3(0,0,1)), hash(i+vec3(1,0,1)), f.x),
            mix(hash(i+vec3(0,1,1)), hash(i+vec3(1,1,1)), f.x), f.y),
        f.z);
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = .5;
    float freq = 4.0;
    int oct = 4;
    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p);
        p *=freq;
        amplitude *= 0.5;
    }
    return value;
}


uniform bool dither;

float cloudDensity(vec3 pos) {
    float height = length(pos);

    if (height < cloudBottom || height > cloudTop) return 0.0;

    float heightFraction = (height - cloudBottom) / (cloudTop - cloudBottom);
    float heightGradient = sin(heightFraction * PI);


    vec3 samplePos = pos * ((cloudTop - cloudBottom) / radiusAtmo) + vec3(u_Time * 0.1, u_Time * 0.07, u_Time * 0.03);
    float density = fbm(samplePos) * heightGradient;

    const float densityCutoff = 0.6;
    density = max(0.0, density - densityCutoff);


    return density;

}

float computeLightTransmittance(vec3 pos, vec3 sunDir, float stepSize) {

    const int steps = 8;
//    float blueNoise = (texture(u_BlueDither, gl_FragCoord.xy/470.0)).r;
//    float offset = (fract(blueNoise));
//    //float t = stepSize * blueNoise ;
    float totalDensity = (0.0);
    for (int i = 0; i < steps; ++i) {
        vec3 p = pos + float(i + 0.5) * sunDir * stepSize;
        totalDensity += cloudDensity(p);
        //t += stepSize;
    }

    return exp(-totalDensity * stepSize * cloudExtinctionCoeff);
}

vec3 computeAmbience(vec3 sunDir, vec3 moonDir) {
    vec3 zenith = normalize(u_Observer);

    float dist = intersectRaySphereFromInside(u_Observer, zenith, radiusAtmo);

    vec3 zenithOut = u_Observer + zenith * dist;

    vec3 ambient = compute_luminance(zenithOut, sunDir, moonDir);

    return ambient;
}

vec2 intersectCloudLayer(vec3 origin, vec3 dir, float innerRadius, float outerRadius) {
    float inner = intersectRaySphereFromOutside(origin, dir, innerRadius);
    float outer = intersectRaySphereFromInside(origin, dir, outerRadius);

    if (inner < 0.0) {
        return vec2(outer < 0.0 ? -1.0 : 0.0, outer);
    }
    return vec2(inner, outer);
}


vec4 marchClouds(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, vec3 moonDir) {

    vec2 tBounds = intersectCloudLayer(rayOrigin, rayDir, cloudBottom, cloudTop);

    if (tBounds.x < 0.0 || tBounds.y < tBounds.x) return vec4(0.0);

    float tStart = tBounds.x;
    float tEnd = tBounds.y;

    const int STEPS = 384;
    float stepSize = (tEnd - tStart) / float(STEPS);

    float totalDist = tEnd - tStart;
    float transmittance = 1.0;
    vec3  scatteredLight = vec3(0.0);

    const float g1 = 0.6;
    const float g2 = -0.6;
    float phaseAngle;
    phaseAngle = acos(dot(-rayDir, sunDir));
    float phaseTerm1 = (1.0 - g1*g1) / (4.0 * PI * pow(1.0 + g1*g1 - 2.0*g1*cos(phaseAngle), 1.5));
    float phaseTerm2 = (1.0 - g2*g2) / (4.0 * PI * pow(1.0 + g2*g2 - 2.0*g2*cos(phaseAngle), 1.5));

    const float phaseBlend = 0.6;
    float phaseTerm = mix(phaseTerm1, phaseTerm2, phaseBlend);


    float _dist = 0.0;
    for (int i = 0; i < STEPS; i++) {

        vec3 pos = rayOrigin + rayDir * (tStart + (float(i) + 0.5) * stepSize);
        float density = cloudDensity(pos);

        if (density > 0.001) {

            vec3 zenithAtPos = normalize(pos);
            float height = length(pos) - radiusEarth;
            float dotProd_sun = (abs(dot(zenithAtPos, sunDir)));

            vec3 luminance;
            vec3 ambientCloud = vec3(0.11, 0.15, 0.2);
            dotProd_sun = max(dotProd_sun, 0.0001);
//            vec3 opticalDepth_sun = (rayleigh * Hr * (exp(-height / Hr) - exp(-(ZenithH) / Hr))/ dotProd_sun) +
//            (mie * Hm * (exp(-height / Hm) - exp(-(ZenithH ) / Hm))/ dotProd_sun) +
//            (ozone * Ho * (exp(-height / Ho) - exp(-(ZenithH) / Ho))/ dotProd_sun);
//
//            vec3 atmosTrans_sun = exp(-opticalDepth_sun);
            vec3 atmosTrans    = computeTransmittance(u_Observer, pos);
            vec3 lightAtmosTrans;
            vec3 lightSource;
            float lightTransmittance;

                float distToSunOut = intersectRaySphereFromInside(pos, sunDir, radiusAtmo);
                lightAtmosTrans  = computeTransmittance(pos, pos + sunDir * distToSunOut);
                lightSource = L_outerspace * sun_solid_angle * lightAtmosTrans ;
                lightTransmittance = computeLightTransmittance(pos, sunDir, 200.0);


            luminance = lightSource * lightTransmittance * phaseTerm + ambientCloud;

            ///  vec3 integScat = (luminance - luminance * stepTransmittance) / extinction;

            float sigma_t = density * cloudExtinctionCoeff;
            float stepAlpha = 1.0 - exp(-sigma_t * stepSize);
            scatteredLight += transmittance * luminance * stepAlpha;
            transmittance *= (1.0 - stepAlpha);
        }

        if (transmittance < 0.01) break;


    }

    vec3 atmosToCloud = computeTransmittance(u_Observer, rayOrigin + rayDir * tStart);
    return vec4(scatteredLight * atmosToCloud, 1.0 - transmittance);
}

void main(){

    vec3 viewDirection = getWorldViewRay(v_Pos);

    float earthHit = intersectRaySphereFromOutside(u_Observer, viewDirection, radiusEarth);
    if (earthHit > 0.0)
    {
        fragColor = vec4(0.02, 0.16, 0.05, 1.0);
        return;
    }

    vec3 brdf = computeBrdf(viewDirection);


    float distOutToSun = intersectRaySphereFromInside(u_Observer, u_SunDir, radiusAtmo);
    float distOutToMoon = intersectRaySphereFromInside(u_Observer, u_MoonDir, radiusAtmo);

    float distance_out =
    intersectRaySphereFromInside(u_Observer, viewDirection, radiusAtmo);
    vec3 view_out = u_Observer + viewDirection * distance_out;

    vec3 luminance = compute_luminance(view_out, u_SunDir, u_MoonDir);
   // luminance += computeAmbience(u_SunDir, u_MoonDir);
    luminance += direct_light_from_sun(viewDirection, view_out, u_SunDir);
    luminance += direct_light_from_moon(viewDirection, view_out, u_MoonDir, brdf);

 //   float blueNoise = (texture(u_BlueDither, gl_FragCoord.xy/470.0)).r;
    //   float offset = fract(blueNoise);
  //  float offset = fract(blueNoise + float(u_Frame%32) / sqrt(0.5));
    vec3 result = luminance;
    if (u_Clouds) {
        vec4 clouds = marchClouds(u_Observer, viewDirection, u_SunDir, u_MoonDir);
        result = mix(luminance, clouds.rgb, clouds.a);
    }

    // tone mapping
    result = ACESFitted(result);
    result = pow(result, vec3(1./2.2));
    fragColor = vec4(result, 1.0);

}

