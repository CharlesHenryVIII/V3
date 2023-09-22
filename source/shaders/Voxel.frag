#version 460
//Voxel index list
layout (binding = 0) uniform usampler3D voxel_indices;
//Color Palette for index list
layout (binding = 1) uniform sampler1D  voxel_color_palette;

out vec4 color;

uniform mat4    u_view_from_projection;
uniform mat4    u_world_from_view;
uniform ivec2   u_screen_size;
uniform ivec3   u_voxel_size;
uniform vec3    u_camera_position;

const float FLT_INF     = 1.0 / 0.0;
const float FLT_MIN     = 1.175494351e-38;
const float FLT_EPSILON = 1.192092896e-07;
//const float FLT_EPSILON = 0.01;

ivec3 MagicaToTexelFetch(ivec3 a)
{
    //NOTE(CSH):  magica voxel to texelFetch translation:
    // x -> z 
    // y -> x
    // z -> y
    // 9, 6, 3 -> 6, 3, 9
    return ivec3(a.y, a.z, a.x);
}

ivec3 GameVoxelToTexelFetch(ivec3 a)
{
    //To go from game voxel coordinates to texelFetch:
    // x -> z
    // y -> y
    // z -> x
    return ivec3(a.z, a.y, a.x);
}

uint GetIndexFromGameVoxelPosition(ivec3 p)
{
    uvec4 voxel_index = texelFetch(voxel_indices, GameVoxelToTexelFetch(p), 0);
    return voxel_index.r;
}

vec4 GetColorFromIndex(uint i)
{
    vec4 vc = texelFetch(voxel_color_palette, int(i), 0);
    return vc;
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

Ray PixelToRay(ivec2 pixel)
{
    float x = (2.0 * pixel.x) / u_screen_size.x - 1.0;
    float y = 1.0 - (2.0 * (u_screen_size.y - pixel.y)) / u_screen_size.y;;
    float z = 1.0;
    vec3 ray_nds = vec3( x, y, z );
    vec4 ray_clip = vec4( ray_nds.xy, +1.0, 1.0 );

    //From Clip to View
    vec4 ray_view = u_view_from_projection * ray_clip;
    ray_view = vec4( ray_view.xy, -1.0, 0.0 );

    //From View to World
    vec3 ray_world = (u_world_from_view * ray_view).xyz;

    //Normalize
    vec3 ray_world_n = normalize(ray_world);
    Ray ray;
    ray.origin      = u_camera_position;
    ray.direction   = ray_world_n;
    return ray;
}

struct RaycastResult {
    uint color_index;
    vec3 p;
    float distance_mag;
    vec3 normal;
};

ivec3 Vec3ToVoxelPosition(const vec3 p)
{
    return ivec3(floor(p));
}

RaycastResult Linecast(const Ray ray, float ray_length, vec3 normal)
{
    RaycastResult result;
    result.color_index  = 0;
    result.p            = vec3(0);
    result.distance_mag = 0;
    result.normal       = vec3(0);


    vec3 p = ray.origin;
    vec3 step;
    step.x = ray.direction.x >= 0 ? 1.0 : -1.0;
    step.y = ray.direction.y >= 0 ? 1.0 : -1.0;
    step.z = ray.direction.z >= 0 ? 1.0 : -1.0;
    const vec3 pClose = floor((round(ray.origin + (step / 2))));
    vec3 tMax = abs((pClose - ray.origin) / ray.direction);
    const vec3 tDelta = abs(1.0 / ray.direction);
    ivec3 voxel_p = ivec3(floor(p));

    if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
        return result;
    if (voxel_p.x >= u_voxel_size.x || voxel_p.y >= u_voxel_size.y || voxel_p.z >= u_voxel_size.z)
        return result;
    result.normal = normal;
    result.color_index = GetIndexFromGameVoxelPosition(voxel_p);

    
    while (result.color_index == 0)
    {
        if (distance(p, ray.origin) > ray_length)
        {
            return result;
        }

        result.normal = vec3(0);
        if (tMax.x <= tMax.y && tMax.x <= tMax.z)
        {
            p.x += step.x;
            tMax.x += tDelta.x;
            result.normal.x = -step.x;
        }
        else if (tMax.y <= tMax.x && tMax.y <= tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            result.normal.y = -step.y;
        }
        else 
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            result.normal.z = -step.z;
        }

        voxel_p = Vec3ToVoxelPosition(p);
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            return result;
        if (voxel_p.x >= u_voxel_size.x || voxel_p.y >= u_voxel_size.y || voxel_p.z >= u_voxel_size.z)
            return result;
        result.color_index = GetIndexFromGameVoxelPosition(voxel_p);
    }
    
    const uint comp = (result.normal.x != 0.0 ? 0 : (result.normal.y != 0.0 ? 1 : 2));
    const float voxel = ray.direction[comp] < 0 ? float(voxel_p[comp]) + 1.0f : float(voxel_p[comp]);
    const float t = (voxel - ray.origin[comp]) / ray.direction[comp];
    result.p = ray.origin + ray.direction * t;

    result.distance_mag = t;
    return result;
}

struct AABB {
    vec3 min;
    vec3 max;
};


RaycastResult RayVsAABB(const Ray ray, const AABB box)
{
    RaycastResult r;
    r.color_index  = 0;
    r.p            = vec3(0);
    r.distance_mag = 0;
    r.normal       = vec3(0);
    float tmin = 0;
    float tmax = FLT_INF;

    for (int slab = 0; slab < 3; ++slab)
    {
        if (abs(ray.direction[slab]) < FLT_EPSILON)
        {
            // Ray is parallel to the slab
            if (ray.origin[slab] < box.min[slab] || ray.origin[slab] > box.max[slab])
                return r;
        }
        else
        {
            float ood = 1.0 / ray.direction[slab];
            float t1 = (box.min[slab] - ray.origin[slab]) * ood;
            float t2 = (box.max[slab] - ray.origin[slab]) * ood;
            if (t1 > t2)
            {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }
            tmin = max(tmin, t1);
            tmax = min(tmax, t2);
            if (tmin > tmax)
                return r;
        }
    }
    r.p = ray.origin + ray.direction * tmin;
    r.distance_mag = tmin;

    const vec3 normals[6] = {
        vec3(  1.0,  0.0,  0.0 ),
        vec3( -1.0,  0.0,  0.0 ),
        vec3(  0.0,  1.0,  0.0 ),
        vec3(  0.0, -1.0,  0.0 ),
        vec3(  0.0,  0.0,  1.0 ),
        vec3(  0.0,  0.0, -1.0 ),
    };

    vec3 v[6];
    v[0] = vec3( box.max.x, r.p.y,      r.p.z );
    v[1] = vec3( box.min.x, r.p.y,      r.p.z );
    v[2] = vec3( r.p.x,     box.max.y,  r.p.z );
    v[3] = vec3( r.p.x,     box.min.y,  r.p.z );
    v[4] = vec3( r.p.x,     r.p.y,      box.max.z );
    v[5] = vec3( r.p.x,     r.p.y,      box.min.z );

    float currentDistance = 0;
    float ClosestDistance = FLT_INF;
    int closestFace = 0;
    for (int i = 0; i < 6; i++)
    {
        currentDistance = distance(r.p, v[i]);
        if (currentDistance < ClosestDistance)
        {
            ClosestDistance = currentDistance;
            closestFace = i;
        }
    }
    r.normal = normals[closestFace];

    r.color_index = 1;
    return r;
}

RaycastResult RayVsVoxel(const Ray ray)
{
    AABB box = AABB(vec3(0), vec3(u_voxel_size.x, u_voxel_size.y, u_voxel_size.z ));
    RaycastResult aabb_result = RayVsAABB(ray, box);

    RaycastResult linecast_result;
    linecast_result.color_index = 0;

    if (aabb_result.color_index != 0)
    {
        vec3 clamped_ray;
        //NOTE: Setting the clamped_ray to zero results in the voxel position to be off by 1.
        //May be some sort of floating point error...
        clamped_ray.x = abs(aabb_result.p.x) <= 0.0001 ? 0.0001 : aabb_result.p.x;
        clamped_ray.y = abs(aabb_result.p.y) <= 0.0001 ? 0.0001 : aabb_result.p.y;
        clamped_ray.z = abs(aabb_result.p.z) <= 0.0001 ? 0.0001 : aabb_result.p.z;

        clamped_ray.x = abs(clamped_ray.x - u_voxel_size.x) <= 0.0001 ? u_voxel_size.x - 0.00001 : clamped_ray.x;
        clamped_ray.y = abs(clamped_ray.y - u_voxel_size.y) <= 0.0001 ? u_voxel_size.y - 0.00001 : clamped_ray.y;
        clamped_ray.z = abs(clamped_ray.z - u_voxel_size.z) <= 0.0001 ? u_voxel_size.z - 0.00001 : clamped_ray.z;
        Ray linecast_ray = Ray( clamped_ray, ray.direction );
        linecast_result = Linecast(linecast_ray, 1000.0, aabb_result.normal);
    }
    return linecast_result;
}

uint PCG_Random(uint state)
{
    return uint((state ^ (state >> 11)) >> (11 + (state >> 30)));
}

float StackOverflow_Random(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

#define RAY_BASIC 0
#define RAY_BOUNCE 1
#define RAY_LIGHT 2
#define RAY_LIGHTS 3
#define RAY_LIGHT_DIR_DOT 4
#define RAY_METHOD RAY_LIGHT_DIR_DOT

void main()
{

    Ray ray = PixelToRay(ivec2(gl_FragCoord.xy));

#if RAY_METHOD == RAY_BASIC
//use basic single ray hit


    RaycastResult ray_voxel_result = RayVsVoxel(ray);
    if (ray_voxel_result.color_index != 0)
    {

        vec4 voxel_color = texelFetch(voxel_color_palette, int(ray_voxel_result.color_index), 0);
        color.xyz = voxel_color.xyz;
        //vec3 dist = min(mod(ray_voxel.p, 1), 1.0 - mod(ray_voxel.p, 1));

        //NOTE: Draw voxel hit position
        //color.xyz = vec3(ray_voxel.p / vec3(u_voxel_size));

#if 0
        //NOTE: Draw Voxel Boundaries
        float min_dist;
        if (ray_voxel.normal.x != 0)
            min_dist = min(dist.y, dist.z);
        else if (ray_voxel.normal.y != 0)
            min_dist = min(dist.x, dist.z);
        else
            min_dist = min(dist.x, dist.y);
        color.xyz += 0.1 * vec3(smoothstep(0.0, 0.1, min_dist));
#endif

        //NOTE: Weird Voxel Boundaries
        //color.xyz = vec3(dist);

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
        color = vec4(0, 0, 0, 1);
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
    const vec4 ambient = vec4(0.1, 0.1, 0.1, 1);
    vec4 c = vec4(0);
    c.a = 1.0;
    c = ambient;
    for (int i = RAY_BOUNCES - 1; i >= 0; i--)
    {
        if (voxel_rays[i].color_index != 0)
        {
            vec4 voxel_color = texelFetch(voxel_color_palette, int(voxel_rays[i].color_index), 0);
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
            c = vec4(1);
        }
    }
    //if (c == vec4(1))
        //discard;
    color = c;//max(c, ambient);
    //color.xyz = abs(voxel_rays[0].normal);
    //color = vec4(0, 0, 0, 1);
    //return;

    //if (ray_voxel.color_index != 0)
    //{
    //    vec4 voxel_color = texelFetch(voxel_color_palette, int(ray_voxel.color_index), 0);
    //    color.xyz = voxel_color.xyz;
    //    color.a = 1.0;
    //    //gl_FragDepth = 0;
    //}
    //else
    //{
    //    discard;
    //}

#elif RAY_METHOD == RAY_LIGHT

    const vec3 sun_position = vec3(0, 50, 50);
    const vec3 sun_color    = vec3(1);
    const vec3 ambient_color= vec3(0.1);

    RaycastResult hit_voxel = RayVsVoxel(ray);
    if (hit_voxel.color_index != 0)
    {
        color.a = 1;
        const vec4 hit_color    = GetColorFromIndex(hit_voxel.color_index);
        //get color from ray from sun
        Ray sun_ray;
        sun_ray.origin = sun_position;
        sun_ray.direction = normalize(hit_voxel.p - sun_position);
        RaycastResult sun_hit_voxel = RayVsVoxel(sun_ray);
        vec3 difference = abs(hit_voxel.p - sun_hit_voxel.p);
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
    vec3 color;
    vec3 point;
    bool hit;
};

    const vec3 sun_position = vec3(0, 50, 50);
    const vec3 sun_color    = vec3(1);
    const vec3 ambient_color= vec3(0.1);

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
                a.color = vec3(0);
                a.point = vec3(0);
                a.hit   = false;
                points_hit[i];
            }
        }
    }

    vec3 final_bounce_color = vec3(0);
    vec3 previous_p         = first_hit.p;
    for (int i = 0; i < RAY_BOUNCES; i++)
    {
        if (!points_hit[i].hit)
            break;
        final_bounce_color += (1 / distance(previous_p, points_hit[i].point)) * points_hit[i].color * sun_color;
        previous_p = points_hit[i].point;
    }

    color.a = 1;
    const vec4 hit_color = GetColorFromIndex(first_hit.color_index);
    Ray sun_ray;
    sun_ray.origin = sun_position;
    sun_ray.direction = normalize(first_hit.p - sun_position);
    RaycastResult sun_hit_voxel = RayVsVoxel(sun_ray);
    vec3 difference = abs(first_hit.p - sun_hit_voxel.p);
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
#define RAY_BOUNCES 2

    const vec3 background_color = vec3(0.263, 0.706, 0.965);
    const vec3 sun_position = vec3(0, 50, 50);
    const vec3 sun_color    = vec3(1, 1, 1);
    const vec3 ambient_color= vec3(0.1);
    const float roughness   = 0.5;


    bool hit = false;
    Ray sun_ray;
    sun_ray.origin = sun_position;
    color = vec4(0);
    color.a = 1;
    float bounce_color_strength = 1;

    for (int i = 0; i < RAY_BOUNCES; i++)
    {
        RaycastResult hit_voxel = RayVsVoxel(ray);
        const vec4 hit_color = GetColorFromIndex(hit_voxel.color_index);
        if (hit_voxel.color_index == 0)
        {
            color.rgb += background_color * bounce_color_strength;
            break;
        }
        else if(i != 0 && hit_voxel.p.x < 1)
        {
            break;
        }
        hit = true;
        //get color from ray from sun
        sun_ray.direction = normalize(hit_voxel.p - sun_position);
        RaycastResult sun_hit_voxel = RayVsVoxel(sun_ray);
        vec3 difference = abs(hit_voxel.p - sun_hit_voxel.p);

        vec3 random_vec3;
        random_vec3.x = StackOverflow_Random(vec2(hit_voxel.p.x + hit_voxel.p.y, hit_voxel.p.x + hit_voxel.p.z));
        random_vec3.y = StackOverflow_Random(vec2(hit_voxel.p.y + hit_voxel.p.z, hit_voxel.p.y + hit_voxel.p.x));
        random_vec3.z = StackOverflow_Random(vec2(hit_voxel.p.z + hit_voxel.p.x, hit_voxel.p.z + hit_voxel.p.y));
        random_vec3 = clamp(random_vec3, vec3(-0.5), vec3(0.5));

        vec3 dir_to_sun = normalize(sun_position - hit_voxel.p);
        vec3 shifted_normal = normalize(hit_voxel.normal + (roughness * random_vec3));
        //color.rgb = (shifted_normal + 1) / 2;
        //break;
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

        color.rgb += hit_color.rgb * sun_color * light_amount * bounce_color_strength;
        bounce_color_strength = bounce_color_strength * 0.3;

        ray.direction = reflect(ray.direction, shifted_normal);

        ray.origin      = hit_voxel.p + ray.direction * 0.00001;

        //color.rgb = ray.origin / 40;
        //break;

    }
    if (!hit)
        discard;

#endif






#if 0 //Getting texel fetch to work with the indices
    ivec3 voxel_pos = ivec3(6, 3, 9);
    voxel_pos = GameVoxelToTexelFetch(ivec3(9, 3, 10));
    //voxel_pos = MagicaToTexelFetch(ivec3(9, 10, 3));

    uvec4 voxel_index = texelFetch(voxel_indices,         voxel_pos,          0);
    if (voxel_index.r == 0)
        discard;
    vec4 voxel_color = texelFetch(voxel_color_palette,  int(voxel_index.r),  0);
    color.xyz = voxel_color.xyz;
    color.a = 1.0;
#endif
}
