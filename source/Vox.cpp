#include "Vox.h"
#include "WinInterop.h"
#include "Math.h"
#include "Misc.h"

#include "SDL.h"

#include <unordered_map>

typedef std::unordered_map<std::string, std::string> Dict;

Vec3 faceNormals[+Face::Count] = {

{  1.0f,  0.0f,  0.0f },
{ -1.0f,  0.0f,  0.0f },
{  0.0f,  1.0f,  0.0f },
{  0.0f, -1.0f,  0.0f },
{  0.0f,  0.0f,  1.0f },
{  0.0f,  0.0f, -1.0f },
};

struct VoxFileData {
    const u8*   data;
    u64         i;
    u64         max;
};

template <typename T>
T GetDataAndIncrement(VoxFileData& v)
{
    T result = *((T*)(&(v.data[v.i])));
    v.i += sizeof(T);
    return result;
}

void GetStringDataAndIncriment(std::string& out, VoxFileData& v)
{
    i32 string_length = GetDataAndIncrement<i32>(v);
    out.resize(string_length);
    for (i32 i = 0; i < string_length; i++)
    {
        out[i] = GetDataAndIncrement<i8>(v);
    }
}

void ReadDictData(Dict& dict, VoxFileData& v)
{
    i32 dictSize = GetDataAndIncrement<i32>(v);
    for (i32 i = 0; i < dictSize; i++)
    {
        std::string key, val;
        GetStringDataAndIncriment(key, v);
        GetStringDataAndIncriment(val, v);
        assert(dict.find(key) == dict.end());
        dict[key] = val;
    }
}

void FourCCToString(std::string& result, u32 FCC)
{
    result = "    ";
    u8* data = (u8*)&FCC;
    for (i32 i = 0; i < sizeof(u32); i++)
    {
        result[i] = (char)data[i];
    }
}

#pragma pack(push, 1)
struct VoxChunk
{
    u8 x;
    u8 y;
    u8 z;
    u8 colorIndex;
};
struct ChunkHeader
{
    u32 FCCID;          //chunk id
    i32 numChunks;      //number of bytes of chunk content
    i32 numChildren;    //number of bytes of child chunks

    std::string FCCAsString()
    {
        std::string mainHeaderString;
        FourCCToString(mainHeaderString, FCCID);
        return mainHeaderString;
    }

};
#pragma pack(pop)
struct MaterialProperties
{
    std::string type; //_diffuse, _metal, _glass, _emit
    float weight;   // ????
    float rough;    //Surface Roughness:        Range from 0 to 100
    float spec;     //Specular reflectivity:    Range from 0 to 100
    float ior;      // ????
    float att;      // ????
    float flux;     //Radiant flux:             Range from 1 to 5
    float emit;
    float ri;       //Refractive index:         Range from 1.00 to 3.00
    float d;        // ????
    float metal;    //metalness
    std::string plastic; //is this in use?
};

#pragma pack(push, 1)
struct LAYR {
    i32     layer_id;
    Dict    dict; //layer attribute;
    i32     reserved_id; //must be -1
};
struct TransformInfo {
    i8 rotation;
    Vec3I translation;
    i32 frame_index;
};
struct nTRN { //transform node chunk
    Dict    node_attributes;
    i32     child_node;
    i32     reserved_id; //must be -1
    i32     layer_id;
    i32     num_of_frames;
    std::vector<TransformInfo> frame_transforms;
};
struct nGRP {
    Dict    node_attributes;
    std::vector<i32> child_nodes;
};
struct nSHP {
    Dict    node_attributes;
    std::unordered_map<i32, Dict> model_attributes;
};
#pragma pack(pop)

ColorInt default_palette[VOXEL_PALETTE_MAX] = {
	0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
    0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
    0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
    0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
    0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
    0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
    0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
    0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
    0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

void GetValueFromDict(std::string& out, Dict& d, const std::string& key)
{
    if (d.find(key) != d.end())
        out = d[key];
}
void GetValueFromDict(float& out, Dict& d, const std::string& key)
{
    if (d.find(key) != d.end())
        out = (float)atof(d[key].c_str());
}
void GetValueFromDict(i8& out, Dict& d, const std::string& key)
{
    if (d.find(key) != d.end())
        out = atoi(d[key].c_str());
    //out = 0xFF & atoi(d[key].c_str());
}
void GetValueFromDict(i32& out, Dict& d, const std::string& key)
{
    if (d.find(key) != d.end())
        out = atoi(d[key].c_str());
}
void GetValueFromDict(Vec3I& out, Dict& d, const std::string& key)
{
    if (d.find(key) != d.end())
    {
        const std::string& s = d[key];
        i32 i = 0;
        for (; i < s.size() && s[i] != ' '; i++) { }
        out.x = atoi(s.substr(0, i).c_str());

        i32 j = i + 1;
        for (; j < s.size() && s[j] != ' '; j++) { }
        out.y = atoi(s.substr(i + 1, j).c_str());
        
        i32 k = j + 1;
        for (; k < s.size() && s[k] != ' '; k++) { }
        out.z = atoi(s.substr(j + 1, k).c_str());
    }
}

const u32 FCCVox  = SDL_FOURCC('V', 'O', 'X', ' ');
const u32 FCCMain = SDL_FOURCC('M', 'A', 'I', 'N');
const u32 FCCPack = SDL_FOURCC('P', 'A', 'C', 'K');
const u32 FCCSize = SDL_FOURCC('S', 'I', 'Z', 'E');
const u32 FCCXYZI = SDL_FOURCC('X', 'Y', 'Z', 'I');
const u32 FCCnTRN = SDL_FOURCC('n', 'T', 'R', 'N');
const u32 FCCnGRP = SDL_FOURCC('n', 'G', 'R', 'P');
const u32 FCCnSHP = SDL_FOURCC('n', 'S', 'H', 'P');
const u32 FCCMATL = SDL_FOURCC('M', 'A', 'T', 'L');
const u32 FCCLAYR = SDL_FOURCC('L', 'A', 'Y', 'R');
const u32 FCCrOBJ = SDL_FOURCC('r', 'O', 'B', 'J');
const u32 FCCrCAM = SDL_FOURCC('r', 'C', 'A', 'M');
const u32 FCCNOTE = SDL_FOURCC('N', 'O', 'T', 'E');
const u32 FCCIMAP = SDL_FOURCC('I', 'M', 'A', 'P');
const u32 FCCRGBA = SDL_FOURCC('R', 'G', 'B', 'A');

struct VoxChunkAttributes {
    nTRN transforms;
    nGRP group_node_chunk;
    nSHP shape_node_chunk;
};
struct Vox {
    Vec3I size;
    std::vector<VoxelBlockData>                 color_indices;
    std::unordered_map<i32, VoxChunkAttributes> vox_chunks;
    std::unordered_map<i32, MaterialProperties> materials;
    std::vector<LAYR>                           layer_dicts;
    U32Pack                                     color_palette[VOXEL_PALETTE_MAX];
};


bool LoadVoxFile_MyImplimentation(VoxData& out, const std::string& filePath)
{
    static_assert(sizeof(VoxChunk)          == 4);
    static_assert(sizeof(ChunkHeader)       == 12);
    static_assert(sizeof(VoxChunk)          == 4);
    static_assert(sizeof(ColorInt::rgba)    == sizeof(u32));

    File file(filePath, File::Mode::Read, false);
    VALIDATE_V(file.m_handleIsValid, false);

    file.GetData();
    VALIDATE_V(file.m_binaryDataIsValid, false);
    VoxFileData v = {
        .data = file.m_dataBinary.data(),
        .i = 0,
        .max = file.m_dataBinary.size(),
    };
    VALIDATE_V(v.data,  false);
    VALIDATE_V(v.max,   false);

    i32 firstFCC = GetDataAndIncrement<i32>(v);
    VALIDATE_V(FCCVox == firstFCC, false);
    i32 version = GetDataAndIncrement<i32>(v);
    //assert(version == 150); //uh oh we are on version 200
    Vox vox;
    i32 numOfModels = 0;  //only used in animations
    i32 color_count = 0;
    while (v.data)
    {
        if (v.i >= v.max)
            break;
        ChunkHeader header = GetDataAndIncrement<ChunkHeader>(v);
        std::string headerString = header.FCCAsString();

        // process the chunk.
        switch (header.FCCID)
        {
        case FCCMain:
            break;
        case FCCPack:
        {
            numOfModels = GetDataAndIncrement<i32>(v); //dont need
            break;
        }
        case FCCSize:
        {
            VALIDATE_V(header.numChunks == 12 && header.numChildren == 0, false);
            vox.size.x = GetDataAndIncrement<i32>(v);
            vox.size.y = GetDataAndIncrement<i32>(v);
            vox.size.z = GetDataAndIncrement<i32>(v);
            VALIDATE_V(vox.size.x <= VOXEL_MAX_SIZE, false);
            VALIDATE_V(vox.size.y <= VOXEL_MAX_SIZE, false);
            VALIDATE_V(vox.size.z <= VOXEL_MAX_SIZE, false);
            break;
        }
        case FCCXYZI:
        {
            i32 numVoxels = GetDataAndIncrement<i32>(v);
            vox.color_indices.push_back({});
            VoxChunk vc = {};
            for (i32 i = 0; i < numVoxels; i++)
            {
                vc = GetDataAndIncrement<VoxChunk>(v); //color index is off by one
                VALIDATE_V(vc.colorIndex > 0, false);
                vox.color_indices[vox.color_indices.size() - 1].e[vc.x][vc.z][vc.y] = vc.colorIndex;
            }

            break;
        }
        case FCCnTRN:
        {
            i32 node_id     = GetDataAndIncrement<i32>(v);
            nTRN n;
            ReadDictData(n.node_attributes, v);
            n.child_node    = GetDataAndIncrement<i32>(v);
            n.reserved_id   = GetDataAndIncrement<i32>(v);
            n.layer_id      = GetDataAndIncrement<i32>(v);
            n.num_of_frames = GetDataAndIncrement<i32>(v);
            VALIDATE_V(n.reserved_id == -1, false);
            VALIDATE_V(n.num_of_frames > 0, false);

            n.frame_transforms.resize(n.num_of_frames);
            for (i32 i = 0; i < n.num_of_frames; i++)
            {
                Dict d;
                ReadDictData(d, v);
                if (d.empty())
                    break;
                //FAIL;
                //TODO: the translation will be incorrect but i do not know how the data is formatted
                TransformInfo t;
                GetValueFromDict(t.rotation,    d, "_r");
                GetValueFromDict(t.translation, d, "_t");
                GetValueFromDict(t.frame_index, d, "_f");
                n.frame_transforms[i] = t;
            }
            vox.vox_chunks[node_id].transforms = n;
            break;
        }
        case FCCnGRP:
        {
            i32 node_id = GetDataAndIncrement<i32>(v);
            nGRP n;
            ReadDictData(n.node_attributes, v);
            i32 num_child_nodes = GetDataAndIncrement<i32>(v);
            n.child_nodes.reserve(num_child_nodes);
            for (i32 i = 0; i < num_child_nodes; i++)
            {
                n.child_nodes.push_back(GetDataAndIncrement<i32>(v));
                //dont need this?
            }
            vox.vox_chunks[node_id].group_node_chunk = n;

            break;
        }
        case FCCnSHP:
        {
            i32 node_id = GetDataAndIncrement<i32>(v);
            nSHP n;
            n.model_attributes;
            ReadDictData(n.node_attributes, v);
            i32 numChildNodes = GetDataAndIncrement<i32>(v);
            for (i32 i = 0; i < numChildNodes; i++)
            {
                i32 child_id = GetDataAndIncrement<i32>(v);
                Dict d;
                ReadDictData(d, v);
                n.model_attributes[child_id] = d;
            }
            vox.vox_chunks[node_id].shape_node_chunk = n;
            break;
        }
        case FCCMATL:
        {
            i32 material_id = GetDataAndIncrement<i32>(v);
            Dict d;
            ReadDictData(d, v);
            MaterialProperties m = {};

            GetValueFromDict(m.type,    d, "_type"      );
            GetValueFromDict(m.plastic, d, "_plastic"   );
            GetValueFromDict(m.weight,  d, "_weight"    );
            GetValueFromDict(m.rough,   d, "_rough"     );
            GetValueFromDict(m.spec,    d, "_spec"      );
            GetValueFromDict(m.ior,     d, "_ior"       );
            GetValueFromDict(m.att,     d, "_att"       );
            GetValueFromDict(m.flux,    d, "_flux"      );
            GetValueFromDict(m.emit,    d, "_emit"      );
            GetValueFromDict(m.ri,      d, "_ri"        );
            GetValueFromDict(m.d,       d, "_d"         );
            GetValueFromDict(m.metal,   d, "_metal"     );
            vox.materials[material_id] = m;
            static_assert(sizeof(ColorInt::rgba) == sizeof(u32));

            break;
        }
        case FCCLAYR:
        {
            LAYR layer;
            layer.layer_id = GetDataAndIncrement<i32>(v);
            layer.dict;
            ReadDictData(layer.dict, v);
            layer.reserved_id = GetDataAndIncrement<i32>(v);
            VALIDATE_V(layer.reserved_id == -1, false);
            vox.layer_dicts.push_back(layer);
            VALIDATE_V(vox.layer_dicts.size() - 1 == layer.layer_id, false);
            break;
        }
        case FCCRGBA:
        {
            for (i32 i = 0; i < VOXEL_PALETTE_MAX; i++)
            {
                vox.color_palette[i].pack = GetDataAndIncrement<u32>(v);
            }
            color_count++;
            assert(color_count == 1);
            break;
        }
        case FCCrOBJ:
        {
            //Used in the Magica voxel renderer only and can safely discard
            Dict d;
            ReadDictData(d, v);
            break;
        }
        case FCCrCAM:
        {
            //Used in the Magica voxel renderer only and can safely discard
            i32 camera_id = GetDataAndIncrement<i32>(v);
            Dict d;
            ReadDictData(d, v);
            break;
        }
        case FCCNOTE:
        {
            i32 color_name_count = GetDataAndIncrement<i32>(v);
            std::string color_name;
            for (i32 i = 0; i < color_name_count; i++)
                GetStringDataAndIncriment(color_name, v);
            break;
        }
        case FCCIMAP:
        case -1:
        default:
        {
            std::string headerString = header.FCCAsString();
            VALIDATE_V(false, false);
        }
        }
    }

    out.color_indices = vox.color_indices;
    out.size          = vox.size;
    //NOTE: Annoying but for magicka voxel this has to be done this way
    //Magicka Voxel's indices are as such:
    //Indicies: 0-255 where 0 is invalid
    //Colors:   0-254 since 255 cannot be hit since 0 is used in the indices as invalid however it fills 255 with zeros for the palette
    //Mats:     1-256 however same thing as above 256 will be zero filled since it can never be hit
    for (i32 i = 1; i < VOXEL_PALETTE_MAX; i++)
    {

        out.materials[i].metalness  = vox.materials[i].metal;
        out.materials[i].roughness  = vox.materials[i].rough;
        out.materials[i].spec       = vox.materials[i].spec;
        out.materials[i].flux       = vox.materials[i].flux;
        out.materials[i].emit       = vox.materials[i].emit;
        out.materials[i].ri         = vox.materials[i].ri;
        out.materials[i].metal      = vox.materials[i].metal;
        out.materials[i].color      = vox.color_palette[i - 1];
    }
    //for (i32 i = 0; i < VOXEL_PALETTE_MAX; i++)
    //{
    //    out.color_palette[i] = vox.color_palette[i];
    //}

    if (v.i == v.max)
        return true;

    return false;
}

//bool CheckForVoxel(Voxels voxels, Vec3Int loc)
//{
//    if (loc.x >= VOXEL_MAX_SIZE || loc.y >= VOXEL_MAX_SIZE || loc.z >= VOXEL_MAX_SIZE)
//        return false;
//    return (voxels.e[loc.x][loc.y][loc.z].pack != 0);
//}
//
//u16 VoxelPositionToIndex(Vec3 p)
//{
//    const u16 result = (i16(p.z) * 16 * 16) + (i16(p.y) * 16) + (i16(p.x));
//    return result;
//}
//
//Vec3Int IndexToVoxelPosition(u16 i)
//{
//    Vec3Int result;
//    result.x = (i) % VOXEL_MAX_SIZE;
//    result.y = ((i - result.x) % (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE)) / VOXEL_MAX_SIZE;
//    result.z = (i - result.x - (result.y * VOXEL_MAX_SIZE)) / (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE);
//    return result;
//}

#if 0
void* OGTAlloc(size_t size, void* user_data)
{
    return malloc(size);
}
void OGTFree(void* ptr, void* user_data)
{
    free(ptr);
}

Vec4 GetUnscaledPos(Vec4 p, const Mat4& rot, const Mat4& tran)
{
    p = rot * p;
    p = tran * p;
    return p;
}


bool OGTImplimentation(VoxelMesh& voxelMeshs, std::vector<Voxels>& blocksVoxels, const std::string& filePath)
{
    assert(voxelMeshs.sizes.size() == 0);
    assert(voxelMeshs.vertices.size() == 0);
    assert(voxelMeshs.indices.size() == 0);
    voxelMeshs = {};

    File file(filePath, File::Mode::Read, false);
    if (!file.m_handleIsValid)
    {
        assert(false);
        return false;
    }
    file.GetData();
    if (!file.m_binaryDataIsValid)
    {
        assert(false);
        return false;
    }

    const ogt_vox_scene* scene = ogt_vox_read_scene(file.m_dataBinary.data(), (u32_t)file.m_dataBinary.size());
    Defer{ ogt_vox_destroy_scene(scene); };
    voxelMeshs.vertices.reserve(scene->num_models);

    //u32_t                num_models;     // number of models within the scene.
    //u32_t                num_instances;  // number of instances in the scene
    //u32_t                num_layers;     // number of layers in the scene
    //u32_t                num_groups;     // number of groups in the scene
    //const ogt_vox_model**   models;         // array of models. size is num_models
    //const ogt_vox_instance* instances;      // array of instances. size is num_instances
    //const ogt_vox_layer*    layers;         // array of layers. size is num_layers
    //const ogt_vox_group*    groups;         // array of groups. size is num_groups
    //ogt_vox_palette         palette;        // the palette for this scene
    //ogt_vox_matl_array      materials;      // the extended materials for this scene


    if (!scene->num_models)
        return false;
    assert(scene->num_instances == 1);
    ogt_voxel_meshify_context context;
    context.alloc_func = &OGTAlloc;
    context.free_func = &OGTFree;
    ogt_mesh* ogtMesh = nullptr;
    for (u32 m = 0; m < scene->num_models; m++)
    {
        voxelMeshs.sizes.push_back({});
        voxelMeshs.vertices.push_back({});
        voxelMeshs.indices.push_back({});
        blocksVoxels.push_back({});
        const ogt_vox_model* ogtM = scene->models[m];
        {
            assert(ogtM->size_x <= VOXEL_MAX_SIZE);
            assert(ogtM->size_y <= VOXEL_MAX_SIZE);
            assert(ogtM->size_z <= VOXEL_MAX_SIZE);
            for (u32 x = 0; x < ogtM->size_x; x++)
            {
                for (u32 y = 0; y < ogtM->size_y; y++)
                {
                    for (u32 z = 0; z < ogtM->size_z; z++)
                    {
                        const auto index = ogtM->voxel_data[(z * ogtM->size_x * ogtM->size_y) + (y * ogtM->size_x) + x];
                        assert(index < 256);
                        blocksVoxels[m].e[x][y][z].r = scene->palette.color[index].r;
                        blocksVoxels[m].e[x][y][z].g = scene->palette.color[index].g;
                        blocksVoxels[m].e[x][y][z].b = scene->palette.color[index].b;
                        blocksVoxels[m].e[x][y][z].a = scene->palette.color[index].a;
                    }
                }
            }
        }
        ogtMesh = ogt_mesh_from_paletted_voxels_greedy(&context, ogtM->voxel_data, ogtM->size_x, ogtM->size_y, ogtM->size_z, (ogt_mesh_rgba*)scene->palette.color);
        voxelMeshs.sizes[m].x = ogtM->size_x;
        voxelMeshs.sizes[m].y = ogtM->size_y;
        voxelMeshs.sizes[m].z = ogtM->size_z;

        Mat4 rot;
        gb_mat4_rotate(&rot, { 1, 0, 0 }, tau / 4.0f * 3.0f);
        Mat4 tran;
        gb_mat4_translate(&tran, { 0, 0, 16 }); //WARNING: This is hardcoded and needs to be based off of "something" TODO: Fix
        Mat4 mov = tran * rot;
        u32 vertIndex = 0;
        Vec3 compoundPos = {};
        for (u32 i = 0; i < ogtMesh->vertex_count; i++)
        {
            Vec4 oldPos = { ogtMesh->vertices[i].pos.x, ogtMesh->vertices[i].pos.y, ogtMesh->vertices[i].pos.z, 1.0f };
#if 1
            Vec4 newPos = { oldPos.x, oldPos.z, VOXEL_MAX_SIZE - oldPos.y };
#else
            Vec4 newPos = GetUnscaledPos(oldPos, rot, tran);
#endif
            Vec4 nonScaledPos = newPos;
            newPos = newPos / 16.0f;
            Vertex_Voxel v;
            v.p = newPos.xyz;
            v.rgba = {};
            v.rgba.r = ogtMesh->vertices[i].color.r;
            v.rgba.g = ogtMesh->vertices[i].color.g;
            v.rgba.b = ogtMesh->vertices[i].color.b;
            v.rgba.a = ogtMesh->vertices[i].color.a;

            Vec4 ogtNormal = { ogtMesh->vertices[i].normal.x, ogtMesh->vertices[i].normal.y, ogtMesh->vertices[i].normal.z, 1.0f };
            Vec4 newNormal = { ogtNormal.x, ogtNormal.z, -ogtNormal.y, };
            //ogtNormal = rot * ogtNormal;

            if (newNormal.x)
            {
                if (newNormal.x > 0)
                    v.n = +Face::Right;
                else
                    v.n = +Face::Left;
            }
            else if (newNormal.y)
            {
                if (newNormal.y > 0)
                    v.n = +Face::Top;
                else
                    v.n = +Face::Bot;
            }
            else if (newNormal.z)
            {
                if (newNormal.z > 0)
                    v.n = +Face::Back;
                else
                    v.n = +Face::Front;
            }
            else
                assert(false);

#if 0
            voxelMeshs.vertices[m].push_back(v);
            compoundPos += nonScaledPos.xyz;
            vertIndex++;
            if ((vertIndex % 4) == 0)
            {
                vertIndex = 0;
                Vec3 center = compoundPos / 4.0f;
                compoundPos = {};
                for (u32 j = i; (i - j) < 4; j--)
                {
                    assert(i == voxelMeshs.vertices[m].size() - 1);
                    Vec4 original = { ogtMesh->vertices[j].pos.x, ogtMesh->vertices[j].pos.y, ogtMesh->vertices[j].pos.z, 1.0f };
                    Vec4 unscaledOriginal = GetUnscaledPos(original, rot, tran);
                    Vec3i destDiff    = Vec3ToVec3i(Round(unscaledOriginal.xyz - center));
                    destDiff.x = Clamp(destDiff.x, -1, 1);
                    destDiff.y = Clamp(destDiff.y, -1, 1);
                    destDiff.z = Clamp(destDiff.z, -1, 1);
                    Vec3i voxPos      = Vec3ToVec3i(Round(original.xyz));
                    Vec3i normal      = Vec3ToVec3i(faceNormals[v.n]);
                    //one of these is useless depending on the face but its simpler to check for all of them:
                    voxelMeshs.vertices[m][j].ao = 0;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.x  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.y  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.z  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff    + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                }
            }
#else
            {
                Vec3i blockN = Vec3ToVec3i(faceNormals[v.n]);
                Vec3i a = *(&vertexBlocksToCheck[v.n].e0 + (vertIndex + 0));
                Vec3i b = *(&vertexBlocksToCheck[v.n].e0 + (vertIndex + 1));
                Vec3i c = a + b;
                Vec3i p = Vec3ToVec3i(nonScaledPos.xyz + 0.5);
                v.ao = {};
                if (CheckForVoxel(blocksVoxels[m], p + a))
                    v.ao += 1;
                if (CheckForVoxel(blocksVoxels[m], p + b))
                    v.ao += 1;
                if (CheckForVoxel(blocksVoxels[m], p + c))
                    v.ao += 1;

                vertIndex += 2;
                vertIndex = vertIndex % 8;
            }
            voxelMeshs.vertices[m].push_back(v);
#endif
        }
        for (u32 i = 0; i < ogtMesh->index_count; i++)
        {
            voxelMeshs.indices[m].push_back(ogtMesh->indices[i]);
        }
        ogt_mesh_destroy(&context, ogtMesh);
        ogtMesh = nullptr;
    }

    return true;
}
#endif

bool LoadVoxFile(VoxData& out_voxels, const std::string& filePath)
{
#if 0
    return OGTImplimentation(voxelMeshs, blocksVoxels, filePath);
#else
    return LoadVoxFile_MyImplimentation(out_voxels, filePath);
#endif
}

u16 VoxelPositionToIndex(Vec3 p)
{
    const u16 result = (i16(p.z) * 16 * 16) + (i16(p.y) * 16) + (i16(p.x));
    return result;
}

Vec3I IndexToVoxelPosition(u16 i)
{
    Vec3I result;
    result.x = (i) % VOXEL_MAX_SIZE;
    result.y = ((i - result.x) % (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE)) / VOXEL_MAX_SIZE;
    result.z = (i - result.x - (result.y * VOXEL_MAX_SIZE)) / (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE);
    return result;
}

union VertexBlockCheck {
    struct { Vec3I e0, e1, e2, e3, e4, e5, e6, e7; };
};

static const VertexBlockCheck vertex_blocks_to_check[+Face::Count] = {
    {//right +X
        Vec3I({  0,  1,  0 }),//Vertex 0
              {  0,  0,  1 },

              {  0,  0,  1 }, //Vertex 1
              {  0, -1,  0 },
              {  0,  1,  0 }, //Vertex 2
              {  0,  0, -1 },

              {  0,  0, -1 }, //Vertex 3
              {  0, -1,  0 },
    },
    {//left -X
        Vec3I({  0,  1,  0 }),//Vertex 0
              {  0,  0, -1 },

              {  0,  0, -1 }, //Vertex 1
              {  0, -1,  0 },
              {  0,  1,  0 }, //Vertex 2
              {  0,  0,  1 },

              {  0,  0,  1 }, //Vertex 3
              {  0, -1,  0 },
    },
    {//Top +Y
        Vec3I({  1,  0,  0 }),//Vertex 0
              {  0,  0,  1 },

              {  1,  0,  0 }, //Vertex 1
              {  0,  0, -1 },
              { -1,  0,  0 }, //Vertex 2
              {  0,  0,  1 },

              { -1,  0,  0 }, //Vertex 3
              {  0,  0, -1 },
    },
    {//Bot -Y
        Vec3I({ -1,  0,  0 }),//Vertex 0
              {  0,  0,  1 },

              { -1,  0,  0 }, //Vertex 1
              {  0,  0, -1 },
              {  1,  0,  0 }, //Vertex 2
              {  0,  0,  1 },

              {  1,  0,  0 }, //Vertex 3
              {  0,  0, -1 },
    },
    {//Front +Z
        Vec3I({ -1,  0,  0 }),//Vertex 0
              {  0,  1,  0 },

              { -1,  0,  0 }, //Vertex 1
              {  0, -1,  0 },
              {  1,  0,  0 }, //Vertex 2
              {  0,  1,  0 },

              {  1,  0,  0 }, //Vertex 3
              {  0, -1,  0 },
    },
    {//Front -Z
        Vec3I({  1,  0,  0 }),//Vertex 0
              {  0,  1,  0 },

              {  1,  0,  0 }, //Vertex 1
              {  0, -1,  0 },
              { -1,  0,  0 }, //Vertex 2
              {  0,  1,  0 },

              { -1,  0,  0 }, //Vertex 3
              {  0, -1,  0 },
    },
};

u8 GetVoxel(const VoxelBlockData& voxels, const Vec3I& p)
{
    if (p.x >= 0 && p.x < VOXEL_MAX_SIZE &&
    p.y >= 0 && p.y < VOXEL_MAX_SIZE &&
    p.z >= 0 && p.z < VOXEL_MAX_SIZE)
        return voxels.e[p.x][p.y][p.z];
    return 0;
}

u32 CreateMeshFromVox(std::vector<Vertex_Voxel>& vertices, const VoxData& voxel_data)
{
    u32 r = 0;
    VALIDATE_V(voxel_data.color_indices.size() == 1, r);

    const VoxelBlockData& voxel_color_is = voxel_data.color_indices[0];
    for (i32 z = 0; z < VOXEL_MAX_SIZE; z++)
        for (i32 y = 0; y < VOXEL_MAX_SIZE; y++)
            for (i32 x = 0; x < VOXEL_MAX_SIZE; x++)
            {
                const Vec3I this_voxel_pos = { x, y, z };
                const u8 this_voxel_i = GetVoxel(voxel_color_is, this_voxel_pos);
                if (this_voxel_i)
                {
                    for (u32 face_i = 0; face_i < +Face::Count; face_i++)
                    {
                        //voxels.e[x][y][z];
                        Vec3I vf = ToVec3I(faceNormals[face_i]);
                        Vec3I checking_block_pos;
                        checking_block_pos = this_voxel_pos + vf;
                        //checkingBlockPos.x = x + vf.x;
                        //checkingBlockPos.y = y + vf.y;
                        //checkingBlockPos.z = z + vf.z;

                        //The Y and Z should be flipped because of magica voxel's coordinate system
                        const u8 face_normal_voxel = GetVoxel(voxel_color_is, checking_block_pos);
                        bool block_normal_is_clear = face_normal_voxel == 0;
                        if (block_normal_is_clear)
                        {
                            for (i32 i = 0; i < 4; i++)
                            {
                                Vertex_Voxel v;
                                v.p = ToVec3(this_voxel_pos) + vertex_cube_indexed[face_i].e[i];
                                v.rgba = voxel_data.materials[this_voxel_i].color;
                                v.n = face_i;

                                Vec3I ap = *(&vertex_blocks_to_check[face_i].e0 + ((i * 2) + 0));
                                Vec3I bp = *(&vertex_blocks_to_check[face_i].e0 + ((i * 2) + 1));
                                Vec3I cp = ap + bp;
                                const u8 ai = GetVoxel(voxel_color_is, checking_block_pos + ap);
                                const u8 bi = GetVoxel(voxel_color_is, checking_block_pos + bp);
                                const u8 ci = GetVoxel(voxel_color_is, checking_block_pos + cp);
                                v.ao = 0;
                                if (ai)
                                    v.ao++;
                                if (bi)
                                    v.ao++;
                                if (ci)
                                    v.ao++;
                                vertices.push_back(v);
                            }
                            r += 6;
                        }
                    }
                }
            }

    return r;
}
