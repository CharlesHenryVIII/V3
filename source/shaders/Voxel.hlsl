#include "GpuSharedData.h"

//**************
//VERTEX SHADER
//**************

struct VS_Output {
    float4 position : SV_POSITION;
};

struct VS_Input {
    float2 pos : POSITION;
};

//cbuffer CB_Vertex : register(b0){
    //matrix projection_from_view; //uniform mat4 u_projection_from_view;
    //matrix view_from_world; //uniform mat4 u_view_from_world;
//};

VS_Output Vertex_Main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.pos, 0, 1);//mul(projection_from_view, mul(view_from_world, float4(input.pos, 1.0)));
    return output;
}






//**************
//PIXEL SHADER
//**************

//Voxel index list
Texture3D<uint1> voxel_indices      TEXTURE_REGISTER(SLOT_VOXEL_INDICES);
sampler     voxel_indices_sampler   SAMPLER_REGISTER(SLOT_VOXEL_INDICES_SAMPLER);
//Random colors
Texture2D   random_texture          TEXTURE_REGISTER(SLOT_RANDOM_TEXTURE);
sampler     random_texture_sampler  SAMPLER_REGISTER(SLOT_RANDOM_TEXTURE_SAMPLER);

//struct VoxData {
//    float metalness;
//    float roughness;    //Surface Roughness:        Range from 0 to 100
//    float spec;         //Specular reflectivity:    Range from 0 to 100
//    float flux;         //Radiant flux:             Range from 1 to 5
//    float emit;
//    float ri;           //Refractive index:         Range from 1.00 to 3.00
//    float metal;        //metalness
//    uint color;
//};
StructuredBuffer<VoxMaterial> materials TEXTURE_REGISTER(SLOT_VOXEL_MATERIALS);

static const float FLT_INF     = 1.#INF;
static const float FLT_MAX     = 3.402823466e+38F;
static const float FLT_MIN     = 1.175494351e-38;
static const float FLT_EPSILON = 1.192092896e-07;
static const float pi = 3.14159;
static const float tau = 2 * pi;

int3 MagicaToTexelFetch(int3 a)
{
    //NOTE(CSH):  magica voxel to texelFetch translation:
    // x -> z
    // y -> x
    // z -> y
    // 9, 6, 3 -> 6, 3, 9
    return int3(a.y, a.z, a.x);
}

int3 GameVoxelToTexelFetch(int3 a)
{
    //To go from game voxel coordinates to texelFetch:
    // x -> z
    // y -> y
    // z -> x
    return int3(a.z, a.y, a.x);
}

uint GetIndexFromGameVoxelPosition(int3 p)
{
    int4 game_pos = 0;
    game_pos.xyz = GameVoxelToTexelFetch(p);
    uint voxel_index = voxel_indices.Load(game_pos);
    return voxel_index;
}

float4 GetColorFromIndex(uint i)
{
    uint pack = materials[i].color;
    float4 vc;
    vc.a = float((pack & 0xFF000000) >> 24) / 255;
    vc.b = float((pack & 0x00FF0000) >> 16) / 255;
    vc.g = float((pack & 0x0000FF00) >>  8) / 255;
    vc.r = float((pack & 0x000000FF)      ) / 255;
    return vc;
}

void PixelToRay(out float3 ray_origin, out float3 ray_direction, float2 pixel)
{
    ray_direction = 0;
    float x = (2.0 * pixel.x) / screen_size.x - 1.0;
    float y = 1.0 - (2.0 * pixel.y) / screen_size.y;
    float z = 1.0;
    float3 ray_nds = float3( x, y, z );
    float4 ray_clip = float4( ray_nds.xy, +1.0, 1.0 );

    //From Clip to View
    float4 ray_view = mul(view_from_projection, ray_clip);
    ray_view = float4( ray_view.xy, -1.0, 0.0 );

    //From View to World
    float3 ray_world = (mul(world_from_view, ray_view)).xyz;

    //Normalize
    float3 ray_world_n = normalize(ray_world);
    ray_origin      = camera_position;
    ray_direction   = ray_world_n;
}

int3 float3ToVoxelPosition(const float3 p)
{
    return int3(floor(p));
}

void Linecast(  out uint      raycast_color_index,
                out float3    raycast_p,
                out float     raycast_distance_mag,
                out float3    raycast_normal,
                const float3    ray_origin,
                const float3    ray_direction,
                const float     ray_length,
                const float3    normal)
{
    raycast_color_index = 0;
    raycast_p = 0;
    raycast_distance_mag = 0;
    raycast_normal = 0;

    float3 p = ray_origin;
    float3 step;
    step.x = ray_direction.x >= 0 ? 1.0 : -1.0;
    step.y = ray_direction.y >= 0 ? 1.0 : -1.0;
    step.z = ray_direction.z >= 0 ? 1.0 : -1.0;
    const float3 pClose = floor((round(ray_origin + (step / 2))));
    float3 tMax = abs((pClose - ray_origin) / ray_direction);
    const float3 tDelta = abs(1.0 / ray_direction);
    int3 voxel_p = int3(floor(p));

    if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
        return;
    if (voxel_p.x >= voxel_size.x || voxel_p.y >= voxel_size.y || voxel_p.z >= voxel_size.z)
        return;
    raycast_normal = normal;
    raycast_color_index = GetIndexFromGameVoxelPosition(voxel_p);


    while (raycast_color_index == 0)
    {
        if (distance(p, ray_origin) > ray_length)
        {
            return;
        }

        raycast_normal = 0;
        if (tMax.x <= tMax.y && tMax.x <= tMax.z)
        {
            p.x += step.x;
            tMax.x += tDelta.x;
            raycast_normal.x = -step.x;
        }
        else if (tMax.y <= tMax.x && tMax.y <= tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            raycast_normal.y = -step.y;
        }
        else
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            raycast_normal.z = -step.z;
        }

        voxel_p = float3ToVoxelPosition(p);
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            return;
        if (voxel_p.x >= voxel_size.x || voxel_p.y >= voxel_size.y || voxel_p.z >= voxel_size.z)
            return;
        raycast_color_index = GetIndexFromGameVoxelPosition(voxel_p);
    }

    const uint comp = (raycast_normal.x != 0.0 ? 0 : (raycast_normal.y != 0.0 ? 1 : 2));
    const float voxel = ray_direction[comp] < 0 ? float(voxel_p[comp]) + 1.0f : float(voxel_p[comp]);
    const float t = (voxel - ray_origin[comp]) / ray_direction[comp];
    raycast_p = ray_origin + ray_direction * t;

    raycast_distance_mag = t;
}

void RayVsAABB( out uint      raycast_color_index,
                out float3    raycast_p,
                out float     raycast_distance_mag,
                out float3    raycast_normal,
                const float3    ray_origin,
                const float3    ray_direction,
                const float3    box_min,
                const float3    box_max)
{
    raycast_color_index  = 0;
    raycast_p            = 0;
    raycast_distance_mag = 0;
    raycast_normal       = 0;
    float tmin = 0;
    float tmax = FLT_MAX;

    for (int slab = 0; slab < 3; ++slab)
    {
        if (abs(ray_direction[slab]) < FLT_EPSILON)
        {
            // Ray is parallel to the slab
            if (ray_origin[slab] < box_min[slab] || ray_origin[slab] > box_max[slab])
            {
                return;
            }
        }
        else
        {
            float ood = 1.0 / ray_direction[slab];
            float t1 = (box_min[slab] - ray_origin[slab]) * ood;
            float t2 = (box_max[slab] - ray_origin[slab]) * ood;
            if (t1 > t2)
            {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }
            tmin = max(tmin, t1);
            tmax = min(tmax, t2);
            if (tmin > tmax)
            {
                return;
            }
        }
    }
    raycast_p = ray_origin + ray_direction * tmin;
    raycast_distance_mag = tmin;

    const float3 normals[6] = {
        float3(  1.0,  0.0,  0.0 ),
        float3( -1.0,  0.0,  0.0 ),
        float3(  0.0,  1.0,  0.0 ),
        float3(  0.0, -1.0,  0.0 ),
        float3(  0.0,  0.0,  1.0 ),
        float3(  0.0,  0.0, -1.0 ),
    };

    float3 v[6];
    v[0] = float3( box_max.x,   raycast_p.y,    raycast_p.z );
    v[1] = float3( box_min.x,   raycast_p.y,    raycast_p.z );
    v[2] = float3( raycast_p.x, box_max.y,      raycast_p.z );
    v[3] = float3( raycast_p.x, box_min.y,      raycast_p.z );
    v[4] = float3( raycast_p.x, raycast_p.y,    box_max.z   );
    v[5] = float3( raycast_p.x, raycast_p.y,    box_min.z   );

    float currentDistance = 0;
    float ClosestDistance = FLT_INF;
    int closestFace = 0;
    for (int i = 0; i < 6; i++)
    {
        currentDistance = distance(raycast_p, v[i]);
        if (currentDistance < ClosestDistance)
        {
            ClosestDistance = currentDistance;
            closestFace = i;
        }
    }
    raycast_normal = normals[closestFace];

    raycast_color_index = 1;
}

void RayVsVoxel(out uint      raycast_color_index,
                out float3    raycast_p,
                out float     raycast_distance_mag,
                out float3    raycast_normal,
                const float3    ray_origin,
                const float3    ray_direction
            )
{
    raycast_color_index = 0;
    raycast_p = 0;
    raycast_normal = 0;
    raycast_distance_mag = 0;
    float3 box_min = 0;
    float3 box_max = voxel_size.xyz;
    uint    aabb_raycast_color_index = 0;
    float3  aabb_raycast_p = 0;
    float   aabb_raycast_distance_mag = 0;
    float3  aabb_raycast_normal = 0;
    RayVsAABB(  aabb_raycast_color_index,
                aabb_raycast_p,
                aabb_raycast_distance_mag,
                aabb_raycast_normal,
                ray_origin,
                ray_direction,
                box_min,
                box_max);
    //raycast_color_index = aabb_raycast_color_index;
    //return;


    if (aabb_raycast_color_index != 0)
    {
        float3 clamped_ray;
        //NOTE: Setting the clamped_ray to zero results in the voxel position to be off by 1.
        //May be some sort of floating point error...
        clamped_ray.x = abs(aabb_raycast_p.x) <= 0.0001 ? 0.0001 : aabb_raycast_p.x;
        clamped_ray.y = abs(aabb_raycast_p.y) <= 0.0001 ? 0.0001 : aabb_raycast_p.y;
        clamped_ray.z = abs(aabb_raycast_p.z) <= 0.0001 ? 0.0001 : aabb_raycast_p.z;

        clamped_ray.x = abs(clamped_ray.x - voxel_size.x) <= 0.0001 ? voxel_size.x - 0.00001 : clamped_ray.x;
        clamped_ray.y = abs(clamped_ray.y - voxel_size.y) <= 0.0001 ? voxel_size.y - 0.00001 : clamped_ray.y;
        clamped_ray.z = abs(clamped_ray.z - voxel_size.z) <= 0.0001 ? voxel_size.z - 0.00001 : clamped_ray.z;
        float3 linecast_ray_origin = clamped_ray;
        float3 linecast_ray_direction = ray_direction;
        Linecast(   raycast_color_index,
                    raycast_p,
                    raycast_distance_mag,
                    raycast_normal,
                    linecast_ray_origin,
                    linecast_ray_direction,
                    1000.0,
                    aabb_raycast_normal);
    }
}

uint PCG_Random(uint state)
{
    return uint((state ^ (state >> 11)) >> (11 + (state >> 30)));
}

float StackOverflow_Random(float2 co)
{
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

float3 Random_Texture(int depth, int sample_index, int2 pixel_position)
{
    float depth_scaled = float(depth * 3);
    float index_scaled = float(sample_index) * 2;
    float pixels_scaled = float(pixel_position.y * screen_size.x + pixel_position.x);
    uint i = uint(depth_scaled + index_scaled + pixels_scaled);
            /*(total_time * 10) + */
    float2 p;
    p.y = ((float(i) / float(random_texture_size.x)) % float(random_texture_size.y)) / float(random_texture_size.y);
    p.x = ((float(i) % float(random_texture_size.x))) / float(random_texture_size.x);

    float2 size = float2(random_texture_size.xy);
    float index = float(i);
    p.y = (index / size.x) / size.y;
    p.x = fmod(index, size.x) / size.x;

    //float4 color = texelFetch(random_texture, p, 0);
    //float4 color = random_texture.Sample(random_texture_sampler, p);
    float4 color = random_texture.SampleLevel(random_texture_sampler, p, 0);
    //return float3(p, 0);
    return color.rgb;
}

//    float3 emittance;
//    float3 inner;
//    Ray next_ray;
//    bool valid;

void PathTracing(   out float3    emittance,
                    out float3    inner,
                    out float3    next_ray_origin,
                    out float3    next_ray_direction,
                    out bool      valid,
                    float3 ray_origin,
                    float3 ray_direction,
                    int depth,
                    int max_depth,
                    int sample_index,
                    int2 pixel_position)
{
    const float reflectance = 0.4;
    emittance = 0;
    inner = 0;
    valid = 0;

    if (depth >= max_depth) {
        return;  // Bounced enough times.
    }

    uint    raycast_color_index;
    float3  raycast_p;
    float   raycast_distance_mag;
    float3  raycast_normal;

    RayVsVoxel( raycast_color_index,
                raycast_p,
                raycast_distance_mag,
                raycast_normal,
                ray_origin,
                ray_direction);

    if (raycast_color_index == 0)
    {
        return;  // Nothing was hit.
    }

    //Material material = ray.thingHit->material;
    float3 this_emittance = GetColorFromIndex(raycast_color_index).rgb;

    // Pick a random direction from here and keep going.
    float3 new_ray_origin = raycast_p;
    float3 new_ray_direction;

    // This is NOT a cosine-weighted distribution!
    float3 random_float3 = normalize(Random_Texture(depth, sample_index, pixel_position));
    if (dot(random_float3, raycast_normal) < 0)
    {
        random_float3 = normalize(-random_float3);
    }

    new_ray_direction = random_float3;

    // Probability of the newRay
    const float p = 1 / (tau);

    // Compute the BRDF for this ray (assuming Lambertian reflection)
    float cos_theta = dot(new_ray_direction, raycast_normal);
    float3 BRDF = reflectance / pi;

#if 0
//CANNOT DO RECURSION
    // Recursively trace reflected light sources.
    float3 incoming = PathTracing(new_ray, depth + 1, max_depth);

    // Apply the Rendering Equation here.
    return emittance + (BRDF * incoming * cos_theta / p);
#endif

    valid               = true;
    emittance           = this_emittance;
    inner               = (BRDF * cos_theta / p);
    next_ray_direction  = new_ray_direction;
    next_ray_origin     = new_ray_origin;
}

#define RAY_BASIC 0
#define RAY_BOUNCE 1
#define RAY_LIGHT 2
#define RAY_LIGHTS 3
#define RAY_LIGHT_DIR_DOT 4
#define RAY_PATH_TRACING 5
#define RAY_METHOD RAY_LIGHT_DIR_DOT

float4 Pixel_Main(VS_Output input) : SV_Target
{
    float4 color = 0;
    float3 ray_origin;
    float3 ray_direction;
    PixelToRay(ray_origin, ray_direction, input.position.xy);
    //color.r = input.position.x / 1280;
    //color.r = input.position.y / 720;
    //color.rg = input.position.xy / float2(1280, 720);
    //return color;
    //return float4(ray_direction.xyz, 1);
    //return float4(input.position.xy, 0, 1);

#if RAY_METHOD == RAY_BASIC
//use basic single ray hit


    RaycastResult ray_voxel_result = RayVsVoxel(ray);
    if (ray_voxel_result.color_index != 0)
    {

        float4 voxel_color = texelFetch(voxel_color_palette, int(ray_voxel_result.color_index), 0);
        color.xyz = voxel_color.xyz;
        //float3 dist = min(mod(ray_voxel.p, 1), 1.0 - mod(ray_voxel.p, 1));

        //NOTE: Draw voxel hit position
        //color.xyz = float3(ray_voxel.p / float3(voxel_size));

#if 0
        //NOTE: Draw Voxel Boundaries
        float min_dist;
        if (ray_voxel.normal.x != 0)
            min_dist = min(dist.y, dist.z);
        else if (ray_voxel.normal.y != 0)
            min_dist = min(dist.x, dist.z);
        else
            min_dist = min(dist.x, dist.y);
        color.xyz += 0.1 * float3(smoothstep(0.0, 0.1, min_dist));
#endif

        //NOTE: Weird Voxel Boundaries
        //color.xyz = float3(dist);

        //NOTE: Hit Position related to each voxel
        //color.xyz = mod(ray_voxel.p, 1.0);

        color.a = 1.0;
        //gl_FragDepth = 0;
    }
    else
    {
#if 1
        discard;
#else
        color = float4(0, 0, 0, 1);
#endif
    }

#elif RAY_METHOD == RAY_BOUNCE

#define RAY_BOUNCES 5
    RaycastResult voxel_rays[RAY_BOUNCES];
    for (int i = 0; i < RAY_BOUNCES; i++)
    {
        voxel_rays[i] = RayVsVoxel(ray);
        if (voxel_rays[i].color_index != 0)
        {
            ray.direction = reflect(ray.direction, voxel_rays[i].normal);
            ray.origin = voxel_rays[i].p + ray.direction * 0.001;
            //ray.origin = voxel_rays[i].p;
        }
        //else
        //{
        //    break;
        //}
    }
    color.a = 1;
    color.xyz = voxel_rays[0].normal;
    if (voxel_rays[0].color_index == 0)
        discard;
    const float4 ambient = float4(0.1, 0.1, 0.1, 1);
    float4 c = float4(0);
    c.a = 1.0;
    c = ambient;
    for (int i = RAY_BOUNCES - 1; i >= 0; i--)
    {
        if (voxel_rays[i].color_index != 0)
        {
            float4 voxel_color = texelFetch(voxel_color_palette, int(voxel_rays[i].color_index), 0);
#if 1
            c.r = c.r * voxel_color.r;
            c.g = c.g * voxel_color.g;
            c.b = c.b * voxel_color.b;
#else
            c.r = c.r * (voxel_color.r / (i + 1));
            c.g = c.g * (voxel_color.g / (i + 1));
            c.b = c.b * (voxel_color.b / (i + 1));
#endif
        }
        else
        {
            c = float4(1);
        }
    }
    //if (c == float4(1))
        //discard;
    color = c;//max(c, ambient);
    //color.xyz = abs(voxel_rays[0].normal);
    //color = float4(0, 0, 0, 1);
    //return;

    //if (ray_voxel.color_index != 0)
    //{
    //    float4 voxel_color = texelFetch(voxel_color_palette, int(ray_voxel.color_index), 0);
    //    color.xyz = voxel_color.xyz;
    //    color.a = 1.0;
    //    //gl_FragDepth = 0;
    //}
    //else
    //{
    //    discard;
    //}

#elif RAY_METHOD == RAY_LIGHT

    const float3 sun_position = float3(0, 50, 50);
    const float3 sun_color    = float3(1);
    const float3 ambient_color= float3(0.1);

    RaycastResult hit_voxel = RayVsVoxel(ray);
    if (hit_voxel.color_index != 0)
    {
        color.a = 1;
        const float4 hit_color    = GetColorFromIndex(hit_voxel.color_index);
        //get color from ray from sun
        Ray sun_ray;
        sun_ray.origin = sun_position;
        sun_ray.direction = normalize(hit_voxel.p - sun_position);
        RaycastResult sun_hit_voxel = RayVsVoxel(sun_ray);
        float3 difference = abs(hit_voxel.p - sun_hit_voxel.p);
        if (difference.x <= 0.001 &&
            difference.y <= 0.001 &&
            difference.z <= 0.001)
        {
            color.rgb = hit_color.rgb * sun_color;
        }
        else
        {
            color.rgb = hit_color.rgb * ambient_color;
        }
    }
    else
        discard;

#elif RAY_METHOD == RAY_LIGHTS

#define RAY_BOUNCES 1

struct PointColor {
    float3 color;
    float3 point;
    bool hit;
};

    const float3 sun_position = float3(0, 50, 50);
    const float3 sun_color    = float3(1);
    const float3 ambient_color= float3(0.1);

    const RaycastResult first_hit = RayVsVoxel(ray);
    if (first_hit.color_index == 0)
        discard;

    Ray previous_ray = ray;
    RaycastResult previous_raycast = first_hit;
    PointColor points_hit[RAY_BOUNCES];
    for (int i = 0; i < RAY_BOUNCES; i++)
    {
        Ray new_ray;
        new_ray.origin = previous_raycast.p;
        new_ray.direction = normalize(reflect(previous_ray.direction, previous_raycast.normal));
        RaycastResult ray_result = RayVsVoxel(new_ray);
        if (ray_result.color_index != 0)
        {
            PointColor a;
            a.color = GetColorFromIndex(ray_result.color_index).rgb;
            a.point = ray_result.p;
            a.hit   = true;
            points_hit[i] = a;
            previous_ray = new_ray;
            previous_raycast = ray_result;
        }
        else
        {
            for (i; i < RAY_BOUNCES; i++)
            {
                PointColor a;
                a.color = float3(0);
                a.point = float3(0);
                a.hit   = false;
                points_hit[i];
            }
        }
    }

    float3 final_bounce_color = float3(0);
    float3 previous_p         = first_hit.p;
    for (int i = 0; i < RAY_BOUNCES; i++)
    {
        if (!points_hit[i].hit)
            break;
        final_bounce_color += (1 / distance(previous_p, points_hit[i].point)) * points_hit[i].color * sun_color;
        previous_p = points_hit[i].point;
    }

    color.a = 1;
    const float4 hit_color = GetColorFromIndex(first_hit.color_index);
    Ray sun_ray;
    sun_ray.origin = sun_position;
    sun_ray.direction = normalize(first_hit.p - sun_position);
    RaycastResult sun_hit_voxel = RayVsVoxel(sun_ray);
    float3 difference = abs(first_hit.p - sun_hit_voxel.p);
    if (difference.x <= 0.001 &&
            difference.y <= 0.001 &&
            difference.z <= 0.001)
    {
        color.rgb = hit_color.rgb * sun_color;
    }
    else
    {
        color.rgb = hit_color.rgb * ambient_color;
    }

    if (points_hit[0].hit)
    {
        color.rgb = color.rgb;// + final_bounce_color;
    }



#elif RAY_METHOD == RAY_LIGHT_DIR_DOT

#define ENABLE_SHADOWS 1
#define RAY_BOUNCES 3
#define RAY_SAMPLES 4

    const float3 background_color   = float3(0.263, 0.706, 0.965);
    const float3 sun_position       = float3(0, 50, 50);
    const float3 sun_color          = float3(1, 1, 1);
    const float3 ambient_color      = 0.1;
    const float  roughness          = 0.1;

    float3 sun_ray_origin;
    float3 sun_ray_direction;
    sun_ray_origin = sun_position;
    color = 0;
    color.a = 1;
    float4 bounce_color = color;
    float4 sample_color = color;
    float bounce_color_strength = 1;
    float3 next_ray_origin = ray_origin;
    float3 next_ray_direction = ray_direction;
    for (int i = 0; i < RAY_SAMPLES; i++)
    {
        for (int j = 0; j < RAY_BOUNCES; j++)
        {
            uint    hit_voxel_color_index;
            float3  hit_voxel_p;
            float   hit_voxel_distance_mag;
            float3  hit_voxel_normal;
            RayVsVoxel(hit_voxel_color_index,
                hit_voxel_p,
                hit_voxel_distance_mag,
                hit_voxel_normal,
                next_ray_origin,
                next_ray_direction);
            //color.rgb = float3(float(hit_voxel_color_index) / 255, 0, 0);
            //color.rgb = float3(float(GetIndexFromGameVoxelPosition(int3(0, 0, 2))) / 255, 0, 0);
            //color.r = float(hit_voxel_color_index) / 1;
            //return color;

            //color.r = float(hit_voxel_color_index) / 255;
            //return color;
            const float4 hit_color = GetColorFromIndex(hit_voxel_color_index);
            //color.r = hit_color.a;
            //return color;
            if (hit_voxel_color_index == 0)
            {
                if (j == 0)
                {
                    discard;
                }
                else
                {
                    bounce_color.rgb += background_color * bounce_color_strength;
                    break;
                }
            }
            else if(j != 0 && hit_voxel_p.x < 1)
            {
                break;
            }
            //get color from ray from sun
            sun_ray_direction = normalize(hit_voxel_p - sun_position);
            uint    sun_hit_voxel_color_index;
            float3  sun_hit_voxel_p;
            float   sun_hit_voxel_distance_mag;
            float3  sun_hit_voxel_normal;
            RayVsVoxel(sun_hit_voxel_color_index,
                sun_hit_voxel_p,
                sun_hit_voxel_distance_mag,
                sun_hit_voxel_normal,
                sun_ray_origin,
                sun_ray_direction);

            float3 difference = abs(hit_voxel_p - sun_hit_voxel_p);
            float3 dir_to_sun = normalize(sun_position - hit_voxel_p);

#if 1
            //float3 random_float3 = normalize(Random_Texture(j, i, input.position.xy) * 2 - 1);
            float3 random_float3 = Random_Texture(j, i, input.position.xy);
            //color.rgb = random_float3;
            //return color;
            if (dot(random_float3, hit_voxel_normal) < 0)
            {
                random_float3 = -random_float3;
            }
#else
            float3 random_float3;
            random_float3.x = StackOverflow_Random(float2(hit_voxel_p.x + hit_voxel_p.y, hit_voxel_p.x + hit_voxel_p.z));
            random_float3.y = StackOverflow_Random(float2(hit_voxel_p.y + hit_voxel_p.z, hit_voxel_p.y + hit_voxel_p.x));
            random_float3.z = StackOverflow_Random(float2(hit_voxel_p.z + hit_voxel_p.x, hit_voxel_p.z + hit_voxel_p.y));
            random_float3 = clamp(random_float3, float3(-1.0), float3(1.0));
#endif


//TODO: shifted_normal is biased and needs to be improved
            float3 shifted_normal = normalize(hit_voxel_normal + (roughness * random_float3));
            float light_amount = dot(dir_to_sun, shifted_normal);
            light_amount = max(light_amount, 0);

#if ENABLE_SHADOWS
            if (difference.x > 0.001 &&
                    difference.y > 0.001 &&
                    difference.z > 0.001)
            {
                light_amount = 0.1;
            }
#endif

            bounce_color.rgb += hit_color.rgb * sun_color * light_amount * bounce_color_strength;
            bounce_color_strength = bounce_color_strength * 0.5;

            next_ray_direction = reflect(next_ray_direction, shifted_normal);

            next_ray_origin      = hit_voxel_p + next_ray_direction * 0.00001;
        }
        sample_color.rgb += bounce_color.rgb;
        bounce_color.rgb = 0;
        bounce_color_strength = 1;
        next_ray_origin = ray_origin;
        next_ray_direction = ray_direction;
    }
    color.rgb = sample_color.rgb / RAY_SAMPLES;
    return color;

#elif RAY_METHOD == RAY_PATH_TRACING

    const int samples   = 4;
    const int max_depth = 3;

    float4 _color = float4(0, 0, 0, 1);

    float3  results_emittance[max_depth];
    float3  results_inner[max_depth];
    float3  results_next_ray_origin[max_depth];
    float3  results_next_ray_direction[max_depth];
    bool    results_valid[max_depth];

    for (int i = 0; i < samples; i++)
    {
        for (int j = 0; j < max_depth; j++)
        {
            if (j == 0)
            {
                PathTracing(results_emittance[j],
                            results_inner[j],
                            results_next_ray_origin[j],
                            results_next_ray_direction[j],
                            results_valid[j],
                            ray_origin,
                            ray_direction,
                            0,
                            max_depth,
                            i,
                            input.position.xy);

                if (!results_valid[j])
                    discard;
            }
            else
            {
                PathTracing(results_emittance[j],
                            results_inner[j],
                            results_next_ray_origin[j],
                            results_next_ray_direction[j],
                            results_valid[j],
                            results_next_ray_origin[j - 1],
                            results_next_ray_direction[j - 1],
                            0,
                            max_depth,
                            i,
                            input.position.xy);

                if (!results_valid[j])
                    break;
            }
        }

        float3 temp_color = 0;
        for (int j = max_depth - 1; j >= 0; j--)
        {
            temp_color = results_emitance[j]+ (results_inner[j] * temp_color);
        }
        _color.rgb += temp_color;
    }
    _color.rgb /= samples;  // Average samples.
    color = _color;


    //PathTracingResult r;
    //r.emittance = emittance;
    //r.inner = (BRDF * cos_theta / p);
    //r.next_ray = new_ray;

#endif

    return color;





#if 0 //Getting texel fetch to work with the indices
    int3 voxel_pos = int3(6, 3, 9);
    voxel_pos = GameVoxelToTexelFetch(int3(9, 3, 10));
    //voxel_pos = MagicaToTexelFetch(int3(9, 10, 3));

    uint4 voxel_index = texelFetch(voxel_indices,         voxel_pos,          0);
    if (voxel_index.r == 0)
        discard;
    float4 voxel_color = texelFetch(voxel_color_palette,  int(voxel_index.r),  0);
    color.xyz = voxel_color.xyz;
    color.a = 1.0;
#endif
}