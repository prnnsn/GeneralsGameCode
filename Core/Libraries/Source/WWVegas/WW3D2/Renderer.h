/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// TheSuperHackers @refactor bobtista 08/10/2026 Static rendering interface so WW3D2
// rendering can be re-targeted to other backends. The build links exactly one
// backend, and that backend defines the methods below. The DX8 backend is the
// reference implementation.

#pragma once

#include "WW3D2/RenderTexture.h"

// Forward declarations keep this header includable without pulling in the full
// WW3D2 header graph. All W3D classes below are passed by pointer or reference.

class LightClass;
class LightEnvironmentClass;
class Matrix3D;
class Matrix4x4;
class Vector3;
class ShaderClass;
class TextureBaseClass;
class TextureClass;
class SurfaceClass;
class VertexMaterialClass;
class VertexBufferClass;
class IndexBufferClass;
class DynamicVBAccessClass;
class DynamicIBAccessClass;
class RefCountClass;
namespace RenderBuffers { struct BufferDesc; }

// A backend texture reference. Mapping borrows it, while creation returns one
// reference that the caller must publish or release.
struct RenderTexture2DHandle
{
    void * value = nullptr;

    explicit operator bool() const { return value != nullptr; }
};

struct RenderViewport
{
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    float min_z;
    float max_z;
};

enum RenderTransform
{
    RenderTransform_World,
    RenderTransform_View,
    RenderTransform_Projection
};

// The interface holds only the methods that callers route through. The rest of
// the DX8Wrapper API is called directly.

class Renderer
{
public:
    // Initialized in WW3D::Init and shut down in WW3D::Shutdown.
    static bool Init(void * window, bool lite);
    static void Shutdown();

    static int Get_Render_Device_Count();
    static int Get_Render_Device();
    static const char * Get_Render_Device_Name(int device_index);
    static void Get_Device_Resolution(int & width, int & height, int & bits, bool & windowed);
    static void Get_Render_Target_Resolution(int & width, int & height, int & bits, bool & windowed);
    static int Get_Device_Resolution_Width();
    static int Get_Device_Resolution_Height();
    static bool Is_Windowed();
    static int Get_Texture_Bitdepth();
    static int Get_Swap_Interval();
    static bool Has_Stencil();
    static WW3DFormat Get_Back_Buffer_Format();

    static void Set_Gamma(float gamma, float bright, float contrast, bool calibrate = true, bool uselimit = true);

    static void Begin_Scene();
    static void End_Scene(bool flip_frame = true);
    static void Flip_To_Primary();
    static void Clear(bool clear_color, bool clear_z_stencil,
                      const Vector3 & color,
                      float dest_alpha = 0.0f, float z = 1.0f, unsigned int stencil = 0);
    static void Set_Viewport(const RenderViewport & viewport);
    static void Invalidate_Cached_Render_States();

    // Existing W3D material and indexed triangle submission semantics.
    // Buffer allocation/binding and texture sampler policy remain separate.
    static void Set_Shader(const ShaderClass & shader);
    static void Set_Texture(unsigned stage, TextureBaseClass * texture);

    // An ordinary sampled texture for the loader. Zero mip levels requests the
    // full chain, preserving the W3D loader's convention.
    static RenderTexture2DHandle Create_Load_Texture_2D(unsigned width, unsigned height,
                                                        WW3DFormat format, unsigned mip_levels);
    static void Release_Texture_2D(RenderTexture2DHandle & texture);
    // The target retains its reference and this consumes the loader's reference.
    static void Apply_Load_Texture_2D(TextureBaseClass & target, RenderTexture2DHandle & texture,
                                      bool initialize);

    // Map all levels or release any partial mapping and return zero.
    // Both arrays must have capacity entries. Call Unmap on the render thread
    // with the successful count before releasing or publishing the resource.
    static unsigned Map_Texture_2D_Mips(RenderTexture2DHandle texture, unsigned char ** data,
                                        unsigned * pitch, unsigned capacity);
    static bool Unmap_Texture_2D_Mips(RenderTexture2DHandle texture, unsigned char ** data,
                                      unsigned * pitch, unsigned mip_count);

    // The caller owns one SurfaceClass reference. Use the usual Release_Ref.
    static SurfaceClass * Get_Texture_2D_Mip_Surface(const TextureClass & texture, unsigned mip);
    // On failure the output arguments keep their previous values.
    static bool Try_Describe_Texture_2D_Mip(const TextureClass & texture, unsigned mip,
                                            unsigned & width, unsigned & height, WW3DFormat & format);

    // Owns a checked write mapping of a single uncompressed mip.
    class TextureMipWrite
    {
    public:
        TextureMipWrite() = default;
        ~TextureMipWrite();

        TextureMipWrite(const TextureMipWrite &) = delete;
        TextureMipWrite & operator=(const TextureMipWrite &) = delete;

        bool Try_Begin(const TextureClass & texture, unsigned mip = 0);
        const RenderTextures::SurfaceWriteView & View() const { return view; }
        bool Finish();

    private:
        void * native_surface = nullptr;
        RenderTextures::SurfaceWriteView view;
    };

    // Zero the active bytes of an uncompressed mip while preserving row padding.
    static bool Clear_Texture_2D_Mip(const TextureClass & texture, unsigned mip = 0);

    // Set addressing and border colour for one sampler. Colour is packed ARGB.
    enum class TextureAddress { Wrap, Mirror, Clamp, Border, MirrorOnce };
    static void Set_Texture_Address_U(unsigned stage, TextureAddress address);
    static void Set_Texture_Address_V(unsigned stage, TextureAddress address);
    static void Set_Texture_Border_Color(unsigned stage, unsigned argb);
    enum class TextureFilter { None, Point, Linear, Anisotropic };
    struct TextureFilterCaps
    {
        bool minLinear, magLinear, mipLinear, minAnisotropic, magAnisotropic;
    };
    static TextureFilterCaps Get_Texture_Filter_Caps();
    static void Set_Texture_Min_Filter(unsigned stage, TextureFilter filter);
    static void Set_Texture_Mag_Filter(unsigned stage, TextureFilter filter);
    static void Set_Texture_Mip_Filter(unsigned stage, TextureFilter filter);
    static void Set_Texture_Max_Anisotropy(unsigned stage, unsigned level);
    static void Set_Material(const VertexMaterialClass * material);
    static void Apply_Render_State_Changes();
    // Scoped terrain bias uses legacy strength units, not a raw API constant.
    // Only acquire when supported and no previous bias is active.
    static bool Try_Begin_Terrain_Depth_Bias(unsigned strength);
    static void End_Terrain_Depth_Bias();
    static void Draw_Triangles(unsigned short start_index, unsigned short polygon_count,
                               unsigned short min_vertex_index, unsigned short vertex_count);

    // Bind existing W3D buffer resources/access ranges; allocation and locking
    // remain in the resource layer. Preserve DX8 index offsets and stream defaults.
    static void Set_Vertex_Buffer(const VertexBufferClass * vb, unsigned stream = 0);
    static void Set_Vertex_Buffer(const DynamicVBAccessClass & vba);
    static void Set_Index_Buffer(const IndexBufferClass * ib, unsigned short index_base_offset);
    static void Set_Index_Buffer(const DynamicIBAccessClass & iba, unsigned short index_base_offset);
    static void Set_Index_Buffer_Index_Offset(unsigned offset);

    // The returned W3D object owns one reference. Use the usual Release_Ref.
    static VertexBufferClass * Create_Rewritable_Vertex_Buffer(const RenderBuffers::BufferDesc & desc);
    static IndexBufferClass * Create_Rewritable_Index_Buffer(const RenderBuffers::BufferDesc & desc);
    static bool Upload_Dynamic_Vertices(DynamicVBAccessClass & access, const void * source, unsigned stride);
    static bool Upload_Dynamic_Indices(DynamicIBAccessClass & access, const unsigned short * source);
    // Bind the validated span after both uploads have succeeded.
    static bool Bind_Dynamic_Triangle_List(const DynamicVBAccessClass & vertices,
                                           const DynamicIBAccessClass & indices);

    class BufferWrite
    {
    public:
        BufferWrite(VertexBufferClass * buffer, unsigned first, unsigned count);
        BufferWrite(IndexBufferClass * buffer, unsigned first, unsigned count);
        ~BufferWrite();

        BufferWrite(const BufferWrite &) = delete;
        BufferWrite & operator=(const BufferWrite &) = delete;

        void * Data() const { return data; }
        // Call Finish before publishing the data. The destructor cleans up an unfinished map.
        bool Finish();

    private:
        RefCountClass * owner = nullptr;
        void * data = nullptr;
        bool (*finish)(RefCountClass *) = nullptr;
    };

    static void Set_Transform(RenderTransform transform, const Matrix4x4 & m);
    static void Set_Transform(RenderTransform transform, const Matrix3D & m);
    static void Get_Transform(RenderTransform transform, Matrix4x4 & m);
    static void Set_World_Identity();
    static void Set_View_Identity();
    static void Set_Projection_Transform_With_Z_Bias(const Matrix4x4 & matrix, float znear, float zfar);

    static void Set_Ambient(const Vector3 & color);
    static void Set_Light_Environment(LightEnvironmentClass * light_env);
    static void Set_Light(unsigned index, const LightClass & light);
    static void Clear_Light(unsigned index);
    static void Set_Fog(bool enable, const Vector3 & color, float start, float end);
};
